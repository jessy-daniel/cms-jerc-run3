import re
import os
import sys
import json
import itertools
import multiprocessing
from ROOT import TFile
sys.dont_write_bytecode = True
sys.path.insert(0, os.getcwd().replace("condor",""))
from Inputs import *
from createSkimJobFiles import createJobs

from tqdm import tqdm
from concurrent.futures import ThreadPoolExecutor, TimeoutError as FutureTimeout, ProcessPoolExecutor, TimeoutError
#-------------------------------------------------
# Configuration for timeout
#-------------------------------------------------
# how long (in seconds) to wait for any TFile.Open before giving up
OPEN_TIMEOUT = 10

#-------------------------------------------------
# Helper: open ROOT file with a hard timeout using threads
#-------------------------------------------------
def open_with_timeout(path, mode="READ"):
    """
    Try to open the ROOT file in a background thread.
    If it takes longer than OPEN_TIMEOUT seconds, return None.
    """
    from ROOT import TFile
    with ThreadPoolExecutor(max_workers=1) as executor:
        future = executor.submit(TFile.Open, path, mode)
        try:
            return future.result(timeout=OPEN_TIMEOUT)
        except FutureTimeout:
            print(f"[Timeout on open] {path}")
            return None

#-------------------------------------------------
# Function to check a single ROOT file
#-------------------------------------------------
def check_file(args):
    """
    Checks a single ROOT file (skim) for corruption, with hard timeouts.
    Returns tuple: (sKey, skim, is_corrupted)
    """
    from ROOT import TFile
    sKey, skim = args
    f = None
    try:
        # Step 1: Quick connectivity check
        f_test = open_with_timeout(skim, "READ")
        if f_test is None:
            print(f"[Timeout on raw-open] {skim}")
            return (sKey, skim, True)
        if not f_test or f_test.IsZombie():
            print(f"[Raw connectivity failure] {skim}")
            return (sKey, skim, True)
        if f_test.GetSize() < 3000:
            print(f"[Raw file too small] {skim}")
            f_test.Close()
            return (sKey, skim, True)
        f_test.Close()

        # Step 2: Full open & content checks
        f = open_with_timeout(skim, "READ")
        if f is None:
            print(f"[Timeout on full-open] {skim}")
            return (sKey, skim, True)
        if not f or f.IsZombie() or f.GetSize() < 3000:
            print(f"[Corrupt/Empty] {skim}")
            return (sKey, skim, True)

        # Check for the histogram
        h = f.Get("Cutflow/h1EventInCutflow")
        if not h:
            print(f"[Missing Cutflow] {skim}")
            return (sKey, skim, True)

        # Check Events TTree
        tree = f.Get("Events")
        if not tree:
            print(f"[Missing Events TTree] {skim}")
            return (sKey, skim, True)

        # Check Runs TTree
        tree = f.Get("Runs")
        if not tree:
            print(f"[Missing Runs TTree] {skim}")
            return (sKey, skim, True)

        # All checks passed
        return (sKey, skim, False)

    except Exception as e:
        print(f"[Exception] while opening {skim}: {e}")
        return (sKey, skim, True)

    finally:
        if f:
            f.Close()

#-------------------------------------------------
# Check each file in the new-style JSON
#-------------------------------------------------
from concurrent.futures import ProcessPoolExecutor, TimeoutError
from tqdm import tqdm

#-------------------------------------------------
# Check each file in flat-style JSON
#-------------------------------------------------
def check_jobs(jsonFile):
    """
    Expects JSON of the form:
    {
        "SampleName": [
            "file1.root", "file2.root", ...
        ],
        ...
    }
    """
    print("Checking for corrupted files using ProcessPoolExecutor with per-file timeouts...")
    unfinished = {}

    # Build list of (sample, file) pairs
    file_list = []
    for sKey, files in jsonFile.items():
        if isinstance(files, list) and all(isinstance(f, str) for f in files):
            for skim in files:
                file_list.append((sKey, skim))
        else:
            print(f"WARNING: '{sKey}' does not conform to flat JSON structure. Skipping.")

    if not file_list:
        print("No valid files found in the JSON!")
        return unfinished

    # Parallel check with per-file timeout
    pool_size = min(20, len(file_list))
    results = []
    max_task_time = OPEN_TIMEOUT * 2  # allow time for raw + full open

    executor = ProcessPoolExecutor(max_workers=pool_size)
    futures = [executor.submit(check_file, arg) for arg in file_list]

    for future, (sKey, skim) in tqdm(zip(futures, file_list), total=len(futures), desc="Checking files"):
        try:
            res = future.result(timeout=max_task_time)
        except TimeoutError:
            print(f"[Overall timeout] {skim}")
            res = (sKey, skim, True)
        results.append(res)

    # Kill leftover workers
    for p in getattr(executor, '_processes', {}).values():
        try:
            p.terminate()
        except Exception:
            pass
    executor.shutdown(wait=False)

    # Build unfinished dict (just corrupted files per sample)
    corrupted_map = {}
    for sKey, skim, is_corrupted in results:
        if is_corrupted:
            corrupted_map.setdefault(sKey, []).append(skim)

    for sKey, bad_files in corrupted_map.items():
        unfinished[sKey] = bad_files

    return unfinished



#-------------------------------------------------
# Main execution block
#-------------------------------------------------
if __name__ == "__main__":
    logDir = "resubLog"
    dResubs = {}
    fResub = "tmpSub/resubFilesSkim.json"

    # Ensure the 'tmpSub' directory exists
    os.makedirs("tmpSub", exist_ok=True)

    with open('tmpSub/resubJobs.jdl', 'w') as jdlFile_:
        if os.path.exists(fResub):
            # If resubmission file exists, check those files
            with open(fResub, "r") as fResub_:
                jsonFile = json.load(fResub_)
            dResub = check_jobs(jsonFile)
            dResubs.update(dResub)
            # Write updated list of files to resubmit
            with open(fResub, "w") as fResub__:
                json.dump(dResubs, fResub__, indent=4)
            # Create jobs for resubmission
            with open(fResub, "r") as fResub___:
                createJobs(fResub___, jdlFile_, logDir)
        else:
            # First-time check: iterate over all years and channels
            for year, ch in itertools.product(list(Years.keys()), Channels):
                for dataOrMc, samples in Years[year].items():
                    for sample in samples:
                        chYearOther = f"{ch}_{year}_{dataOrMc}_{sample}" 
                        print(f"\nProcessing {chYearOther}")
                        json_file_path = f"skim_files/FilesSkim_{chYearOther}.json"
                        if not os.path.exists(json_file_path):
                            print(f"ERROR: JSON file {json_file_path} does not exist.")
                            sys.exit(1)
                        with open(json_file_path, "r") as fSkim:
                            jsonFile = json.load(fSkim)
                        dResub = check_jobs(jsonFile)
                        dResubs.update(dResub)
            # Write the list of files to resubmit
            with open(fResub, "w") as fResub_:
                json.dump(dResubs, fResub_, indent=4)
            # Create jobs for resubmission
            with open(fResub, "r") as fResub__:
                createJobs(fResub__, jdlFile_, logDir)

    # Calculate the total number of files to resubmit
    totResub = sum(len(files) for files in dResubs.values())
    print(f"\n=========> Total files to be resubmitted: {totResub} <==========\n")
    print(f"{fResub}\n")
    
