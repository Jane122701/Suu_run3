////////////////////////////HELP////////////////////////////////
// Run 3 version of calculateBtagEfficiencyMaps.
//
// Counts true b / c / light AK4 PUPPI jets, and how many of them pass the medium and tight working points of the
// era b-tagger, in bins of jet pt and eta. The MC b-tagging efficiency maps read by clusteringAnalyzerAll_*
// (bTagEff_path) are the ratios of these histograms, made with test/makeBtagEfficiencyMaps.C.
//
// Jets are treated as in the Run 3 clusteringAnalyzerAll: nominal JEC from the JME POG JSON applied to the uncorrected
// MiniAOD jet, JER smearing, TightLeptonVeto jet ID, |eta| < 2.5. As in the Run 2 analyzer, every jet is entered once
// per JEC source group (pt shifted up by the uncertainty), once for JER up, once for JER down and once at nominal,
// so that the maps cover the jet pt values seen in the systematic variations.
/////////////////////////////////////////////////////////////////

#include <memory>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include "TH2.h"
#include "TRandom3.h"

#include "correction.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/JetReco/interface/GenJet.h"

using correction::CorrectionSet;
using correction::Correction;
using correction::CompoundCorrection;

class calculateBtagEfficiencyMaps : public edm::one::EDAnalyzer<edm::one::SharedResources>
{
public:
   explicit calculateBtagEfficiencyMaps(const edm::ParameterSet&);
private:
   void analyze(const edm::Event&, const edm::EventSetup&) override;
   bool passesJetID(const pat::Jet& jet) const;
   double getJECUncertaintyFromSources(const std::vector<std::string>& sources, double pt, double eta) const;
   std::vector<std::string> sourcesForSystematic(const std::string& systematic) const;

   template <typename CorrRef>
   static double evalNamedCorrection(const CorrRef& corr, const std::map<std::string, double>& values)
   {
      std::vector<correction::Variable::Type> in;
      for(const auto& v : corr->inputs())
      {
         auto it = values.find(v.name());
         if(it == values.end()) throw cms::Exception("calculateBtagEfficiencyMaps") << "correction " << corr->name() << " needs input " << v.name() << ", which is not provided";
         in.emplace_back(it->second);
      }
      return corr->evaluate(in);
   }

   std::string runType;
   std::string year;
   std::string jecTag, jerTag, bTagAlgo;

   edm::EDGetTokenT<std::vector<pat::Jet>> jetToken_;
   edm::EDGetTokenT<double> m_rho_token;

   std::unique_ptr<CorrectionSet> jerc_cset_AK4, jetID_cset, bTag_cset;
   CompoundCorrection::Ref JEC_AK4;
   Correction::Ref JER_res_AK4, JER_sf_AK4, JER_sfUnc_AK4, jetID_AK4;
   std::map<std::string, Correction::Ref> JEC_map_AK4;

   double bTag_wp_med = -1., bTag_wp_tight = -1.;

   TH2F *h_nLightJets_tight;
   TH2F *h_nTruebJets_tight;
   TH2F *h_nTruecJets_tight;
   TH2F *h_nLightJets_tight_btagged;
   TH2F *h_nTruebJets_tight_btagged;
   TH2F *h_nTruecJets_tight_btagged;

   TH2F *h_nLightJets_med;
   TH2F *h_nTruebJets_med;
   TH2F *h_nTruecJets_med;
   TH2F *h_nLightJets_med_btagged;
   TH2F *h_nTruebJets_med_btagged;
   TH2F *h_nTruecJets_med_btagged;

   TRandom3 *randomNum = new TRandom3(); // for JERs

   std::vector<std::string> systematics;
};

calculateBtagEfficiencyMaps::calculateBtagEfficiencyMaps(const edm::ParameterSet& iConfig)
{
   usesResource("TFileService");

   runType = iConfig.getParameter<std::string>("runType");
   year    = iConfig.getParameter<std::string>("year");

   // same era tags as clusteringAnalyzerAll
   if(year == "2022")          { jecTag = "Summer22_22Sep2023_V4";   jerTag = "Summer22_22Sep2023_JRV2";         bTagAlgo = "deepJet"; }
   else if(year == "2022EE")   { jecTag = "Summer22EE_22Sep2023_V4"; jerTag = "Summer22EE_22Sep2023_JRV2";       bTagAlgo = "deepJet"; }
   else if(year == "2023")     { jecTag = "Summer23Prompt23_V4";     jerTag = "Summer23Prompt23_RunCv1234_JRV3"; bTagAlgo = "deepJet"; }
   else if(year == "2023BPix") { jecTag = "Summer23BPixPrompt23_V4"; jerTag = "Summer23BPixPrompt23_RunD_JRV3";  bTagAlgo = "deepJet"; }
   else if(year == "2024")     { jecTag = "Summer24Prompt24_V5";     jerTag = "Summer24Prompt24_JRV2";           bTagAlgo = "UParTAK4"; }
   else throw cms::Exception("calculateBtagEfficiencyMaps") << "Incorrect year: " << year << ". Valid Run 3 values: 2022, 2022EE, 2023, 2023BPix, 2024.";

   jetToken_   = consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jetCollection"));
   m_rho_token = consumes<double>(edm::InputTag("fixedGridRhoFastjetAll"));

   jerc_cset_AK4 = CorrectionSet::from_file(iConfig.getParameter<edm::FileInPath>("jercFile_AK4").fullPath());
   JEC_AK4       = jerc_cset_AK4->compound().at(jecTag + "_MC_L1L2L3Res_AK4PFPuppi");
   JER_res_AK4   = jerc_cset_AK4->at(jerTag + "_MC_PtResolution_AK4PFPuppi");
   JER_sf_AK4    = jerc_cset_AK4->at(jerTag + "_MC_ScaleFactor_AK4PFPuppi");
   JER_sfUnc_AK4 = jerc_cset_AK4->at(jerTag + "_MC_SFUncertainty_AK4PFPuppi");

   jetID_cset = CorrectionSet::from_file(iConfig.getParameter<edm::FileInPath>("jetIDFile").fullPath());
   jetID_AK4  = jetID_cset->at("AK4PUPPI_TightLeptonVeto");

   bTag_cset     = CorrectionSet::from_file(iConfig.getParameter<edm::FileInPath>("bTagSF_path").fullPath());
   bTag_wp_med   = bTag_cset->at(bTagAlgo + "_wp_values")->evaluate({"M"});
   bTag_wp_tight = bTag_cset->at(bTagAlgo + "_wp_values")->evaluate({"T"});
   std::cout << "calculateBtagEfficiencyMaps " << year << ": tagger " << bTagAlgo << ", medium WP " << bTag_wp_med << ", tight WP " << bTag_wp_tight << std::endl;

   // JEC source groups, as in the Run 3 clusteringAnalyzerAll
   systematics =
   {
      "JEC",
      "BBEC1_year",
      "Absolute_year",
      "RelativeSample_year",
      "FlavorQCD",
      "RelativeBal",
      "AbsoluteCal",
      "AbsolutePU",
      "Absolute",
      "AbsoluteTheory",
      "AbsoluteScale",
      "Fragmentation",
      "AbsoluteMPFBias",
      "RelativeFSR"
   };
   for(const auto& systematic : systematics)
   {
      for(const auto& source : sourcesForSystematic(systematic))
      {
         if(JEC_map_AK4.count(source) == 0) JEC_map_AK4.emplace(source, jerc_cset_AK4->at(jecTag + "_MC_" + source + "_AK4PFPuppi"));
      }
   }

   // pt binning as in Run 2 (40 bins up to 6 TeV); eta covers the Run 3 jet acceptance |eta| < 2.5
   const int NBINSX = 40;
   const double PTMAX = 6000.;
   const int NBINSY = 20;
   const double ETAMAX = 2.5;

   edm::Service<TFileService> fs;
   h_nLightJets_tight = fs->make<TH2F>("h_nLightJets_tight" ,"total number of true light jets; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruebJets_tight = fs->make<TH2F>("h_nTruebJets_tight" ,"total number of true b jets; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruecJets_tight = fs->make<TH2F>("h_nTruecJets_tight" ,"total number of true c jets; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nLightJets_tight_btagged = fs->make<TH2F>("h_nLightJets_tight_btagged" ,"total number of true light jets that are b-tagged; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruebJets_tight_btagged = fs->make<TH2F>("h_nTruebJets_tight_btagged" ,"total number of true b jets that are b-tagged; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruecJets_tight_btagged = fs->make<TH2F>("h_nTruecJets_tight_btagged" ,"total number of true c jets that are b-tagged; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);

   h_nLightJets_med = fs->make<TH2F>("h_nLightJets_med" ,"total number of true light jets; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruebJets_med = fs->make<TH2F>("h_nTruebJets_med" ,"total number of true b jets; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruecJets_med = fs->make<TH2F>("h_nTruecJets_med" ,"total number of true c jets; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nLightJets_med_btagged = fs->make<TH2F>("h_nLightJets_med_btagged" ,"total number of true light jets that are b-tagged; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruebJets_med_btagged = fs->make<TH2F>("h_nTruebJets_med_btagged" ,"total number of true b jets that are b-tagged; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
   h_nTruecJets_med_btagged = fs->make<TH2F>("h_nTruecJets_med_btagged" ,"total number of true c jets that are b-tagged; jet p_{T} [GeV];jet eta",   NBINSX ,0, PTMAX, NBINSY, -ETAMAX, ETAMAX);
}

// uncertainty sources of each reduced JEC source group (same grouping and matching order as clusteringAnalyzerAll)
std::vector<std::string> calculateBtagEfficiencyMaps::sourcesForSystematic(const std::string& systematic) const
{
   if      (systematic == "JEC")                                             return {"Total"};
   else if (systematic.find("BBEC1_year") != std::string::npos)              return {"RelativeJEREC1","RelativePtEC1","RelativeStatEC"};
   else if (systematic.find("Absolute_year") != std::string::npos)           return {"AbsoluteStat","RelativeStatFSR","TimePtEta"};
   else if (systematic.find("RelativeSample_year") != std::string::npos)     return {"RelativeSample"};
   else if (systematic.find("FlavorQCD") != std::string::npos)               return {"FlavorQCD"};
   else if (systematic.find("RelativeBal") != std::string::npos)             return {"RelativeBal"};
   else if (systematic.find("AbsoluteCal") != std::string::npos)             return {"SinglePionECAL","SinglePionHCAL"};
   else if (systematic.find("AbsolutePU") != std::string::npos)              return {"PileUpDataMC","PileUpPtRef"};
   else if (systematic.find("AbsoluteTheory") != std::string::npos)          return {"AbsoluteScale", "Fragmentation","AbsoluteMPFBias","RelativeFSR"};
   else if (systematic.find("AbsoluteScale") != std::string::npos)           return {"AbsoluteScale"};
   else if (systematic.find("AbsoluteMPFBias") != std::string::npos)         return {"AbsoluteMPFBias"};
   else if (systematic.find("Absolute") != std::string::npos)                return {"AbsoluteMPFBias", "AbsoluteScale", "Fragmentation","PileUpDataMC","PileUpPtRef","RelativeFSR","SinglePionECAL","SinglePionHCAL"};
   else if (systematic.find("Fragmentation") != std::string::npos)           return {"Fragmentation"};
   else if (systematic.find("RelativeFSR") != std::string::npos)             return {"RelativeFSR"};
   throw cms::Exception("calculateBtagEfficiencyMaps") << "unknown JEC systematic " << systematic;
}

// quadrature sum of the relative JEC uncertainties of the given sources
double calculateBtagEfficiencyMaps::getJECUncertaintyFromSources(const std::vector<std::string>& sources, double pt, double eta) const
{
   double uncert = 0.0;
   for(const auto& source : sources)
   {
      const double value = fabs(evalNamedCorrection(JEC_map_AK4.at(source), { {"JetEta", eta}, {"JetPt", pt} }));
      uncert += pow(value,2);
   }
   return sqrt(uncert);
}

// AK4 PUPPI TightLeptonVeto ID from the JME POG jetid JSON, evaluated on the unmodified MiniAOD jet
bool calculateBtagEfficiencyMaps::passesJetID(const pat::Jet& jet) const
{
   std::vector<correction::Variable::Type> in;
   for(const auto& v : jetID_AK4->inputs())
   {
      const std::string n = v.name();
      if      (n == "eta")            in.emplace_back(double(jet.eta()));
      else if (n == "chHEF")          in.emplace_back(double(jet.chargedHadronEnergyFraction()));
      else if (n == "neHEF")          in.emplace_back(double(jet.neutralHadronEnergyFraction()));
      else if (n == "chEmEF")         in.emplace_back(double(jet.chargedEmEnergyFraction()));
      else if (n == "neEmEF")         in.emplace_back(double(jet.neutralEmEnergyFraction()));
      else if (n == "muEF")           in.emplace_back(double(jet.muonEnergyFraction()));
      else if (n == "chMultiplicity") in.emplace_back(int(jet.chargedMultiplicity()));
      else if (n == "neMultiplicity") in.emplace_back(int(jet.neutralMultiplicity()));
      else if (n == "multiplicity")   in.emplace_back(int(jet.chargedMultiplicity() + jet.neutralMultiplicity()));
      else throw cms::Exception("calculateBtagEfficiencyMaps") << "jet ID correction needs unknown input " << n;
   }
   return jetID_AK4->evaluate(in) > 0.5;
}

void calculateBtagEfficiencyMaps::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
   edm::Handle<std::vector<pat::Jet> > smallJets;
   iEvent.getByToken(jetToken_, smallJets);

   edm::Handle<double> rho;
   iEvent.getByToken(m_rho_token, rho);

   const double runNumber = iEvent.id().run();

   for(auto iJet = smallJets->begin(); iJet != smallJets->end(); iJet++)
   {
      if( !(iJet->isPFJet()) ) continue;

      // nominal Run 3 JEC on the uncorrected jet
      const auto rawP4 = iJet->correctedP4("Uncorrected");
      const double jec = evalNamedCorrection(JEC_AK4, { {"JetA", iJet->jetArea()}, {"JetEta", rawP4.eta()}, {"JetPhi", rawP4.phi()}, {"JetPt", rawP4.pt()}, {"Rho", *rho}, {"run", runNumber} });
      const double jet_pt  = jec*rawP4.pt();
      const double jet_eta = rawP4.eta();

      if( (jet_pt < 10. ) || ( fabs(jet_eta) >= 2.5 )) continue;   //don't even bother with these jets, lost causes

      // JER smearing factors (nominal, up, down), same method as clusteringAnalyzerAll
      double AK4_JER_corr_factor = 1.0, AK4_JER_corr_factor_up = 1.0, AK4_JER_corr_factor_down = 1.0;
      {
         const std::map<std::string, double> values = { {"JetEta", jet_eta}, {"JetPt", jet_pt}, {"Rho", *rho} };
         const double sigmaJER  = evalNamedCorrection(JER_res_AK4, values);
         const double sJER      = evalNamedCorrection(JER_sf_AK4, values);
         const double sJER_unc  = evalNamedCorrection(JER_sfUnc_AK4, values);
         const double sJER_up   = sJER + sJER_unc;
         const double sJER_down = sJER - sJER_unc;

         const reco::GenJet *genJet = iJet->genJet();
         if( genJet)   // try the first technique
         {
            AK4_JER_corr_factor      = 1 + (sJER - 1     )*(jet_pt-genJet->pt())/jet_pt;
            AK4_JER_corr_factor_up   = 1 + (sJER_up - 1  )*(jet_pt-genJet->pt())/jet_pt;
            AK4_JER_corr_factor_down = 1 + (sJER_down - 1)*(jet_pt-genJet->pt())/jet_pt;
         }
         else   // if no gen jet is matched, try the second technique
         {
            randomNum->SetSeed( abs(static_cast<int>(iJet->phi()*1e4)) );
            double JERrand = randomNum->Gaus(0.0, sigmaJER);
            AK4_JER_corr_factor      = std::max(0., 1 + JERrand*sqrt(std::max(pow(sJER,2)-1,0.)));   //want to make sure this is truncated at 0
            AK4_JER_corr_factor_up   = std::max(0., 1 + JERrand*sqrt(std::max(pow(sJER_up,2)-1,0.)));
            AK4_JER_corr_factor_down = std::max(0., 1 + JERrand*sqrt(std::max(pow(sJER_down,2)-1,0.)));
         }
      }

      std::vector<double> correction_shifts;
      for(const auto& systematic: systematics)
      {
         const double AK4_JEC_shift = getJECUncertaintyFromSources(sourcesForSystematic(systematic), jet_pt, jet_eta);
         correction_shifts.push_back(1.00+AK4_JEC_shift);
      }
      if(AK4_JER_corr_factor > 0.)
      {
         correction_shifts.push_back(AK4_JER_corr_factor_up/AK4_JER_corr_factor);     // nominal JEC and up JER
         correction_shifts.push_back(AK4_JER_corr_factor_down/AK4_JER_corr_factor);   // nominal JEC and down JER
      }
      correction_shifts.push_back(1.0);                                               // nominal JEC and nominal JER

      if( !passesJetID(*iJet) ) continue;

      const int jetHadronFlavor = iJet->hadronFlavour();

      // discriminator of the era b-tagger (deepJet for 2022-2023, UParTAK4 for 2024)
      double bTagScore = iJet->bDiscriminator("pfDeepFlavourJetTags:probb") + iJet->bDiscriminator("pfDeepFlavourJetTags:probbb")+ iJet->bDiscriminator("pfDeepFlavourJetTags:problepb");
      if(bTagAlgo == "UParTAK4") bTagScore = iJet->bDiscriminator("pfUnifiedParticleTransformerAK4JetTags:probb") + iJet->bDiscriminator("pfUnifiedParticleTransformerAK4JetTags:probbb")+ iJet->bDiscriminator("pfUnifiedParticleTransformerAK4JetTags:problepb");

      for(const auto correction_shift: correction_shifts)
      {
         if( (AK4_JER_corr_factor*correction_shift*jet_pt) < 30) continue;

         const double corr_pt = correction_shift*jet_pt;
         if(jetHadronFlavor == 0)   //light jets
         {
            h_nLightJets_tight->Fill(corr_pt,jet_eta);
            h_nLightJets_med->Fill(corr_pt,jet_eta);
            if(bTagScore > bTag_wp_tight) h_nLightJets_tight_btagged->Fill(corr_pt,jet_eta);
            if(bTagScore > bTag_wp_med)   h_nLightJets_med_btagged->Fill(corr_pt,jet_eta);
         }
         else if(jetHadronFlavor == 4) //charm jets
         {
            h_nTruecJets_tight->Fill(corr_pt,jet_eta);
            h_nTruecJets_med->Fill(corr_pt,jet_eta);
            if(bTagScore > bTag_wp_tight) h_nTruecJets_tight_btagged->Fill(corr_pt,jet_eta);
            if(bTagScore > bTag_wp_med)   h_nTruecJets_med_btagged->Fill(corr_pt,jet_eta);
         }
         else if(jetHadronFlavor == 5) // b jets
         {
            h_nTruebJets_tight->Fill(corr_pt,jet_eta);
            h_nTruebJets_med->Fill(corr_pt,jet_eta);
            if(bTagScore > bTag_wp_tight) h_nTruebJets_tight_btagged->Fill(corr_pt,jet_eta);
            if(bTagScore > bTag_wp_med)   h_nTruebJets_med_btagged->Fill(corr_pt,jet_eta);
         }
      }
   }
}

DEFINE_FWK_MODULE(calculateBtagEfficiencyMaps);
