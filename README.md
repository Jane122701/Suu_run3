# Suu_run3 — Run 3 ntuple production for the S_uu → χχ all-hadronic search

Run 3 (2022–2024) version of the `clusteringAnalyzerAll_*` ntuple step of the Run 2 analysis
(`github.com/Jane122701/Suu_run2`, `preprocess/plugins/`). It runs on MiniAOD in `CMSSW_15_0_22`
and writes the same `tree_<systematic>` trees as the Run 2 analyzer.

This directory is `$CMSSW_BASE/src/preprocess`. The compiled package is `preprocess/clustering`.

## Layout

| Path | Contents |
|---|---|
| `clustering/plugins/clusteringAnalyzerAll.cc` | The analyzer. One source file registers the eight plugins `clusteringAnalyzerAll_100`, `_150`, `_200`, `_250`, `_300`, `_350`, `_400`, `_nocut`; they differ only in the AK8 ET cut, as in Run 2. |
| `clustering/plugins/calculateBtagEfficiencyMaps.cc` | Jet counts for the MC b-tagging efficiency maps. |
| `clustering/plugins/leptonVeto.cc`, `noiseFilter.cc` | Event filters that run before the analyzer. |
| `clustering/plugins/BEST*.{cc,h}`, `CacheHandler.*`, `sortJets.*` | BEST NN evaluation and clustering helpers (from Run 2). |
| `clustering/test/clusteringAnalyzer_cfg.py` | cmsRun configuration for the ntuples (data and MC, all eras). |
| `clustering/test/btagEfficiency_cfg.py`, `makeBtagEfficiencyMaps.C` | Efficiency-map production. |
| `clustering/data/POG/` | JME, BTV and LUM correction files (correctionlib JSON). |
| `clustering/data/POG_sources.tsv`, `POG_SHA256SUMS` | Source path, version and checksum of every correction file. |
| `clustering/data/BEST_models/`, `BESTScalerParameters_all_mass_combine_0.txt` | BEST network and input scaling, copied unchanged from the Run 2 area. |

## Setup

```bash
cd <area>/CMSSW_15_0_22/src        # el8_amd64_gcc12
cmsenv
git clone <this repository> preprocess
scram b -j 8
```

## Eras

`year` selects the corrections. Data and MC of one era use the same value.

| `year` | Data | MC campaign | JEC | JER | b-tagger |
|---|---|---|---|---|---|
| `2022` | Run2022C–D | Run3Summer22 | Summer22_22Sep2023_V4 | Summer22_22Sep2023_JRV2 | deepJet |
| `2022EE` | Run2022E–G | Run3Summer22EE | Summer22EE_22Sep2023_V4 | Summer22EE_22Sep2023_JRV2 | deepJet |
| `2023` | Run2023C | Run3Summer23 | Summer23Prompt23_V4 | Summer23Prompt23_RunCv1234_JRV3 | deepJet |
| `2023BPix` | Run2023D | Run3Summer23BPix | Summer23BPixPrompt23_V4 | Summer23BPixPrompt23_RunD_JRV3 | deepJet |
| `2024` | Run2024C–I | RunIII2024Summer24 | Summer24Prompt24_V5 | Summer24Prompt24_JRV2 | UParTAK4 |

The correction files are the `latest` versions in `/cvmfs/cms-griddata.cern.ch/cat/metadata/{JME,BTV,LUM}/`
on 2026-11-24. To update them, copy the new files over the old ones, regenerate `POG_sources.tsv` and
`POG_SHA256SUMS`, and change the tags in the constructor of `clusteringAnalyzerAll.cc` and
`calculateBtagEfficiencyMaps.cc` if the tag names changed.

## Running

Ntuples (from `$CMSSW_BASE/src`):

```bash
# MC, nominal
cmsRun preprocess/clustering/test/clusteringAnalyzer_cfg.py year=2022 runType=QCDMC2000toInf systematic=nom \
       inputFiles=root://cmsxrootd.fnal.gov//store/mc/... maxEvents=-1
# MC, systematic groups (as in Run 2): JEC1, JEC2, JER
cmsRun preprocess/clustering/test/clusteringAnalyzer_cfg.py year=2022 runType=QCDMC2000toInf systematic=JEC1 inputFiles=...
# data
cmsRun preprocess/clustering/test/clusteringAnalyzer_cfg.py year=2022 runType=dataC inputFiles=...
```

`runType` uses the Run 2 keywords: a name containing `data` is data; `MC` or `Suu` is simulation;
`QCDMC_Pt` is the Pythia pt-binned QCD; `TTto`, `TTTo` or `TTJets` switches on the top-pt weight; `Suu` is signal.
Other options: `ETcut` (default `300`, the plugin used in the Run 2 production), `output`, `bTagEffFile`,
`metFilterProcess`, `lumiJSON`, `verbose`. Output names follow Run 2
(`clusteringAnalyzer_<runType>_<year>_<systematic>_output.root`, `<runType>_<year>_combined.root` for signal).

As in Run 2, the nominal and signal jobs keep the full selection (at least 2 AK8 jets, HT > 1500 GeV) and all
other systematic jobs use the slimmed selection (`slimmedSelection`: at least 3 AK8 jets, HT > 1600 GeV), so
their trees are smaller.

MC needs a b-tagging efficiency map for its sample group and era,
`clustering/data/btaggingEffMaps/btag_efficiency_map_<group>_combined_<year>.root`
(`<group>` = `QCDMC`, `TTbarMC`, `STMC`, `WJetsMC`, `SuuToChiChi`). These maps are not in the repository yet;
they have to be produced from the Run 3 MC first:

```bash
cmsRun preprocess/clustering/test/btagEfficiency_cfg.py year=2022 runType=QCDMC2000toInf inputFiles=... maxEvents=-1
# hadd the outputs of all samples of the group, then
root -l -b -q 'preprocess/clustering/test/makeBtagEfficiencyMaps.C("btagCounts_QCDMC_2022.root","preprocess/clustering/data/btaggingEffMaps/btag_efficiency_map_QCDMC_combined_2022.root")'
```

The efficiency lookup is the Run 2 one: a jet above the last pt bin with non-zero efficiency of its eta row reads
that last bin. A map with too few jets (bins with efficiency exactly 0 or 1) therefore gives non-finite
`bTag_eventWeight_*` for some events; the weights are stored as computed, as in Run 2.

## Validation

On 2026-11-24 the ntuple cfg was run with `cmsRun` on one MiniAOD file per era, 2000–3000 MC events
(QCD-4Jets HT-2000) and 20000 JetMET data events each: nominal MC and data for all five eras, the `JEC1`,
`JEC2` and `JER` groups for 2022 and `JEC1` for 2024. All jobs ended with exit code 0 and filled trees. The
b-tagging efficiency maps used in these tests were made from the same few thousand events and are not in the
repository. No production has been run.

## Differences from the Run 2 analyzer

The event selection, superjet clustering, BEST evaluation and tree branches are unchanged. The changes are the
ones forced by the Run 3 data format and POG recommendations:

- **AK4 jets are PUPPI jets** (`slimmedJetsPuppi`); Run 2 used CHS jets with the pileup jet ID. There is no
  pileup jet ID for PUPPI jets: `passesJetPUID` is kept and is always false.
- **JEC and JER come from the JME correctionlib files and are applied inside the analyzer.** The nominal
  L1L2L3(+residual) correction is applied to the uncorrected MiniAOD jets (AK4, AK8 and the AK8 soft-drop
  subjets); in Run 2 this was done by `updateJetCollection` in the cfg with the global-tag JEC. The uncertainty
  sources and the JER resolution and scale factors are read from the same files; the Run 2 text files and the
  global-tag JER are no longer used. The source grouping of each `systematicType` is unchanged. JER for AK8 jets
  uses the AK8PFPuppi payload.
- **Rho** is `fixedGridRhoFastjetAll`, the definition the Run 3 JEC and JER parametrisations use (Run 2 code:
  `fixedGridRhoAll`).
- **Jet ID** is the Run 3 PUPPI `TightLeptonVeto` ID evaluated from the JME `jetid` file. The eta acceptance is
  the Run 2 2017–2018 one (|η| < 2.5 for AK4 and the two leading AK8 jets, |η| ≤ 1.4 for further AK8 jets).
- **Jet veto maps** are the JME correctionlib maps, type `jetvetomap`; the per-jet flags `AK4_fails_veto_map`
  and `AK8_fails_veto_map` are filled as before.
- **b-tagging**: working points and scale factors are read from the BTV file of the era (`<tagger>_mujets` for
  b and c jets, `<tagger>_light` for light jets). 2022–2023 use DeepJet as in Run 2. BTV provides no DeepJet
  scale factors for 2024, so 2024 uses UParT AK4. New branch `AK4_bTag_disc` holds the score of the era tagger;
  `AK4_DeepJet_disc` still holds the DeepJet score.
- **Triggers**: `HLT_PFJet500` and `HLT_PFHT1050` for all eras.
- **MET filters** (`noiseFilter`): `Flag_HBHENoiseFilter` and `Flag_HBHENoiseIsoFilter` are dropped and
  `Flag_hfNoisyHitsFilter` is added.
- **Tau IDs** in the lepton veto and the tau counters use DeepTau 2018v2p5 for all eras (cfg option
  `deepTauVersion`). Run 2 used 2017v2p1, which 2024 MiniAOD does not store; the working points are unchanged.
  Muon and electron IDs are the Run 2 ones (electrons: `mvaEleID-Fall17-*-V2`).
- **No L1 prefiring weight and no HEM veto** in Run 3. `prefiringWeight_*` are filled with 1 and the `*_isHEM`
  branches with false, so the tree layout stays the same.
- **Pileup weights** use the LUM file and golden-JSON tag of the era (2024: `Collisions24_CDEFGHI_goldenJSON`).
- The plugin is built with the default optimisation flags (the Run 2 `BuildFile.xml` had `-O0 -g3`).

## Open items

- Produce the MC b-tagging efficiency maps for every sample group and era.
- CRAB configurations (dataset lists, lumi masks, splitting) are not part of this repository yet.
- The BEST network and its scaling file are the Run 2 training.
- The MET filter list, the 2024 b-tagger choice and the DeepTau version should be confirmed against the current
  JME, BTV and TAU pages.
- LHE weight positions used for the scale variations were checked against the Run3Summer22 QCD-4Jets HT sample
  only (see the comments in the PDF section of the analyzer); the other MadGraph samples and the signal need the
  same check before `doPDF` results are used.
