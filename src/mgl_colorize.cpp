#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include "mgl_cmpr.h"
#include "util/TPngConversion.h"
#include "util/TSerialize.h"

using namespace std;
using namespace BlackT;

char* findarg(int argc, char* argv[], const char* name) {
  for (int i = 0; i < argc - 1; i++) {
    if (strcmp(argv[i], name) == 0) return argv[i + 1];
  }
  
  return NULL;
}

namespace ProgramModes {
  enum ProgramMode {
    colorize,
    decolorize
  };
}

int findColorIndex(vector<TColor>& palette, TColor color) {
  for (int i = 0; i < palette.size(); i++) {
    if (palette[i] == color) return i;
  }
  
  cerr << "Color not in palette: "
    << color.r() << "," << color.g() << ","
    << color.b() << "," << color.a() << endl;
  exit(1);
}

void colorizeImage(TGraphic& image, vector<TColor>& palette, TGraphic& dst,
                   bool transparency) {
  dst.resize(image.w(), image.h());
  
  for (int j = 0; j < image.h(); j++) {
    for (int i = 0; i < image.w(); i++) {
      TColor color = image.getPixel(i, j);
      int colorIndex = (color.r() & 0xF0) >> 4;
      TColor realColor = palette[colorIndex];
      
      if (transparency && (colorIndex == 0)) {
        realColor.setA(TColor::fullAlphaTransparency);
      }
      
      dst.setPixel(i, j, realColor);
    }
  }
}

void decolorizeImage(TGraphic& image, vector<TColor>& palette, TGraphic& dst
                     ) {
  dst.resize(image.w(), image.h());
  
  for (int j = 0; j < image.h(); j++) {
    for (int i = 0; i < image.w(); i++) {
      TColor color = image.getPixel(i, j);
      int colorIndex = findColorIndex(palette, color);
      int value = (colorIndex << 4) | colorIndex;
      TColor realColor(value, value, value, TColor::fullAlphaOpacity);
      
      if ((colorIndex == 0)) {
        realColor.setA(TColor::fullAlphaTransparency);
      }
      
      dst.setPixel(i, j, realColor);
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Utility to (de)colorize grayscale PNGs using a 16-bit palette\n";
    cout << "Usage: " << argv[0] << " <command> [options]\n\n";
    cout << "<command> may be one of:\n";
      cout << "     c        Colorize an image\n";
      cout << "     d        Decolorize an image\n";
    cout << "\n";
    cout << "Options:\n";
      cout << "     -i       Input image file\n";
      cout << "     -p       Input palette file\n";
      cout << "     -o       Output image file\n";
    return 0;
  }
  
  char* command = argv[1];
  ProgramModes::ProgramMode mode;
  if (strcmp(command, "c") == 0) {
    mode = ProgramModes::colorize;
  }
  else if (strcmp(command, "d") == 0) {
    mode = ProgramModes::decolorize;
  }
  else {
    cerr << "Unknown command '" << argv[1] << "'" << endl;
    return 1;
  }
  
  char* infilename = findarg(argc, argv, "-i");
  char* palettename = findarg(argc, argv, "-p");
  char* outfilename = findarg(argc, argv, "-o");
  
  if (infilename == NULL) {
    cerr << "Missing input image (-i)" << endl;
    return 1;
  }
  
  if (palettename == NULL) {
    cerr << "Missing palette (-p)" << endl;
    return 1;
  }
  
  if (outfilename == NULL) {
    cerr << "Missing output image file name (-o)" << endl;
    return 1;
  }
  
  TGraphic image;
  TPngConversion::RGBAPngToGraphic(string(infilename), image);
  
  vector<TColor> palette;
  
  ifstream ifs;
  ifs.open(palettename, ios_base::binary);
  while (ifs.good()) {
    int color = TSerialize::readInt(ifs, 2,
      EndiannessTypes::big, SignednessTypes::nosign);
    
    if (!ifs.good()) break;
      
    TColor realColor = saturnToRealColor(color);
    // first color is transparency
    if (palette.size() == 0) realColor.setA(TColor::fullAlphaTransparency);
    palette.push_back(realColor);
  }
  
  TGraphic dst;
  switch (mode) {
  case ProgramModes::colorize:
    colorizeImage(image, palette, dst,
                  true);
/*    {
    TGraphic pal;
    pal.resize(256, 16);
    pal.clearTransparent();
    for (int i = 0; i < palette.size(); i++) {
      TRect box(i * 16, 0, i * 16, 0);
      pal.fillRect(i * 16, 0, 16, 16, palette[i]);
    }
    TPngConversion::graphicToRGBAPng(
      string(outfilename).substr(0, strlen(outfilename) - 4)
        + "_colors.png", pal);
    } */
    break;
  case ProgramModes::decolorize:
    decolorizeImage(image, palette, dst
                    );
    break;
  default:
    cerr << "Invalid command??" << endl;
    return 1;
    break;
  }
  
  TPngConversion::graphicToRGBAPng(string(outfilename), dst);
  
  return 0;
}
