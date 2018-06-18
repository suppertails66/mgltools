#include "mgl_cmpr.h"
#include "util/ByteConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TTwoDArray.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
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

bool implicitWh;
ImageEncodings::ImageEncoding encoding;
TGraphic grp;
map<int, int> palette;
map<int, int> paletteRev;
vector<TGraphic> tiles;
vector<TGraphic> tilesFlipX;
vector<TGraphic> tilesFlipY;
vector<TGraphic> tilesFlipXY;
TTwoDArray<int> tilemap;

int findMatchingIndex(vector<TGraphic>& tiles,
                      TGraphic& tile) {
  for (unsigned int i = 0; i < tiles.size(); i++) {
    if (tiles[i] == tile) return i;
  }
  
  return -1;
}

void doTile(int tX, int tY) {
  int offsetX = tX * tileW;
  int offsetY = tY * tileH;
  
  TGraphic tile(tileW, tileH);
  
  // Iterate over pixels in the target tile's area
  for (int j = 0; j < tileH; j++) {
    for (int i = 0; i < tileW; i++) {
      // Get color of target pixel
      int x = offsetX + i;
      int y = offsetY + j;
      TColor color = grp.getPixel(x, y);
      
      // transparency = index 0
      if (color.a() == TColor::fullAlphaTransparency) {
        tile.setPixel(i, j, TColor(0, 0, 0, TColor::fullAlphaTransparency));
        continue;
      }
      
      int satColor = realToSaturnColor(color);
      
      // Add color to palette if not already present
      map<int, int>::iterator findIt = palette.find(satColor);
      if (findIt == palette.end()) {
        // index is +1 to skip over transparent index 0
        int index = palette.size() + 1;
        palette[satColor] = index;
        paletteRev[index] = satColor;
//        std::cerr << hex << satColor << " " << dec << palette[satColor] << endl;
      }
      
      // Compute value to put in generated tile
      int index = palette.at(satColor);
      int value;
      switch (encoding) {
      case ImageEncodings::bpp4:
        value = (index << 4) | index;
        break;
      case ImageEncodings::bpp8:
        value = index;
        break;
      default:
        // can't happen
        return;
        break;
      }
      
      tile.setPixel(i, j,
                    TColor(value, value, value,
                           TColor::fullAlphaOpacity));
    }
  }
  
  // See if any existing tile matches this one
  int tileIndex;
  
  // Regular orientation
  if ((tileIndex = findMatchingIndex(tiles, tile)) != -1) {
    tilemap.data(tX, tY) = ((tileIndex * 2));
    return;
  }
  
  // Flipped in X
  tile.flipHorizontal();
  if ((tileIndex = findMatchingIndex(tilesFlipX, tile)) != -1) {
    tilemap.data(tX, tY) = ((tileIndex * 2) | hFlipFlag);
    return;
  }
  
  // Flipped in XY
  tile.flipVertical();
  if ((tileIndex = findMatchingIndex(tilesFlipXY, tile)) != -1) {
    tilemap.data(tX, tY) = ((tileIndex * 2) | hFlipFlag | vFlipFlag);
    return;
  }
  
  // Flipped in Y
  tile.flipHorizontal();
  if ((tileIndex = findMatchingIndex(tilesFlipY, tile)) != -1) {
    tilemap.data(tX, tY) = ((tileIndex * 2) | vFlipFlag);
    return;
  }
  
  // revert to original orientation
  tile.flipVertical();
  
  // Tile doesn't exist: add new one
  tiles.push_back(tile);
  tile.flipHorizontal();
  tilesFlipX.push_back(tile);
  tile.flipVertical();
  tilesFlipXY.push_back(tile);
  tile.flipHorizontal();
  tilesFlipY.push_back(tile);
  
  // Add tilemap entry
  tilemap.data(tX, tY) = (tiles.size() - 1) * 2;
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    cout << "Mahou Gakuen Lunar! tilemap-based image generator" << endl;
    cout << "Usage: " << argv[0]
      << " <infile> <outfile> <colormode> <whflag>" << endl;
    cout << "  infile     Source image" << endl
         << "  outfile    Destination file" << endl
         << "  colormode  16 or 256" << endl
         << "  whflag     1 if tilemap has implicit 320px width" << endl;
    return 0;
  }
  
  char* infileName = argv[1];
  char* outfileName = argv[2];
  int colormode = TStringConversion::stringToInt(string(argv[3]));
  implicitWh = TStringConversion::stringToInt(string(argv[4])) != 0;
  
  switch (colormode) {
  case 16:
    encoding = ImageEncodings::bpp4;
    break;
  case 256:
    encoding = ImageEncodings::bpp8;
    break;
  default:
    cerr << "Error: Unknown color mode " << colormode << endl;
    return 1;
    break;
  }

  // Set palette color 0 to transparent
//  palette[TColor(0, 0, 0, TColor::fullAlphaTransparency)] = 0;
  
  // Read source image
  TPngConversion::RGBAPngToGraphic(string(infileName), grp);
//  TIfstream ifs(infileName, ios_base::binary);
  
  // Do the tilemap and tile data conversion
  tilemapW = grp.w() / tileW;
  int tilemapH = grp.h() / tileH;
  tilemap.resize(tilemapW, tilemapH);
  for (int j = 0; j < tilemapH; j++) {
    for (int i = 0; i < tilemapW; i++) {
//      cerr << i << " " << j << endl;
      doTile(i, j);
    }
  }
  
  // Generate raw tile data
  unsigned char rawTileData[0x10000];
  int rawTileDataSize = 0;
  for (unsigned int i = 0; i < tiles.size(); i++) {
    switch (encoding) {
    case ImageEncodings::bpp4:
      rawTileDataSize
        += writeGraphic4bpp(tiles[i], rawTileData + rawTileDataSize);
      break;
    case ImageEncodings::bpp8:
      rawTileDataSize
        += writeGraphic8bpp(tiles[i], rawTileData + rawTileDataSize);
      break;
    default:
      cerr << "illegal encoding" << endl;
      break;
    }
  }
  
  // Compress tile data
  unsigned char compressedTileData[0x10000];
  int compressedTileDataSize =
    compressMglImg(rawTileData, rawTileDataSize, compressedTileData);
  
  // Generate final palette
  TBufStream outputPalette(0x200);
  // color 0 = transparency
  outputPalette.writeu16be(0);
  for (std::map<int, int>::iterator it = paletteRev.begin();
       it != paletteRev.end();
       ++it) {
    outputPalette.writeu16be(it->second);
  }
  // pad with dummy entries
  switch (encoding) {
  case ImageEncodings::bpp4:
    while (outputPalette.size() < 0x20) outputPalette.writeu16be(0);
    break;
  case ImageEncodings::bpp8:
    while (outputPalette.size() < 0x200) outputPalette.writeu16be(0);
    break;
  default:
    cerr << "illegal encoding" << endl;
    break;
  }
  outputPalette.seek(0);
  
  TBufStream output(0x10000);
  // space for chunk pointers
  output.seekoff(8);
  
  // palette
  int palettePos = output.tell();
  output.writeFrom(outputPalette, outputPalette.size());
  
  // tilemap
  int tilemapPos = output.tell();
  if (!implicitWh) {
    output.writeu16be(tilemap.w());
    output.writeu16be(tilemap.h());
  }
  for (int j = 0; j < tilemap.h(); j++) {
    for (int i = 0; i < tilemap.w(); i++) {
      output.writeu32be(tilemap.data(i, j));
    }
  }
  
  // tile data
  int tilePos = output.tell();
  output.writeu32be(compressedTileDataSize);
  output.write((char*)compressedTileData, compressedTileDataSize);
  
  // pointers
  output.seek(0);
  output.writeu32be(tilePos);
  output.writeu32be(tilemapPos);
  
  // write to file
  output.save(outfileName);
  
//  TOfstream tmp(outfileName, ios_base::binary);
//  for (int j = 0; j < tilemap.h(); j++) {
//    for (int i = 0; i < tilemap.w(); i++) {
//      tmp.writeu32be(tilemap.data(i, j));
//    }
//  }
  
//  TOfstream tmp(outfileName, ios_base::binary);
//  for (unsigned int i = 0; i < tiles.size(); i++) {
//    unsigned char buffer[0x10000];
//    int sz = writeGraphic8bpp(tiles[i], buffer);
//    tmp.write((char*)buffer, sz);
//  }
  
/*  // Read header data:
  // 4b offset to compressed image data
  // 4b offset to tilemap
  BufferedFile imgchunk = getFile(imgchunkname);

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
    compressedDataSize, decompressedData);
    
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
  
  delete imgchunk.buffer; */
  
  return 0;
}
