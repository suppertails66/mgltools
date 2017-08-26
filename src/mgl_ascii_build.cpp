#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include "util/TGraphic.h"
#include "util/TPngConversion.h"

using namespace std;
using namespace BlackT;

const static int numTallChars = 16;
const static int tallCharHalfOffset = 16;

const static int normalCharStartIndex = 32;
const static int normalCharSize = 8;
const static int normalCharW = 8;
const static int normalCharH = 8;

int fsize(std::istream& ifs) {
  int orig = ifs.tellg();
  ifs.seekg(0, ios_base::end);
  int sz = ifs.tellg();
  ifs.seekg(orig);
  return sz;
}

bool fileExists(const string& filename) {
  ifstream ifs(filename.c_str());
  return (fsize(ifs) >= 0);
}

int charToOneBppData(unsigned char* data, TGraphic& dst) {
  int numBytes = (dst.w() * dst.h()) / 8;
  memset(data, 0, numBytes);
  
  int pixnum = 0;
  for (int i = 0; i < numBytes; i++) {
    for (int bitmask = 0x80; bitmask >= 0x01; bitmask >>= 1) {
      int xpos = pixnum % dst.w();
      int ypos = pixnum / dst.w();
      
      if (dst.getPixel(xpos, ypos).a() == TColor::fullAlphaOpacity) {
        data[i] |= bitmask;
      }
      
      ++pixnum;
    }
  }
  
  return numBytes;
}

string getExpNumStr(int num) {
  ostringstream oss;
  oss << setw(3) << setfill('0') << num;
  return oss.str();
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "ASCII.FNT builder for Mahou Gakuen Lunar!" << endl;
    cout << "Usage: " << argv[0] << " <8x16path> <8x8path> <outfile>"
      << endl;
    return 0;
  }
  
  char* bigPath = argv[1];
  char* smallPath = argv[2];
  char* asciiPath = argv[3];
  
  // Read tall chars
  vector<TGraphic> tallChars;
  while (true) {
    string filename(bigPath);
    filename += getExpNumStr(tallChars.size()) + ".png";
    if (!fileExists(filename)) break;
    
    TGraphic g;
    TPngConversion::RGBAPngToGraphic(filename, g);
    tallChars.push_back(g);
  }
  
  vector<TGraphic> smallChars;
  
  // Convert tall chars to small chars.
  // First, the top half of each tall char is written sequentially, then
  // the bottom half
  
  for (int i = 0; i < tallChars.size(); i++) {
    TGraphic top(normalCharW, normalCharH);
    top.copy(tallChars[i], TRect(0, 0, 0, 0), TRect(0, 0, 0, 0));
    smallChars.push_back(top);
  }
  
  for (int i = 0; i < tallChars.size(); i++) {
    TGraphic bottom(normalCharW, normalCharH);
    bottom.copy(tallChars[i], TRect(0, 0, 0, 0), TRect(0, normalCharH, 0, 0));
    smallChars.push_back(bottom);
  }
  
  // Read small chars
  while (true) {
    string filename(smallPath);
    // numbering for small chars resumes after tall chars
    filename += getExpNumStr(smallChars.size()) + ".png";
    if (!fileExists(filename)) break;
    
    TGraphic g;
    TPngConversion::RGBAPngToGraphic(filename, g);
    smallChars.push_back(g);
  }
  
  // Generate data buffer
  int totalChars = smallChars.size();
  int totalSize = normalCharSize * totalChars;
  char* data = new char[totalSize];
  
  // Convert graphics to data
  int putpos = 0;
  for (int i = 0; i < smallChars.size(); i++) {
    putpos += charToOneBppData((unsigned char*)data + putpos, smallChars[i]);
  }
  
  // Write generated data
  ofstream ofs(asciiPath, ios_base::binary);
  ofs.write(data, totalSize);
  delete data;
  
  return 0;
}
