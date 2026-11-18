
#include "RunZeeJet.h"
#include "RunZmmJet.h"
#include "RunGamJet.h"
#include "RunMultiJet.h"
#include "RunWqqm.h"
#include "RunWqqe.h"

#include <iostream>
#include <filesystem>
#include <sys/stat.h>
#include <nlohmann/json.hpp>

#include <TFile.h>

#include "NanoTree.h"
#include "GlobalFlag.h"


namespace fs = std::filesystem;

int main(int argc, char* argv[]){
	// Check if no arguments are provided
    if (argc == 1) {
      std::cerr << "Error: No arguments provided. Use -h for help." << std::endl;
      return 1;
    }
    
    std::string jsonDir = "input/nano_files/";
    std::vector<std::string> jsonFiles;
    
    //Read only FilesNano_*.json files in the directory
    for (const auto& entry : fs::directory_iterator(jsonDir)) {
      if (entry.path().extension() == ".json" && 
    	entry.path().filename().string().rfind("FilesNano_", 0) == 0) {
    	jsonFiles.push_back(entry.path().string());
      }
    }
    
    if (jsonFiles.empty()) {
      std::cerr << "No JSON files found in directory: " << jsonDir << std::endl;
      return 1;
    }
    
    nlohmann::json js;
    std::string outName;
    
    //--------------------------------
    // Parse command-line options
    //--------------------------------
    int opt;
    while ((opt = getopt(argc, argv, "o:h")) != -1) {
      switch (opt) {
        case 'o':
          outName = optarg;
          break;
        case 'h':
          // Loop through each JSON file and print available keys
          for (const auto& jsonFile : jsonFiles) {
            std::ifstream file(jsonFile);
            if (!file.is_open()) {
              std::cerr << "Could not open file: " << jsonFile << std::endl;
              continue;
            }
            
            try {
              js = nlohmann::json::parse(file);
            } catch (const std::exception& e) {
              std::cerr << "EXCEPTION: Error parsing file: " << jsonFile << std::endl;
              std::cerr << e.what() << std::endl;
              continue;
            }
            
            std::cout << "\nFor file: " << jsonFile << std::endl;
            for (auto& element : js.items()) {
              std::cout <<"./runMain -o "<<element.key()<<"_Skim_1of100.root" << std::endl;
            }
          }
          return 0;
        default:
          std::cerr << "Use -h for help" << std::endl;
          return 1;
      }
    }
    
    cout<<"\n--------------------------------------"<<endl;
    cout<<" Set GlobalFlag.cpp"<<endl;
    cout<<"--------------------------------------"<<endl;
    GlobalFlag globalFlag(outName);
    globalFlag.printFlag();
    
    
    cout<<"\n--------------------------------------"<<endl;
    cout<<" Set and load NanoTree"<<endl;
    cout<<"--------------------------------------"<<endl;
    std::shared_ptr<NanoTree> nanoT = std::make_shared<NanoTree>(globalFlag);
    nanoT->setInput(outName);
    nanoT->loadInput();
    nanoT->loadInputJson(jsonDir);
    nanoT->loadJobFileNames();
    nanoT->loadTree();

	  std::string outDir = "output";
    mkdir(outDir.c_str(), S_IRWXU);
    auto fout = std::make_unique<TFile>((outDir + "/" + outName).c_str(), "RECREATE");

    if (globalFlag.isZeeJet) {
        std::cout << "==> Running ZeeJet" << std::endl;
        auto zeeJet = std::make_unique<RunZeeJet>(globalFlag);
        zeeJet->Run(nanoT, fout.get());
    }
    if (globalFlag.isZmmJet) {
        std::cout << "==> Running ZmmJet" << std::endl;
        auto zmmJet = std::make_unique<RunZmmJet>(globalFlag);
        zmmJet->Run(nanoT, fout.get());
    }
    if (globalFlag.isGamJet) {
        std::cout << "==> Running GamJet" << std::endl;
        auto gamJet = std::make_unique<RunGamJet>(globalFlag);
        gamJet->Run(nanoT, fout.get());
    }
    if (globalFlag.isMultiJet) {
        std::cout << "==> Running MultiJet" << std::endl;
        auto multiJet = std::make_unique<RunMultiJet>(globalFlag);
        multiJet->Run(nanoT, fout.get());
    }
    if (globalFlag.isWqqm) {
        std::cout << "==> Running Wqqm" << std::endl;
        auto wqqm = std::make_unique<RunWqqm>(globalFlag);
        wqqm->Run(nanoT, fout.get());
    }
    if (globalFlag.isWqqe) {
        std::cout << "==> Running Wqqe" << std::endl;
        auto wqqe = std::make_unique<RunWqqe>(globalFlag);
        wqqe->Run(nanoT, fout.get());
    }
	return 0;
}
