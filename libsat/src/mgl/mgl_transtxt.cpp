#include "mgl_transtxt.h"
#include "util/TSerialize.h"
#include "util/TStringConversion.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cctype>

using namespace std;
using namespace BlackT;

namespace Sat {


  // Offset that converts from regular to reduced-width ASCII font
  const int smallConvertOffset = 96;

  int fsize(std::istream& ifs) {
    int pos = ifs.tellg();
    ifs.seekg(0, std::ios_base::end);
    int sz = ifs.tellg();
    ifs.seekg(pos);
    return sz;
  }

  std::string nextToken(std::istream& ifs) {
    string str;
    ifs >> str;
    
    while (ifs.good() && str.size() && (str[0] == '#')) {
      getline(ifs, str);
      ifs >> str;
    }
    
    return str;
  }

  void escapeString(const std::string& src, std::string& dst) {
    // is this a half-width string?
    if (src.size() && 
        (((src[0] >= 161) && (src[0] <= 223))
          || (src[0] == 1)
          || (src[0] == 2))) {
      for (unsigned int i = 0; i < src.size(); i++) {
        unsigned char next = src[i];
        // katakana mode
        if ((next == 1)) {
          dst += "{k}";
        }
        // hiragana mode
        else if ((next == 2)) {
          dst += "{h}";
        }
        else {
          dst += next;
        }
      }
    }
    else {
      dst = src;
    }
    
  /*    if ((next < 0x0A) || (next >= 0xF0)) {
        char buffer[2];
        sprintf(buffer, "%02x", next);
        dst += "\\x";
        dst += buffer[0];
        dst += buffer[1];
      }
      else {
        dst += next;
      }
    } */
  }

  void escapeStringNew(const std::string& src, std::string& dst) {
    dst += "\"";
    for (int i = 0; i < src.size(); i++) {
      // check for 2-byte sjis sequences
  /*    unsigned int sjischeck = src[i];
      if ((i < src.size() - 1)
           && (((sjischeck >= 0x81) && (sjischeck <= 0x9F))
                || ((sjischeck >= 0xE0) && (sjischeck <= 0xEF)))) {
        dst += src[i];
        dst += src[i + 1];
        ++i;
        continue;
      } */
      
      // newlines
  //    if (src[i] == '\n') {
  //      dst += "\"\n\"";
  //      continue;
  //    }
      
      dst += src[i];
    }
    
    dst += "\"";
  }

  void skipWhitespace(std::istream& ifs) {
    while (ifs.good() && isspace(ifs.peek())) ifs.get();
  }

  void advance(std::istream& ifs) {
    skipWhitespace(ifs);
    while (ifs.good() && (ifs.peek() == '#')) {
      string str;
      getline(ifs, str);
      skipWhitespace(ifs);
    }
  }

  void advanceNew(std::istream& ifs) {
  //  skipWhitespace(ifs);
    while (ifs.good() && (ifs.peek() == '"')) {
      string str;
      getline(ifs, str);
      skipWhitespace(ifs);
    }
  }

  void readCsvCell(std::istream& ifs, std::string& dst) {
    dst = "";

    bool escaping = false;
    while (ifs.good()) {
      char next = ifs.get();
      // escaped literal
      if (next == '"') escaping = !escaping;
      else {
        // check for end of cell
        if (!escaping && ((next == ',') || (next == '\n'))) break;
        else dst += next;
      }
    }
  }

  void printCommented(std::ostream& ofs, std::string& str) {
    ofs << "# ";
    for (unsigned int i = 0; i < str.size(); i++) {
      ofs << str[i];
      
      if (str[i] == '\n') {
        ofs << "# ";
      }
    }
    ofs << endl;
  }

  void readStringLiteral(std::istream& ifs, std::string& str) {
    str = "";

    advance(ifs);
    
    // skip initial "<\n"
    string temp;
    getline(ifs, temp);
    
  //  if (ifs.tellg() >= 0x1892e) {
  //    cout << hex << ifs.tellg() << " " << temp << endl;
  //  }
    
    while (true) {
      // check for 2-byte SJIS sequences
      unsigned int sjischeck = ifs.peek();
      if (((sjischeck >= 0x81) && (sjischeck <= 0x9F))
          || ((sjischeck >= 0xE0) && (sjischeck <= 0xEF))) {
        str += ifs.get();
        str += ifs.get();
        continue;
      }
    
      // check escape sequences
      //
      // BUG: this is used to read the original text, which consists of 16-bit
      // SJIS characters, but they're read byte-by-byte, causing the low
      // byte to sometimes be interpreted as a backslash
      // the only real consequence of this is that auto-computed lengths
      // may be shorter than the original, which causes no harm
      // ... except in the very bad case that the fake backslash is the last
      // character before the terminator, which would make everything go
      // to hell
      //
      // FIXME!
      if (ifs.peek() == '\\') {
        ifs.get();
        
        if (ifs.peek() == 'x') {
          ifs.get();
          
          // read 2-digit hex literal
          string value;
          value += ifs.get();
          value += ifs.get();
          istringstream iss;
          iss.str(value);
          int c = 0;
          iss >> hex >> c;
          str += (char)c;
          continue;
        }
        else {
          str += ifs.get();
          continue;
        }
      }
      
      char next = ifs.get();
      // terminator: "\n>"
      if ((next == '\n') && (ifs.peek() == '>')) break;
      
      str += next;
    }
    
    ifs.get();
  }

  void readStringLiteralNew(std::istream& ifs, std::string& str) {
    str = "";

  //  advance(ifs);
    
    // skip initial "<\n"
  //  string temp;
  //  getline(ifs, temp);
    
    bool escaping = false;
    bool smallconvert = false;
    while (true) {
      // check for 2-byte SJIS sequences
      unsigned int sjischeck = ifs.peek();
      if (((sjischeck >= 0x81) && (sjischeck <= 0x9F))
          || ((sjischeck >= 0xE0) && (sjischeck <= 0xEF))) {
        str += ifs.get();
        str += ifs.get();
        continue;
      }
      
      // escaped literals
      if (ifs.peek() == '"') {
        ifs.get();
        escaping = !escaping;
        continue;
      }
    
      // check escape sequences
      if (ifs.peek() == '\\') {
        ifs.get();
        
        if (ifs.peek() == 'x') {
          ifs.get();
          
          // read 2-digit hex literal
          string value;
          value += ifs.get();
          value += ifs.get();
          istringstream iss;
          iss.str(value);
          int c = 0;
          iss >> hex >> c;
          str += (char)c;
          continue;
        }
        else if (ifs.peek() == 's') {
          // toggle small font conversion
          ifs.get();
          smallconvert = !smallconvert;
          continue;
        }
        else {
          str += ifs.get();
          continue;
        }
      }
      
      char next = ifs.get();
      // terminator: non-escaped comma or newline
      if (!escaping && ((next == ',') || (next == '\n'))) break;
      
      if (smallconvert)  str += (next + smallConvertOffset);
      else str += next;
    }
    
  //  ifs.get();
  }

  void escapeRawString(const std::string& src, std::string& dst) {
    dst = "";
    
    bool smallconvert = false;
    int pos = 0;
    while (pos < src.size()) {
      // check escape sequences
      if ((pos < src.size() - 1) && (src[pos] == '\\')) {
        ++pos;
        
        if (src[pos] == 'x') {
          ++pos;
          
          // read 2-digit hex literal
          string value;
          value += src[pos++];
          value += src[pos++];
          istringstream iss;
          iss.str(value);
          int c = 0;
          iss >> hex >> c;
          dst += (char)c;
          continue;
        }
        else if (src[pos] == 's') {
          // toggle small font conversion
          ++pos;
          smallconvert = !smallconvert;
          continue;
        }
        else {
          dst += src[pos++];
          continue;
        }
      }
      
      char next = src[pos++];
      
      if (smallconvert) dst += (next + smallConvertOffset);
      else dst += next;
    }
  }
    
  void TranslationEntry::save(std::ostream& ofs) {
    ofs << "#############################" << endl;
    ofs << "### source file and offset" << endl;
    ofs << sourceFile << " " << hex << sourceFileOffset << endl;
    ofs << endl;
    
    ofs << "### pointers" << endl;
    ofs << pointers.size();
    for (unsigned int i = 0; i < pointers.size(); i++) {
      ofs << " " << pointers[i];
    }
    ofs << endl;
    ofs << endl;
    
    ofs << "### size" << endl;
  //  ofs << originalSize << endl;
    if (originalSize == originalText.size()) ofs << dec << -1 << endl;
    else ofs << dec << originalSize << endl;
    ofs << endl;
    
    ofs << "### original" << endl;
    ofs << "<" << endl;
    ofs << originalText << endl;
    ofs << ">" << endl;
    ofs << endl;
    
    ofs << "### translation" << endl;
    ofs << "<" << endl;
    ofs << translatedText << endl;
    ofs << ">" << endl;
    ofs << "#############################" << endl;
    
    ofs << endl;
  }
    
  void TranslationEntry::saveNew(std::ostream& ofs) {
    ofs << sourceFile << "," << "0x" << hex << sourceFileOffset << ",";
    
    ofs << "0x" << hex << pointers.size();
    for (unsigned int i = 0; i < pointers.size(); i++) {
      ofs << " " << "0x" << hex << pointers[i];
    }
    ofs << ",";
    
    ofs << "0x" << hex << originalSize << ",";
    
  //  ofs << originalText << ",";
  //  ofs << translatedText;

    string originalTextEscaped;
    string translatedTextEscaped;
    escapeStringNew(originalText, originalTextEscaped);
    escapeStringNew(translatedText, translatedTextEscaped);
    
    ofs << originalTextEscaped << ",";
    ofs << translatedTextEscaped;
    
    ofs << ",";
    
    ofs << endl;
  }

  void TranslationEntry::load(std::istream& ifs) {
    advance(ifs);
    ifs >> sourceFile;
    
    advance(ifs);
    ifs >> hex >> sourceFileOffset;
    
    advance(ifs);
    unsigned int numPointers;
    ifs >> numPointers;

    for (unsigned int i = 0; i < numPointers; i++) {
      advance(ifs);
      unsigned int p;
      ifs >> p;
      pointers.push_back(p);
    }
    
    advance(ifs);
    ifs >> dec >> originalSize;
    
  //  if (sourceFileOffset == 0x46c64) {
  //    cout << hex << ifs.tellg() << endl;
  //  }
    
    readStringLiteral(ifs, originalText);
    readStringLiteral(ifs, translatedText);
    
  //  if (sourceFileOffset == 0x46c64) {
  //    for (int i = 0; i < originalText.size(); i++) {
  //      cout << hex << (unsigned int)((unsigned char)(originalText[i])) << " ";
  //    }
  //    cout << endl;
  //  }
    
    if ((signed int)originalSize == -1) originalSize = originalText.size();
  }

  void TranslationEntry::loadNew(std::istream& ifs) {
    readCsvCell(ifs, sourceFile);
    
    istringstream iss;
    
    string sourceFileOffsetCell;
    readCsvCell(ifs, sourceFileOffsetCell);
    iss.str(sourceFileOffsetCell);
    iss.clear();
    
    iss >> hex >> sourceFileOffset;
    
    string pointersCell;
    readCsvCell(ifs, pointersCell);
    iss.str(pointersCell);
    iss.clear();
    
    unsigned int numPointers;
    iss >> hex >> numPointers;
    
  //  cout << numPointers << endl;
  //  cout << iss.str() << endl;
  //  char c; cin >> c;

    for (unsigned int i = 0; i < numPointers; i++) {
      unsigned int p;
      iss >> hex >> p;
      pointers.push_back(p);
    }
    
    string originalSizeCell;
    readCsvCell(ifs, originalSizeCell);
    iss.str(originalSizeCell);
    iss.clear();
    
    iss >> hex >> originalSize;
    
    readStringLiteralNew(ifs, originalText);
    readStringLiteralNew(ifs, translatedText);
    
    // comments field
    string commentsCell;
    readCsvCell(ifs, commentsCell);
    
  //  cout << sourceFile << " " << translatedText << " " << numPointers;
  //  char c; cin >> c;
    
    if ((signed int)originalSize == -1) originalSize = originalText.size();
  }

  void TranslationEntry::fromCsvUtf8(std::vector<BigChars>& fields) {
    if (fields.size() < 6) {
      cerr << "Error: fromCsvUtf8: Not enough fields ("
        << fields.size() << ")" << endl;
      return;
    }
    
    bigCharsToString(fields[0], sourceFile);
    
    string offset;
    bigCharsToString(fields[1], offset);
    sourceFileOffset = TStringConversion::stringToInt(offset);
    
    string pointersString;
    bigCharsToString(fields[2], pointersString);
    
    // fuck the c++ stdlib
    // forever
    istringstream iss(pointersString);
    string numPointersString;
    iss >> numPointersString;
    int numPointers = TStringConversion::stringToInt(numPointersString);
    
    for (int i = 0; i < numPointers; i++) {
      string pointString;
      iss >> pointString;
      int point = TStringConversion::stringToInt(pointString);
      pointers.push_back(point);
    }
    
    string originalSizeString;
    bigCharsToString(fields[3], originalSizeString);
    iss.str(originalSizeString);
    originalSize = TStringConversion::stringToInt(originalSizeString);
    
    // ignore original text -- we don't need it
    
    string translatedTextRaw;
    bigCharsToString(fields[5], translatedTextRaw);
    escapeRawString(translatedTextRaw, translatedText);
  }
    
  void TranslationFile::load(std::istream& ifs) {
    while (ifs.good()) {
      TranslationEntry entry;
      entry.load(ifs);
  //    cout << entry.sourceFileOffset << endl;
      filenameEntriesMap[entry.sourceFile].push_back(entry);
      
      advance(ifs);
    }
  }
    
  void TranslationFile::loadNew(std::istream& ifs) {
    // skip header row
    string garbage;
    getline(ifs, garbage);

    while (ifs.good()) {
      TranslationEntry entry;
      entry.loadNew(ifs);
  //    cout << entry.sourceFileOffset << endl;
      filenameEntriesMap[entry.sourceFile].push_back(entry);
      
      advance(ifs);
    }
  }

  void TranslationFile::fromCsvUtf8(std::vector < std::vector<BigChars> >& csv) {
    // skip header row
    for (int i = 1; i < csv.size(); i++) {
      TranslationEntry entry;
      entry.fromCsvUtf8(csv[i]);
      filenameEntriesMap[entry.sourceFile].push_back(entry);
    }
  }
    
  void TranslationFile::save(std::ostream& ofs) {
    for (FilenameTranslationEntryMap::iterator it = filenameEntriesMap.begin();
         it != filenameEntriesMap.end();
         ++it) {
      vector<TranslationEntry>& entries = it->second;
      for (int i = 0; i < entries.size(); i++) {
        entries[i].save(ofs);
      }
    }
  }
    
  void TranslationFile::saveNew(std::ostream& ofs) {
    for (FilenameTranslationEntryMap::iterator it = filenameEntriesMap.begin();
         it != filenameEntriesMap.end();
         ++it) {
      vector<TranslationEntry>& entries = it->second;
      for (int i = 0; i < entries.size(); i++) {
        entries[i].saveNew(ofs);
      }
    }
  }

  /*int main(int argc, char* argv[]) {
    TranslationEntry e;
  //  e.sourceFile = "1";
  //  e.sourceFileOffset = 12345;
  //  e.originalText = "awieubvtoaiweubtv\noianewt\naowietn";
  //  e.translatedText = "amwetbio0\naertno\naertn";
  //  e.pointers.push_back(54321);
    
    ifstream ifs("mgl_tttest.txt");
    e.load(ifs);
    ifs.close();
    
    e.save(cout);
    
    return 0;
  } */


} // end namespace Sat

