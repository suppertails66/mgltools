#ifndef MGLTRANSTXT_H
#define MGLTRANSTXT_H


#include "csv_utf8.h"
#include <string>
#include <vector>
#include <map>

namespace Sat {


  int fsize(std::istream& ifs);

  std::string nextToken(std::istream& ifs);
  void escapeString(const std::string& src, std::string& dst);
  void escapeStringNew(const std::string& src, std::string& dst);
  void skipWhitespace(std::istream& ifs);
  void advance(std::istream& ifs);
  void advanceNew(std::istream& ifs);
  void readStringLiteral(std::istream& ifs, std::string& str);
  void readStringLiteralNew(std::istream& ifs, std::string& str);
  void readCsvCell(std::istream& ifs, std::string& dst);

  struct TranslationEntry {

    std::string sourceFile;
    unsigned int sourceFileOffset;
    
    std::string originalText;
    unsigned int originalSize;
    std::string translatedText;
    
    std::vector<unsigned int> pointers;
    
    void save(std::ostream& ofs);
    void saveNew(std::ostream& ofs);
    void load(std::istream& ifs);
    void loadNew(std::istream& ifs);
    void fromCsvUtf8(std::vector<BigChars>& fields);
    
  };

  typedef std::vector<TranslationEntry> TranslationEntryCollection;
  typedef std::map<std::string, TranslationEntryCollection>
    FilenameTranslationEntryMap;

  struct TranslationFile {
    
    FilenameTranslationEntryMap filenameEntriesMap;
    
    void load(std::istream& ifs);
    void loadNew(std::istream& ifs);
    void save(std::ostream& ofs);
    void saveNew(std::ostream& ofs);
    void fromCsvUtf8(std::vector < std::vector<BigChars> >& csv);
  };

  struct FreeSpaceEntry {
    
    unsigned int offset;
    unsigned int size;
    
  };

  struct FreeSpaceList {
    
  };


} // end namespace Sat

#endif 
