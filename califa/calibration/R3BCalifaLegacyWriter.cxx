#include "R3BCalifaLegacyWriter.h"


#include "R3BCalifaMappedData.h"
#include "R3BCalifaCrystalCalData.h"
#include "R3BCalifaClusterData.h"


InitStatus R3BCalifaLegacyWriter::Init()
{
  CreateConverter<R3BCalifaMappedData>();
  CreateConverter<R3BCalifaCrystalCalData>();
  CreateConverter<R3BCalifaClusterData>();
  for (auto f: fOnInit)
    {
      f();
    }
  LLOG(debug) << "Registered "<< fOnExec.size() << " converters.";
  return kSUCCESS;
}


void R3BCalifaLegacyWriter::Exec(const char*)
{
  for (auto& f: fOnExec)
    f();
}

ClassImp(R3BCalifaLegacyWriter);
