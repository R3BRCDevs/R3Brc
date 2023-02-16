#ifndef R3BCALIFALEGACYWRITER_H
#define R3BCALIFALEGACYWRITER_H

#include <vector>
#include <functional>
#include <FairTask.h>

#include <FairRootManager.h>
#include <TClonesArray.h>
#include "R3BLogger.h"


class R3BCalifaLegacyWriter: public FairTask
{
 public:
  R3BCalifaLegacyWriter():
    FairTask("R3BCalifaLegacyWriter")
  {}
  InitStatus Init() override;
  void Exec(const char* ) override;
  
  template<typename DataType>
  void AddConverter(std::string inName, std::string outName)
  {
    // This would be much simpler if the FairRoot people did not insist on that overly complicated
    // Multi-stage initialization process. 
    fOnInit.push_back([inName, outName, this]() { this->CreateConverter<DataType>(inName, outName); } );
  }


 protected:

  template<class K, class V>
  static const V& extractValue(const std::pair<K, V>& val)
  {
    return val.second;
  }

  template<class V>
  static const V& extractValue(V& val)
  {
    return val;
  }
  
  template<typename T>
  void CreateConverter(std::string stl_name=T::default_container_name, std::string tca_name=T::tca_name)
  {
    FairRootManager* frm = FairRootManager::Instance();
    const char *name=T::Class()->GetName();
    assert(frm);  
    auto stlptr=frm->InitObjectAs<const typename T::container_t*>(stl_name.c_str());
    if (!stlptr)
      {
	LLOG(warning) << "Container " << stl_name << " for "<< name <<" not found, will NOT create " << tca_name ;
	return;
      }
    auto& stl=*stlptr;
    auto& tca = *new TClonesArray(name);
    frm->Register(tca_name.c_str(), name, &tca, 1);
    LLOG(info) << "Converting " << name << " from " << stl_name << " to TClonesArray " << tca_name ;
    auto f=[&stl, &tca, tca_name]()
	   {
	     tca.Clear();
	     for (auto& h: stl)
	       new (tca[tca.GetEntries()]) T(extractValue(h));
	     LLOG(debug) << "converted " << tca.GetEntries() << " entries to TClonesArray " << tca_name;
	   };
    fOnExec.push_back(f);
  }

  std::vector<std::function<void(void)>> fOnExec;
  std::vector<std::function<void(void)>> fOnInit;
  ClassDefOverride(R3BCalifaLegacyWriter, 1);
};

#endif
