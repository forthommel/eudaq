#include "eudaq/OptionParser.hh"
#include "eudaq/Producer.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/Monitor.hh"
#include "eudaq/Utils.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line Modules Lister", "2.0", "The list of EUDAQ modules");
  eudaq::Option<std::string> type(op, "t", "type", "", "string",
				  "Type of modules to list");
  try{
    op.Parse(argv);
  }
  catch(...){
    std::ostringstream err;
    return op.HandleMainException(err);
  }
  const auto mod_types = eudaq::split(type.Value());
  for (const auto& mod_type : mod_types){
    if (mod_type == "prod" || mod_type == "all"){
      std::cout <<"List of producers registered:"<<std::endl;
      for (const auto& it : eudaq::Factory<eudaq::Producer>::Instance()){
        std::cout<<">>"<<it.first<<"\n";
      }
    }
    if (mod_type == "conv" || mod_type == "all"){
      std::cout <<"List of file readers registered:"<<std::endl;
      for (const auto& it : eudaq::Factory<eudaq::FileReader>::Instance()){
        std::cout<<">>"<<it.first<<"\n";
      }
    }
    if (mod_type == "mon" || mod_type == "all"){
      std::cout <<"List of monitors registered:"<<std::endl;
      for (const auto& it : eudaq::Factory<eudaq::Monitor>::Instance()){
        std::cout<<">>"<<it.first<<"\n";
      }
    }
  }
  return 0;
}
