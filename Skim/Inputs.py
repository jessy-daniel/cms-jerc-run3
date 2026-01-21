# Skim/Inputs.py

# Directory where skimmed files will be stored
outSkimDir = "/eos/cms/store/group/phys_b2g/HHbbgg/jdaniel/JME/Skimmed_files_datasets" 

# Years and Channels to process
Year2022 = {
    "MC": [
        "GJets", "QCD"
    ],
    "Data": [
        #"2022B", "2022C", "2022D", "2022E", "2022F", "2022G"
    ]
}

Year2023 = {
    "MC": [
        "GJets", "QCD", "Other"
    ],
    "Data": [
        #"2023B", "2023C", "2023D"
    ]
}

Year2024 = {
    #"MC": [
        #"GJets", "QCD"
        #"TTtoLNu2Q"
    #],
    "MCSummer24": [
        "GJets", 
        "QCD"
    ],
    "Data": [
        #"2024FCCv2DIv3", 
        #"2024A", "2024B", "2024C", "2024D", "2024E", "2024F", "2024G", "2024H", "2024I"
        "2024C", "2024D", "2024E", "2024F", "2024G", "2024H", "2024I",
        #"2024C_ZB", "2024D_ZB", "2024E_ZB", "2024F_ZB", "2024G_ZB", "2024H_ZB", "2024I_ZB" # ZB datasets for dijet
    ],
    #"DataReprocessing": [
    #    "2024C", "2024D", "2024E",
    #    "2024C_ZB", "2024D_ZB", "2024E_ZB" # ZB datasets for dijet
    #]
}

Year2025 = {
    "MCWinter25": [
        "GJets", "QCD"
    ],
    "Data": [
        "2025B", "2025C"
        #"2025C"
    ],
}

Years = {}
#Years['2022'] = Year2022
#Years['2023'] = Year2023
Years['2024'] = Year2024
#Years['2025'] = Year2025


Channels = [
    #'ZeeJet',
    #'ZmmJet',
    'GamJet',
    #'Wqqm',
    #'Wqqe',
    #'WqqDiLep',
    #'MultiJet',
]

# VOMS Proxy path (adjust as needed)
vomsProxy = "x509up_u140910"

# Events per job
eventsPerJobMC = 2e6  # Number of events per job for MC
eventsPerJobData = 8e6  # Number of events per job for Data

tmpSubDir = "tmpSub_Gamjet"
