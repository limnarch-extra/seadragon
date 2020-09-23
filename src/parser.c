#include "parser.h"
#include "list.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>

#define ERROR(msg) do { fprintf(stderr, "%s:%d: error: Parser: %s\n", __FILE__, __LINE__, msg); longjmp(env, 1); } while(0);

seadragon_ast_t *seadragon_parse(seadragon_ast_t *ast, seadragon_lexer_t *lexer) {
	if (!ast) {
		return NULL;
	}

	ast->constants = list_create();
	ast->functions = list_create();
	ast->structures = list_create();

	list_t *tokens = list_create();
	jmp_buf env;

	// The call to setjmp returns zero. Later, a call to longjmp(N) will jump back
	// to here with a return value of N.
	if (setjmp(env) == 0) {
		while(true) {
			seadragon_token_t *token = malloc(sizeof(seadragon_token_t));
			*token = seadragon_lexer_next(lexer, SEADRAGON_LEXER_CATEGORY_PARSER);
			if (token->kind == SEADRAGON_TK_ERROR) {
				free(token);
				ERROR("Lexer error encountered");
			}
			if (token->kind == SEADRAGON_TK_EOF) {
				break;
			}
			list_add(tokens, token);
		}

		for (unsigned int i = 0; i < tokens->length; i += 1) {
			seadragon_token_t *token = tokens->items[i];
			seadragon_token_dump_simple_DBG(token, 0);
			printf(" ");
		}
		printf("\n");
		fflush(stdout);

		// FN IDENT LBRACE [IDENT_1...IDENT_N] DDASH [IDENT_1...IDENT_N] RBRACE [INSTRUCTION_1...INSTRUCTION_N] END
		{
			unsigned int i = 0;
			while (i < tokens->length) {
				seadragon_token_t *token = tokens->items[i];
				i += 1;
				if (token->kind == SEADRAGON_TK_FN) {
					token = tokens->items[i];
					i += 1;
					if (token->kind != SEADRAGON_TK_IDENT) {
						ERROR("Expected identifier after `fn`");
					}
					seadragon_function_t *function = malloc(sizeof(seadragon_function_t));
					function->autos = list_create();
					function->inputs = list_create();
					function->outputs = list_create();
					function->u.instructions = list_create();
					function->name = seadragon_token_read(*token);
					token = tokens->items[i];
					i += 1;
					if (token->kind != SEADRAGON_TK_LBRACE) {
						ERROR("Expected '{' in function declaration");
					}
					token = tokens->items[i];
					i += 1;
					if (token->kind != SEADRAGON_TK_DDASH) {
						ERROR("TODO: function inputs");
					}
					token = tokens->items[i];
					i += 1;
					while (token->kind != SEADRAGON_TK_RBRACE) {
						if (token->kind != SEADRAGON_TK_IDENT) {
							ERROR("Expected identifier for output name");
						}
						list_add(function->outputs, seadragon_token_read(*token));
						token = tokens->items[i];
						i += 1;
					}
					while (token->kind != SEADRAGON_TK_END) {
						token = tokens->items[i];
						i += 1;
						if (token->kind == SEADRAGON_TK_AUTO) {
							token = tokens->items[i];
							i += 1;
							if (token->kind != SEADRAGON_TK_IDENT) {
								ERROR("Expected identifier after 'auto'");
							}
							list_add(function->autos, seadragon_token_read(*token));
							token = tokens->items[i];
							i += 1;
							continue;
						}
						seadragon_instruction_t *instruction = malloc(sizeof(seadragon_instruction_t));
						instruction->argument = NULL;
						switch (token->kind) {
						case SEADRAGON_TK_INTEGER:
							instruction->argument = malloc(sizeof(seadragon_value_t));
							instruction->type = INSTRUCTION_TYPE_PUSH;
							instruction->argument->type = VALUE_TYPE_LITERAL;
							uint64_t val = seadragon_token_read_number(*token);
							if (val > UINT32_MAX) {
								ERROR("Integer literal does not fit into 32 bits");
							}
							instruction->argument->u.literal = (uint32_t)val;
							break;
						case SEADRAGON_TK_IDENT:
							instruction->argument = malloc(sizeof(seadragon_value_t));
							instruction->type = INSTRUCTION_TYPE_PUSH;
							instruction->argument->type = VALUE_TYPE_IDENTIFIER;
							instruction->argument->u.identifier = seadragon_token_read(*token);
							break;
						case SEADRAGON_TK_SLONG:
							instruction->type = INSTRUCTION_TYPE_SLONG;
							break;
						default:
							ERROR("TODO: function instructions");
						}
						list_add(function->u.instructions, instruction);
						token = tokens->items[i];
						i += 1;
					}
					list_add(ast->functions, function);
				}
				else {
					ERROR("Unknown pattern");
				}
			}
		}
	}
	else {
		for (unsigned int i = 0; i < tokens->length; i += 1) {
			free(tokens->items[i]);
		}
		list_free(tokens);
		list_free(ast->structures);
		list_free(ast->functions);
		list_free(ast->constants);
		return NULL;
	}

	for (unsigned int i = 0; i < tokens->length; i += 1) {
		seadragon_token_t *token = tokens->items[i];
		free(token);
	}
	list_free(tokens);

	return ast;
}

