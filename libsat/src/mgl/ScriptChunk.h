#ifndef SCRIPTCHUNK_H
#define SCRIPTCHUNK_H


#include "util/ByteConversion.h"
#include "util/TArray.h"
#include "util/TSerialize.h"
#include "util/TStream.h"
#include "mgl/MglScriptHeader.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

namespace Sat {


  class ScriptChunk {
  public:
    int unknown1;
    int unknown2;
    std::vector<MglScriptHeader> scriptHeaders;
    BlackT::TArray<unsigned char> scriptData;
    std::vector<std::string> dialogues;
    std::vector<int> dialogueOffsets;
    
    void read(unsigned char* src, int srcsize);
//    void write(BlackT::TStream& ofs) const;
  protected:
    static bool morePrintableContentExists(unsigned char* str);
  };


} // end namespace Sat


#endif
