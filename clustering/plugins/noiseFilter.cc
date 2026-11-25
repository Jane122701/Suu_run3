// -*- C++ -*-
//
// Package:    noiseFilter/noiseFilter
// Class:      noiseFilter
// 
/**\class noiseFilter noiseFilter.cc noiseFilter/noiseFilter/plugins/noiseFilter.cc

 Description: filter that applies met/noise filters for a given Run 2 era

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Ethan Cannaert
//         Created:  Mon, 05 Mar 2026
//
//

// CMSSW Framework
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include <iostream>
#include <string>
// class declaration
//

class noiseFilter : public edm::stream::EDFilter<> {
   public:
      explicit noiseFilter(const edm::ParameterSet&);
      ~noiseFilter();
      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      virtual void beginStream(edm::StreamID) override;
      virtual bool filter(edm::Event&, const edm::EventSetup&) override;
      virtual void endStream() override;

      edm::EDGetTokenT<edm::TriggerResults> metFilters_;
	  std::string year;

      std::vector<std::string> targetFilters_;
};

noiseFilter::noiseFilter(const edm::ParameterSet& iConfig)
{
    // MET filter decisions are in the TriggerResults of the MiniAOD step: process "PAT" for MC and re-MiniAOD data,
    // "RECO" for prompt-reco data. Set in the cfg.
    metFilters_  = consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("metFilterBits"));
    year    = iConfig.getParameter<std::string>("year");

    // MET filters for Run 3 (2022-2024) data and MC.
    // Relative to the Run 2 UL list used in the Run 2 analyzer: Flag_HBHENoiseFilter and Flag_HBHENoiseIsoFilter are
    // dropped and Flag_hfNoisyHitsFilter is added (JME Run 3 recommendation; check against the JME MET filter twiki).
    targetFilters_ = {
        "Flag_goodVertices",
        "Flag_globalSuperTightHalo2016Filter",
        "Flag_EcalDeadCellTriggerPrimitiveFilter",
        "Flag_BadPFMuonFilter",
        "Flag_BadPFMuonDzFilter",
        "Flag_hfNoisyHitsFilter",
        "Flag_eeBadScFilter",
        "Flag_ecalBadCalibFilter"
    };

}


noiseFilter::~noiseFilter()
{
}


bool noiseFilter::filter(edm::Event& iEvent, const edm::EventSetup& iSetup)
{

    edm::Handle<edm::TriggerResults> metFilters;
    iEvent.getByToken(metFilters_, metFilters);

    const edm::TriggerNames &names = iEvent.triggerNames(*metFilters);
    
    // Loop over met filters
    for (unsigned int i = 0; i < metFilters->size(); i++) 
    {
        std::string name = names.triggerName(i);
        bool passed = metFilters->accept(i);
        
        if (!passed) // noise filter == 0 is bad, want to skip these events
        {
            if( std::find(targetFilters_.begin(), targetFilters_.end(), name) != targetFilters_.end() )  // if this filter passed (=good) and is one of the necessary ones for this era, set true 
            {
                std::cout << "Event failed due to " << name << " filter." << std::endl;
                return false;
            }
        }
    } 
    return true;
}

void
noiseFilter::beginStream(edm::StreamID)
{
}

void
noiseFilter::endStream() 
{
}

 void
noiseFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}
//define this as a plug-in
DEFINE_FWK_MODULE(noiseFilter);
