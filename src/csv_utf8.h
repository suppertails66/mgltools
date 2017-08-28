#ifndef CSV_UTF8_H
#define CSV_UTF8_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "utf8.h"
#include "util/TStringConversion.h"
#include "util/ByteConversion.h"
#include "util/TTwoDArray.h"

typedef std::vector<short int> BigChars;

int filesize(std::istream& ifs);

int findDelimiter(BigChars& chars, int pos, std::string delimiters);

void bigCharsToString(BigChars& chars, int pos, int endpos, std::string& dst);

void bigCharsToString(BigChars& chars, std::string& dst);

int readCsvFieldToBigChars(BigChars& chars, int pos, BigChars& dst);

int readCsvFieldToString(BigChars& chars, int pos, std::string& dst);

int readCsvRow(BigChars& chars, int pos, std::vector<BigChars>& dst);

void readCsvUtf8(std::istream& ifs,
                 std::vector< std::vector<BigChars> >& dst);

#endif
