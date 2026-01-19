#include "RunGamJet.h"
#include "HistCutflow.h"
#include "Helper.h"
   
// Constructor implementation
RunGamJet::RunGamJet(GlobalFlag& globalFlags)
    :globalFlags_(globalFlags) {
}

auto RunGamJet::Run(std::shared_ptr<NanoTree>& nanoT, TFile *fout) -> int{
    fout->cd();

	//----------------------------------
	// Add channel specific branches 
	//----------------------------------
    nanoT->fChain->SetBranchStatus("Photon_energyErr",1);
   	nanoT->fChain->SetBranchStatus("Photon_eta",1);
   	nanoT->fChain->SetBranchStatus("Photon_hoe",1);
   	nanoT->fChain->SetBranchStatus("Photon_phi",1);
   	nanoT->fChain->SetBranchStatus("Photon_pt",1);
   	nanoT->fChain->SetBranchStatus("Photon_r9",1);
   	nanoT->fChain->SetBranchStatus("Photon_cutBased",1);
   	nanoT->fChain->SetBranchStatus("Photon_jetIdx",1);
   	nanoT->fChain->SetBranchStatus("Photon_seedGain",1);
   	nanoT->fChain->SetBranchStatus("Photon_pixelSeed",1);
   	nanoT->fChain->SetBranchStatus("nPhoton",1);
   	nanoT->fChain->SetBranchStatus("Photon_eCorr",1); 
   	nanoT->fChain->SetBranchStatus("Photon_mass",1);
   	nanoT->fChain->SetBranchStatus("Photon_sieie",1);
   	nanoT->fChain->SetBranchStatus("Photon_pfChargedIso",1);
  	if(globalFlags_.isMC){
      nanoT->fChain->SetBranchStatus("GenIsolatedPhoton_*",true);
      nanoT->fChain->SetBranchStatus("nGenIsolatedPhoton",true);
    }

    // Additional flags branches
    nanoT->fChain->SetBranchStatus("Flag_HBHENoiseFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_HBHENoiseIsoFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_CSCTightHaloFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_CSCTightHaloTrkMuUnvetoFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_CSCTightHalo2015Filter",1);
    nanoT->fChain->SetBranchStatus("Flag_globalTightHalo2016Filter",1);
    nanoT->fChain->SetBranchStatus("Flag_HcalStripHaloFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_hcalLaserEventFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_EcalDeadCellBoundaryEnergyFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_ecalLaserCorrFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_trkPOGFilters",1);
    nanoT->fChain->SetBranchStatus("Flag_chargedHadronTrackResolutionFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_muonBadTrackFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_BadChargedCandidateFilter",1);
    nanoT->fChain->SetBranchStatus("Flag_BadChargedCandidateSummer16Filter",1);
    nanoT->fChain->SetBranchStatus("Flag_BadPFMuonSummer16Filter",1);
    nanoT->fChain->SetBranchStatus("Flag_trkPOG_manystripclus53X",1);
    nanoT->fChain->SetBranchStatus("Flag_trkPOG_toomanystripclus53X",1);
    nanoT->fChain->SetBranchStatus("Flag_trkPOG_logErrorTooManyClusters",1);
    nanoT->fChain->SetBranchStatus("Flag_METFilters",1);
    
	//----------------------------------
	// Set trigger list
	//----------------------------------
    if(globalFlags_.is2022){
        trigList_ = { 
        	"HLT_Photon300_NoHE",                                             
			"HLT_Photon20",                                                   
			"HLT_Photon33",                                                   
			"HLT_Photon50",                                                   
			"HLT_Photon75",                                                   
			"HLT_Photon90",                                                   
			"HLT_Photon120",                                                  
			"HLT_Photon150",                                                  
			"HLT_Photon175",                                                  
            "HLT_Photon200",                                                  
            "HLT_Photon30EB_TightID_TightIso",                                
            "HLT_Photon100EB_TightID_TightIso",                               
            "HLT_Photon110EB_TightID_TightIso",                               
            "HLT_Photon120EB_TightID_TightIso",                               
            "HLT_Photon100EBHE10",                                            
            "HLT_Photon100EEHE10",                                            
            "HLT_Photon100EE_TightID_TightIso",                               
            "HLT_Photon50_R9Id90_HE10_IsoM",                                  
            "HLT_Photon75_R9Id90_HE10_IsoM",                                  
            "HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_CaloMJJ300_PFJetsMJJ400DEta3",
            "HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_CaloMJJ400_PFJetsMJJ600DEta3",
            "HLT_Photon90_R9Id90_HE10_IsoM",                                  
            "HLT_Photon120_R9Id90_HE10_IsoM",                                 
            "HLT_Photon165_R9Id90_HE10_IsoM",                                 
            "HLT_Photon35_TwoProngs35",                                       
            "HLT_Photon60_R9Id90_CaloIdL_IsoL",                               
            "HLT_Photon60_R9Id90_CaloIdL_IsoL_DisplacedIdL",                  
            "HLT_Photon60_R9Id90_CaloIdL_IsoL_DisplacedIdL_PFHT350MinPFJet15",
            "HLT_Photon20_HoverELoose",                                       
            "HLT_Photon30_HoverELoose",                                       
            "HLT_Photon50_R9Id90_HE10_IsoM_EBOnly_PFJetsMJJ300DEta3_PFMET50", 
            "HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_PFJetsMJJ300DEta3",         
            "HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_PFJetsMJJ600DEta3"         
        };
    }

    if(globalFlags_.is2023 || globalFlags_.is2024 || globalFlags_.is2025){
        trigList_ = { 
             "HLT_Photon300_NoHE",                                           
             "HLT_Photon33",                                          
             "HLT_Photon50",                                          
             "HLT_Photon75",                                          
             "HLT_Photon90",                                          
             "HLT_Photon120",                                         
             "HLT_Photon150",                                         
             "HLT_Photon175",                                         
             "HLT_Photon200",                                         
             "HLT_Photon30EB_TightID_TightIso",                       
             "HLT_Photon40EB_TightID_TightIso",
             "HLT_Photon45EB_TightID_TightIso",
             "HLT_Photon50EB_TightID_TightIso",                       
             "HLT_Photon75EB_TightID_TightIso",                       
             "HLT_Photon90EB_TightID_TightIso",                       
             "HLT_Photon110EB_TightID_TightIso",                      
             "HLT_Photon130EB_TightID_TightIso",                      
             "HLT_Photon150EB_TightID_TightIso",                      
             "HLT_Photon175EB_TightID_TightIso",                      
             "HLT_Photon200EB_TightID_TightIso",                      
             "HLT_Photon100EBHE10",                                   
             "HLT_Photon50_R9Id90_HE10_IsoM",                         
             "HLT_Photon75_R9Id90_HE10_IsoM",                         
             "HLT_Photon90_R9Id90_HE10_IsoM",                         
             "HLT_Photon120_R9Id90_HE10_IsoM",                        
             "HLT_Photon165_R9Id90_HE10_IsoM",                        
             "HLT_Photon35_TwoProngs35",                              
             "HLT_Photon60_R9Id90_CaloIdL_IsoL_DisplacedIdL_PFHT350", 
             "HLT_Photon60_R9Id90_CaloIdL_IsoL_DisplacedIdL_PFHT380", 
             "HLT_Photon60_R9Id90_CaloIdL_IsoL_DisplacedIdL_PFHT400", 
             "HLT_Photon20_HoverELoose",                              
             "HLT_Photon30_HoverELoose",                              
             "HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_PFJetsMJJ300DEta3",
             "HLT_Photon32_OneProng32_M50To105"                      

        };
    }

    if (trigList_.empty()) {
        std::cerr << "No triggers found for channel: GamJet" << '\n';
        exit(EXIT_FAILURE);
    }
	for (const auto& trigN : trigList_) {
		nanoT->fChain->SetBranchStatus(trigN.c_str(), true);
	    nanoT->fChain->SetBranchAddress(trigN.c_str(), &trigVals_[trigN], &trigTBranches_[trigN]);
	} 

    // *** GUARD #1: make sure fChain actually has a TTree ***
    if (nanoT->fChain->GetNtrees() == 0) {
        std::cerr << "[RunGamJet] ERROR: fChain has no trees (GetNtrees()==0)."
                  << " Did loadTree() fail to add any files?\n";
        return EXIT_FAILURE;
    }

    // *** GUARD #2: force the chain to load its first tree ***
    Long64_t firstEntry = nanoT->fChain->LoadTree(0);
    if (firstEntry < 0) {
        std::cerr << "[RunGamJet] ERROR: LoadTree(0) returned " << firstEntry
                  << ". No valid tree to read from.\n";
        return EXIT_FAILURE;
    }

    // *** GUARD #3: GetTree() must be non‐null ***
    TTree* baseTree = nanoT->fChain->GetTree();
    if (!baseTree) {
        std::cerr << "[RunGamJet] ERROR: fChain->GetTree() is nullptr. Cannot CloneTree.\n";
        return EXIT_FAILURE;
    }

    // Now it’s safe:
    TTree* newTree = baseTree->CloneTree(0);
    if (!newTree) {
        std::cerr << "[RunGamJet] ERROR: CloneTree(0) returned nullptr. Check your input TTree.\n";
        return EXIT_FAILURE;
    }
    newTree->SetDirectory(fout);
    newTree->SetCacheSize(Helper::tTreeCatchSize);
    Long64_t nentries = nanoT->getEntries();
    std::cout << "\nSample has "<<nentries << " entries" << '\n';

    //------------------------------------
    // Cutflow histograms
    //------------------------------------
    std::vector<std::string> cuts = { "NanoAOD", "Filter", "Trigger"};
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

        bool passFilter = true;
		for (const auto& filterN : nanoT->filterList) {
            if (!nanoT->filterTBranches[filterN]) continue;
		    nanoT->filterTBranches[filterN]->GetEntry(entry);//Read only content of MET Filter branches from NanoTree
		    if (!nanoT->filterVals[filterN]) {
                passFilter = false;
		        break; 
		    }
		}
        if(!passFilter) continue;
        h1EventInCutflow->fill("Filter");

        bool passTrigger = false;
		for (const auto& trigN : trigList_) {
            if (!trigTBranches_[trigN]) continue;
		    trigTBranches_[trigN]->GetEntry(entry);//Read only content of HLT branches from NanoTree
		    if (trigVals_[trigN]) {
                passTrigger = true;
		        break; 
		    }
		}
        
        if(!passTrigger) continue;
        h1EventInCutflow->fill("Trigger");
        
        //Now read all of the branches and fill the tree
        nanoT->fChain->GetTree()->GetEntry(entry);
        newTree->Fill();
    }

    std::cout << "\nNow process the RunsTree \n";
    // Force loading the first tree in the chain.
    Long64_t centry = nanoT->fChainRuns->LoadTree(0);
    if (centry < 0) {
        std::cerr << "Error: fChainRuns->LoadTree(0) returned " << centry 
                  << ", no valid Runs tree found. Skipping Runs processing." << std::endl;
        return EXIT_FAILURE; // or handle the error as needed.
    }

    TTree* runsTree = nanoT->fChainRuns->GetTree();
    if (!runsTree) {
        std::cerr << "Error: No valid Runs tree found in the TChain. Skipping Runs processing." << std::endl;
        return EXIT_FAILURE;
    }
    TTree* newTreeRuns = runsTree->CloneTree(0);

    newTreeRuns->SetDirectory(fout);
    Long64_t nentriesRuns = nanoT->getEntriesRuns();
    for (Long64_t i = 0; i < nentriesRuns; i++) {
       Long64_t entry = nanoT->loadEntryRuns(i);
       nanoT->fChainRuns->GetTree()->GetEntry(entry);
       newTreeRuns->Fill();      
    }
    Helper::printCutflow(h1EventInCutflow->getHistogram());
    std::cout<<"nEvents_Skim = "<<newTree->GetEntries()<<'\n';
    std::cout<<"nRuns_Skim = "<<newTreeRuns->GetEntries()<<'\n';
    std::cout << "Output file path = "<<fout->GetName()<<'\n';
    fout->cd();
    fout->Write();


    return EXIT_SUCCESS; 
}
