#include "mgl/MglScriptHeader.h"
#include "util/ByteConversion.h"

using namespace BlackT;

namespace Sat {


  void MglScriptHeader::read(unsigned char* src) {
    id = ByteConversion::fromBytes(
      src + 0, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    offset = ByteConversion::fromBytes(
      src + 4, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
  }


} // end namespace Sat
