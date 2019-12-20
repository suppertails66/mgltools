#include "mgl/ScriptChunk.h"

using namespace BlackT;

namespace Sat {

  void ScriptChunk::read(unsigned char* src, int srcsize) {
    unknown1 = ByteConversion::fromBytes(src + 0, 2,
      EndiannessTypes::big, SignednessTypes::nosign);
    unknown2 = ByteConversion::fromBytes(src + 2, 2,
      EndiannessTypes::big, SignednessTypes::nosign);
  
    int scriptInfoOffset = ByteConversion::fromBytes(src + 4, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    
    int numDialogues = ByteConversion::fromBytes(
      src + scriptInfoOffset + 0, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    int dialoguesOffset = ByteConversion::fromBytes(
      src + scriptInfoOffset + 4, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    int numScripts = ByteConversion::fromBytes(
      src + scriptInfoOffset + 8, 4,
      EndiannessTypes::big, SignednessTypes::nosign);
    
    scriptHeaders.clear();
    for (int i = 0; i < numScripts; i++) {
      MglScriptHeader header;
      header.read(src + scriptInfoOffset + 12 + (i * MglScriptHeader::size));
      
      scriptHeaders.push_back(header);
    }
    
    // scripts are located starting at the offset of the first script
    // and ending at the start of the first dialogue chunk
    if ((numScripts > 0) && (numDialogues > 0)) {
      int scriptStartOffset = scriptHeaders[0].offset;
      int scriptEndOffset = ByteConversion::fromBytes(
        src + dialoguesOffset + 0, 4,
        EndiannessTypes::big, SignednessTypes::nosign);
      int scriptSize = scriptEndOffset - scriptStartOffset;
      
      scriptData.resize(scriptSize);
      memcpy(scriptData.data(), src + scriptStartOffset, scriptSize);
    }
    
    dialogueOffsets.clear();
    for (int i = 0; i < numDialogues; i++) {
      int dialogueOffset = ByteConversion::fromBytes(
        src + dialoguesOffset + (i * 4), 4,
        EndiannessTypes::big, SignednessTypes::nosign);
      dialogueOffsets.push_back(dialogueOffset);
      
      int nextOffset;
      if (i < (numDialogues - 1)) {
        nextOffset = ByteConversion::fromBytes(
          src + dialoguesOffset + ((i + 1) * 4), 4,
          EndiannessTypes::big, SignednessTypes::nosign);
      }
      else {
        nextOffset = srcsize;
      }
      
      std::string dialogue;
      
      int endcheck = dialogueOffset;
      bool endedWith08 = false;
      while (endcheck < nextOffset) {
        unsigned char next = src[endcheck];
        
        // 2-byte SJIS sequences
        if (next >= 0x80) {
          dialogue += (char)next;
          dialogue += (char)(src[endcheck + 1]);
          
          endcheck += 2;
          
          endedWith08 = false;
          
          continue;
        }
        
        // 0x08 = pause for player input
        if (next == 0x08) {
          // add double linebreak *only* if more text follows
          if (morePrintableContentExists(src + endcheck + 1)) {
            dialogue += "\n\n";
          }
          ++endcheck;
          
          endedWith08 = true;
          
          continue;
        }
        
        // 0x0C = clear text box, continue printing
        if (next == 0x0C) {
          // this is sometimes misused at the beginning or end of a piece
          // of dialogue, where it has no effect
          // we ignore it in those cases to avoid clutter
          if ((endcheck != dialogueOffset)
              && morePrintableContentExists(src + endcheck + 1)) {
            dialogue += "\\c";
          }
          ++endcheck;
          continue;
        }
        
        // 0x0D = printing delay?
        if (next == 0x0D) {
          dialogue += "\\p";
          ++endcheck;
          continue;
        }
      
        // 00 = terminator
        if ((src[endcheck] == 0x00)) {
          break;
        }
        
        // anything else: add to string
        dialogue += next;
        ++endcheck;
        // newlines don't count as "printable input"
        // this fixes a rare issue where, if a wait-for-button opcode is
        // followed by one or more newlines and then the message terminator,
        // it will be incorrectly detected as a NOWAIT
        // the fact that this error was even possible doesn't speak well of
        // the utilities used to write this game's dialogue
        if (next != '\n') endedWith08 = false;
      }
      
      // If the dialogue window was *not* closed with a 0x08 command,
      // mark it as such; otherwise, this is implicit to avoid needless
      // clutter
      if (!endedWith08) {
        dialogue += "[NOWAIT]";
      }
      
      dialogues.push_back(dialogue);
    }
  }
  
//  void ScriptChunk::write(BlackT::TStream& ofs) const {
//    
//  }
  
  bool ScriptChunk::morePrintableContentExists(unsigned char* str) {
    while (true) {
      if (*str == 0x00) return false;
      else if (*str < 0x20) ++str;
      else return true;
    }
  }


} // end namespace Sat
