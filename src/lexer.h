#ifndef SEADRAGON_LEXER_H_
#define SEADRAGON_LEXER_H_

#include "token.h"

#include <stddef.h>

typedef struct seadragon_lexer
{
    char* fname;
    size_t srclen;
    char* src;          //< we'll just copy it into this (again, KISS)
    size_t offset;
    seadragon_source_pos_t pos;
    seadragon_token_t token;
} seadragon_lexer_t;

seadragon_lexer_t* seadragon_lexer_init(seadragon_lexer_t* lexer, const char* fname, const char* src, size_t srclen);
void seadragon_lexer_deinit(seadragon_lexer_t* lexer);

// NOTE: don't depend on the actual category values being stable
#define SEADRAGON_LEXER_CATEGORY_PARSER      0x00    //< default, and unignorable
#define SEADRAGON_LEXER_CATEGORY_IGNORABLE   0x80
seadragon_token_t seadragon_lexer_next(seadragon_lexer_t* lexer, uint32_t categories);

#endif /* SEADRAGON_LEXER_H_ */
