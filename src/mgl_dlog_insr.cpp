#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "util/utf8.h"
#include "mgl/csv_utf8.h"
#include "mgl/mgl_cmpr.h"
#include "mgl/FldFile.h"
#include "mgl/ScriptChunk.h"
#include "util/TStringConversion.h"
#include "util/ByteConversion.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TFileManip.h"
#include "exception/TGenericException.h"

using namespace std;
using namespace BlackT;
using namespace Sat;

typedef vector<short int> BigChars;

// TODO: remove KANJI.FNT-related stuff.
// we no longer use it and probably won't need it.
// keeping it around just in case.

// Offset at which to start placing data in KANJI.FNT
const static int kanjiStartingPos = 0x4800;

// Load address of KANJI.FNT
const static unsigned int kanjiLoadAddr = 0x002f2000;

// Amount the game adds to SXX.FLD chunk offsets to produce physical
// addresses (which we need to know in order to redirect those offsets
// to KANJI.FNT)
const static unsigned int sxxLoadAddr = 0x06010000;

const static int sectorSize = 0x800;

const static int maxKanjidatSz = 0xE000;

// Text printing opcodes
const static char opcodeTerminateMessage  = 0x00;
const static char opcodeWaitForInput      = 0x08;
const static char opcodeClearBox          = 0x0C;
const static char opcodePausePrinting     = 0x0D;

struct TransFileEntry {
  int chunk;
  int offset;
  BigChars character;
  BigChars expression;
  BigChars japanese;
  BigChars english;
  
  void read(vector<BigChars>& csv16) {
    // field 1: chunk
    string chunkstr;
    readCsvFieldToString(csv16[0], 0, chunkstr);
    chunk = TStringConversion::stringToInt(chunkstr);
    
    // field 2: offset
    string offsetstr;
    readCsvFieldToString(csv16[1], 0, offsetstr);
    offset = TStringConversion::stringToInt(offsetstr);
    
    // field 3: character
    character = csv16[2];
    
    // field 4: expression
    expression = csv16[3];
    
    // field 5: japanese
    japanese = csv16[4];
    
    // field 6: english
    english = csv16[5];
  }
};

int countConsecutiveNewlines(string& chars, int pos) {
  int count = 0;
  
  while (pos < chars.size()) {
    // screen for windows-style line \r\n breaks
    if ((pos <= (chars.size() - 2))
        && (chars[pos] == '\r')) ++pos;
    
    if (chars[pos++] != '\n') break;
    
    ++count;
  }
  
  return count;
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
  else if ((chars[pos] == '\r') || (chars[pos] == '\n')) {
    // HACK (but probably will work fine): assume if we see a \r, this is
    // the start of a series of windows-style linebreaks
    bool windowsStyleBr = (chars[pos] == '\r');
    
    int newlines = countConsecutiveNewlines(chars, pos);
    
    // 2+ newlines: wait-for-input
    if (newlines >= 2) {
      if (windowsStyleBr) pos += 4;
      else pos += 2;
      return opcodeWaitForInput;
    }
    // 1 newline: literal
    else {
      if (windowsStyleBr) pos += 2;
      else ++pos;
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

std::string formatRawString(std::string input) {
  std::string result;
  
  int getpos = 0;
  bool nowait = false;
  while (getpos < input.size()) {
    // read next character (accounting for control codes, etc.)
    char nextchar = readNextChar(input, getpos);
    
    // early terminator = stop!
    // terminator will be added below
    if (nextchar == opcodeTerminateMessage) {
      nowait = true;
      break;
    }
    
    // add to message
    result += nextchar;
    
    // HACK: assume that if we encounter a "wait" command, we
    // will always clear the box afterward.
    // i originally intended for this to be tracked explicitly in the
    // script file, with two consecutive linebreaks performing a "wait"
    // but not moving to the next line when the text continues.
    // but no one in their right mind wants to keep
    // track of two vs. three linebreaks having a semantic difference.
    // if this actually turns out to matter, we can find a workaround.
    if ((nextchar == opcodeWaitForInput)
        && (getpos < input.size())) {
      // skip over any trailing linebreaks
      while ((getpos < input.size())
              && ((input[getpos] == '\r')
                  || (input[getpos] == '\n'))) ++getpos;
      
      if (getpos < input.size()) {
        // peek at next character
        int oldpos = getpos;
        nextchar = readNextChar(input, getpos);
        getpos = oldpos;
        
        // if not a clear command, add a linebreak
        if (nextchar != opcodeClearBox) {
          result += '\n';
        }
      }
    }
  }
  
  // add final wait-for-button command, unless nowait is on
  if (!nowait) {
    result += opcodeWaitForInput;
  }
  
  // add terminator
  result += opcodeTerminateMessage;
  
  return result;
}

void updateChunk(BlackT::TArray<TByte>& rawChunkData,
                 vector<TransFileEntry>& transFileEntries,
                 int startIndex, int endIndex,
                 TStream& kanjiBuffer) {
  // do nothing if no strings
  if (transFileEntries.size() == 0) return;
  
  //============================================
  // set up using info from old chunk
  //============================================
  
  // size of old chunk
  int oldChunkSize = rawChunkData.size();

  // convert raw chunk to a TBufStream for editing
  TBufStream ofs;
  for (unsigned int i = 0; i < rawChunkData.size(); i++)
    ofs.put(rawChunkData[i]);
  
  // get offset of the dialogue offset table
  ofs.seek(4);
  int scriptchunkPos = ofs.readu32be();
  ofs.seek(scriptchunkPos + 4);
  int dialogueOffsetTablePos = ofs.readu32be();
  
  // initial putpos = first dialogue in chunk
  int oldDialogueChunkStartPos = transFileEntries[startIndex].offset;
  // final putpos = start of last dialogue in chunk.
  // NOTE: this is incorrect; we actually need the location of the
  // *end* of this string, not the start.
  // but i didn't save that information in the dumped CSVs, and we're
  // generally only losing a few bytes that we shouldn't need anyway,
  // so i'm not going to go too crazy over it.
  int oldDialogueChunkEndPos = transFileEntries[endIndex - 1].offset;
  
  //============================================
  // replace old dialogue strings with new
  //============================================
  
  // stream to hold new dialogue that will be appended after chunk
  TBufStream appendOfs;
  int putpos = oldDialogueChunkStartPos;
  
  for (unsigned int i = startIndex; i < endIndex; i++) {
    // index number of string within this chunk
    int dialogueIndex = i - startIndex; 
    
    // position of the string offset for this entry
    int stringOffsetPos = dialogueOffsetTablePos + (dialogueIndex * 4);
    
    //============================================
    // create the english string
    //============================================
    
    TransFileEntry& entry = transFileEntries[i];
    
    // illegal offset == something went wrong with reading
    if (entry.offset <= 0) {
      throw TGenericException(T_SRCANDLINE,
                              "updateChunk()",
                              std::string("Illegal offset ("
                                + TStringConversion::intToString(entry.offset)
                                + ") on row ")
                                + TStringConversion::intToString(i));
    }
    
    // get english string
    string englishString;
    bigCharsToString(entry.english, englishString);
    
    // reformat english string for display
    string formattedString = formatRawString(englishString);
    
    //============================================
    // find a place to put the new string
    //============================================
    
    // if there's room, place string into the old chunk
    if (putpos + formattedString.size() <= oldDialogueChunkEndPos) {
//      cout << "Placing in old chunk, " << hex << putpos
//        << ": " << englishString << endl;
//      cout << dec;

      // update string offset to target its new position
      ofs.seek(stringOffsetPos);
      ofs.writeu32be(putpos);
    
      // write string data to new position
      ofs.seek(putpos);
      ofs.write(formattedString.c_str(), formattedString.size());
      
      // move to next position in dialogue chunk
      putpos += formattedString.size();
    }
    // otherwise, put it in the append chunk
    else {
//      cout << "Relocating to append chunk, " << hex << appendOfs.tell()
//        << ": " << englishString << endl;
//      cout << dec;
      
      // write offset
      ofs.seek(stringOffsetPos);
      // append chunk goes after the entire rest of the old chunk,
      // so we include that in the offset calculation
      ofs.writeu32be(oldChunkSize + appendOfs.tell());
      
      // write string data
      appendOfs.write(formattedString.c_str(), formattedString.size());
    }
  }
  
  //============================================
  // finalize and write new data
  //============================================
  
  // append the append chunk to the old chunk
  ofs.seek(ofs.size());
  appendOfs.seek(0);
  ofs.writeFrom(appendOfs, appendOfs.size());
  
  // convert updated stream back to raw data
  rawChunkData.resize(ofs.size());
  ofs.seek(0);
  for (unsigned int i = 0; i < rawChunkData.size(); i++)
    rawChunkData[i] = (TByte)ofs.get();
  
  std::cout << "  Done: used "
    << putpos - oldDialogueChunkStartPos
    << " bytes in old chunk out of "
    << oldDialogueChunkEndPos - oldDialogueChunkStartPos
    << " available, and placed "
    << appendOfs.size()
    << " bytes in the append chunk"
    << std::endl;
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
  
  std::cout << "Preparing to read script CSV \"" << csvfile << "\""
    << std::endl;
  
  // Initialize the kanjitxt if it doesn't exist
  ifs.open(kanjitxt);
  int kanjitxtSz = TFileManip::getFileSize(ifs);
  ifs.close();
  if (kanjitxtSz <= 0) {
    ofstream ofs(kanjitxt);
    ofs << kanjiStartingPos;
  }
  
  // Read in the input CSV, converting the content to UTF-16
  vector< vector<BigChars> > csv16;
  {
    ifs.open(csvfile);
    readCsvUtf8(ifs, csv16);
    ifs.close();
  }
  
  // Read each row
  vector<TransFileEntry> transEntries;
  for (int i = 1; i < csv16.size(); i++) {
    TransFileEntry entry;
    entry.read(csv16[i]);
    transEntries.push_back(entry);
  }
  
  // Read the input FLD
  FldFile fld;
  {
    TIfstream ifs(infile, ios_base::binary);
    fld.read(ifs);
  }
  
  // Get the kanjidat write address from the kanjitxt
  ifs.open(kanjitxt);
  int kanjiPutPos;
  ifs >> kanjiPutPos;
  ifs.close();
  
  // open kanji file
  TBufStream kanjiBuf;
  kanjiBuf.open(kanjidat);
  // seek to putpos
  kanjiBuf.seek(kanjiPutPos);
  
  int startIndex = 0;
  int endIndex = 0;
  while (startIndex < transEntries.size()) {
    // Find the indices of the start and end of the dialogue for each chunk
    while (transEntries[endIndex].chunk == transEntries[startIndex].chunk)
      ++endIndex;
    
    // Get chunk number
    int chunknum = transEntries[startIndex].chunk;
    
    if (chunknum >= fld.numChunks()) {
      throw TGenericException(T_SRCANDLINE,
                              "main()",
                              std::string("Chunknum exceeds chunks in file: ")
                                + " file = "
                                + infile
                                + ", chunks = "
                                + TStringConversion::intToString(fld.numChunks())
                                + ", chunknum = "
                                + TStringConversion::intToString(chunknum));
    }
    
    std::cout << "  Adding strings from chunk " << chunknum << std::endl;
    
    updateChunk(fld.chunk(chunknum),
                transEntries, startIndex, endIndex,
                kanjiBuf);
    
    // Prepare for next loop
    startIndex = endIndex;
  }
  
  // Write the modified FLD
  {
    TBufStream ofs;
    fld.write(ofs);
    ofs.save(outfile);
  }
  
  // Write the modified kanjidat
  kanjiBuf.save(kanjidat);
  
  // Update kanjitxt
  {
    std::ofstream ofs(kanjitxt);
    ofs << kanjiPutPos;
    ofs.close();
  }
  
  return 0;
}
