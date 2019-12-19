#ifndef MGL_CMPR_H
#define MGL_CMPR_H


#include "util/TGraphic.h"
#include "util/ByteConversion.h"
#include "util/TStringConversion.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>


namespace Sat {


  const int decmpBufferSize = 0x1000;
  const int decmpBufferWrap = 0xFFF;
  const int decmpBufferStartPos = 0xFEE;

  const int minimumLookbackSize = 3;
  const int maximumLookbackSize = 18;

  const int vramTableEntrySize = 0x18;

  // Size of a palette (assumed 4bpp / 16 16-bit entries)
  const int paletteSize = 0x20;

  const extern char* glueFileName;
  const extern char* trailerFileName;
  const extern char* paletteBaseName;

  const int vramCharacterDataAlignment = 0x20;

  struct BufferedFile {
    char* buffer;
    int size;
  };

  struct VramTableEntry {
    int cmdpmod;
    int cmdcolr;
    int cmdsrca;
    int cmdsize;
    int cmdxa;
    int cmdya;
    int cmdxb;
    int cmdyb;
    int cmdxc;
    int cmdyc;
    int cmdxd;
    int cmdyd;
    
    int width() const;
    
    int height() const;
    
    void setDimensions(int w, int h);
    
    // Returns true if cmda-cmdd specify that the image is not centered,
    // or is otherwise doing something abnormal
    // w and h are the true image dimensions (as opposed to the nominal ones in
    // cmdsize)
    bool isDoingSomethingStupid(int w, int h);
    
    int sourceOffset() const;
    
    void setSourceOffset(int offset);
    
    int colorMode() const;
    
    int paletteOffset() const;
    
    void setPaletteOffset(int offset);
    
    int fromData(const char* src);
    
    int toData(char* dst) const;
    
    void load(std::istream& ifs);
    
    void save(std::ostream& ofs) const;
  };

  struct GlueFileEntry {
    int num;
    std::string imageFileName;
    std::string paletteFileName;
    VramTableEntry vramTableEntry;
    
    void load(std::istream& ifs);
    
    void save(std::ostream& ofs) const;
  };

  int getVramAlignedOffset(int offset);

  std::string makeImageFileName(int num);

  std::string makePaletteFileName(int num);

  //int fsize(std::istream& ifs);

  BufferedFile getFile(const char* filename);

  void saveFile(BufferedFile file, const char* filename);

  bool fileExists(const std::string& filename);

  BlackT::TColor saturnToRealColor(int rawColor);

  int realToSaturnColor(BlackT::TColor color);

  // Decompresses image data, returning size of the decompressed data
  int decompressMglImg(const unsigned char* input,
                       int length,
                       unsigned char* outputBuf);

  // Special alternate decompression routine.
  // This is exactly the same as the regular one, except rather than knowing the
  // size of the compressed data and stopping when it's all been handled, we
  // now know the size of the *output* data and stop when we've done that much.
  // Don't ask me why someone thought this was necessary.
  void decompressMglImgOfSize(const unsigned char* input,
                       int length,
                       unsigned char* outputBuf);

  struct MatchResult {
    int pos;
    int len;
  };

  // Check how many characters match in two sequences, up to the
  // maximum lookback length
  MatchResult checkMatch(const unsigned char* input,
                         int length,
                         const unsigned char* check);

  // Find the best possible lookback in an input stream
  MatchResult findBestMatch(const unsigned char* input,
                            int length);

  // Compresses image data, returning size of the compressed data
  int compressMglImg(const unsigned char* rawInput,
                     int length,
                     unsigned char* outputBuf);

  inline BlackT::TColor pixelToColor4bpp(int value);

  inline BlackT::TColor pixelToColor8bpp(int value);

  void placePixel(BlackT::TGraphic& dst, BlackT::TColor color, int pixelNum);

  // Read a 4bpp graphic from raw data into a TGraphic.
  // The resulting TGraphic is grayscale, converting 0 -> #000000, 1 -> #111111,
  // etc.
  // Zero values are treated as transparent.
  void read4bppGraphicGrayscale(const unsigned char* src,
                                BlackT::TGraphic& dst,
                                int width,
                                int height);

  // Read a 4bpp graphic from raw data into a TGraphic.
  // The resulting TGraphic is colored according to the provided palette.
  // Zero values are treated as transparent.
  void read4bppGraphicPalettized(const unsigned char* src,
                                BlackT::TGraphic& dst,
                                int width,
                                int height,
                                const std::vector<BlackT::TColor>& palette);

  // Read an 8bpp graphic from raw data into a TGraphic.
  // The resulting TGraphic is grayscale, converting 0 -> #000000, 1 -> #010101,
  // etc.
  // Zero values are treated as transparent.
  void read8bppGraphicGrayscale(const unsigned char* src,
                                BlackT::TGraphic& dst,
                                int width,
                                int height);

  void read8bppGraphicPalettized(const unsigned char* src,
                                BlackT::TGraphic& dst,
                                int width,
                                int height,
                                const std::vector<BlackT::TColor>& palette);

  // Converts a graphic to 4bpp Saturn format, returning the size of the
  // converted data.
  // Input graphics are assumed to be grayscale; only the high nybble of the red
  // component is actually used to determine the color index.
  //
  // Any pixel with nonzero alpha is assumed to be transparent, i.e. color zero!
  int writeGraphic4bpp(BlackT::TGraphic& src,
                             unsigned char* dst);

  int writeGraphic8bpp(BlackT::TGraphic& src,
                             unsigned char* dst);


} // end namespace Sat


#endif
