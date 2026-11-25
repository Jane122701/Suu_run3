# cmsRun configuration for the Run 3 S_uu -> chi chi ntuple production (clusteringAnalyzerAll_*).
#
# One parametrised cfg replaces the per-sample cfgs that createClusterCfgTemplate.py writes in the Run 2 area.
# The sample conventions (runType keywords, systematic groups, slimmed selection, PDF switches, output names)
# follow the Run 2 template.
#
# Examples (run from $CMSSW_BASE/src after cmsenv):
#   cmsRun preprocess/clustering/test/clusteringAnalyzer_cfg.py year=2022 runType=QCDMC2000toInf systematic=nom \
#          inputFiles=root://cmsxrootd.fnal.gov//store/mc/... maxEvents=1000
#   cmsRun preprocess/clustering/test/clusteringAnalyzer_cfg.py year=2022 runType=dataC inputFiles=... maxEvents=1000
#
# runType keywords understood by the analyzer (as in Run 2):
#   contains "data"            -> data (dataC, dataD, ...)
#   contains "MC" or "Suu"     -> simulation (JER, pileup weights, b-tag weights, generator information)
#   contains "QCDMC_Pt"        -> Pythia pt-binned QCD (no LHE weights)
#   contains "TTto"/"TTTo"/"TTJets" -> top pt reweighting
#   contains "Suu"             -> signal

import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing("analysis")
options.register("year", "2022", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "Run 3 era: 2022, 2022EE, 2023, 2023BPix or 2024")
options.register("runType", "QCDMC2000toInf", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "sample name, e.g. dataC, QCDMC2000toInf, QCDMC_Pt_1000to1400, TTto4QMC, ST_tW-top_4QMC, WJetsMC, Suu8_chi3")
options.register("systematic", "nom", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "systematic group as in Run 2: nom, JEC1, JEC2 or JER")
options.register("ETcut", "300", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "AK8 ET cut variant of the analyzer plugin: 100, 150, 200, 250, 300, 350, 400 or nocut (Run 2 production used 300)")
options.register("bTagEffFile", "", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "MC b-tag efficiency map (FileInPath); default preprocess/clustering/data/btaggingEffMaps/btag_efficiency_map_<group>_combined_<year>.root")
options.register("metFilterProcess", "PAT", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "process name of the TriggerResults holding the MET filter flags (PAT for MC and re-MiniAOD data, RECO for prompt-reco data)")
options.register("deepTauVersion", "2018v2p5", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "DeepTau version of the tau veto IDs; 2018v2p5 is stored in all Run 3 MiniAOD, 2017v2p1 (Run 2 analyzer) only up to 2023")
options.register("lumiJSON", "", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "optional golden JSON for local data tests (CRAB applies the lumi mask itself)")
options.register("verbose", False, VarParsing.multiplicity.singleton, VarParsing.varType.bool, "analyzer printouts")
options.register("output", "", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "output ROOT file name (default: Run 2 naming convention)")
options.register("reportEvery", 1000, VarParsing.multiplicity.singleton, VarParsing.varType.int, "MessageLogger report frequency")
options.parseArguments()

year       = options.year
sample     = options.runType
systematic = options.systematic

# ---------------------------------------------------------------------------------------------------------------------
# era-dependent inputs: POG correctionlib files copied to preprocess/clustering/data/POG (see data/POG_sources.tsv)
# ---------------------------------------------------------------------------------------------------------------------
era_dirs = {
    "2022":     "2022_Summer22",
    "2022EE":   "2022_Summer22EE",
    "2023":     "2023_Summer23",
    "2023BPix": "2023_Summer23BPix",
    "2024":     "2024_Summer24",
}
jet_veto_map_names = {
    "2022":     "Summer22_23Sep2023_RunCD_V1",
    "2022EE":   "Summer22EE_23Sep2023_RunEFG_V1",
    "2023":     "Summer23Prompt23_RunC_V1",
    "2023BPix": "Summer23BPixPrompt23_RunD_V1",
    "2024":     "Summer24Prompt24_RunBCDEFGHI_V1",
}
pu_files = {
    "2022":     "puWeights.json.gz",
    "2022EE":   "puWeights.json.gz",
    "2023":     "puWeights.json.gz",
    "2023BPix": "puWeights.json.gz",
    "2024":     "puWeights_CDEFGHI.json.gz",
}
if year not in era_dirs:
    raise ValueError("year must be one of %s, got %s" % (sorted(era_dirs), year))

data_dir = "preprocess/clustering/data"
pog_dir  = "%s/POG" % data_dir
era      = era_dirs[year]

isData   = "data" in sample
isMC     = ("MC" in sample) or ("Suu" in sample)
isSignal = "Suu" in sample
if isData == isMC:
    raise ValueError("runType %s must contain either 'data' or one of 'MC' / 'Suu'" % sample)

# ---------------------------------------------------------------------------------------------------------------------
# sample-dependent switches, as in the Run 2 createClusterCfgTemplate.py
# ---------------------------------------------------------------------------------------------------------------------
includeAllBranches = False
useOptimizedWP     = False
skipReclustering   = False
runSideband        = False
includeTightBTag   = False
runNoiseFilters    = True

def get_pdf_set_type(sample):
    if "QCDMC_Pt" in sample: return "relic"
    for hess_samp in ["QCD", "TTJets", "Suu", "WJets"]:
        if hess_samp in sample: return "hessian"
    raise ValueError("ERROR: correct PDF set type not determined for %s." % sample)

# systematic groups -> list of systematicType values run in this job
if isData:
    if systematic != "nom":
        raise ValueError("data is only run with systematic=nom")
    systematic_list = ["nom"]
elif isSignal:
    if   systematic == "nom":  systematic_list = ["nom", "JER_eta193", "JER_193eta25", "JER", "JEC_Absolute", "JEC_BBEC1"]
    elif systematic == "JEC1": systematic_list = ["JEC_FlavorQCD", "JEC_RelativeBal", "JEC_BBEC1_year", "JEC_Absolute_year", "JEC_RelativeSample_year", "JEC"]
    else: raise ValueError("signal samples are run with systematic=nom or JEC1")
else:
    if   systematic == "nom":  systematic_list = ["nom"]
    elif systematic == "JEC1": systematic_list = ["JEC_FlavorQCD", "JEC_RelativeBal", "JEC_BBEC1_year"]
    elif systematic == "JEC2": systematic_list = ["JEC_Absolute_year", "JEC_RelativeSample_year", "JEC_Absolute", "JEC_BBEC1", "JEC"]
    elif systematic == "JER":  systematic_list = ["JER_eta193", "JER_193eta25", "JER"]
    else: raise ValueError("systematic must be nom, JEC1, JEC2 or JER")

# b-tag efficiency map group
bTagEff_file = options.bTagEffFile
if isMC and bTagEff_file == "":
    if   "QCDMC" in sample:                                              bTagSF_sample = "QCDMC"
    elif "TTTo" in sample or "TTto" in sample or "TTJetsMC" in sample:   bTagSF_sample = "TTbarMC"
    elif "ST_" in sample:                                                bTagSF_sample = "STMC"
    elif "Suu" in sample:                                                bTagSF_sample = "SuuToChiChi"
    elif "WJets" in sample:                                              bTagSF_sample = "WJetsMC"
    elif "WW" in sample or "ZZ" in sample:                               bTagSF_sample = "TTbarMC"
    else: raise ValueError("ERROR: b tagging SF sample is not found. Sample = %s" % sample)
    bTagEff_file = "%s/btaggingEffMaps/btag_efficiency_map_%s_combined_%s.root" % (data_dir, bTagSF_sample, year)

# ---------------------------------------------------------------------------------------------------------------------
# process
# ---------------------------------------------------------------------------------------------------------------------
process = cms.Process("analysis")

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = options.reportEvery
process.options = cms.untracked.PSet(wantSummary = cms.untracked.bool(True))
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(options.maxEvents))

process.source = cms.Source("PoolSource", fileNames = cms.untracked.vstring(options.inputFiles))
if options.lumiJSON != "":
    import FWCore.PythonUtilities.LumiList as LumiList
    process.source.lumisToProcess = LumiList.LumiList(filename = options.lumiJSON).getVLuminosityBlockRange()

if options.output != "":
    output_name = options.output
elif isSignal:
    output_name = "%s_%s_combined.root" % (sample, year)
else:
    output_name = "clusteringAnalyzer_%s_%s_%s_output.root" % (sample, year, systematic)
process.TFileService = cms.Service("TFileService", fileName = cms.string(output_name))

process.leptonVeto = cms.EDFilter("leptonVeto",
    muonCollection     = cms.InputTag("slimmedMuons"),
    electronCollection = cms.InputTag("slimmedElectrons"),
    metCollection      = cms.InputTag("slimmedMETs"),
    tauCollection      = cms.InputTag("slimmedTaus"),
    deepTauVersion     = cms.string(options.deepTauVersion),
)

process.noiseFilter = cms.EDFilter("noiseFilter",
    year          = cms.string(year),
    metFilterBits = cms.InputTag("TriggerResults", "", options.metFilterProcess),
)

sequence = process.leptonVeto
if runNoiseFilters:
    sequence = sequence * process.noiseFilter

for syst in systematic_list:
    suffixes = [""] if syst == "nom" else ["_up", "_down"]
    for suffix in suffixes:
        systematicType = syst + suffix

        # nominal runs and signal keep the loose selection, the JEC/JER variations of the backgrounds use the slimmed one
        slimmedSelection = False if (syst == "nom" or isSignal) else True

        doPDF = isMC and syst == "nom"
        if "QCDMC_Pt" in sample or "ST_" in sample or "TTTo" in sample or "TTto" in sample:
            doPDF = False

        module = cms.EDAnalyzer("clusteringAnalyzerAll_%s" % options.ETcut,
            runType        = cms.string(sample),
            year           = cms.string(year),
            systematicType = cms.string(systematicType),

            # BEST NN (trained on Run 2, same graph and scaler file as the Run 2 analysis)
            BESTname  = cms.string("BESTGraph"),
            BESTpath  = cms.FileInPath("%s/BEST_models/constantgraph_combine_0.pb" % data_dir),
            BESTscale = cms.FileInPath("%s/BESTScalerParameters_all_mass_combine_0.txt" % data_dir),

            # POG corrections
            PUfile_path    = cms.FileInPath("%s/LUM/%s/%s" % (pog_dir, era, pu_files[year])),
            jercFile_AK4   = cms.FileInPath("%s/JME/%s/jet_jerc.json.gz" % (pog_dir, era)),
            jercFile_AK8   = cms.FileInPath("%s/JME/%s/fatJet_jerc.json.gz" % (pog_dir, era)),
            jetIDFile      = cms.FileInPath("%s/JME/%s/jetid.json.gz" % (pog_dir, era)),
            jetVetoMapFile = cms.FileInPath("%s/JME/%s/jetvetomaps.json.gz" % (pog_dir, era)),
            jetVetoMapName = cms.string(jet_veto_map_names[year]),
            jetVetoMapType = cms.string("jetvetomap"),

            # collections: Run 3 uses PUPPI for AK4 and AK8 jets; the JEC is applied inside the analyzer
            fatJetCollection   = cms.InputTag("slimmedJetsAK8"),
            jetCollection      = cms.InputTag("slimmedJetsPuppi"),
            muonCollection     = cms.InputTag("slimmedMuons"),
            electronCollection = cms.InputTag("slimmedElectrons"),
            metCollection      = cms.InputTag("slimmedMETs"),
            tauCollection      = cms.InputTag("slimmedTaus"),
            deepTauVersion     = cms.string(options.deepTauVersion),
            pileupCollection   = cms.InputTag("slimmedAddPileupInfo"),
            genEventInfoTag    = cms.InputTag("generator"),
            bits               = cms.InputTag("TriggerResults", "", "HLT"),

            includeAllBranches = cms.bool(includeAllBranches),
            slimmedSelection   = cms.bool(slimmedSelection),
            useOptimizedWP     = cms.bool(useOptimizedWP),
            skipReclustering   = cms.bool(skipReclustering),
            verbose            = cms.bool(options.verbose),
            runSideband        = cms.bool(runSideband),
            includeTightBTag   = cms.bool(includeTightBTag),
            doPDF              = cms.bool(doPDF),
        )
        if isMC:
            module.genPartCollection  = cms.InputTag("prunedGenParticles")
            module.genParticles       = cms.InputTag("genParticles")
            module.packedGenParticles = cms.InputTag("packedGenParticles")
            module.bTagSF_path        = cms.FileInPath("%s/BTV/%s/btagging.json.gz" % (pog_dir, era))
            module.bTagEff_path       = cms.FileInPath(bTagEff_file)
        if doPDF or ("QCDMC_Pt" in sample and syst == "nom"):
            module.pdfSetType = cms.string(get_pdf_set_type(sample))

        label = "clusteringAnalyzerAll_%s_%s" % (options.ETcut, systematicType)
        setattr(process, label, module)
        sequence = sequence * module

process.p = cms.Path(sequence)
