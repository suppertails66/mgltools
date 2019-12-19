#ifndef SATPATTERN_H
#define SATPATTERN_H


#include "sat/SatConsts.h"
#include "util/TArray.h"
#include "util/TTwoDArray.h"
#include "util/TStream.h"
#include "util/TGraphic.h"

namespace Sat {

  
  class SatPattern {
  public:
    SatPattern();
    SatPattern(int w__, int h__, SatColorModes::SatColorMode colorMode__);
    
    SatColorModes::SatColorMode colorMode() const;
    void setColorMode(SatColorModes::SatColorMode colorMode__);
    
    int w() const;
    int h() const;
    void resize(int w__, int h__);
    
    /**
     * Read raw-data representation of this pattern.
     * Uses current width/height/color mode settings.
     */
    void readRaw(BlackT::TStream& ifs);
    
    void drawGrayscale(BlackT::TGraphic& dst,
                       int xOffset, int yOffset,
                       SatTransparencyModes::SatTransparencyMode transparency
                         = SatTransparencyModes::on);
    
  protected:
    BlackT::TTwoDArray<int> data_;
    SatColorModes::SatColorMode colorMode_;
    
    void readRaw4Bit(BlackT::TStream& ifs);
    void readRaw8Bit(BlackT::TStream& ifs);
    void readRaw16Bit(BlackT::TStream& ifs);
    
    void drawGrayscale4Bit(BlackT::TGraphic& dst,
                       int xOffset, int yOffset,
                       SatTransparencyModes::SatTransparencyMode transparency
                         = SatTransparencyModes::on);
    
    /**
     * Scales a color value from the specified bits-per-pixel to 8-bit.
     * For example, a value of 0xF with a BPP of 4 would scale to 0xFF.
     */
    static int to8BitComponent(int rawValue, int bpp);
    
  };


}


#endif
