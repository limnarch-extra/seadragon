#include "lexer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>

#define SEADRAGON_LEXER_EOF_ -1

seadragon_lexer_t* seadragon_lexer_init(seadragon_lexer_t* lexer, const char* fname, const char* src, size_t srclen)
{
	if(!lexer) return NULL;
	size_t fnlen = strlen(fname);
	char* buf = malloc(fnlen + 1 + srclen + 1); // so that we allocate both in 1 alloc
	lexer->fname = buf;
	memcpy(lexer->fname, fname, fnlen + 1);

	lexer->src = &buf[fnlen + 1];
	lexer->offset = (size_t)-1; // this signifies first iteration
	lexer->srclen = 0;
	// yeah, preprocessing; this is *HORRIBLE*, but I can do it properly later
	for(size_t i = 0; i < srclen; i++)
	{
		if(src[i] == '\r')
		{
			lexer->src[lexer->srclen++] = '\n';
			if(i + 1 < srclen && src[i+1] == '\n')
				++i;	// jump over another i, to get over the LF of CRLF
		}
		else
			lexer->src[lexer->srclen++] = src[i];
	}
	lexer->src[lexer->srclen] = 0; // not technically necessary, but just for defense in depth

	lexer->pos = (seadragon_source_pos_t){.line=0, .col=0};

	lexer->token = (seadragon_token_t){.kind=SEADRAGON_TK_ERROR};
	return lexer;
}

void seadragon_lexer_deinit(seadragon_lexer_t* lexer)
{
	if(!lexer) return;
	free(lexer->fname);
	//lexer->src is in the same allocation, so we mustn't free it
}

static void seadragon_lexer_advancec_(seadragon_lexer_t* lexer, size_t c)
{
	if(lexer->offset + c >= lexer->srclen)
		c = lexer->srclen - lexer->offset;
	for(size_t i = 0; i < c; i++)
	{
		if(lexer->src[lexer->offset + i] == '\n')
		{
			++lexer->pos.line;
			lexer->pos.col = 0;
		}
		else
			++lexer->pos.col;   // ignore UTF-8 for now
	}
	lexer->offset += c;
}

static int seadragon_lexer_peekc_(seadragon_lexer_t* lexer, size_t c)
{
	c += lexer->offset;
	return c < lexer->srclen ? (uint8_t)lexer->src[c] : SEADRAGON_LEXER_EOF_;
	// return current character unless we've reached EOF, in which case return SEADRAGON_LEXER_EOF_ 
}

static void seadragon_lexer_skip_until_(seadragon_lexer_t* lexer, int until, bool inclusive)
{
	size_t i = 0;
	for(;;)
	{
		int c = seadragon_lexer_peekc_(lexer, i);
		if(c < 0 || c == until)
		{
			if(inclusive && c >= 0)
				++i;
			break;
		}
		++i;
	}
	seadragon_lexer_advancec_(lexer, i);
}

seadragon_token_t seadragon_lexer_mktoken_(seadragon_lexer_t* lexer, seadragon_token_kind_t kind, size_t c)
{
	lexer->token.kind = kind;
	seadragon_lexer_advancec_(lexer, c);
	lexer->token.len = &lexer->src[lexer->offset] - lexer->token.ptr;
	lexer->token.range.tail = lexer->pos;
	return lexer->token;
}

#define SEADRAGON_LEXER_ISKEYWORD_(lexer, keyword) ((lexer)->token.len == strlen(keyword) && !memcmp((lexer)->token.ptr, keyword, strlen(keyword)))

const char *types[] = {
	"int", "uint", "u32"
};

seadragon_token_t seadragon_lexer_next(seadragon_lexer_t* lexer, uint32_t categories)
{
	//if(!categories) categories = SEADRAGON_LEXER_CATEGORY_PARSER;
	if(lexer->offset == (size_t)-1)
	{
		lexer->offset = 0;
		// First iteration; not too important otherwise, but I wanted to get it out of the way, lest I forget.
		if(lexer->srclen >= 3 && !memcmp(&lexer->src[lexer->offset], "\xEF\xBB\xBF", 3))
			lexer->offset += 3;  //< skip UTF-8 BOM (TODO: warning?)
		while(lexer->srclen >= 2 && !memcmp(&lexer->src[lexer->offset], "#!", 2))
			seadragon_lexer_skip_until_(lexer, '\n', true);  //< skip shebangs
	}
	for(;;)
	{
		lexer->token.kind = SEADRAGON_TK_ERROR;
		lexer->token.ptr = &lexer->src[lexer->offset];
		lexer->token.len = 0;
		lexer->token.range.head = lexer->pos;

		int c = seadragon_lexer_peekc_(lexer, 0);
		size_t i;
		switch(c)
		{
		case SEADRAGON_LEXER_EOF_: return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_EOF, 0);
		case '+': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_ADD, 1);
		case '<': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_CLT, 1);
		case '>': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_CGT, 1);
		case '-': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_SUB, 1);
		case '*': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_MUL, 1);
		case '/': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_DIV, 1);
		case '(': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_LPAREN, 1);
		case ')': return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_RPAREN, 1);
		case '\n': case '\t': case ' ':
			for(i = 1;; i++)
			{
				c = seadragon_lexer_peekc_(lexer, i);
				if(c == SEADRAGON_LEXER_EOF_ || !strchr("\n\t ", c))
					break;
			}
			if(categories & SEADRAGON_LEXER_CATEGORY_IGNORABLE)
				return seadragon_lexer_mktoken_(lexer, SEADRAGON_TKI_WSPACE, i);
			seadragon_lexer_advancec_(lexer, i);
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for(i = 1; c >= '0' && c <= '9'; i++)
				c = seadragon_lexer_peekc_(lexer, i);
			return seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_INTEGER, i - 1);
		// ugh ... but at least it's efficient! (I didn't want to cheat with `default`)
		case '_':
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y':
		case 'z':
			for(i = 1;; i++)
			{
				c = seadragon_lexer_peekc_(lexer, i);
				if(!('0' <= c && c <= '9') && !('A' <= c && c <= 'Z') && !('a' <= c && c <= 'z') && c != '_')
					break;
			}
			seadragon_lexer_mktoken_(lexer, SEADRAGON_TK_IDENT, i);
			if(SEADRAGON_LEXER_ISKEYWORD_(lexer, "return"))
				lexer->token.kind = SEADRAGON_TK_RETURN;
			else if (SEADRAGON_LEXER_ISKEYWORD_(lexer, "if"))
				lexer->token.kind = SEADRAGON_TK_IF;
			else if (SEADRAGON_LEXER_ISKEYWORD_(lexer, "fn"))
				lexer->token.kind = SEADRAGON_TK_FN;
			else if (SEADRAGON_LEXER_ISKEYWORD_(lexer, "end"))
				lexer->token.kind = SEADRAGON_TK_END;
			else if (SEADRAGON_LEXER_ISKEYWORD_(lexer, "while"))
				lexer->token.kind = SEADRAGON_TK_WHILE;
			else if (SEADRAGON_LEXER_ISKEYWORD_(lexer, "buffer"))
				lexer->token.kind = SEADRAGON_TK_BUFFER;
			else if (SEADRAGON_LEXER_ISKEYWORD_(lexer, "var"))
				lexer->token.kind = SEADRAGON_TK_VAR;
			else if (SEADRAGON_LEXER_ISKEYWORD_(lexer, "auto"))
				lexer->token.kind = SEADRAGON_TK_AUTO;
			return lexer->token;
		default:
			fprintf(stderr, "\nlexer error near '%c' (\\x%.2X)\n", c, c);
			lexer->token.kind = SEADRAGON_TK_ERROR;
			return lexer->token;
		}
	}
	__builtin_trap();
}

