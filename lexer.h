
#pragma once

#include <string>
#include <iostream>
#include <cctype>
#include<algorithm>




enum Token {
    tok_eof = -1,
  
    // commands
    tok_def = -2,
    tok_extern = -3,
  
    // primary
    tok_identifier = -4,
    tok_number = -5,
    tok_if=-6,
    tok_then=-7,
    tok_else=-8
};

extern std::string IdentifierStr;
extern double NumVal;

int gettok();




