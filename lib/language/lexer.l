%{
#include <stdio.h>
#include <string>
#include "preprocessing/ASTNode.h"
#include "parser.h"
extern int yylineno;
void yyerror(const char* s, ...);
#define SAVE_TOKEN yylval.string = new std::string(yytext);
%}

%option noyywrap nodefault yylineno

digs (-)?[0-9]+
chars [a-zA-Z_]+


%%
"#"[^\n]*                      { /* # as comments */ }
[ \t\n\r]                      ; //ignores spaces
"for"			               { return TFOR; }
"from"                         { return TFROM; }
"to"                           { return TTO; }
"View"                         { return TVIEW; }
"touint"                       { return TTOUINT; }

"uint"                          { SAVE_TOKEN; return TYUINT; }

"uint1"                          { SAVE_TOKEN; return TYUINT1; }
"uint4"                          { SAVE_TOKEN; return TYUINT4; }
"uint6"                          { SAVE_TOKEN; return TYUINT6; }
"uint8"                          { SAVE_TOKEN; return TYUINT8; }
"uint12"                         { SAVE_TOKEN; return TYUINT12; }
"uint16"                         { SAVE_TOKEN; return TYUINT16; }
"uint32"                         { SAVE_TOKEN; return TYUINT32; }
"uint64"                         { SAVE_TOKEN; return TYUINT64; }
"uint128"                        { SAVE_TOKEN; return TYUINT128; }

"@cipher"                        { SAVE_TOKEN; return TCIPHER; }
"spn"                            { return TSPN; }
"bfn"                            { return TBFN; }
"block_size"                     { return TBSIZE; }
"round_num"                      { return TRNUM; }
"sbox_size"                      { return TSSIZE; }
"nibble_size"                    { return TNIBSIZE; }

"sbox"                        {  return TSBOX; }
"pbox"                        {  return TPBOX; }
"pboxm"                       {  return TPBOXM; }
"ffm"                         {  return TFFM; }

"r_fn"                        {  return TRFN; }
"schd_fn"                     {  return TSCHDFN; }
"s_fn"                        {  return TSFN; }
"fn"                          {  return TFN; }

"return"		               { return TRETURN; }
{digs}                         { SAVE_TOKEN; return TINTEGER;}
[a-zA-Z_][a-zA-Z0-9_]*         { SAVE_TOKEN; return TIDENTIFIER;}
"+"                            { return TPLUS;}
"-"                            { return TMINUS;}
"*"                            { return TFFTIMES;}
"/"                            { return TDIVIDE;}
","                            { return TCOMMA;}
";"                            { return TSEMICOLON; }
"="                            { return TASSIGN;}
"<<"                           { return TSHIFTL; }
">>"                           { return TSHIFTR; }
"<<<"                           { return TRSHIFTL; }
">>>"                           { return TRSHIFTR; }
"~"                            { return TNOT;}
"("                            { return TLPAREN;}
")"                            { return TRPAREN;}
"["                            { return TLBRACKET; }
"]"                            { return TRBRACKET; }
"{"                            { return TLBRACE; }
"}"                            { return TRBRACE; }
"<"                            { return TLABRACKET; }
">"                            { return TRABRACKET; }
"%"                           { return TMOD;  }
"&"			                   {  return TAND; }
"|"			                   {  return TOR; }
"^"			                   {  return TXOR; }
.                              {  printf("Unknown token! %s\n", yytext);} /*complains about every unknown character*/

%%
void yyerror(const char* s, ...) {

}
