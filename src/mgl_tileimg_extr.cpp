#include "mgl_cmpr.h"
#include "util/ByteConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
using namespace BlackT;

const static int decompressionBufferSize = 0x100000;

const static int tileW = 8;
const static int tileH = 8;

const static int tileSize4bpp = 0x20;
const static int tileSize8bpp = 0x20;

const static int vFlipFlag      = 0x80000000;
const static int hFlipFlag      = 0x40000000;
const static int flag3Flag      = 0x20000000;
const static int flag4Flag      = 0x10000000;
const static int tileIndexMask  = 0x0FFFFFFF;

// 40 tiles * 8 pixels per tile = 320 pixels (full screen width)
static int tilemapW = 0x28;

const static int headerSize = 8;

namespace ImageEncodings {
  enum ImageEncoding {
    bpp4,
    bpp8
  };
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Tilemapped image extractor for Mahou Gakuen Lunar!" << endl;
    cout << "Usage: " << argv[0] << " <imgchunk>"
      << " <outfile>" << endl;
    return 0;
  }
  
  char* imgchunkname = argv[1];
  char* outfilename = argv[2];
  
  BufferedFile imgchunk = getFile(imgchunkname);
  
  // Read header data:
  // 4b offset to compressed image data
  // 4b offset to tilemap
  int compressedDataOffset
    = ByteConversion::fromBytes(imgchunk.buffer + 0, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
  int tilemapOffset
    = ByteConversion::fromBytes(imgchunk.buffer + 4, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
  
  // For standardly-packed images, the palette comes immediately after
  // the header and before the tilemap, so we can compute the size of
  // the palette (and thus the encoding of the image -- 4bpp or 8bpp)
  // like so
  int colorsInPalette = (tilemapOffset - headerSize) / 2;
  
  if (colorsInPalette > 256) {
    cerr << "Error: too many colors (" << colorsInPalette
      << ") in palette" << endl;
    exit(1);
  }
  
  ImageEncodings::ImageEncoding encoding;
  if (colorsInPalette <= 16) encoding = ImageEncodings::bpp4;
  else encoding = ImageEncodings::bpp8;
  
  // Read the palette
  vector<TColor> palette;
  for (int i = 0; i < colorsInPalette; i++) {
    int rawColor = ByteConversion::fromBytes(
      imgchunk.buffer + headerSize + (i * 2), 2,
      EndiannessTypes::big, SignednessTypes::nosign);
    palette.push_back(saturnToRealColor(rawColor));
  }
  
  // Get compressed image data chunk size
  int compressedDataSize = ByteConversion::fromBytes(
    imgchunk.buffer + compressedDataOffset, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
  
  // Decompress image data
  unsigned char decompressedData[decompressionBufferSize];
  memset((char*)decompressedData, 0, decompressionBufferSize);
  int decompressedDataSize = decompressMglImg(
    (unsigned char*)(imgchunk.buffer + compressedDataOffset + 4),
    compressedDataSize + 1, decompressedData);
    
  int tileSize;
  switch (encoding) {
  case ImageEncodings::bpp4:
    tileSize = tileSize4bpp;
    break;
  case ImageEncodings::bpp8:
    tileSize = tileSize8bpp;
    break;
  default:
    cerr << "Unknown encoding" << endl;
    exit(1);
    break;
  }
  
  // Read image data to tiles
  vector<TGraphic> tiles;
  int numGraphicTiles = decompressedDataSize / tileSize;
  // if the last tile nominally ends partway through, the remaining data
  // should be assumed to be all zeroes
  if ((decompressedDataSize % tileSize) != 0) ++numGraphicTiles;
  
  for (int i = 0; i < numGraphicTiles; i++) {
    tiles.push_back(TGraphic());
    TGraphic& tile = tiles[tiles.size() - 1];
    
    int tilePos = (i * tileSize);
    
    switch (encoding) {
    case ImageEncodings::bpp4:
      read4bppGraphicPalettized(
        (unsigned char*)decompressedData + tilePos,
        tile, tileW, tileH, palette);
      break;
    case ImageEncodings::bpp8:
      read8bppGraphicPalettized(
        (unsigned char*)decompressedData + tilePos,
        tile, tileW, tileH, palette);
      break;
    default:
      cerr << "Unknown encoding" << endl;
      exit(1);
      break;
    }
    
  }
  
  // Compute tilemap dimensions.
  // Tilemaps have a fixed width (40 tiles); we can compute the height from
  // the size of the tilemap chunk
  int tilemapH = ((compressedDataOffset - tilemapOffset) / 4) / tilemapW;
    
//  cout << tilemapW << " " << tilemapH << endl;
  
  // because of EXPERT PROGRAMMING, the tilemap may or may not be preceded
  // by a 4b width/height pair
  // use some heuristics to determine whether this exists
  int tilemapWhTest = ByteConversion::fromBytes(
      imgchunk.buffer + tilemapOffset, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
  if ((tilemapWhTest < 0x10000000) && (tilemapWhTest >= 0x00100000)) {
    tilemapOffset += 4;
    tilemapW = ((tilemapWhTest & 0xFFFF0000) >> 16) & 0xFFFF;
    tilemapH = ((tilemapWhTest & 0x0000FFFF)) & 0xFFFF;
  }

  // Compute number of tiles in tilemap
  int numTilesInMap = tilemapW * tilemapH;
  
  // Create image to hold output
  TGraphic img(tilemapW * tileW, tilemapH * tileH);
  
  // Read the tilemap
  for (int i = 0; i < numTilesInMap; i++) {
    // position of tile identifier in map chunk
    int tileIDPos = (i * 4);
    
    int tilePutX = (i % tilemapW) * tileW;
    int tilePutY = (i / tilemapW) * tileH;
    TRect srcrect(0, 0, tileW, tileH);
    TRect dstrect(tilePutX, tilePutY, tileW, tileH);
    
    int tileID = ByteConversion::fromBytes(
      imgchunk.buffer + tilemapOffset + tileIDPos, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    bool vFlip = tileID & vFlipFlag;
    bool hFlip = tileID & hFlipFlag;
    bool flag3 = tileID & flag3Flag;
    bool flag4 = tileID & flag4Flag;
    int tileIndex = tileID & tileIndexMask;
    
//    cout << hex << tileID << endl;
    
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
  
  delete imgchunk.buffer;
  
  return 0;
}
