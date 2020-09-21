#ifndef SEADRAGON_PARSER_H_
#define SEADRAGON_PARSER_H_

#include "lexer.h"
#include "list.h"

#include <stdint.h>

/// Structure layout
/// name is owned by the struct_t, and must be freed when the structure is.
// TODO
typedef struct {
	char *name;
} seadragon_struct_t;

typedef enum {
	INSTRUCTION_TYPE_PUSH,
	INSTRUCTION_TYPE_STORE,
	INSTRUCTION_TYPE_RETURN,
} seadragon_instruction_type_t;

typedef enum {
	VALUE_TYPE_LITERAL,
	VALUE_TYPE_IDENTIFIER,
} seadragon_value_type_t;

typedef struct {
	seadragon_value_type_t type;
	union {
		uint32_t literal;
		const char *identifier;
	} u;
} seadragon_value_t;

typedef struct {
	seadragon_instruction_type_t type;
	seadragon_value_t argument;
} seadragon_instruction_t;

typedef struct {
	char *name;
	list_t *inputs;
	list_t *outputs;
	list_t *autos;
	union {
		list_t *instructions;
	} u;
} seadragon_function_t;

/// All lists are owned by the ast_t and must be freed when the tree is.
typedef struct {
	list_t *structures;
	list_t *functions;
	list_t *constants;
} seadragon_ast_t;

/// Parser context
/// ast is owned by the parser context and must be deinitialized when the context is.
typedef struct {
	seadragon_ast_t ast;
} seadragon_parser_ctx_t;

seadragon_parser_ctx_t *seadragon_parse(seadragon_parser_ctx_t *ctx, seadragon_lexer_t *lexer);

#endif // SEADRAGON_PARSER_H_

/*

fn Main { -- ret }
	0 0 0 + * ret!
end

zero on the value stack
ret on value stack
*(top_of_stack) = *(stack+1)
value_stack += 2
implicit return

*/


