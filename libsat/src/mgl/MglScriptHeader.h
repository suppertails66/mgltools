#ifndef MGLSCRIPTHEADER_H
#define MGLSCRIPTHEADER_H


namespace Sat {


  class MglScriptHeader {
  public:
    const static int size = 8;

    int id;
    int offset;
    
    void read(unsigned char* src);
  protected:
    
  };


} // end namespace Sat


#endif
