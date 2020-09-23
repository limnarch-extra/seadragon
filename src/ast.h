#ifndef SEADRAGON_AST_H_
#define SEADRAGON_AST_H_

#include "lexer.h"
#include "list.h"

#include <stdint.h>

/// Structure layout
/// name is owned by the struct_t, and must be freed when the structure is.
// TODO
typedef struct {
	char *name;
} seadragon_struct_t;

typedef struct {
	enum {
		VALUE_TYPE_IDENTIFIER,
		VALUE_TYPE_LITERAL,
	} type;
	union {
		uint32_t literal;
		const char *identifier;
	} u;
} seadragon_value_t;

typedef struct {
	enum {
		INSTRUCTION_TYPE_PUSH,
		INSTRUCTION_TYPE_SLONG,
		INSTRUCTION_TYPE_RETURN,
	} type;
	seadragon_value_t *argument;
} seadragon_instruction_t;

// 0 ret !
// OPERATION_SLONG { {value=ret} {value=0} } 

// { op = OPERATION_SLONG, left = { type = LEAF_VALUE, u.value = { identifier = ret } } }

typedef struct seadragon_instruction_leaf seadragon_instruction_leaf_t;

typedef struct {
	enum {
		OPERATION_SLONG,
	} op;
	seadragon_instruction_leaf_t *left, *right;
} seadragon_instruction_node_t;

struct seadragon_instruction_leaf {
	enum {
		LEAF_NODE,
		LEAF_VALUE,
	} type;
	union {
		seadragon_instruction_node_t node;
		seadragon_value_t value;
	} u;
};

typedef struct {
	char *name;
	list_t *inputs;
	list_t *outputs;
	list_t *autos;
	union {
		list_t *instructions;
		seadragon_instruction_node_t root;
	} u;
} seadragon_function_t;

/// All lists are owned by the ast_t and must be freed when the tree is.
typedef struct {
	list_t *structures;
	list_t *functions;
	list_t *constants;
} seadragon_ast_t;

#endif // SEADRAGON_AST_H_
