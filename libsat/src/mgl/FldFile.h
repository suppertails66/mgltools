#ifndef FLDFILE_H
#define FLDFILE_H


#include "util/TArray.h"
#include "util/TStream.h"
#include "mgl/MglConsts.h"
#include <vector>

namespace Sat {

  
  /**
   * Class representing a raw .FLD file.
   * Handles reading, writing of chunk data.
   */
  class FldFile {
  public:
    typedef std::vector< BlackT::TArray<BlackT::TByte> > ChunkCollection;
    
    ChunkCollection chunks;
    
    FldFile();
    
    void read(BlackT::TStream& ifs);
    void write(BlackT::TStream& ifs) const;
  protected:
    const static int indexChunkSize = 0x800;
    const static int chunkIdSize = 0x8;
    const static int maxChunks = indexChunkSize / chunkIdSize;
        
  };


}


#endif
