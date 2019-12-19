#include "sat/SatPattern.h"
#include "exception/TGenericException.h"
#include "util/TStringConversion.h"
#include <string>

using namespace BlackT;

namespace Sat {


  SatPattern::SatPattern()
    : colorMode_(SatColorModes::colorBank16) { }
  
  SatPattern::SatPattern(
      int w__, int h__, SatColorModes::SatColorMode colorMode__)
    : data_(w__, h__),
      colorMode_(colorMode__) { }
  
  SatColorModes::SatColorMode SatPattern::colorMode() const {
    return colorMode_;
  }
  
  void SatPattern::setColorMode(SatColorModes::SatColorMode colorMode__) {
    colorMode_ = colorMode__;
  }
  
  int SatPattern::w() const {
    return data_.w();
  }
  
  int SatPattern::h() const {
    return data_.h();
  }
  
  void SatPattern::resize(int w__, int h__) {
    data_.resize(w__, h__);
  }
  
  void SatPattern::readRaw(BlackT::TStream& ifs) {
    switch (colorMode_) {
    case SatColorModes::colorBank16:
    case SatColorModes::lookupTable16:
      readRaw4Bit(ifs);
      break;
    case SatColorModes::colorBank64:
    case SatColorModes::colorBank128:
    case SatColorModes::colorBank256:
      readRaw8Bit(ifs);
      break;
    case SatColorModes::rgb:
      readRaw16Bit(ifs);
      break;
    default:
      throw TGenericException(T_SRCANDLINE,
                              "SatPattern::readRaw()",
                              std::string("Tried to read using unsupported")
                                + "color mode: "
                                + TStringConversion::toString(colorMode_));
      break;
    }
  }
  
  void SatPattern::drawGrayscale(BlackT::TGraphic& dst,
                     int xOffset, int yOffset,
                     SatTransparencyModes::SatTransparencyMode transparency) {
    switch (colorMode_) {
    case SatColorModes::colorBank16:
    case SatColorModes::lookupTable16:
      drawGrayscale4Bit(dst, xOffset, yOffset, transparency);
      break;
    case SatColorModes::rgb:
      throw TGenericException(T_SRCANDLINE,
                              "SatPattern::drawGrayscale()",
                              std::string("Tried to draw grayscale in RGB ")
                                + "color mode");
      break;
    default:
      throw TGenericException(T_SRCANDLINE,
                              "SatPattern::readRaw()",
                              std::string("Tried to draw grayscale using ")
                                + "unsupported color mode: "
                                + TStringConversion::toString(colorMode_));
      break;
    };
    
  }
  
  int SatPattern::to8BitComponent(int rawValue, int bpp) {
    int maxValue = (1 << bpp) - 1;
    double proportion = (double)rawValue / (double)maxValue;
    return (int)(proportion * (double)255);
  }
  
  void SatPattern::drawGrayscale4Bit(BlackT::TGraphic& dst,
                     int xOffset, int yOffset,
                     SatTransparencyModes::SatTransparencyMode transparency) {
    for (int j = 0; j < h(); j++) {
      for (int i = 0; i < w(); i++) {
        int x = xOffset + i;
        int y = yOffset + j;
        
        int rawIndex = data_.data(i, j);
        
/*        if ((rawIndex != 0) && (rawIndex <= 15)) {
//          rawIndex = 15 - rawIndex;
          rawIndex = 1;
        } */
        
        int level = to8BitComponent(rawIndex, 4);
        
        TColor color = TColor(level, level, level, TColor::fullAlphaOpacity);
        
        if ((transparency == SatTransparencyModes::on)
            && (rawIndex == 0)) {
          color.setA(TColor::fullAlphaTransparency);
        }
        
        dst.setPixel(x, y, color);
      }
    }
  }
  
  void SatPattern::readRaw4Bit(BlackT::TStream& ifs) {
    bool rowWidthIsOdd = ((w() % 2) != 0);
    
    for (int j = 0; j < h(); j++) {
      for (int i = 0; i < w(); i += 2) {
        TByte next = ifs.readu8();
        
        TByte first = (next & 0xF0) >> 4;
        data_.data(i, j) = first;
        
        // no second pixel if on last row and width is odd
        if (rowWidthIsOdd
            && (i >= w() - 1)) break;
        
        TByte second = (next & 0x0F);
        data_.data(i + 1, j) = second;
      }
    }
  }
  
  void SatPattern::readRaw8Bit(BlackT::TStream& ifs) {
    for (int j = 0; j < h(); j++) {
      for (int i = 0; i < w(); i++) {
        TByte next = ifs.readu8();
        data_.data(i, j) = next;
      }
    }
  }
  
  void SatPattern::readRaw16Bit(BlackT::TStream& ifs) {
    for (int j = 0; j < h(); j++) {
      for (int i = 0; i < w(); i++) {
        int next = ifs.readu16be();
        data_.data(i, j) = next;
      }
    }
  }


}
