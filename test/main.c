#include "lexer.h"
#include "test.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static size_t count_lines(const char* str, size_t slen)
{
	size_t num = 0;
	while(slen--)
		if(str[slen] == '\n')
			++num;
	return num;
}
// returned as `int` so that it can be passed directly into printf
static int num_digits(uint32_t n)
{
	if(!n) return 1;
	int nd = 0;
	do
	{
		++nd;
		n /= 10;
	}
	while(n);
	return nd;
}

TEST(lexer)
{
	static const char src[] = "fn main ret end";
	static seadragon_token_kind_t kinds[] = {
		SEADRAGON_TK_FN, SEADRAGON_TK_IDENT, SEADRAGON_TK_RETURN, SEADRAGON_TK_END, SEADRAGON_TK_EOF,
	};
	seadragon_lexer_t lexer;
	PRECONDITION(seadragon_lexer_init(&lexer, "<src>", src, sizeof(src) - 1));

	size_t nlines = count_lines(src, sizeof(src) - 1);
	int nlines_ndigits = num_digits(nlines);

	uint32_t pline = (uint32_t)-1;
	for(size_t i = 0;; i += 1)
	{
		seadragon_token_t token = seadragon_lexer_next(&lexer, SEADRAGON_LEXER_CATEGORY_PARSER);
		ASSERT_EQ_STR(seadragon_token_kind_tostr_DBG(token.kind), seadragon_token_kind_tostr_DBG(kinds[i]));
		if(token.kind == SEADRAGON_TK_EOF)
			break;
		if(pline != token.range.head.line)  // if we had a newline
		{
			if(pline != (uint32_t)-1)
				printf("\n");
			pline = token.range.head.line;
			printf(TEST_COLOR(90) "%*" PRIu32 ":" TEST_COLOR(0) " %*s", nlines_ndigits, token.range.head.line, (int)token.range.head.col, "");
		}
		else if(pline != (uint32_t)-1)
			putchar(' ');
		seadragon_token_dump_simple_DBG(&token, TEST_USE_COLOR);
		fflush(stdout);
		ASSERT_NE_INT(token.kind, SEADRAGON_TK_ERROR);
		if(token.kind == SEADRAGON_TK_ERROR)
			break;
	}
	if(pline != (uint32_t)-1)
		putchar('\n');

	seadragon_lexer_deinit(&lexer);
}

int main()
{
	TEST_EXEC(lexer);
	return TEST_REPORT();
}
