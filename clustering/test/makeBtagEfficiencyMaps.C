// Makes the MC b-tagging efficiency maps read by clusteringAnalyzerAll_* (cfg parameter bTagEff_path) from the
// jet-count histograms written by the calculateBtagEfficiencyMaps analyzer.
//
// Input : ROOT file with the directory calculateBtagEfficiencyMaps/ (output of test/btagEfficiency_cfg.py; for a full
//         sample, hadd the CRAB outputs of all files of the sample group first).
// Output: ROOT file with h_effbJets_tight, h_effcJets_tight, h_effLightJets_tight, h_effbJets_med, h_effcJets_med,
//         h_effLightJets_med (efficiency = b-tagged jets / all jets in each pt-eta bin; binomial errors).
//
// Usage : root -l -b -q 'makeBtagEfficiencyMaps.C("btagCounts_QCDMC_2022.root","btag_efficiency_map_QCDMC_combined_2022.root")'

#include "TFile.h"
#include "TH2F.h"
#include "TString.h"
#include <iostream>

void makeBtagEfficiencyMaps(const char* inputFile, const char* outputFile, const char* dirName = "calculateBtagEfficiencyMaps")
{
    TFile* fin = TFile::Open(inputFile, "READ");
    if(!fin || fin->IsZombie()) { std::cout << "ERROR: cannot open " << inputFile << std::endl; return; }

    const char* flavours[3]    = {"TruebJets", "TruecJets", "LightJets"};   // count histogram names: h_n<flavour>_<wp>[_btagged]
    const char* outFlavours[3] = {"bJets",     "cJets",     "LightJets"};   // map names:             h_eff<flavour>_<wp>
    const char* wps[2]         = {"tight", "med"};

    TFile* fout = TFile::Open(outputFile, "RECREATE");
    for(int iwp = 0; iwp < 2; iwp++)
    {
        for(int ifl = 0; ifl < 3; ifl++)
        {
            TH2F* h_all    = (TH2F*)fin->Get(Form("%s/h_n%s_%s", dirName, flavours[ifl], wps[iwp]));
            TH2F* h_tagged = (TH2F*)fin->Get(Form("%s/h_n%s_%s_btagged", dirName, flavours[ifl], wps[iwp]));
            if(!h_all || !h_tagged) { std::cout << "ERROR: missing count histograms for " << flavours[ifl] << " " << wps[iwp] << " in " << inputFile << std::endl; return; }

            h_all->Sumw2();
            h_tagged->Sumw2();
            TH2F* h_eff = (TH2F*)h_tagged->Clone(Form("h_eff%s_%s", outFlavours[ifl], wps[iwp]));
            h_eff->SetTitle(Form("b-tagging efficiency (%s WP) for true %s; jet p_{T} [GeV];jet eta", wps[iwp], outFlavours[ifl]));
            h_eff->Divide(h_tagged, h_all, 1., 1., "B");
            h_eff->SetDirectory(fout);
            std::cout << h_eff->GetName() << ": " << h_tagged->GetEntries() << " tagged / " << h_all->GetEntries() << " jet entries" << std::endl;
        }
    }
    fout->Write();
    fout->Close();
    fin->Close();
    std::cout << "Wrote " << outputFile << std::endl;
}
