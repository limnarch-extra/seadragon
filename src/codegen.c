#include "codegen.h"

#include <setjmp.h>
#include <stdlib.h>

#define ERRORF(msg, ...) do { fprintf(stderr, "%s:%d: error: Codegen: " msg "\n", __FILE__, __LINE__, __VA_ARGS__); longjmp(ctx.env, 1); } while(0);
#define ERROR(msg) do { fprintf(stderr, "%s:%d: error: Codegen: %s\n", __FILE__, __LINE__, msg); longjmp(ctx.env, 1); } while(0);

typedef struct {
	jmp_buf env;
	seadragon_backend_t *backend;
	FILE *out;
} seadragon_cg_ctx_t;

static void *seadragon_cg_node(seadragon_cg_ctx_t ctx, seadragon_instruction_node_t node);

static void seadragon_cg_leaf_node(seadragon_cg_ctx_t ctx, seadragon_instruction_leaf_t **leaf) {
	void *reg = seadragon_cg_node(ctx, (*leaf)->u.node);
	if (reg) {
		(*leaf)->type = LEAF_CODEGENED;
		(*leaf)->u.mcval = reg;
	}
	else {
		*leaf = NULL;
	}
}

static void seadragon_cg_leaf(seadragon_cg_ctx_t ctx, seadragon_instruction_leaf_t **leaf) {
	if (*leaf) {
		switch ((*leaf)->type) {
		case LEAF_NODE:
			seadragon_cg_leaf_node(ctx, leaf);
			break;
		case LEAF_VALUE:{
			seadragon_value_t *val = (*leaf)->u.value;
			switch (val->type) {
			case VALUE_TYPE_IDENTIFIER:{
				void *reg = ctx.backend->register_allocate(ctx.backend, val->u.identifier);
				if (!reg) {
					ERROR("Register allocation failed.");
				}
				free(val);
				(*leaf)->type = LEAF_CODEGENED;
				(*leaf)->u.mcval = reg;
				break;}
			case VALUE_TYPE_LITERAL:
				break;
			}
			break;}
		default:
			ERROR("TODO: cg leaf !node");
		}
	}
}

static void *seadragon_cg_node(seadragon_cg_ctx_t ctx, seadragon_instruction_node_t node) {
	seadragon_cg_leaf(ctx, &node.left);
	seadragon_cg_leaf(ctx, &node.right);
	switch (node.op) {
		case OPERATION_NONE:
			ERROR("Internal error: OPERATION_NONE propagated to codegen");
		case OPERATION_SLONG:
			// lhs should be a LEAF_CODEGENED, rhs should be LEAF_CODEGENED or LEAF_VALUE
			if (node.left->type != LEAF_CODEGENED) {
				ERROR("Expected backend machine code value as slong target!");
			}
			switch (node.right->type) {
			case LEAF_NODE:
				ERROR("Internal error: leaf node not overwritten");
			case LEAF_VALUE:
				ctx.backend->set_long(ctx.backend, node.left->u.mcval, node.right->u.value);
				break;
			default:
				ERROR("TODO cg slong");
				break;
			}
			return NULL;
		default:
			ERROR("Unknown operation??");
	}
}

bool seadragon_cg(seadragon_ast_t *ast, FILE *out, seadragon_backend_t *(*_backend)(jmp_buf*, FILE*)) {
	seadragon_cg_ctx_t ctx;
	if (setjmp(ctx.env) == 0) {
		if (!ast || !out || !_backend) {
			return false;
		}
		if (!ast->constants || !ast->functions || !ast->structures) {
			return false;
		}
		if (ast->constants->length || ast->structures->length) {
			return false;
		}
		seadragon_backend_t *backend = _backend(&ctx.env, out);
		if (!backend->begin_function || !backend->register_allocate || !backend->set_long || !backend->ret) {
			ERROR("Backend is missing required functionality!");
		}
		ctx.backend = backend;
		ctx.out = out;
		for (unsigned int i = 0; i < ast->functions->length; i += 1) {
			seadragon_function_t *func = ast->functions->items[i];
			backend->begin_function(backend, func);
			if (seadragon_cg_node(ctx, func->u.root)) {
				ERROR("Unexpectedly received value for function's root node");
			}
			if (func->u.root.op != OPERATION_RETURN) {
				backend->ret(backend);
			}
		}
		return true;
	} else {
		return false;
	}
}
