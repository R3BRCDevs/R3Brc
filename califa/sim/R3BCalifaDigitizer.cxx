/******************************************************************************
 *   Copyright (C) 2019 GSI Helmholtzzentrum für Schwerionenforschung GmbH    *
 *   Copyright (C) 2019-2023 Members of R3B Collaboration                     *
 *                                                                            *
 *             This software is distributed under the terms of the            *
 *                 GNU General Public Licence (GPL) version 3,                *
 *                    copied verbatim in the file "LICENSE".                  *
 *                                                                            *
 * In applying this license GSI does not waive the privileges and immunities  *
 * granted to it by virtue of its status as an Intergovernmental Organization *
 * or submit itself to any jurisdiction.                                      *
 ******************************************************************************/

#include "R3BCalifaDigitizer.h"
#include "FairRootManager.h"
#include "R3BCalifa.h"
#include "R3BCalifaCrystalCalData.h"
#include "R3BCalifaPoint.h"
#include "TArrayD.h"
#include "TClonesArray.h"
#include "TMath.h"
#include "TRandom.h"
#include <iostream>
#include <stdlib.h>
#include "R3BLogger.h"

R3BCalifaDigitizer::R3BCalifaDigitizer()
    : FairTask("R3B CALIFA Digitizer")
    , fCalifaPointDataCA(NULL)
    , fRealConfig(0)
    , fNonUniformity(0.) // perfect crystals
    , fResolution(0.)    // perfect crystals
    , fComponentRes(0.)  // perfect crystals
    , fThreshold(0.)     // no threshold
{
}

void R3BCalifaDigitizer::SetParContainers()
{
    if (fRealConfig)
    {
        FairRuntimeDb* rtdb = FairRuntimeDb::instance();
        LOG_IF(error, !rtdb) << "R3BCalifaDigitizer::FairRuntimeDb not opened!";

        fSim_Par = dynamic_cast<R3BCalifaCrystalPars4Sim*>(rtdb->getContainer("califaCrystalPars4Sim"));
        if (!fSim_Par)
        {
            LOG(error) << "R3BCalifaDigitizer::Init() Couldn't get handle on "
                          "califaCrystalPars4Sim container";
        }
        else
        {
            LOG(info) << "R3BCalifaDigitizer:: califaCrystalPars4Sim container opened";
        }
    }
}

void R3BCalifaDigitizer::SetParameter()
{
    if (fRealConfig)
    {
        //--- Parameter Container ---
        fNumCrystals = fSim_Par->GetNumCrystals();          // Number of Crystals
        fNumberOfParams = fSim_Par->GetNumParameters4Sim(); // Number of Parameters

        fSim_Par->printParams();

        LOG(info) << "R3BCalifaDigitizer:: Max Crystal ID " << fNumCrystals;
        LOG(info) << "R3BCalifaDigitizer:: Nb of parameters used in the Simulation " << fNumberOfParams;
    }
}

InitStatus R3BCalifaDigitizer::Init()
{
    LOG(info) << "R3BCalifaDigitizer::Init ";

    FairRootManager* rootManager = FairRootManager::Instance();
    if (!rootManager)
        LOG(fatal) << "Init: No FairRootManager";

    fCalifaPointDataCA = dynamic_cast<TClonesArray*>(rootManager->GetObject("CrystalPoint"));
    if (!fCalifaPointDataCA)
    {
        LOG(fatal) << "Init: No CrystalPoint CA";
        return kFATAL;
    }

    rootManager->RegisterAny(R3BCalifaCrystalCalData::default_container_name, fCalifaCryCalData, true);

    SetParameter();
    return kSUCCESS;
}

void R3BCalifaDigitizer::Exec(Option_t* option)
{
    // Reset entries in output arrays, local arrays
    Reset();

    // Reading the Input -- Point data --
    Int_t nHits = fCalifaPointDataCA->GetEntries();
    LLOG(debug) << "processing "<< nHits <<" R3BCalifaPoint.";
    R3BCalifaPoint** pointData = NULL;
    pointData = new R3BCalifaPoint*[nHits];
    for (Int_t i = 0; i < nHits; i++)
        pointData[i] = dynamic_cast<R3BCalifaPoint*>(fCalifaPointDataCA->At(i));

    for (Int_t i = 0; i < nHits; i++)
    {
      Int_t crystalId;
      Double_t Nf;
      Double_t Ns;
      Double_t time;
      Double_t energy;

        crystalId = pointData[i]->GetCrystalId();
        Nf = pointData[i]->GetNf();
        Ns = pointData[i]->GetNs();
        time = pointData[i]->GetTime();
        energy = pointData[i]->GetEnergyLoss();
	LLOG(debug1) << "Adding " << energy <<"GeV for crystal id "<<crystalId;

	AddCrystalCal(crystalId, NUSmearing(energy), Nf, Ns, time, 0); 
    }

    if (pointData)
        delete[] pointData;


   for (auto& it: *fCalifaCryCalData)
    {
      auto crId=it.first;
      auto& hit=it.second;
      bool skip;
      if (!fRealConfig)
        {
	  skip = hit.fEnergy < fThreshold;
	}
      else
	{
	  bool inUse = fSim_Par->GetInUse(crId - 1);
	  fResolution = fSim_Par->GetResolution(crId - 1);
	  double parThres = fSim_Par->GetThreshold(crId - 1);
	  skip=!inUse || parThres < hit.fEnergy *1e6; // GeV (sim, root units) -> keV (exp calibration)
	}
      if (skip)
	{
	  LLOG(debug) << "skipping crId ==" << crId;
	  fCalifaCryCalData->erase(crId);
	  continue;
	}
      if (fResolution > 0)
	hit.fEnergy=ExpResSmearing(hit.fEnergy);
      
      if (fComponentRes > 0)
	{
	  hit.fNf=CompSmearing(hit.fNf);
	  hit.fNs=CompSmearing(hit.fNs);
	}
      
    }
}

void R3BCalifaDigitizer::Reset()
{
    // Clear the CA structure
    LOG(debug) << "Clearing CalifaCrystalCalData Structure";
    fCalifaCryCalData->clear();
    ResetParameters();
}

void R3BCalifaDigitizer::SetDetectionThreshold(Double_t thresholdEne)
{
    fThreshold = thresholdEne;
    LOG(info) << "R3BCalifaDigitizer::SetDetectionThreshold to " << fThreshold << " GeV.";
}

void R3BCalifaDigitizer::SetRealConfig(Bool_t isRealSet)
{
    fRealConfig = isRealSet;
    LOG(info) << "R3BCalifaDigitizer::SetRealConfig to " << isRealSet;
}

R3BCalifaCrystalCalData* R3BCalifaDigitizer::AddCrystalCal(Int_t crystalId,
                                                           Double_t energy,
                                                           Double_t Nf,
                                                           Double_t Ns,
                                                           ULong64_t time,
                                                           Double_t tot_energy)
{
    
  auto res=fCalifaCryCalData->emplace(std::make_pair(crystalId, R3BCalifaCrystalCalData(crystalId, 0, 0, 0, 0, 0)));
  LLOG(debug) << " Adding "<<(res.second?"a new":"to existing")<<" CrystalCalData "
	      << " with crystal Id " << crystalId << ": energy" << energy * 1e06 << " keV Nf=" << Nf
	      << " Ns=" << Ns << " Time=" << time << " tot_energy=" << tot_energy;
  auto& hit=res.first->second;
    hit.fEnergy+=energy;
    hit.fNf+=Nf;
    hit.fNs+=Ns;
    if (time>hit.fWrts)
      hit.fWrts=time;
    hit.fToTEnergy+=tot_energy;
    return  &hit;
}

void R3BCalifaDigitizer::SetExpEnergyRes(Double_t crystalRes)
{
    fResolution = crystalRes;
    LOG(info) << "R3BCalifaDigitizer::SetExpEnergyRes to " << fResolution << "% @ 1 MeV.";
}

void R3BCalifaDigitizer::SetComponentRes(Double_t componentRes)
{
    fComponentRes = componentRes;
    LOG(info) << "R3BCalifaDigitizer::SetComponentRes to " << fComponentRes;
}

Double_t R3BCalifaDigitizer::NUSmearing(Double_t inputEnergy)
{
    // Very simple preliminary scheme where the NU is introduced as a flat random
    // distribution with limits fNonUniformity (%) of the energy value.
    //
    return gRandom->Uniform(inputEnergy - inputEnergy * fNonUniformity / 100,
                            inputEnergy + inputEnergy * fNonUniformity / 100);
}

void R3BCalifaDigitizer::SetNonUniformity(Double_t nonU)
{
    fNonUniformity = nonU;
    LOG(info) << "R3BCalifaDigitizer::SetNonUniformity to " << fNonUniformity << " %";
}

Double_t R3BCalifaDigitizer::ExpResSmearing(Double_t inputEnergy)
{
    // Smears the energy according to some Experimental Resolution distribution
    // Very simple  scheme where the Experimental Resolution
    // is introduced as a gaus random distribution with a width given by the
    // parameter fResolution(in % @ MeV). Scales according to 1/sqrt(E)
    //
    // The formula is   TF1("name","0.058*x/sqrt(x)",0,10) for 3% at 1MeV (3.687 @
    // 662keV)
    //  ( % * energy ) / sqrt( energy )
    // and then the % is given at 1 MeV!!
    //
    if (fResolution == 0)
        return inputEnergy;
    else
    {
        // Energy in MeV, that is the reason for the factor 1000...
        Double_t randomIs = gRandom->Gaus(0, inputEnergy * fResolution * 1000 / (235 * sqrt(inputEnergy * 1000)));
        return inputEnergy + randomIs / 1000;
    }
}

Double_t R3BCalifaDigitizer::CompSmearing(Double_t inputComponent)
{
    // Smears the components Ns and Nf according to fComponentRes
    //
    if (fComponentRes == 0)
        return inputComponent;
    else if (fComponentRes != 0 && inputComponent != 0)
    {
        Double_t randomIs =
            gRandom->Gaus(0, inputComponent * fComponentRes * 1000 / (235 * sqrt(inputComponent * 1000)));
        return inputComponent + randomIs / 1000;
    }

    else
        return inputComponent;
}

ClassImp(R3BCalifaDigitizer);
