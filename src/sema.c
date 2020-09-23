#include "sema.h"
#include "list.h"
#include "ast.h"

#include <stdio.h>

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
		for (unsigned int i = 0; i < instructions->length; i += 1) {
			seadragon_instruction_t *instruction = instructions->items[i];
			switch (instruction->type) {
			case INSTRUCTION_TYPE_PUSH:
				list_add(value_stack, instruction->argument);
				break;
			case INSTRUCTION_TYPE_SLONG:{
				seadragon_value_t *target = list_pop(value_stack);
				seadragon_value_t *value = list_pop(value_stack);
				switch (target->type) {
				case VALUE_TYPE_IDENTIFIER:
					ERROR("TODO slong identifier");
					break;
				case VALUE_TYPE_LITERAL:
					ERROR("TODO slong literal");
				default:
					ERROR("TODO: slong non-identifier");
				}
				ERROR("TODO: slong");
			}
				break;
			default:
				ERROR("Unrecognized instruction by sema");
			}
		}
		list_free(value_stack);
		list_free(instructions);
	}
	return true;
}
