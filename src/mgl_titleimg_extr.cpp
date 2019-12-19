#include "mgl/mgl_cmpr.h"
#include "util/ByteConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TStringConversion.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
using namespace BlackT;
using namespace Sat;

const static int decompressionBufferSize = 0x100000;

const static int colorsInPalette = 256;
const static int paletteChunkSize = 512;

struct CompressedImageHeader {
  const static int size = 12;

  int width;
  int height;
  int compressedDataOffset;
  int compressedDataSize;
  
  int read(const char* src) {
    width
      = ByteConversion::fromBytes(src + 0, 2,
        EndiannessTypes::big, SignednessTypes::nosign);
    height
      = ByteConversion::fromBytes(src + 2, 2,
        EndiannessTypes::big, SignednessTypes::nosign);
    compressedDataOffset
      = ByteConversion::fromBytes(src + 4, 4,
        EndiannessTypes::big, SignednessTypes::nosign);
    compressedDataSize
      = ByteConversion::fromBytes(src + 8, 4,
        EndiannessTypes::big, SignednessTypes::nosign);
    return size;
  }
};

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Title image extractor for Mahou Gakuen Lunar!" << endl;
    cout << "Usage: " << argv[0] << " <imgchunk>"
      << " <outpath>" << endl;
    return 0;
  }
  
  char* imgchunkname = argv[1];
  char* outpath = argv[2];
  
  BufferedFile imgchunk = getFile(imgchunkname);
  
  // Read the palette
  vector<TColor> palette;
  for (int i = 0; i < colorsInPalette; i++) {
    int rawColor = ByteConversion::fromBytes(
      imgchunk.buffer + (i * 2), 2,
      EndiannessTypes::big, SignednessTypes::nosign);
    palette.push_back(saturnToRealColor(rawColor));
  }
  
  // Read header
  int getpos = paletteChunkSize;
  int numImages
    = ByteConversion::fromBytes(imgchunk.buffer + getpos, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
  getpos += 4;
  
  vector<CompressedImageHeader> headers;
  for (int i = 0; i < numImages; i++) {
    CompressedImageHeader header;
    getpos += header.read(imgchunk.buffer + getpos);
    headers.push_back(header);
  }
  
  for (int i = 0; i < headers.size(); i++) {
    CompressedImageHeader& header = headers[i];
    
    // Decompress image data
    unsigned char decompressedData[decompressionBufferSize];
    memset((char*)decompressedData, 0, decompressionBufferSize);
    int decompressedDataSize = decompressMglImg(
      (unsigned char*)(imgchunk.buffer + header.compressedDataOffset),
      header.compressedDataSize, decompressedData);
    
    TGraphic g;
    read8bppGraphicPalettized(decompressedData,
                              g,
                              header.width,
                              header.height,
                              palette);
    
    TPngConversion::graphicToRGBAPng(
      string(outpath) + TStringConversion::intToString(i) + ".png",
      g);
  }
  
  delete imgchunk.buffer;
  
  return 0;
}
