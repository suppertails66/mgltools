#include "mgl/mgl_cmpr.h"
#include "util/ByteConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
using namespace BlackT;
using namespace Sat;

const static int decompressionBufferSize = 0x100000;

const static int tileW = 8;
const static int tileH = 8;

const static int tileSize = 0x20;

const static int vFlipFlag      = 0x80000000;
const static int hFlipFlag      = 0x40000000;
const static int tileIndexMask  = 0x3FFFFFFF;

// 40 tiles * 8 pixels per tile = 320 pixels (full screen width)
const static int tilemapW = 0x28;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "Credits image extractor for Mahou Gakuen Lunar!" << endl;
    cout << "Usage: " << argv[0] << " <datachunk> <mapchunk>"
      << " <outfile>" << endl;
    return 0;
  }
  
  char* datachunkname = argv[1];
  char* mapchunkname = argv[2];
  char* outfilename = argv[3];
  
  BufferedFile datachunk = getFile(datachunkname);
  BufferedFile mapchunk = getFile(mapchunkname);
  
  // Read the palette (16 colors, first 32 bytes of datachunk)
  vector<TColor> palette;
  for (int i = 0; i < 16; i++) {
    int rawColor = ByteConversion::fromBytes(datachunk.buffer + (i * 2), 2,
      EndiannessTypes::big, SignednessTypes::nosign);
    palette.push_back(saturnToRealColor(rawColor));
  }
  
  // Get compressed image data chunk size
  int compressedDataSize = ByteConversion::fromBytes(datachunk.buffer + 32, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
  
  // Decompress image data
  unsigned char decompressedData[decompressionBufferSize];
  memset((char*)decompressedData, 0, decompressionBufferSize);
  int decompressedDataSize = decompressMglImg(
    (unsigned char*)(datachunk.buffer + 36), compressedDataSize,
    decompressedData);
  
  // Read image data to tiles
  vector<TGraphic> tiles;
  int numGraphicTiles = decompressedDataSize / tileSize;
  // if the last tile nominally ends partway through, the remaining data
  // should be assumed to be all zeroes
  if ((decompressedDataSize % tileSize) != 0) ++numGraphicTiles;
  for (int i = 0; i < numGraphicTiles; i++) {
    int tilePos = (i * tileSize);
    tiles.push_back(TGraphic());
    read4bppGraphicPalettized((unsigned char*)decompressedData + tilePos,
                              tiles[tiles.size() - 1], tileW, tileH, palette);
  }
  
  // Get tilemap height
  int tilemapH = ByteConversion::fromBytes(mapchunk.buffer + 0, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
    
//  cout << tilemapW << " " << tilemapH << endl;

  // Compute number of tiles in tilemap
  int numTilesInMap = tilemapW * tilemapH;
  
  // Create image to hold output
  TGraphic img(tilemapW * tileW, tilemapH * tileH);
  
  // Read the tilemap
  for (int i = 0; i < numTilesInMap; i++) {
    // position of tile identifier in map chunk
    int tileIDPos = (i * 4) + 4;
    
    int tilePutX = (i % tilemapW) * tileW;
    int tilePutY = (i / tilemapW) * tileH;
    TRect srcrect(0, 0, tileW, tileH);
    TRect dstrect(tilePutX, tilePutY, tileW, tileH);
    
    int tileID = ByteConversion::fromBytes(mapchunk.buffer + tileIDPos, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    bool vFlip = tileID & vFlipFlag;
    bool hFlip = tileID & hFlipFlag;
    int tileIndex = tileID & tileIndexMask;
    
    // if flipping is needed, make a local copy of the tile first
    if (vFlip || hFlip) {
      TGraphic tile = tiles[tileIndex];
      if (vFlip) tile.flipVertical();
      if (hFlip) tile.flipHorizontal();
      img.copy(tile, dstrect, srcrect);
    }
    // otherwise, we can use the data as-is
    else {
      img.copy(tiles[tileIndex], dstrect, srcrect);
    }
  }
  
  // Save output image
  TPngConversion::graphicToRGBAPng(string(outfilename), img);
  
  delete datachunk.buffer;
  delete mapchunk.buffer;
  
  return 0;
}
