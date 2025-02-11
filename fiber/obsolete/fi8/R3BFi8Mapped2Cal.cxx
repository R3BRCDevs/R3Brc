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

#include "R3BFi8Mapped2Cal.h"

R3BFi8Mapped2Cal::R3BFi8Mapped2Cal(Int_t a_verbose)
    : R3BBunchedFiberMapped2Cal("Fi8", a_verbose, R3BTCalEngine::CTDC_8_12_FWD_250, false)
{
}

R3BFi8Mapped2Cal::R3BFi8Mapped2Cal(Int_t a_verbose,
                                   enum Electronics a_electronics,
                                   enum R3BTCalEngine::CTDCVariant a_variant)
    : R3BBunchedFiberMapped2Cal("Fi8", a_verbose, a_electronics, a_variant, false)
{
}

R3BFi8Mapped2Cal::~R3BFi8Mapped2Cal() {}

ClassImp(R3BFi8Mapped2Cal);
