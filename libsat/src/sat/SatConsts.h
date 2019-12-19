#ifndef SATCONSTS_H
#define SATCONSTS_H


namespace Sat {

  
  /**
   * Enum of VDP color modes.
   * Enum numbering corresponds to internal mode ID.
   */
  namespace SatColorModes {
    enum SatColorMode {
      colorBank16   = 0,
      lookupTable16 = 1,
      colorBank64   = 2,
      colorBank128  = 3,
      colorBank256  = 4,
      rgb           = 5,
      invalid6 = 6,
      invalid7 = 7
    };
  }
  
  namespace SatTransparencyModes {
    enum SatTransparencyMode {
      on,
      off
    };
  }


}


#endif
