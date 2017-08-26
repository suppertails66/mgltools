#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
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

void oneBppDataToGraphic(const unsigned char* data, TGraphic& dst) {
  dst.clearTransparent();
  int numBytes = (dst.w() * dst.h()) / 8;
  
  int pixnum = 0;
  for (int i = 0; i < numBytes; i++) {
    for (int bitmask = 0x80; bitmask >= 0x01; bitmask >>= 1) {
      bool bit = (data[i] & bitmask) != 0;
      
      if (bit) {
        int xpos = pixnum % dst.w();
        int ypos = pixnum / dst.w();
        dst.setPixel(xpos, ypos, TColor(0, 0, 0,
          TColor::fullAlphaOpacity));
      }
      
      ++pixnum;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "ASCII.FNT extractor for Mahou Gakuen Lunar!" << endl;
    cout << "Usage: " << argv[0] << " <ASCII.FNT> <8x16path> <8x8path>"
      << endl;
    return 0;
  }
  
  char* asciiPath = argv[1];
  char* bigPath = argv[2];
  char* smallPath = argv[3];
  
  ifstream ifs(asciiPath);
  int asciiSize = fsize(ifs);
  char* asciidat = new char[asciiSize];
  ifs.read(asciidat, asciiSize);
  
  int totalNumChars = asciiSize / normalCharSize;
  
  vector<TGraphic> characters;
  for (int i = 0; i < totalNumChars; i++) {
    characters.push_back(TGraphic(normalCharW, normalCharH));
    oneBppDataToGraphic((unsigned char*)asciidat + (i * normalCharSize),
      characters[characters.size() - 1]);
  }
  
  // Output tall characters
  for (int i = 0; i < numTallChars; i++) {
    TGraphic& top = characters[i];
    TGraphic& bottom = characters[i + tallCharHalfOffset];
    
    TGraphic both(normalCharW, normalCharH * 2);
    both.copy(top, TRect(0, 0, 0, 0), TRect(0, 0, 0, 0));
    both.copy(bottom, TRect(0, normalCharH, 0, 0), TRect(0, 0, 0, 0));
    
    string filename(bigPath);
    ostringstream oss;
    oss << setw(3) << setfill('0') << i;
    filename += oss.str() + ".png";
    TPngConversion::graphicToRGBAPng(filename, both);
  }
  
  // Output small characters
  for (int i = normalCharStartIndex; i < characters.size(); i++) {
    string filename(smallPath);
    ostringstream oss;
    oss << setw(3) << setfill('0') << i;
    filename += oss.str() + ".png";
    TPngConversion::graphicToRGBAPng(filename, characters[i]);
  }
  
  delete asciidat;
  
  return 0;
}
