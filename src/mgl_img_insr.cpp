#include "mgl_cmpr.h"
#include "util/ByteConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TStringConversion.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>

using namespace std;
using namespace BlackT;

// Size of the buffer used to concatenate the VRAM data
const int vramCatBufferSize = 0x100000;

// Size of the buffer used to compress the VRAM data
const int vramCompressionBufferSize = 0x100000;

const int sectorSize = 0x800;

int main(int argc, char* argv[]) {
  
  // mgl_img_insr indexfile indexoffset outindex imagefile imageoffset outimage inpath
  if (argc < 8) {
    cout << "Mahou Gakuen Lunar! image data extractor" << endl;
    cout << "Usage: " << argv[0] << " <ifile> <ioffset> <iout>"
           << " <imgfile> <imgoffset> <imgout> <outpath>" << endl << endl;
    cout << "  ifile      Index file containing VRAM command table list\n";
    cout << "  ioffset    Offset in the file of the aforementioned list\n";
    cout << "  iout       Output filename for the modified ifile\n";
    cout << "  imgfile    File containing image data to read\n";
    cout << "  imgoffset  Offset of the aforementioned image data\n";
    cout << "  imgout     Output filename for the modified imgfile\n";
    cout << "  outpath    Directory to which to output files\n";
    
    return 0;
  }
  
  string indexFileName(argv[1]);
  int indexOffset = TStringConversion::stringToInt(string(argv[2]));
  string indexOutputFileName(argv[3]);
  string imageFileName(argv[4]);
  int imageOffset = TStringConversion::stringToInt(string(argv[5]));
  string imageOutputFileName(argv[6]);
  string inPath(argv[7]);
  
  // Buffer to hold combined image and palette data
  char vramCatBuffer[vramCatBufferSize];
  int vramCatBufferPos = 0;
  std::memset(vramCatBuffer, 0, vramCatBufferSize);
  
  // Maps of file names to their offsets in the VRAM buffer
  map<string, int> imageNameToOffset;
  map<string, int> paletteNameToOffset;
  map<string, int> imageNameToIndex;
  vector<TGraphic> images;
  
  // Load the glue file
  vector<GlueFileEntry> glueFileEntries;
  ifstream glueIfs(inPath + glueFileName);
  while (glueIfs.good()) {
    GlueFileEntry entry;
    entry.load(glueIfs);
    if (!glueIfs.good()) break;
//    cout << entry.num << endl;
    glueFileEntries.push_back(entry);
  }
  
  // Open each sequential image file, convert to 4bpp, and place in the buffer
  for (int i = 0; ; i++) {
    string baseFilename = makeImageFileName(i);
    string filename = inPath + baseFilename;
    if (!fileExists(filename.c_str())) break;
    
    TGraphic g;
    TPngConversion::RGBAPngToGraphic(filename, g);
    
    // Record image data offset
    imageNameToOffset[baseFilename] = vramCatBufferPos;
    
    // Record index position
    imageNameToIndex[baseFilename] = i;
    
    // Record image
    images.push_back(g);
    
    // Convert to 4bpp
    vramCatBufferPos += writeGraphic4bpp(g,
      (unsigned char*)vramCatBuffer + vramCatBufferPos);
    
    // Maintain required byte alignment
    vramCatBufferPos = getVramAlignedOffset(vramCatBufferPos);
  }
  
  // Open each sequential palette and place in buffer
  for (int i = 0; ; i++) {
    string baseFilename = makePaletteFileName(i);
    string filename = inPath + baseFilename;
    if (!fileExists(filename.c_str())) break;
    
    // Record palette data offset
    paletteNameToOffset[baseFilename] = vramCatBufferPos;
    
    // Copy palette into buffer
    BufferedFile file = getFile(filename.c_str());
    std::memcpy(vramCatBuffer + vramCatBufferPos, file.buffer, file.size);
    vramCatBufferPos += file.size;
    delete file.buffer;
    
    // Maintain required byte alignment
//    vramCatBufferPos = getVramAlignedOffset(vramCatBufferPos);
  }
  
  // Add trailer, if it exists, to buffer
  if (fileExists(inPath + trailerFileName)) {
    BufferedFile file = getFile((inPath + trailerFileName).c_str());
    std::memcpy(vramCatBuffer + vramCatBufferPos, file.buffer, file.size);
    vramCatBufferPos += file.size;
    delete file.buffer;
  }
  
//  ofstream temp("concatenated.bin");
//  temp.write((char*)vramCatBuffer, vramCatBufferPos);
  
  // Compress the combined VRAM data
  char vramCompressionBuffer[vramCompressionBufferSize];
  int compressedDataSize
    = compressMglImg((unsigned char*)vramCatBuffer,
                   vramCatBufferPos,
                   (unsigned char*)vramCompressionBuffer);
  
  // Read the target image file
  BufferedFile imageFile = getFile((imageFileName).c_str());
  char* imageP = imageFile.buffer + imageOffset;
  int originalImageSize = ByteConversion::fromBytes(imageP + 0, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
  
  // The image data is followed by sector padding, so we have that much extra
  // space available
  if ((originalImageSize % sectorSize) != 0) {
    originalImageSize
      = ((originalImageSize / sectorSize) * sectorSize) + sectorSize;
  }
  
  // Make sure there's enough room for the new data
  if (compressedDataSize > originalImageSize) {
    cerr << "Error: compressed data too large!" << endl;
    cerr << "Original: " << originalImageSize << endl;
    cerr << "New: " << compressedDataSize << endl;
    return 1;
  }
  
  // Update the data and size
  std::memcpy(imageP + 4, vramCompressionBuffer, compressedDataSize);
  ByteConversion::toBytes(compressedDataSize, imageP + 0, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
    
  // Update the chunk size.
  // This is really hacky, but we know that the offset we were given is
  // the start of a chunk, so we can just search the index until we find
  // the chunk entry, then update its size
  int indexcheck;
  for (indexcheck = 0; indexcheck < 0x800; indexcheck += 8) {
    int offset = ByteConversion::fromBytes(
      imageFile.buffer + indexcheck, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
      
    // write updated size to chunk index
    if (offset == imageOffset) {
      ByteConversion::toBytes(
        compressedDataSize + 4, imageFile.buffer + indexcheck + 4, 4,
        EndiannessTypes::big, SignednessTypes::nosign);
      break;
    }
  }
  
  // couldn't find chunk
  if (indexcheck >= 0x800) {
    cerr << "Error: couldn't find chunk index entry" << endl;
    return 1;
  }
  
//  cout << originalImageSize << " " << compressedDataSize << endl;

  // Write to file
  saveFile(imageFile, imageOutputFileName.c_str());

  // Clean up
  delete imageFile.buffer;

  // Read the target index file
  BufferedFile indexFile = getFile((indexFileName).c_str());
  char* indexP = indexFile.buffer + indexOffset;
  
  unsigned int vramTablesPointer = ByteConversion::fromBytes(indexP + 4, 4,
    EndiannessTypes::big, SignednessTypes::nosign);
//  unsigned int numVramTableEntries = ByteConversion::fromBytes(indexP + 8, 4,
//    EndiannessTypes::big, SignednessTypes::nosign);
  
//  cout << vramTablesPointer << endl;
  
  // Update the index
  for (unsigned int i = 0; i < glueFileEntries.size(); i++) {
    GlueFileEntry& entry = glueFileEntries[i];
    
    // Update target character and palette offsets
    entry.vramTableEntry.setSourceOffset(
      imageNameToOffset[entry.imageFileName]);
    entry.vramTableEntry.setPaletteOffset(
      paletteNameToOffset[entry.paletteFileName]);
    
    // Update image dimensions *only* if the image is not purposely offset
    TGraphic& g = images[imageNameToIndex[entry.imageFileName]];
    if (!entry.vramTableEntry.isOffset()) {
      entry.vramTableEntry.setDimensions(g.w(), g.h());
    }
    
//    cout << entry.vramTableEntry.paletteOffset() << endl;
    
    // Write to index
    entry.vramTableEntry.toData(indexP + vramTablesPointer
      + (i * vramTableEntrySize));
  }

  // Write to file
  saveFile(indexFile, indexOutputFileName.c_str());

  // Clean up
  delete indexFile.buffer;
  
  return 0;
} 
