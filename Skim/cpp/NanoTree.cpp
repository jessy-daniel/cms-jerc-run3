#include "NanoTree.h"
#include "Helper.h"
#include <TError.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <TFile.h>
#include <TChain.h>
#include <nlohmann/json.hpp>

NanoTree::NanoTree(GlobalFlag& globalFlags)
  : globalFlags_(globalFlags),
    fChain(nullptr),
    fChainRuns(nullptr),
    fCurrent_(-1),
    fCurrentRuns_(-1)
{
    // Allocate both chains right away:
    fChain     = new TChain("Events");
    fChainRuns = new TChain("Runs");
}

NanoTree::~NanoTree() {
    delete fChain;
    delete fChainRuns;
}

void NanoTree::setInput(const std::string& outputName) {
    outputName_ = outputName;
    std::cout << "+ setInput() = " << outputName_ << '\n';
}

void NanoTree::loadInput() {
    std::cout << "==> loadInput()" << '\n';
    try {
        std::vector<std::string> nameParts = Helper::splitString(outputName_, "_Skim_");
        loadedSampleKey_ = nameParts.at(0);
        std::cout << "Loaded sample key: " << loadedSampleKey_ << '\n';

        std::vector<std::string> jobParts = Helper::splitString(nameParts.at(1), ".root");
        std::string jobInfo = jobParts.at(0);
        std::vector<std::string> jobSplit = Helper::splitString(jobInfo, "of");

        loadedNthJob_    = std::stoul(jobSplit.at(0));
        loadedTotalJobs_ = std::stoul(jobSplit.at(1));
    } catch (const std::exception& e) {
        std::cerr << "\nEXCEPTION: Invalid output name format: " << outputName_ << '\n';
        std::cerr << "Expected format: DataOrMC_Year_Channel_Sample_Skim_nofN.root\n";
        std::cerr << "Run ./runMain -h for more details\n";
        std::cerr << e.what() << '\n';
        std::abort();
    }
}

void NanoTree::loadInputJson(const std::string& searchDir) {
    std::cout << "==> loadInputJson()" << '\n';
    std::vector<std::filesystem::path> jsonFiles;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(searchDir)) {
            if (entry.path().extension() == ".json" &&
                entry.path().filename().string().rfind("FilesNano_", 0) == 0) {
                jsonFiles.push_back(entry.path());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory " << searchDir << ": " << e.what() << "\n";
        std::abort();
    }

    if (jsonFiles.empty()) {
        std::cerr << "No JSON files found in directory: " << searchDir << "\n";
        std::abort();
    }

    loadedAllFileNames_.clear();
    int foundCount = 0;
    nlohmann::json jsonContent;

    for (const auto& jsonFile : jsonFiles) {
        std::ifstream inputFile(jsonFile);
        if (!inputFile.is_open()) {
            std::cerr << "Could not open JSON file: " << jsonFile << "\n";
            continue;
        }
        try {
            jsonContent = nlohmann::json::parse(inputFile);
        } catch (const std::exception& e) {
            std::cerr << "\nEXCEPTION: Error parsing JSON at " << jsonFile << "\n";
            std::cerr << e.what() << "\n";
            continue;
        }

        std::string channel;
        try {
            channel = Helper::splitString(loadedSampleKey_, "_").at(2);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing loadedSampleKey_: " << loadedSampleKey_ << "\n";
            std::cerr << e.what() << "\n";
            std::abort();
        }

        if (jsonContent.contains(loadedSampleKey_)) {
            foundCount++;
            const auto& value = jsonContent.at(loadedSampleKey_).at(1);
            if (!value.is_array()) {
                std::cerr << "Error: JSON key '" << loadedSampleKey_
                          << "' in file " << jsonFile
                          << " is not an array. Aborting.\n";
                std::abort();
            }
            try {
                loadedAllFileNames_ = value.get<std::vector<std::string>>();
            } catch (const std::exception& e) {
                std::cerr << "\nEXCEPTION: Error extracting key '" << loadedSampleKey_
                          << "' from file " << jsonFile << "\n";
                std::cerr << e.what() << "\n";
                std::abort();
            }
        }
    }

    if (foundCount == 0) {
        std::cerr << "\nEXCEPTION: Key '" << loadedSampleKey_
                  << "' was not found in any JSON file in " << searchDir << "\n";
        std::abort();
    }
    if (foundCount > 1) {
        std::cerr << "\nEXCEPTION: Key '" << loadedSampleKey_
                  << "' found in multiple JSON files. Please ensure uniqueness.\n";
        std::abort();
    }
}

void NanoTree::loadJobFileNames() {
    std::cout << "==> loadJobFileNames()" << '\n';
    std::size_t totalFiles = loadedAllFileNames_.size();
    std::cout << "Total files: " << totalFiles << '\n';

    if (loadedTotalJobs_ > totalFiles) {
        std::cerr << "Warning: loadedTotalJobs exceeds total files; adjusting to " << totalFiles << '\n';
        loadedTotalJobs_ = totalFiles;
    }
    if (loadedNthJob_ > loadedTotalJobs_) {
        std::cerr << "Error: loadedNthJob exceeds loadedTotalJobs\n";
        std::abort();
    }
    if (loadedNthJob_ > 0 && loadedTotalJobs_ > 0) {
        std::cout << "Processing job " << loadedNthJob_
                  << " of " << loadedTotalJobs_ << '\n';
        std::cout << "Approximately " << totalFiles / loadedTotalJobs_
                  << " files per job" << '\n';
    } else {
        std::cerr << "Error: loadedNthJob and loadedTotalJobs must be greater than zero\n";
        std::abort();
    }

    loadedJobFileNames_ =
        Helper::splitVector(loadedAllFileNames_, loadedTotalJobs_).at(loadedNthJob_ - 1);
}

void NanoTree::loadTree() {
    std::cout << "==> loadTree()" << '\n';
    // By now, fChain and fChainRuns are valid TChain* instances, so we can use them:
    fChain->SetCacheSize(Helper::tTreeCatchSize);
    fChainRuns->SetCacheSize(Helper::tTreeCatchSize);

    bool isCopy = false;
    std::string dir = "root://cms-xrd-global.cern.ch/";

    int totalFiles = 0;
    int addedFiles = 0;
    int failedFiles = 0;

    // xrdcp parameters, etc.
    const int streams   = 15;
    const int tcpBufSize = 1048576;

    for (const auto& fileName : loadedJobFileNames_) {
        totalFiles++;
        std::string fullPath;

        if (isCopy) {
            // … code to xrdcp locally, etc. (same as before) …
        } else {
            std::filesystem::path filePath = "/eos/cms/" + fileName;
            if (std::filesystem::exists(filePath)) {
                dir = "/eos/cms/";
                fullPath = dir + fileName;
            } else {
                dir = "root://cms-xrd-global.cern.ch/";
                fullPath = dir + fileName;
            }
        }

        TFile* f = TFile::Open(fullPath.c_str(), "READ");
        if (!f || f->IsZombie()) {
            std::cerr << "Error: Failed to open or corrupted file " << fullPath << '\n';
            if (f) f->Close();
            failedFiles++;
            continue;
        }

        std::cout << "Opened " << fullPath << ", size = " << f->GetSize() << '\n';
        if (f->GetSize() < 3000) {
            std::cerr << "Warning: file " << fullPath << " has < 3000 bytes, skipping.\n";
            f->Close();
            failedFiles++;
            continue;
        }

        if (!f->GetListOfKeys()->Contains("Events")) {
            std::cerr << "Error: 'Events' not found in " << fullPath << '\n';
            f->Close();
            failedFiles++;
            continue;
        }

        TTree* tree = f->Get<TTree>("Events");
        Long64_t fileEntries = tree->GetEntries();
        if (fileEntries == 0) {
            std::cerr << "\nWarning: 'Events' TTree in file " << fullPath
                      << " has 0 entries. Skipping file.\n\n";
            f->Close();
            failedFiles++;
            continue;
        }

        // Now actually add this file to both chains:
        int added = fChain->Add(fullPath.c_str());
        fChainRuns->Add(fullPath.c_str());
        if (added == 0) {
            std::cerr << "Warning: TChain::Add failed for " << fullPath << '\n';
            f->Close();
            failedFiles++;
            continue;
        }

        std::cout << " → fChain now has " << fChain->GetEntries()
                  << " entries; fChainRuns has " << fChainRuns->GetEntries() << '\n';
        addedFiles++;
        f->Close();
    }

    // Summary
    std::cout << "==> Finished loading files into fChain/fChainRuns\n"
              << "   Total candidates:    " << totalFiles << "\n"
              << "   Successfully added: " << addedFiles << "\n"
              << "   Failed to add:      " << failedFiles << "\n";

    if (fChain->GetNtrees() == 0) {
        std::cerr << "Error: No valid ROOT files were added to the TChain. Exiting.\n";
        return;
    }

    // Now set branch‐status flags for everything you need:
    fChainRuns->SetBranchStatus("*", true);
    fChain->SetBranchStatus("*", false);

    fChain->SetBranchStatus("run", true);
    fChain->SetBranchStatus("event", true);
    fChain->SetBranchStatus("luminosityBlock", true);

//--------------------------------------- 
    // Jet branches for all channels 
    //--------------------------------------- 
    fChain->SetBranchStatus("Jet_btagDeep*",1);
    fChain->SetBranchStatus("Jet_btagPNetQvG",1);
    fChain->SetBranchStatus("Jet_btagUParTAK4QvG",1);
    fChain->SetBranchStatus("Jet_chEmEF",1);
    fChain->SetBranchStatus("Jet_chHEF" ,1);
    fChain->SetBranchStatus("Jet_eta"   ,1);
    fChain->SetBranchStatus("Jet_mass"  ,1);
    fChain->SetBranchStatus("Jet_muEF"  ,1);
    fChain->SetBranchStatus("Jet_neEmEF",1);
    fChain->SetBranchStatus("Jet_hfEmEF",1);
    fChain->SetBranchStatus("Jet_hfHEF",1);
    fChain->SetBranchStatus("Jet_neHEF" ,1);
    fChain->SetBranchStatus("Jet_chMultiplicity" ,1);
    fChain->SetBranchStatus("Jet_neMultiplicity" ,1);
    fChain->SetBranchStatus("Jet_phi"   ,1);
    fChain->SetBranchStatus("Jet_pt"    ,1);
    fChain->SetBranchStatus("Jet_rawFactor",1);
    fChain->SetBranchStatus("Jet_PNetRegPtRawCorr",1);
    fChain->SetBranchStatus("Jet_PNetRegPtRawCorrNeutrino",1);
    fChain->SetBranchStatus("Jet_UParTAK4RegPtRawCorr",1);
    fChain->SetBranchStatus("Jet_UParTAK4RegPtRawCorrNeutrino",1);
    fChain->SetBranchStatus("Jet_jetId",1);
    fChain->SetBranchStatus("Jet_area",1);
    fChain->SetBranchStatus("nJet",1);

    //common branches
	fChain->SetBranchStatus("PV_z");
    fChain->SetBranchStatus("PV_npvs",1);
    fChain->SetBranchStatus("PV_npvsGood",1);
    fChain->SetBranchStatus("PFMET_pt",1);
    fChain->SetBranchStatus("PFMET_phi",1);
    fChain->SetBranchStatus("PuppiMET_pt",1);
    fChain->SetBranchStatus("PuppiMET_phi",1);
    fChain->SetBranchStatus("RawPuppiMET_phi",1);
    fChain->SetBranchStatus("RawPuppiMET_pt",1);
    fChain->SetBranchStatus("RawPuppiMET_sumEt",1);
    fChain->SetBranchStatus("MET_pt",1);
    fChain->SetBranchStatus("MET_phi",1);
    fChain->SetBranchStatus("ChsMET_pt",1);
    fChain->SetBranchStatus("ChsMET_phi",1);

	fChain->SetBranchStatus("Rho_fixed*", 1);

    //Photons
    fChain->SetBranchStatus("nPhoton",1);
    fChain->SetBranchStatus("Photon_energyErr",1);
    fChain->SetBranchStatus("Photon_eta",1);
    fChain->SetBranchStatus("Photon_hoe",1);
    fChain->SetBranchStatus("Photon_phi",1);
    fChain->SetBranchStatus("Photon_pt",1);
    fChain->SetBranchStatus("Photon_r9",1);
    fChain->SetBranchStatus("Photon_cutBased",1);
    fChain->SetBranchStatus("Photon_jetIdx",1);
    fChain->SetBranchStatus("Photon_seedGain",1);

    // Additional flags branches
    fChain->SetBranchStatus("Flag_HBHENoiseFilter",1);
    fChain->SetBranchStatus("Flag_HBHENoiseIsoFilter",1);
    fChain->SetBranchStatus("Flag_CSCTightHaloFilter",1);
    fChain->SetBranchStatus("Flag_CSCTightHaloTrkMuUnvetoFilter",1);
    fChain->SetBranchStatus("Flag_CSCTightHalo2015Filter",1);
    fChain->SetBranchStatus("Flag_globalTightHalo2016Filter",1);
    fChain->SetBranchStatus("Flag_HcalStripHaloFilter",1);
    fChain->SetBranchStatus("Flag_hcalLaserEventFilter",1);
    fChain->SetBranchStatus("Flag_EcalDeadCellBoundaryEnergyFilter",1);
    fChain->SetBranchStatus("Flag_ecalLaserCorrFilter",1);
    fChain->SetBranchStatus("Flag_trkPOGFilters",1);
    fChain->SetBranchStatus("Flag_chargedHadronTrackResolutionFilter",1);
    fChain->SetBranchStatus("Flag_muonBadTrackFilter",1);
    fChain->SetBranchStatus("Flag_BadChargedCandidateFilter",1);
    fChain->SetBranchStatus("Flag_BadChargedCandidateSummer16Filter",1);
    fChain->SetBranchStatus("Flag_BadPFMuonSummer16Filter",1);
    fChain->SetBranchStatus("Flag_trkPOG_manystripclus53X",1);
    fChain->SetBranchStatus("Flag_trkPOG_toomanystripclus53X",1);
    fChain->SetBranchStatus("Flag_trkPOG_logErrorTooManyClusters",1);
    fChain->SetBranchStatus("Flag_METFilters",1);

    // Additional HLT branches
    fChain->SetBranchStatus("HLT_Photon300_NoHE",1);
    fChain->SetBranchStatus("HLT_Photon33",1);
    fChain->SetBranchStatus("HLT_Photon50",1);
    fChain->SetBranchStatus("HLT_Photon75",1);
    fChain->SetBranchStatus("HLT_Photon90",1);
    fChain->SetBranchStatus("HLT_Photon120",1);
    fChain->SetBranchStatus("HLT_Photon150",1);
    fChain->SetBranchStatus("HLT_Photon175",1);
    fChain->SetBranchStatus("HLT_Photon200",1);
    fChain->SetBranchStatus("HLT_Photon50_R9Id90_HE10_IsoM",1);
    fChain->SetBranchStatus("HLT_Photon75_R9Id90_HE10_IsoM",1);
    fChain->SetBranchStatus("HLT_Photon90_R9Id90_HE10_IsoM",1);
    fChain->SetBranchStatus("HLT_Photon120_R9Id90_HE10_IsoM",1);
    fChain->SetBranchStatus("HLT_Photon165_R9Id90_HE10_IsoM",1);
    fChain->SetBranchStatus("HLT_Photon20_HoverELoose",1);
    fChain->SetBranchStatus("HLT_Photon30_HoverELoose",1);
    fChain->SetBranchStatus("HLT_Photon30EB_TightID_TightIso",1);
    fChain->SetBranchStatus("HLT_Photon100EBHE10",1);
    fChain->SetBranchStatus("HLT_Photon110EB_TightID_TightIso",1);
    fChain->SetBranchStatus("HLT_Photon50EB_TightID_TightIso",1);
    fChain->SetBranchStatus("HLT_Photon55EB_TightID_TightIso",1);
    fChain->SetBranchStatus("HLT_Photon75EB_TightID_TightIso",1);
    fChain->SetBranchStatus("HLT_Photon90EB_TightID_TightIso",1);
 
    if (globalFlags_.isMC) {
        fChain->SetBranchStatus("Jet_genJetIdx",1);
		fChain->SetBranchStatus("genWeight");
		fChain->SetBranchStatus("nPSWeight");
		fChain->SetBranchStatus("PSWeight");
		fChain->SetBranchStatus("LHE_HT");
	    fChain->SetBranchStatus("Pileup_*",1);
        fChain->SetBranchStatus("GenJet_*",1);
	    fChain->SetBranchStatus("GenVtx_z");
        fChain->SetBranchStatus("nGenJet",1);

        fChain->SetBranchStatus("Generator_weight",1);
        fChain->SetBranchStatus("nLHEScaleWeight",1);
        fChain->SetBranchStatus("LHEScaleWeight",1);
        fChain->SetBranchStatus("nLHEPdfWeight",1);
        fChain->SetBranchStatus("nLHEPart",1);
        fChain->SetBranchStatus("LHEPart_*",1);
        fChain->SetBranchStatus("PSWeight",1);
        fChain->SetBranchStatus("nPSWeight",1);
        fChain->SetBranchStatus("genWeight",1);

        fChain->SetBranchStatus("nGenIsolatedPhoton",1);
        fChain->SetBranchStatus("GenIsolatedPhoton_eta",1);
        fChain->SetBranchStatus("GenIsolatedPhoton_mass",1);
        fChain->SetBranchStatus("GenIsolatedPhoton_phi",1);
        fChain->SetBranchStatus("GenIsolatedPhoton_pt",1);
		/*
        fChain->SetBranchStatus("GenPart_*",1);
        fChain->SetBranchStatus("nGenPart",1);
        fChain->SetBranchStatus("GenJetAK8_*",1);
        fChain->SetBranchStatus("nGenJetAK8",1);
        fChain->SetBranchStatus("LHEPdfWeight",1);
        // weight
		*/
    }//MC

    filterList = { 
        "Flag_goodVertices",
        "Flag_globalSuperTightHalo2016Filter",
        "Flag_EcalDeadCellTriggerPrimitiveFilter",
        "Flag_BadPFMuonFilter",
        "Flag_BadPFMuonDzFilter",
        "Flag_hfNoisyHitsFilter",
        "Flag_eeBadScFilter",
        "Flag_ecalBadCalibFilter"
    };
	for (const auto& filterN : filterList) {
		fChain->SetBranchStatus(filterN.c_str(), true);
	    fChain->SetBranchAddress(filterN.c_str(), &filterVals[filterN], &filterTBranches[filterN]);
	} 
}

Long64_t NanoTree::getEntries() const {
    return fChain->GetEntries();
}

Long64_t NanoTree::getEntriesRuns() const {
    return fChainRuns->GetEntries();
}

Int_t NanoTree::getEntry(Long64_t entry) {
    return fChain->GetEntry(entry);
}

Long64_t NanoTree::loadEntry(Long64_t entry) {
    if (!fChain) return -1;
    Long64_t centry = fChain->LoadTree(entry);
    if (centry < 0) return centry;
    if (fChain->GetTreeNumber() != fCurrent_) {
        fCurrent_ = fChain->GetTreeNumber();
    }
    return centry;
}

Long64_t NanoTree::loadEntryRuns(Long64_t entry) {
    if (!fChainRuns) return -1;
    Long64_t centry = fChainRuns->LoadTree(entry);
    if (centry < 0) return centry;
    if (fChainRuns->GetTreeNumber() != fCurrentRuns_) {
        fCurrentRuns_ = fChainRuns->GetTreeNumber();
    }
    return centry;
}

