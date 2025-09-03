#include"lexer.h"



std::string IdentifierStr;
double NumVal;

int gettok() {
    static int LastChar = ' ';

    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar()))){
            IdentifierStr += LastChar;
        }
        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        if (IdentifierStr == "if")
            return tok_if;
        if (IdentifierStr == "then")
            return tok_then;
        if (IdentifierStr == "else")
            return tok_else;
        
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        // int cnt=std::count(NumStr.begin(),NumStr.end(),'.');

        // if(cnt>1){
        //     return tok_error;
        // }
        
        
        
        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if (LastChar == '#') {
        // Comment until end of line.
        do{
            LastChar = getchar();
        }while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        
        if (LastChar!=EOF)
        {
            return gettok();
        }
        
    }

    if (LastChar == EOF) {
        return tok_eof;  // <-- Ensure EOF is correctly returned here
    }

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

