#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "codegen.h"
#include "backends/limn2k.h"

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

	ASSERT_EQ_UINT(function->u.instructions->length, 21);
	static seadragon_instruction_type_t types[21] = {
		INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_SINT,
		INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_GINT, INSTRUCTION_TYPE_SUB,
		INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_SBYTE,
		INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_GBYTE,
		INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_SLONG,
		INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_GLONG, INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_SLONG,
		INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_DROP, INSTRUCTION_TYPE_PUSH, INSTRUCTION_TYPE_DROP, 
	};
	for (unsigned int i = 0; i < 21; i += 1) {
		seadragon_instruction_t *inst = function->u.instructions->items[i];
		ASSERT_EQ_UINT(types[i], inst->type);
	}

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
	ASSERT(tree.right);
	ASSERT(tree.left);
	ASSERT_EQ_INT(tree.left->type, LEAF_VALUE);
	ASSERT_EQ_INT(tree.right->type, LEAF_VALUE);
	ASSERT_EQ_UINT(tree.left->u.value->type, VALUE_TYPE_IDENTIFIER);
	ASSERT_EQ_STR(tree.left->u.value->u.identifier, "ret");
	ASSERT_EQ_UINT(tree.right->u.value->type, VALUE_TYPE_LITERAL);
	ASSERT_EQ_UINT(tree.right->u.value->u.literal, 0);
}

TEST(codegen) {
	static const char src[] = "fn main {-- ret} 0 ret ! end ";
	seadragon_lexer_t lexer;
	PRECONDITION(seadragon_lexer_init(&lexer, "<src>", src, sizeof(src) - 1));
	seadragon_ast_t ast;
	PRECONDITION(seadragon_parse(&ast, &lexer));
	PRECONDITION(seadragon_sema(&ast));
	char buf[1024*1024];
	FILE *outfile = fmemopen(buf, 1024 * 1024, "w+");
	seadragon_backend_t *(*backend)(jmp_buf*,FILE*) = seadragon_backend_limn2k;
	bool codegen_success = seadragon_cg(&ast, outfile, backend);
	long len = ftell(outfile);
	ASSERT(len >= 0);
	fclose(outfile);
	buf[len] = 0;
	printf("Generated code: \n========\n%s========\n", buf);
	ASSERT(codegen_success && "delayed assertion so partially generated code prints");
}

int main()
{
	TEST_EXEC(lexer);
	TEST_EXEC(parser);
	TEST_EXEC(sema);
	TEST_EXEC(codegen);
	return TEST_REPORT();
}
