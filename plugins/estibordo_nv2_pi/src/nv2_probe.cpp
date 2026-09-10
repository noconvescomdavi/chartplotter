#include "nv2_reader.h"
#include <iomanip>
#include <iostream>
int main(int argc,char**argv){
  if(argc!=2){std::cerr<<"usage: nv2probe <chart.nv2>\n";return 2;}
  try{
    auto d=estibordo::nv2::Reader::ParseFile(argv[1]);
    auto sw=estibordo::nv2::MercatorToLonLat(d.header.extent.min_x,d.header.extent.min_y);
    auto ne=estibordo::nv2::MercatorToLonLat(d.header.extent.max_x,d.header.extent.max_y);
    std::cout<<"confidence="<<estibordo::nv2::ToString(d.validation.confidence)<<"\nchart_id="<<d.metadata.chart_id
             <<"\ntitle="<<d.metadata.title<<"\nvendor="<<d.metadata.vendor<<"\nedition="<<d.header.edition_stamp<<"\n"
             <<std::fixed<<std::setprecision(6)<<"bbox="<<sw.first<<","<<sw.second<<","<<ne.first<<","<<ne.second<<"\n";
    if(d.dictionary_block)std::cout<<"dictionary_tag=0x"<<std::hex<<d.dictionary_block->tag<<std::dec<<"\ndictionary_offset="<<d.dictionary_block->offset<<"\ndictionary_length="<<d.dictionary_block->length<<"\n";
    for(auto&w:d.validation.warnings)std::cout<<"warning="<<w<<"\n";
    return d.validation.confidence==estibordo::nv2::Confidence::StructuralConfirmed?0:1;
  }catch(const std::exception&e){std::cerr<<"NV2 parse failure: "<<e.what()<<"\n";return 1;}
}
