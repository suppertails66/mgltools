#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "utf8.h"
#include "util/TStringConversion.h"
#include "util/ByteConversion.h"

using namespace std;
using namespace BlackT;

typedef vector<short int> BigChars;

// Offset at which to start placing data in KANJI.FNT
const static int kanjiStartingPos = 0x4800;

// Load address of KANJI.FNT
const static unsigned int kanjiLoadAddr = 0x002f2000;

// Amount the game adds to SXX.FLD chunk offsets to produce physical
// addresses (which we need to know in order to redirect those offsets
// to KANJI.FNT)
const static unsigned int sxxLoadAddr = 0x06010000;

const static int sectorSize = 0x800;

// Text printing opcodes
const static char opcodeTerminateMessage  = 0x00;
const static char opcodeWaitForInput      = 0x08;
const static char opcodeClearBox          = 0x0C;
const static char opcodePausePrinting     = 0x0D;

int chunkIndexEntrySize = 8;

int fsize(istream& ifs) {
  int old = ifs.tellg();
  ifs.seekg(0, ios_base::end);
  int sz = ifs.tellg();
  ifs.seekg(old);
  return sz;
}

int findDelimiter(BigChars& chars, int pos, string delimiters) {
/*  bool escaping = false;
  while (pos < chars.size()) {
    if (chars[pos] == (int)'"') escaping = !escaping;
    else if (!escaping) {
      // check for each delimiter
      for (int j = 0; j < delimiters.size(); j++) {
        if (chars[pos] == (int)(delimiters[j])) return pos;
      }
    }
    
    ++pos;
  }
  
  // reached EOF: assume end of row
  return pos; */
  
  while (pos < chars.size()) {
    // check for each delimiter
    for (int j = 0; j < delimiters.size(); j++) {
      if (chars[pos] == (int)(delimiters[j])) return pos;
    }
    
    ++pos;
  }
  
  // reached EOF: assume end of row
  return pos;
}

void bigCharsToString(BigChars& chars, int pos, int endpos, string& dst) {
  for (int i = pos; i < endpos; i++) {
    // input better be ascii!
    dst += (char)(chars[i]);
  }
}

void bigCharsToString(BigChars& chars, string& dst) {
  // input better be ascii!
  for (int i = 0; i < chars.size(); i++) {
    dst += (char)(chars[i]);
  }
}

int readCsvFieldToBigChars(BigChars& chars, int pos, BigChars& dst) {
  int endpos;
  bool isEscaped = false;
  // if cell is escaped, search for quotation mark delimiter
  if (chars[pos] == '"') {
    isEscaped = true;
    ++pos;
    endpos = findDelimiter(chars, pos, "\"");
  }
  // otherwise, search for comma or newline
  else {
    endpos = findDelimiter(chars, pos, ",\n");
  }
  
  for (int i = pos; i < endpos; i++) {
    // fuck fucking shit
    // why is libreoffice using double right quotes instead of real ones
    if (chars[i] == 0x201D) {
      dst.push_back('"');
    }
    else {
      dst.push_back(chars[i]);
    }
  }
  
  // if cell is escaped, skip end quote
  if (isEscaped) ++endpos;
  
  return endpos + 1;
}

int readCsvFieldToString(BigChars& chars, int pos, string& dst) {
  BigChars temp;
  int endpos = readCsvFieldToBigChars(chars, pos, temp);
  bigCharsToString(temp, dst);
  return endpos;
}

struct TransFileEntry {
  int chunk;
  int offset;
  BigChars character;
  BigChars expression;
  BigChars japanese;
  BigChars english;
  
  int read(BigChars& csv16, int pos) {
  
    // field 1: chunk
    string chunkstr;
    pos = readCsvFieldToString(csv16, pos, chunkstr);
    chunk = TStringConversion::stringToInt(chunkstr);
    
    // field 2: offset
    string offsetstr;
    pos = readCsvFieldToString(csv16, pos, offsetstr);
    offset = TStringConversion::stringToInt(offsetstr);
    
    // field 3: character
    pos = readCsvFieldToBigChars(csv16, pos, character);
    
    // field 4: expression
    pos = readCsvFieldToBigChars(csv16, pos, expression);
    
    // field 5: japanese
    pos = readCsvFieldToBigChars(csv16, pos, japanese);
    
    // field 6: english
    pos = readCsvFieldToBigChars(csv16, pos, english);
    
    return pos;
  }
};

int countConsecutiveNewlines(string& chars, int pos) {
  int endpos = pos;
  while ((endpos < chars.size()) && (chars[endpos++] == '\n'));
  return (endpos - pos) - 1;
}

char readNextChar(string& chars, int& pos) {
  // control characters
  if ((chars[pos] == '\\') && (pos < (chars.size() - 1))) {
    switch (chars[pos + 1]) {
    // clear box
    case 'c':
      pos += 2;
      return opcodeClearBox;
      break;
    // pause
    case 'p':
      pos += 2;
      return opcodePausePrinting;
      break;
    // ???????
    default:
      return chars[(pos++)];
      break;
    }
  }
  // newline: check if a literal newline or a wait-for-input command
  else if (chars[pos] == '\n') {
    int newlines = countConsecutiveNewlines(chars, pos);
    
//    cout << newlines << endl;
    
    // 2+ newlines: wait-for-input
    if (newlines >= 2) {
      pos += 2;
      return opcodeWaitForInput;
    }
    // 1 newline: literal
    else {
      ++pos;
      return '\n';
    }
  }
  // assume that if we see a '[', it's the start of a "[NOWAIT]"
  else if (chars[pos] == '[') {
    ++pos;
    return opcodeTerminateMessage;
  }
  // literal
  else {
    return chars[(pos++)];
  }
}

void updateChunk(char* buffer, int realChunkSize,
                 vector<TransFileEntry>& transFileEntries,
                 int startIndex, int endIndex,
                 char* kanjiBuffer, int& kanjiPos) {
  // Get offset of the dialogue offset table
  int scriptchunkOffset = ByteConversion::fromBytes(buffer + 4, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
  int dialogueOffsetTableOffset = ByteConversion::fromBytes(
      buffer + scriptchunkOffset + 4, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
  
  // Start inserting dialogue at the offset of the first piece of dialogue
  int putpos = transFileEntries[startIndex].offset;
  for (int i = startIndex; i < endIndex; i++) {
    TransFileEntry& entry = transFileEntries[i];
    
    // offset of zero == something went wrong with reading
    if (entry.offset == 0) continue;
    
    // Offset of the offset to update for this entry
    int textOffsetOffset = dialogueOffsetTableOffset + ((i - startIndex) * 4);
    
    string englishString;
    bigCharsToString(entry.english, englishString);
    
    string formattedString;
    
    int getpos = 0;
    bool nowait = false;
    while (getpos < englishString.size()) {
      // read next character (accounting for control codes, etc.)
      char nextchar = readNextChar(englishString, getpos);
      
      // early terminator = stop!
      // terminator will be added below
      if (nextchar == opcodeTerminateMessage) {
        nowait = true;
        break;
      }
      
      // add to message
      formattedString += nextchar;
    }
    
    // add final wait-for-button command, unless nowait is on
    if (!nowait) {
      formattedString += opcodeWaitForInput;
    }
    
    // add terminator
    formattedString += opcodeTerminateMessage;
    
//    cout << dec << entry.chunk << " " << hex << entry.offset << endl;
//    cout << dec;
    
    // if there's enough room, insert into original file
    if (putpos + formattedString.size() <= realChunkSize) {
//      cout << hex << textOffsetOffset << endl;
//      cout << dec;
    
      ByteConversion::toBytes(putpos, buffer + textOffsetOffset, 4,
        EndiannessTypes::big, SignednessTypes::nosign);
      for (int i = 0; i < formattedString.size(); i++) {
        buffer[putpos++] = formattedString[i];
      }
    }
    // otherwise, insert into KANJI.FNT
    else {
      cout << "Relocating to KANJI.FNT, " << hex << kanjiPos
        << ": " << englishString << endl;
      cout << dec;
    
      // The game converts the chunk-local offsets into physical addresses
      // at runtime by adding the address to which the chunk is loaded
      // (0x06010000) to the offset. We need to change the offset in such a
      // way that when this conversion is performed, the resulting address
      // will instead point to the string's new position in KANJI.FNT, which
      // is loaded to 0x002F2000. Therefore:
      long int target = (long)kanjiLoadAddr + (long)kanjiPos;
      target -= (long)sxxLoadAddr;
      // add range of 32-bit int
      target += (long)0x100000000;
      
      ByteConversion::toBytes(target, buffer + textOffsetOffset, 4,
        EndiannessTypes::big, SignednessTypes::nosign);
      for (int i = 0; i < formattedString.size(); i++) {
        kanjiBuffer[kanjiPos++] = formattedString[i];
      }
    }
    
//    cout << formattedString << endl;
  }
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    cout << "Dialogue insertion utility for Mahou Gakuen Lunar!" << endl;
    cout << "Usage: " << argv[0] << " <csvfile> <infile>"
      " <kanjidat> <kanjitxt> <outfile>" << endl;
    return 0;
  }
  
  char* csvfile = argv[1];
  char* infile = argv[2];
  char* kanjidat = argv[3];
  char* kanjitxt = argv[4];
  char* outfile = argv[5];

  ifstream ifs;
  
  // Initialize the kanjitxt if it doesn't exist
  ifs.open(kanjitxt);
  int kanjitxtSz = fsize(ifs);
  ifs.close();
  if (kanjitxtSz <= 0) {
    ofstream ofs(kanjitxt);
    ofs << kanjiStartingPos;
  }
  
  char* buffer;
  
  // Read the entire UTF-8-encoded CSV file into memory
  ifs.open(csvfile);
  int sz = fsize(ifs);
  buffer = new char[sz];
  ifs.read(buffer, sz);
  ifs.close();
  string rawString(buffer, sz);
  delete buffer;
  
  // Convert the content to UTF-16
  BigChars csv16;
  utf8::utf8to16(rawString.begin(), rawString.end(), back_inserter(csv16));
  
  // Skip header row
  int pos = findDelimiter(csv16, 0, "\n") + 1;
  
  // Read each row
  vector<TransFileEntry> transEntries;
  while (pos < csv16.size()) {
    TransFileEntry entry;
    pos = entry.read(csv16, pos);
    transEntries.push_back(entry);
    
//    if (pos >= csv16.size() - 2) break;
  }
  
//  cout << transEntries[transEntries.size() - 1].offset << endl;
  
  // last entry is invalid because of stupidity
//  transEntries.pop_back();
  
  // Read the input FLD
  ifs.open(infile, ios_base::binary);
  int infileSz = fsize(ifs);
  buffer = new char[infileSz];
  ifs.read(buffer, infileSz);
  ifs.close();
  
  // Read the input kanjidat
  ifs.open(kanjidat, ios_base::binary);
  int kanjidatSz = fsize(ifs);
  char* kanjibuf = new char[kanjidatSz];
  ifs.read(kanjibuf, kanjidatSz);
  ifs.close();
  
  // Get the kanjidat write address from the kanjitxt
  ifs.open(kanjitxt);
  int kanjiPutPos;
  ifs >> kanjiPutPos;
  ifs.close();
  
  int startIndex = 0;
  int endIndex = 0;
  while (startIndex < transEntries.size()) {
    // Find the indices of the start and end of the dialogue for each chunk
    while (transEntries[endIndex++].chunk == transEntries[startIndex].chunk);
    --endIndex;
    
    // Get chunk number
    int chunknum = transEntries[startIndex].chunk;
    
    // Look up chunk address from index
    int indexAddr = chunknum * chunkIndexEntrySize;
    int chunkAddr = ByteConversion::fromBytes(buffer + indexAddr + 0, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    int chunkSize = ByteConversion::fromBytes(buffer + indexAddr + 4, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    
    // Compute actual size of chunk (accounting for sector-boundary padding)
    int realChunkSize = ((chunkSize / sectorSize) * sectorSize) + sectorSize;
    
    updateChunk(buffer + chunkAddr, realChunkSize,
                transEntries, startIndex, endIndex,
                kanjidat, kanjiPutPos);
    
    // Prepare for next loop
    startIndex = endIndex;
  }
  
  // Write the modified FLD
  ofstream ofs(outfile, ios_base::binary);
  ofs.write(buffer, infileSz);
  ofs.close();
  delete buffer;
  
  // Write the modified kanjidat
  ofs.open(kanjidat, ios_base::binary);
  ofs.write(kanjibuf, kanjidatSz);
  ofs.close();
  delete kanjibuf;
  
  // Update kanjitxt
  ofs.open(kanjitxt);
  ofs << kanjiPutPos;
  ofs.close();
  
  return 0;
}
