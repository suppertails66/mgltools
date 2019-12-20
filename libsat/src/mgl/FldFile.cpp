#include "mgl/FldFile.h"
#include "util/TStringConversion.h"
#include "exception/TGenericException.h"
#include <iostream>

using namespace BlackT;

namespace Sat {

  
  FldFile::FldFile() { }
  
  void FldFile::read(BlackT::TStream& ifs) {
    int baseFldPos = ifs.tell();
    int ifsEnd = ifs.size();
    
    for (int i = 0; i < maxChunks; i++) {
      // read next entry from index
      ifs.seek(baseFldPos + (i * chunkIdSize));
      
      // offset of FFFFFFFF = end of index
      int chunkOffset = ifs.reads32be();
      if (chunkOffset <= -1) break;
      
      int chunkSize = ifs.reads32be();
      if (chunkSize <= -1) break;
      
      if (baseFldPos + chunkOffset + chunkSize >= ifsEnd) {
        throw TGenericException(T_SRCANDLINE,
                                "FldFile::read()",
                                std::string("Chunk "
                                  + TStringConversion::intToString(i)
                                  + " extends past end of input: ")
                                  + "start = "
                                  + TStringConversion::intToString(baseFldPos
                                      + chunkOffset)
                                  + ", end = "
                                  + TStringConversion::intToString(baseFldPos
                                      + chunkOffset + chunkSize)
                                  + ", eof = "
                                  + TStringConversion::intToString(ifsEnd));
      }
      
      // create new chunk
      chunks.push_back(BlackT::TArray<BlackT::TByte>());
      
      // read data into chunk
      ifs.seek(baseFldPos + chunkOffset);
      chunks.back().resize(chunkSize);
      ifs.read((char*)chunks.back().data(), chunkSize);
    }
  }
  
  void FldFile::write(BlackT::TStream& ifs) const {
    if ((int)chunks.size() > maxChunks) {
      throw TGenericException(T_SRCANDLINE,
                              "FldFile::write()",
                              std::string("Too many chunks to fit in FLD: ")
                                + " max "
                                + TStringConversion::intToString(maxChunks)
                                + ", actual "
                                + TStringConversion::intToString(chunks.size()));
    }
    
    int baseFldPos = ifs.tell();
    
    int chunkNum = 0;
    // start placing data at first chunk after index
    int currentSectorNum = 1;
    for (ChunkCollection::const_iterator it = chunks.cbegin();
         it != chunks.cend();
         ++it) {
      int chunkOffset = (currentSectorNum * MglConsts::sectorSize);
      int chunkSize = it->size();
      
      // write index
      ifs.seek(baseFldPos + (chunkNum * chunkIdSize));
      ifs.writeu32be(chunkOffset);
      ifs.writeu32be(chunkSize);
      
      // compute number of sectors chunk occupies
      int chunkSectorSize = chunkSize / MglConsts::sectorSize;
      if ((chunkSize % MglConsts::sectorSize) != 0) ++chunkSectorSize;
      
      // chunks must be padded to next sector boundary.
      // compute size of chunk's sector padding
      int chunkPadding
        = MglConsts::sectorSize - (chunkSize % MglConsts::sectorSize);
      // no padding if already at sector boundary
      if (chunkPadding == MglConsts::sectorSize) chunkPadding = 0;
      
      // write chunk data
      ifs.seek(baseFldPos + (currentSectorNum * MglConsts::sectorSize));
      ifs.write((char*)it->data(), it->size());
      
      // write chunk padding
      for (int i = 0; i < chunkPadding; i++) ifs.put(0xFF);
      
      // move to next chunk
      ++chunkNum;
      currentSectorNum += chunkSectorSize;
    }
    
    // pad index sector with 0xFF
    while (chunkNum < maxChunks) {
      ifs.seek(baseFldPos + (chunkNum * chunkIdSize));
      for (int i = 0; i < chunkIdSize; i++) ifs.put(0xFF);
      ++chunkNum;
    }
  }


} // end namespace Sat 
