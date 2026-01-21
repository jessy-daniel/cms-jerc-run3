#include "RunMultiJet.h"
#include "HistCutflow.h"
#include "Helper.h"
   
// Constructor implementation
RunMultiJet::RunMultiJet(GlobalFlag& globalFlags)
    :globalFlags_(globalFlags) {
}


auto RunMultiJet::Run(std::shared_ptr<NanoTree>& nanoT, TFile *fout) -> int{
    fout->cd();

    // Add some channel specific branches
    nanoT->fChain->SetBranchStatus("HLT_ZeroBias",1);

	//----------------------------------
	// Set trigger list
	//----------------------------------
    std::vector<std::string> patterns;
    patterns = { "HLT_PFJet*", "HLT_DiPFJet*"};
    trigList_  = Helper::GetMatchingBranchNames(nanoT->fChain->GetTree(), patterns);

    if (trigList_.empty()) {
        std::cerr << "No triggers found for channel: MultiJet" << '\n';
        exit(EXIT_FAILURE);
    }
	for (const auto& trigN : trigList_) {
		nanoT->fChain->SetBranchStatus(trigN.c_str(), true);
	    nanoT->fChain->SetBranchAddress(trigN.c_str(), &trigVals_[trigN], &trigTBranches_[trigN]);
	} 

    TTree* newTree = nanoT->fChain->GetTree()->CloneTree(0);
    newTree->SetCacheSize(Helper::tTreeCatchSize);
    Long64_t nentries = nanoT->getEntries();
    std::cout << "\nSample has "<<nentries << " entries" << '\n';

    //------------------------------------
    // Cutflow histograms
    //------------------------------------
    std::vector<std::string> cuts = { "NanoAOD", "Trigger"};
    auto h1EventInCutflow = std::make_unique<HistCutflow>("h1EventInCutflow", cuts, fout->mkdir("Cutflow"));
    
    
    //--------------------------------
    //Event loop
    //--------------------------------
    double totalTime = 0.0;
	auto startClock = std::chrono::high_resolution_clock::now();
    Helper::initProgress();
	for(Long64_t i= 0; i < nentries; i++){
        //if(i>100000) break;
        Helper::printProgress(i, nentries, startClock, totalTime);
        
        Long64_t entry = nanoT->loadEntry(i);
        h1EventInCutflow->fill("NanoAOD");

		for (const auto& trigN : trigList_) {
            if (!trigTBranches_[trigN]) continue;
		    trigTBranches_[trigN]->GetEntry(entry);//Read only content of HLT branches from NanoTree
		    if (trigVals_[trigN]) {
            	nanoT->fChain->GetTree()->GetEntry(entry);// Then read content of ALL branches
                h1EventInCutflow->fill("Trigger");
            	newTree->Fill();
		        break; // Event passes if any trigger is true
		    }
		}
    }

    TTree* newTreeRuns = nullptr;

    if (nanoT->fChainRuns && nanoT->fChainRuns->GetTree()) {
        newTreeRuns = nanoT->fChainRuns->GetTree()->CloneTree(0);
        newTreeRuns->SetDirectory(fout);

        Long64_t nentriesRuns = nanoT->getEntriesRuns();
        for (Long64_t i = 0; i < nentriesRuns; i++) {
            Long64_t entry = nanoT->loadEntryRuns(i);
            nanoT->fChainRuns->GetTree()->GetEntry(entry);
            newTreeRuns->Fill();
        }
    } 
    else {
        std::cout << "[INFO] No Runs tree found — skipping Runs cloning." << std::endl;
    }

    Helper::printCutflow(h1EventInCutflow->getHistogram());
    std::cout<<"nEvents_Skim = "<<newTree->GetEntries()<<'\n';
    if (newTreeRuns) {
        std::cout << "nRuns_Skim = " << newTreeRuns->GetEntries() << '\n';
    } 
    else {
        std::cout << "nRuns_Skim = 0 (Runs tree missing)" << '\n';
    }
    std::cout << "Output file path = "<<fout->GetName()<<'\n';
    fout->Write();

    return EXIT_SUCCESS;
}
