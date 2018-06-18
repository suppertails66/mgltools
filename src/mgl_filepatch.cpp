#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TStringConversion.h"
#include <iostream>

using namespace std;
using namespace BlackT;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "File patching utility" << endl;
    cout << "Usage: " << argv[0]
      << " <patchfile> <dstfile> <offset> [sizelimit]" << endl;
    cout << "  patchfile    Data to be patched to dstfile" << endl;
    cout << "  dstfile      File to be patched" << endl;
    cout << "  offset       Position within dstfile to place patchfile"
         << endl;
    cout << "  sizelimit    If present, maximum permissible size of patchfile"
         << endl;
    
    return 0;
  }
  
  TBufStream ifs(1);
  ifs.open(argv[1]);
  
  TBufStream ofs(1);
  ofs.open(argv[2]);
  
  int offset = TStringConversion::stringToInt(string(argv[3]));
  
  int sizeLimit = -1;
  if (argc >= 5) {
    sizeLimit = TStringConversion::stringToInt(string(argv[4]));
  }
  
  if ((sizeLimit != -1)
      && (ifs.size() > sizeLimit)) {
    cerr << "Error: patch file '" << argv[1] << "' exceeds size limit"
         << " for patching to '" << argv[2] << "'"
         << " (max " << sizeLimit << " bytes, actual"
         << " ifs.size()" << endl;
    return 1;
  }
  
  ifs.seek(0);
  ofs.seek(offset);
  ofs.writeFrom(ifs, ifs.size());
  
  ofs.save(argv[2]);
  
  return 0;
}
