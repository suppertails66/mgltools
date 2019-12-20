#include "mgl/mgl_cmpr.h"
#include "mgl/ScriptChunk.h"
#include "mgl/MglScriptHeader.h"
#include "util/ByteConversion.h"
#include "util/TArray.h"
#include "util/TSerialize.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstring>

using namespace std;
using namespace BlackT;
using namespace Sat;

const static int indexSize = 0x800;

struct CharacterExpression {
  char* name;
  char* expression;
};

CharacterExpression portraitMappings[] = {
  // 0 -- reserved for "no portrait"
  { NULL, NULL },
  // 1
  { "Elie", "neutral" },
  { "Elie", "serious" },
  { "Elie", "sad" },
  { "Elie (v2)", "neutral" },
  { "Elie (v2)", "serious" },
  { "Lena", "neutral" },
  { "Lena", "serious" },
  { "Lena", "happy" },
  { "Senia", "neutral" },
  { "Senia", "serious" },
  // 11
  { "Senia", "happy" },
  { "Blade", "neutral" },
  { "Blade", "serious" },
  { "Wynn", "neutral" },
  { "Wynn", "serious" },
  { "Wynn", "happy" },
  { "Wynn (possessed)", NULL },
  { "Azu", NULL },
  { "Glen", NULL },
  { "Ant", "neutral" },
  // 21
  { "Ant", "angry" },
  { "Layla", NULL },
  { "Steel", NULL },
  { "Dadis", NULL },
  { "Terena", NULL },
  { "Emma", NULL },
  { "Eleonora", NULL },
  { "Brown", NULL },
  { "Kule", NULL },
  { "Rick", NULL },
  // 31
  { "Shiela", NULL },
  { "D", NULL },
  { "Barua", NULL },
  { "Memphis", NULL },
  { "Hyde", NULL },
  { "Richter", NULL },
  { "Kent", NULL },
  { "Brune", NULL },
  { "Stella", NULL },
  { "Iason", NULL },
  // 41
  { "Ralph", NULL },
  { "Blue Dragon", NULL },
  { "Elie (v2)", "happy" },
  { "Elie (v2)", "sad" }
};

void doChunk(ScriptChunk& chunk, int chunkIndex) {
  
  // no dialogue or scripts means we don't care about this chunk
  if ((chunk.dialogues.size() <= 0) || (chunk.scriptHeaders.size() <= 0)) {
    return;
  }
  
  map<int, bool> usedDialogues;
  map<int, int> dialoguePortraits;
  
  int scriptpos = 0;
  int portraitnum = 0;
  while (scriptpos < chunk.scriptData.size()) {
    switch (chunk.scriptData[scriptpos]) {
    // change portrait
    case 0x16:
      {
        int op1 = chunk.scriptData[scriptpos + 1];
        int op2 = chunk.scriptData[scriptpos + 2];
        
        // sanity checks
        
        // position must be 0 or 1
        if ((op1 != 0) && (op1 != 1)) break;
        // valid portrait numbers are 0 to 2C
        // (there are 2C portrait images; index zero is reserved for "no
        // portrait")
        if ((op2 > 0x2C)) break;
        
        portraitnum = op2;
        
        scriptpos += 3;
        continue;
      }
      break;
    // print message
    case 0x17:
      {
        int op1 = chunk.scriptData[scriptpos + 1];
        int op2 = chunk.scriptData[scriptpos + 2];
        
        // sanity checks
        
        // position must be 0 or 1
//        if ((op1 != 0) && (op1 != 1)) break;
        // dialogue number must be in valid range
        if ((op2 >= chunk.dialogues.size())) break;
        // no using dialogue more than once
        if (usedDialogues.find(op2) != usedDialogues.end()) {
          cerr << "Possible dialogue reuse: " << usedDialogues[op2] << endl;
          break;
        }
        
//        cout << portraitnum << " " << chunk.dialogues[op2] << endl;
        dialoguePortraits[op2] = portraitnum;
        
        usedDialogues[op2] = true;
        
        scriptpos += 3;
        continue;
      }
      break;
    }
    
    ++scriptpos;
  }
  
  for (int i = 0; i < chunk.dialogues.size(); i++) {
    if (!usedDialogues[i]) {
      cerr << "Dialogue " << i << " not used: "
        << chunk.dialogues[i] << endl;
    }
  }
  
  // print all dialogue, with portrait if possible
  for (int i = 0; i < chunk.dialogues.size(); i++) {
    cout << dec << chunkIndex << "," << "0x" << hex << chunk.dialogueOffsets[i]
      << ",";
    
    map<int, int>::iterator findIt = dialoguePortraits.find(i);
    if (findIt != dialoguePortraits.end()) {
      // no portrait
      if (findIt->second == 0) {
//        cout << dec << findIt->second << "," << findIt->second << ",";
        cout << ",,";
      }
      else {
//        cout << dec << findIt->second << "," << findIt->second << ",";
        cout << portraitMappings[findIt->second].name << ",";
        if (portraitMappings[findIt->second].expression != NULL) {
          cout << portraitMappings[findIt->second].expression << ",";
        }
        else {
          cout << ",";
        }
      }
    }
    // portrait not detected for this line
    else {
      cout << "?,?,";
    }
    
    cout << "\"" << chunk.dialogues[i] << "\"";
    
    cout << "," << endl;
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Mahou Gakuen Lunar! script/dialogue extraction tool\n";
    cout << "Usage: " << argv[0] << " <dialoguefile>" << endl;
  }
  
/*  ifstream ifs(argv[1], ios_base::binary);
  
  int insize = fsize(ifs);
  if (insize <= 0) return 0;
  
  ifs.close();
  
  BufferedFile scriptfile = getFile(argv[1]);
  
  ScriptChunk chunk;
  chunk.read((unsigned char*)(scriptfile.buffer), scriptfile.size); */
  
  cout << "chunk,offset,character,expression,japanese,english" << endl;
  
  ifstream ifs(argv[1], ios_base::binary);
  char index[indexSize];
  ifs.read(index, indexSize);
  int chunkOffset, chunkSize;
  int indexpos = 0;
  while (true) {
    chunkOffset = ByteConversion::fromBytes(index + (indexpos * 8) + 0, 4,
      EndiannessTypes::big, SignednessTypes::sign);
    chunkSize = ByteConversion::fromBytes(index + (indexpos * 8) + 4, 4,
      EndiannessTypes::big, SignednessTypes::sign);
    
    ++indexpos;
    
    if (chunkOffset == -1) break;
    
    if (chunkOffset == 0) continue;
    
    unsigned char* rawChunk = new unsigned char[chunkSize];
    ifs.seekg(chunkOffset);
    ifs.read((char*)rawChunk, chunkSize);
    
    ScriptChunk chunk;
    chunk.read(rawChunk, chunkSize);
  
    doChunk(chunk, indexpos - 1);
    
    delete rawChunk;
  }
  
//  cout << chunk.dialogues.size() << endl;
  
  return 0;
} 
