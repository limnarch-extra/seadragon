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
		char *identifier;
	} u;
} seadragon_value_t;

typedef enum {
	INSTRUCTION_TYPE_PUSH,
	INSTRUCTION_TYPE_GLONG,
	INSTRUCTION_TYPE_GINT,
	INSTRUCTION_TYPE_GBYTE,
	INSTRUCTION_TYPE_SLONG,
	INSTRUCTION_TYPE_SINT,
	INSTRUCTION_TYPE_SBYTE,
	INSTRUCTION_TYPE_DROP,
	INSTRUCTION_TYPE_SUB,
	INSTRUCTION_TYPE_RETURN,
} seadragon_instruction_type_t;

typedef struct {
	seadragon_instruction_type_t type;
	seadragon_value_t *argument;
} seadragon_instruction_t;

// 0 ret !
// OPERATION_SLONG { {value=ret} {value=0} } 

// { op = OPERATION_SLONG, left = { type = LEAF_VALUE, u.value = { identifier = ret } } }

typedef struct seadragon_instruction_leaf seadragon_instruction_leaf_t;

typedef struct {
	enum {
		/// For internal use only, should never reach codegen.
		/// in sema, when a NONE node is encountered, it is overwritten directly with
		/// an operation.
		OPERATION_NONE,
		/// Stores the RHS value to the LHS.
		OPERATION_SLONG,
		OPERATION_RETURN,
	} op;
	seadragon_instruction_leaf_t *left, *right;
} seadragon_instruction_node_t;

struct seadragon_instruction_leaf {
	enum {
		LEAF_NODE,
		LEAF_VALUE,
		LEAF_CODEGENED,
	} type;
	union {
		seadragon_instruction_node_t node;
		seadragon_value_t *value;
		// The backend-specific register in which this leaf is stored
		void *mcval;
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

