# cmsRun configuration for the Run 3 MC b-tagging efficiency counts (calculateBtagEfficiencyMaps).
#
# The output holds the jet-count histograms; test/makeBtagEfficiencyMaps.C turns the (hadd-ed) counts of a sample group
# into data/btaggingEffMaps/btag_efficiency_map_<group>_combined_<year>.root, which clusteringAnalyzer_cfg.py reads for MC.
#
# Example (from $CMSSW_BASE/src after cmsenv):
#   cmsRun preprocess/clustering/test/btagEfficiency_cfg.py year=2022 runType=QCDMC2000toInf inputFiles=... maxEvents=-1

import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing("analysis")
options.register("year", "2022", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "Run 3 era: 2022, 2022EE, 2023, 2023BPix or 2024")
options.register("runType", "QCDMC2000toInf", VarParsing.multiplicity.singleton, VarParsing.varType.string, "MC sample name")
options.register("output", "", VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "output ROOT file name (default: Run 2 naming convention)")
options.register("reportEvery", 1000, VarParsing.multiplicity.singleton, VarParsing.varType.int, "MessageLogger report frequency")
options.parseArguments()

era_dirs = {
    "2022":     "2022_Summer22",
    "2022EE":   "2022_Summer22EE",
    "2023":     "2023_Summer23",
    "2023BPix": "2023_Summer23BPix",
    "2024":     "2024_Summer24",
}
year = options.year
if year not in era_dirs:
    raise ValueError("year must be one of %s, got %s" % (sorted(era_dirs), year))
pog_dir = "preprocess/clustering/data/POG"
era     = era_dirs[year]

process = cms.Process("btagEff")
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = options.reportEvery
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(options.maxEvents))
process.source = cms.Source("PoolSource", fileNames = cms.untracked.vstring(options.inputFiles))

output_name = options.output if options.output != "" else "btagCounts_%s_%s.root" % (options.runType, year)
process.TFileService = cms.Service("TFileService", fileName = cms.string(output_name))

process.calculateBtagEfficiencyMaps = cms.EDAnalyzer("calculateBtagEfficiencyMaps",
    runType       = cms.string(options.runType),
    year          = cms.string(year),
    jetCollection = cms.InputTag("slimmedJetsPuppi"),
    jercFile_AK4  = cms.FileInPath("%s/JME/%s/jet_jerc.json.gz" % (pog_dir, era)),
    jetIDFile     = cms.FileInPath("%s/JME/%s/jetid.json.gz" % (pog_dir, era)),
    bTagSF_path   = cms.FileInPath("%s/BTV/%s/btagging.json.gz" % (pog_dir, era)),
)

process.p = cms.Path(process.calculateBtagEfficiencyMaps)
