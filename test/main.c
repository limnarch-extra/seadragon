#include "lexer.h"
#include "parser.h"
#include "sema.h"

#define TEST_USE_COLOR 0

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

static const char src[] = 
"fn main { -- ret }	\
	auto test	\
\
	1 test si	\
	1 test gi -	\
	test sb		\
	test gb		\
\
	ret!		\
	ret@ ret!	\
	1 drop 1 drop   \
end";


TEST(lexer)
{
	seadragon_lexer_t lexer;
	PRECONDITION(seadragon_lexer_init(&lexer, "<src>", src, sizeof(src) - 1));

	size_t nlines = count_lines(src, sizeof(src) - 1);
	int nlines_ndigits = num_digits(nlines);

	uint32_t pline = (uint32_t)-1;
	for(size_t i = 0;; i += 1)
	{
		seadragon_token_t token = seadragon_lexer_next(&lexer, SEADRAGON_LEXER_CATEGORY_PARSER);
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

TEST(parser) {
	seadragon_lexer_t lexer;
	PRECONDITION(seadragon_lexer_init(&lexer, "<src>", src, sizeof(src) - 1));
	seadragon_ast_t ast;
	ASSERT(seadragon_parse(&ast, &lexer));
	ASSERT_EQ_INT(ast.functions->length, 1);
	seadragon_function_t *function = ast.functions->items[0];
	ASSERT(function);
	char *name = function->outputs->items[0];
	ASSERT_EQ_STR(name, "ret");
	ASSERT_EQ_UINT(function->u.instructions->length, 3);
	seadragon_instruction_t *instruction = function->u.instructions->items[0];
	ASSERT(instruction);
	ASSERT_EQ_UINT(instruction->type, INSTRUCTION_TYPE_PUSH);
	ASSERT(instruction->argument);
	ASSERT_EQ_UINT(instruction->argument->type, VALUE_TYPE_LITERAL);
	ASSERT_EQ_UINT(instruction->argument->u.literal, 0);
	instruction = function->u.instructions->items[1];
	ASSERT_EQ_UINT(instruction->type, INSTRUCTION_TYPE_PUSH);
	ASSERT_EQ_UINT(instruction->argument->type, VALUE_TYPE_IDENTIFIER);
	ASSERT_EQ_STR(instruction->argument->u.identifier, "ret");
	instruction = function->u.instructions->items[2];
	ASSERT_EQ_UINT(instruction->type, INSTRUCTION_TYPE_SLONG);
}

TEST(sema) {
	static const char src[] = "fn main {-- ret} 0 ret ! end ";
	seadragon_lexer_t lexer;
	PRECONDITION(seadragon_lexer_init(&lexer, "<src>", src, sizeof(src) - 1));
	seadragon_ast_t ast;
	ASSERT(seadragon_parse(&ast, &lexer));
	ASSERT(seadragon_sema(&ast));
	// Functions' instruction lists are no longer valid, tree is now
	ASSERT_EQ_INT(ast.functions->length, 1);
	seadragon_function_t *function = ast.functions->items[0];
	ASSERT(function);
	char *name = function->outputs->items[0];
	ASSERT_EQ_STR(name, "ret");
	seadragon_instruction_node_t tree = function->u.root;
	ASSERT_EQ_UINT(tree.op, OPERATION_SLONG);
	ASSERT_EQ_PTR(tree.right, NULL);
	ASSERT_EQ_PTR(tree.left, NULL);

}

int main()
{
	TEST_EXEC(lexer);
	TEST_EXEC(parser);
	TEST_EXEC(sema);
	return TEST_REPORT();
}
