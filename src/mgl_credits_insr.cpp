#include "mgl_cmpr.h"
#include "util/ByteConversion.h"
#include "util/TStringConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TTwoDArray.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace BlackT;

const static int compressionBufferSize = 0x100000;

const static int tileW = 8;
const static int tileH = 8;

const static int tileSize = 0x20;

const static int vFlipFlag      = 0x80000000;
const static int hFlipFlag      = 0x40000000;
const static int tileIndexMask  = 0x3FFFFFFF;

// 40 tiles * 8 pixels per tile = 320 pixels (full screen width)
const static int tilemapW = 0x28;

const static int dataSizeLimit = 0x11800;

bool areaMatchesTile(TGraphic& img, int baseX, int baseY, TGraphic& tile) {
  for (int j = 0; j < tile.h(); j++) {
    for (int i = 0; i < tile.w(); i++) {
      int imgX = baseX + i;
      int imgY = baseY + j;
      if (img.getPixel(imgX, imgY) != tile.getPixel(i, j)) return false;
    }
  }
  
  return true;
}

void imageToGrayscale(TGraphic& src, TGraphic& dst,
                      map<int, int> colorsToIndices) {
  dst.resize(src.w(), src.h());
  
  for (int j = 0; j < src.h(); j++) {
    for (int i = 0; i < src.w(); i++) {
      int rawColor = realToSaturnColor(src.getPixel(i, j));
      map<int, int>::iterator findIt = colorsToIndices.find(rawColor);
      // Ensure the color used actually exists
      if (findIt == colorsToIndices.end()) {
        cerr << "Error: used color not in palette: " << hex << rawColor
          << " (at " << i << ", " << j << ")"
          << endl;
        exit(1);
      }
      
      int index = findIt->second;
      int value = (index << 4) | index;
      dst.setPixel(i, j,
        TColor(value, value, value,
          (index == 0) ? TColor::fullAlphaTransparency
            : TColor::fullAlphaOpacity));
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    cout << "Credits image inserter for Mahou Gakuen Lunar!" << endl;
    cout << "Usage: " << argv[0] << " <image>"
      << " <outfile> <datapos> <mappos>" << endl;
    return 0;
  }
  
  char* imagename = argv[1];
//  char* palettename = argv[2];
  char* outfilename = argv[2];
  int datapos = TStringConversion::stringToInt(string(argv[3]));
  int mappos = TStringConversion::stringToInt(string(argv[4]));
  
  // Read the source image
  TGraphic img;
  TPngConversion::RGBAPngToGraphic(string(imagename), img);
  
  // Read the palette
/*  BufferedFile palettefile = getFile(palettename);
  vector<TColor> palette;
  vector<int> rawPalette;
  for (int i = 0; i < 16; i++) {
    int rawColor = ByteConversion::fromBytes(palettefile.buffer + (i * 2), 2,
      EndiannessTypes::big, SignednessTypes::nosign);
    palette.push_back(saturnToRealColor(rawColor));
    rawPalette.push_back(rawColor);
  } */
  
  // Generate palette from source image
  vector<TColor> palette;
  vector<int> rawPalette;
  map<int, int> colorsToIndices;
  for (int j = 0; j < img.h(); j++) {
    for (int i = 0; i < img.w(); i++) {
      TColor color = img.getPixel(i, j);
      int rawColor = realToSaturnColor(color);
      
      // If color isn't already in palette, add it
      map<int, int>::iterator findIt = colorsToIndices.find(rawColor);
      if (findIt == colorsToIndices.end()) {
        palette.push_back(color);
        rawPalette.push_back(rawColor);
        colorsToIndices[rawColor] = palette.size() - 1;
      }
    }
  }
  
  if (palette.size() > 16) {
    cerr << "Error: image uses more than 16 colors!" << endl;
    exit(3);
  }
  
  // Map raw palette colors to indices
//  for (int i = 0; i < palette.size(); i++) {
//    colorsToIndices[rawPalette[i]] = i;
//  }
  
  // Initialize the tilemap
  int tilemapH = (img.h() / tileH);
  TTwoDArray<int> tilemap(tilemapW, tilemapH);
  
  // Container for tiles, as well as horizontally and/or vertically flipped
  // versions used to optimize uniqueness evaluation
  vector<TGraphic> tiles;
  vector<TGraphic> tilesFlippedX;
  vector<TGraphic> tilesFlippedY;
  vector<TGraphic> tilesFlippedXY;
  
  // Iterate over every 8x8 area of the image, and do the following:
  // 1. Evaluate this area to see if it matches an existing tile, and thus
  //    can be reused (also accounting for horizontal/vertical flipping).
  // 2. If this is a unique tile, generate a corresponding new tile.
  // 3. Update the tilemap for this position.
  for (int j = 0; j < tilemap.h(); j++) {
    for (int i = 0; i < tilemap.w(); i++) {
      int tileX = (i * tileW);
      int tileY = (j * tileH);
      
      int tileID = 0;
      
      // Check against each tile for uniqueness
      bool isUnique = true;
      for (int k = 0; k < tiles.size(); k++) {
        
        // normal
        if (areaMatchesTile(img, tileX, tileY, tiles[k])) {
          isUnique = false;
          tileID |= (k & tileIndexMask);
          break;
        }
        // vflip
        else if (areaMatchesTile(img, tileX, tileY, tilesFlippedY[k])) {
          isUnique = false;
          tileID |= (k & tileIndexMask);
          tileID |= vFlipFlag;
          break;
        }
        // hflip
        else if (areaMatchesTile(img, tileX, tileY, tilesFlippedX[k])) {
          isUnique = false;
          tileID |= (k & tileIndexMask);
          tileID |= hFlipFlag;
          break;
        }
        // bothflip
        else if (areaMatchesTile(img, tileX, tileY, tilesFlippedXY[k])) {
          isUnique = false;
          tileID |= (k & tileIndexMask);
          tileID |= vFlipFlag;
          tileID |= hFlipFlag;
          break;
        }
        
      }
      
      // If tile isn't unique, no need to create new tile
      if (!isUnique) {
        tilemap.data(i, j) = tileID;
      }
      else {
        // Tile is unique: create new tile
        TGraphic tile(tileW, tileH);
        tile.copy(img,
          TRect(0, 0, tileW, tileH),
          TRect(tileX, tileY, tileW, tileH));
        tiles.push_back(tile);
        tile.flipHorizontal();
        tilesFlippedX.push_back(tile);
        tile.flipVertical();
        tilesFlippedXY.push_back(tile);
        tile.flipHorizontal();
        tilesFlippedY.push_back(tile);
        
        // Assign new tile ID to tilemap
        tilemap.data(i, j) = tiles.size() - 1;
      }
      
    }
  }
  
  BufferedFile outfile = getFile(outfilename);
  
  // Update data
  
  // Write palette
  for (int i = 0; i < rawPalette.size(); i++) {
    ByteConversion::toBytes(rawPalette[i],
      outfile.buffer + datapos + 0 + (i * 2), 2,
      EndiannessTypes::big, SignednessTypes::nosign);
  }
  
  // Convert tiles to data
  char compressionBuffer[compressionBufferSize];
  int dataPutPos = 0;
  for (int i = 0; i < tiles.size(); i++) {
    TGraphic convertedTile;
    imageToGrayscale(tiles[i], convertedTile, colorsToIndices);
//    datapos += writeGraphic4bpp(convertedTile,
//      (unsigned char*)(outfile.buffer) + datapos + dataPutPos);
    dataPutPos += writeGraphic4bpp(convertedTile,
      (unsigned char*)compressionBuffer + dataPutPos);
  }
  
  // Compress data
  int compressedDataSize = compressMglImg((unsigned char*)compressionBuffer,
                                          dataPutPos,
                                          (unsigned char*)(outfile.buffer)
                                            + datapos + paletteSize + 4);
  
//  cout << tiles.size() << " " << compressedDataSize << endl;
  
  // Check that we didn't overflow the prescribed limits
  int totalDataSize = compressedDataSize + paletteSize + 4;
  if (totalDataSize > dataSizeLimit) {
    cerr << "Error: compressed credits image data too big!" << endl;
    exit(2);
  }
  
  // Write compressed data size
  ByteConversion::toBytes(compressedDataSize,
    outfile.buffer + datapos + paletteSize + 0, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
  
  // asdifuabgsdflksadgaerglasdkng, I should have handled this differently
  // update the size of the 6th chunk entry, since that's the only one
  // we need to target
  ByteConversion::toBytes(totalDataSize,
    outfile.buffer + (6 * 8) + 4, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
    
  // Update tilemap
  
  // Write tilemap height
  ByteConversion::toBytes(tilemapH, outfile.buffer + mappos + 0, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
      
  // Write tile IDs to outfile
  int tileNumber = 0;
  for (int j = 0; j < tilemap.h(); j++) {
    for (int i = 0; i < tilemap.w(); i++) {
      ByteConversion::toBytes(tilemap.data(i, j),
        outfile.buffer + mappos + 4 + (tileNumber * 4), 4,
        EndiannessTypes::big, SignednessTypes::nosign);
      ++tileNumber;
    }
  }
  
  saveFile(outfile, outfilename);
  
//  delete palettefile.buffer;
  delete outfile.buffer;
  
  return 0;
}
