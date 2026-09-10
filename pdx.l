%{
#include "paradox_type.h"
#include "y.tab.h"
#include <iostream>


%}

%option yylineno

%option case-sensitive

%%

#.*\n {}
#.* {}
yes {
	//
	yylval.num = 1;
	return T_BOOLEAN;
}
no {
	yylval.num = 0;
	return T_BOOLEAN;
}

([\+\-])?[0-9]+ {
	yylval.num = atoi(yytext) * 1000;
	return T_NUM_CONSTANT;
}
([\+\-])?[0-9]+\.[0-9]+ {
	yylval.num = strtof(yytext, NULL) * 1000;
    return T_NUM_CONSTANT;
}
([\r\n\t\040])+=([\r\n\t\040])* {
	return '=';
}
[\t\040]+   { return ' ';  }
[\r\n]+     { return ' '; }
[\{\}=]([\r\n\t\040])* {
	return yytext[0];
}
\"(\\.|[^\"])*\" {
	int f = strlen(yytext);
	yylval.str = new std::string(yytext);
	yylval.str->erase(0,1);
	yylval.str->erase(f-2,1);
	return T_LITERAL;
}

[A-Za-z]([A-za-z0-9_]+)?\.[0-9]{1,4} {
	strncpy(yylval.name,yytext,96);	
	return T_EVENT_IDENTIFIER;
}
[A-Za-z_][0-9a-zA-Z@\%^\*_\+\\\./\-?`~':]+ {
	strncpy(yylval.name,yytext,96);	
	return T_IDENT;
}
\d{1,4}.\d{1,2}.\d{1,2} {
	strncpy(yylval.name,yytext,96);	
	return T_DATE;
}
%%


int yywrap()
{
	return 1;
}
