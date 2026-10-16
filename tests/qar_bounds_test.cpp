#include "engine/fs/qar.h"
#include "engine/core/log.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

namespace pt {
bool LogEnabled(){return false;}
void LogWrite(LogLevel,std::string_view){}
namespace os {
FILE* OpenFile(const std::filesystem::path& path,const char* mode){return std::fopen(path.c_str(),mode);}
int SeekFile(FILE* file,int64_t offset,int origin){return fseeko(file,offset,origin);}
std::string PathToUtf8(const std::filesystem::path& path){return path.string();}
}}

int main(){
    auto path=std::filesystem::temp_directory_path()/"pt-qar-bounds-test.qar";
    std::vector<uint8_t> valid(0x44);
    auto put=[](auto& bytes,size_t at,auto value){std::memcpy(bytes.data()+at,&value,sizeof(value));};
    std::memcpy(valid.data(),"hello",5);put(valid,16,uint64_t{123});put(valid,24,uint32_t{0});put(valid,28,uint32_t{5});
    put(valid,0x20+0x10,uint32_t{1});put(valid,0x20+0x16,uint16_t{0x7161});put(valid,0x20+0x18,uint32_t{1});
    auto write=[&](const auto& bytes){std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());};
    pt::QarArchive archive;write(valid);assert(archive.Open(path));auto read=archive.Read(123);assert(read&&std::string(read->begin(),read->end())=="hello");
    auto malformed=valid;put(malformed,0x20+0x10,UINT32_MAX);write(malformed);assert(!archive.Open(path)&&!archive.Contains(123));
    malformed=valid;put(malformed,0x20+0x18,UINT32_MAX);write(malformed);assert(!archive.Open(path));
    malformed=valid;put(malformed,24,UINT32_MAX);write(malformed);assert(!archive.Open(path));
    malformed=valid;put(malformed,28,UINT32_MAX);write(malformed);assert(!archive.Open(path));
    malformed.resize(8);write(malformed);assert(!archive.Open(path));
    write(valid);assert(archive.Open(path)&&archive.Contains(123));std::filesystem::remove(path);
    std::cout<<"QAR bounds passed: valid read, oversized count/table/offset/size, truncation and reopen\n";
}
