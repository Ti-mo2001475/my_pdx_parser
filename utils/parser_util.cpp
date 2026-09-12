
#include "parser_util.h"
#include <stdio.h>
#include <iostream>
struct yy_buffer_state;
typedef yy_buffer_state* YY_BUFFER_STATE;

extern YYSTYPE yylval;
extern YY_BUFFER_STATE yy_scan_string ( const char *yy_str );
extern void yy_delete_buffer (YY_BUFFER_STATE  b );
extern FILE* yyin;
extern FILE* yyout;
extern int yyparse();
extern int yylex();
extern ParadoxTag* ROOT;
extern std::vector<ParadoxBase*> parsedObject;
extern void yyrestart ( FILE *input_file );


ParadoxTag* parseFile(std::string str){
    ROOT = nullptr;
    log_info(current_location(),str);
    yyrestart(fopen(str.c_str(),"r"));
    yyparse();
    return ROOT;
}
//
ParadoxTag* parseString(std::string str){
    ROOT = nullptr;

    YY_BUFFER_STATE state = yy_scan_string(str.c_str());
    yyparse();
    yy_delete_buffer(state);
    return ROOT;
}
void clearParserDatas(){
    for(ParadoxBase* ptr : parsedObject){
        delete ptr;
    }
    parsedObject.clear();
    parsedObject.shrink_to_fit();
}
size_t getParserDataSize(){
    return parsedObject.size();
}