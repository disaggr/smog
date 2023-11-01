/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef LEXER_H_
#define LEXER_H_

#include "./parser.h"

int yylex(YYSTYPE *yylval, YYLTYPE *yyloc, struct parser_state *state);

void yyerror(YYLTYPE *loc, struct parser_state *state, char const *s);

void yyferror(YYLTYPE *loc, struct parser_state *state, char const *f, ...);


#endif  // LEXER_H_
