#include "sema.h"
#include "list.h"
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

#define ERROR(msg) do { fprintf(stderr, "%s:%d: error: Sema: %s\n", __FILE__, __LINE__, msg); list_free(value_stack); list_free(instructions); return false; } while(0);

bool seadragon_sema(seadragon_ast_t *ast) {
	if (!ast) {
		return false;
	}
	for (unsigned int i = 0; i < ast->functions->length; i += 1) {
		seadragon_function_t *func = ast->functions->items[i];
		list_t *instructions = func->u.instructions;
		// list of seadragon_value_t
		list_t *value_stack = list_create();
		list_t *active_nodes = list_create();
		list_add(active_nodes, &func->u.root);
		func->u.root.op = OPERATION_NONE;
		func->u.root.left = func->u.root.right = NULL;
		for (unsigned int i = 0; i < instructions->length; i += 1) {
			seadragon_instruction_t *instruction = instructions->items[i];
			seadragon_instruction_node_t *current = list_last(active_nodes);
			if (!current) {
				ERROR("Internal error: no active node");
			}
			if (current->op != OPERATION_NONE) {
				ERROR("TODO: append leaves to nodes");
			}
			seadragon_instruction_node_t *target = current;
			switch (instruction->type) {
			case INSTRUCTION_TYPE_PUSH:
				list_add(value_stack, instruction->argument);
				break;
			case INSTRUCTION_TYPE_SLONG:{
				seadragon_value_t *lhs = list_pop(value_stack);
				seadragon_value_t *rhs = list_pop(value_stack);
				if (rhs->type != VALUE_TYPE_LITERAL) {
					ERROR("TODO: slong nonliteral value");
				}
				switch (lhs->type) {
				case VALUE_TYPE_IDENTIFIER:{
					target->op = OPERATION_SLONG;
					target->left = malloc(sizeof(seadragon_instruction_leaf_t));
					target->right = malloc(sizeof(seadragon_instruction_leaf_t));
					target->left->type = LEAF_VALUE;
					target->left->u.value = lhs;
					target->right->type = LEAF_VALUE;
					target->right->u.value = rhs;
				}
					break;
				case VALUE_TYPE_LITERAL:
					ERROR("TODO slong literal");
				default:
					ERROR("TODO: slong non-identifier");
				}
				break;}
			default:
				ERROR("Unrecognized instruction by sema");
			}
		}
		list_free(value_stack);
		list_free(instructions);
	}
	return true;
}
