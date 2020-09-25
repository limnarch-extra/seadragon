#include "limn2k.h"
#include "list.h"

#include <stdlib.h>
#include <string.h>

#define ERROR(msg) do { fprintf(stderr, "%s:%d: error: limn2k: %s\n", __FILE__, __LINE__, msg); longjmp(*backend->env, 1); } while(0);

typedef struct {
	seadragon_backend_t base;
	// Contains strings of autos currently assigned to registers. Index + 1 is register number.
	char *registers[26];
	FILE *out;
	jmp_buf *env;
} seadragon_limn2k;

typedef uint8_t seadragon_limn2k_register;

static void limn2k_begin_function(void *_backend, seadragon_function_t *func) {
	seadragon_limn2k *backend = _backend;
	memset(backend->registers, 0, sizeof(backend->registers));
	if (func->outputs->length > 2) {
		ERROR("TODO: outputs.length > 2");
	}
	if (func->outputs->length > 0) {
		backend->registers[9] = func->outputs->items[0];
		if (func->outputs->length > 1) {
			backend->registers[10] = func->outputs->items[1];
		}
	}
	fprintf(backend->out, "%s:\n", func->name);
}

static void limn2k_set_long(void *_backend, void *reg, seadragon_value_t *val) {
	seadragon_limn2k *backend = _backend;
	seadragon_limn2k_register *r = reg;
	switch (val->type) {
	case VALUE_TYPE_LITERAL:
		if (val->u.literal <= UINT16_MAX) {
			fprintf(backend->out, "\tli %u, %u\n", *r, val->u.literal);
		}
		else {
			ERROR("TODO: slong >u16");
		}
		break;
	default:
		ERROR("TODO: slong");
	}
}

static void *limn2k_register_allocate(void *_backend, char *ident) {
	seadragon_limn2k *backend = _backend;
	seadragon_limn2k_register *reg = malloc(sizeof(seadragon_limn2k_register));
	*reg = 0;
	unsigned int first_unused = 27;
	for (unsigned int i = 0; i < 26; i += 1) {
		if (backend->registers[i] && !strcmp(backend->registers[i], ident)) {
			*reg = i + 1;
			break;
		}
		if (!backend->registers[i] && first_unused == 27) {
			first_unused = i;
		}
	}
	if (!*reg) {
		if (first_unused == 27) {
			ERROR("TODO: register spilling or error.");
		}
		backend->registers[first_unused] = ident;
		*reg = first_unused + 1;
	}
	return reg;
}

static void seadragon_limn2k_ret(void *_backend) {
	seadragon_limn2k *backend = _backend;
	fprintf(backend->out, "\tret\n");
}

seadragon_backend_t *seadragon_backend_limn2k(jmp_buf *env, FILE *out) {
	seadragon_limn2k *backend = malloc(sizeof(seadragon_limn2k));
	backend->out = out;
	backend->env = env;
	memset(&backend->base, 0, sizeof(seadragon_backend_t));
	backend->base.begin_function = &limn2k_begin_function;
	backend->base.register_allocate = limn2k_register_allocate;
	backend->base.set_long = limn2k_set_long;
	backend->base.ret = seadragon_limn2k_ret;
	return &backend->base;
}

void seadragon_backend_limn2k_deinit(seadragon_backend_t *backend) {
	free(backend);
}
