#ifndef SEADRAGON_BACKEND_H_
#define SEADRAGON_BACKEND_H_

#include <stdbool.h>
#include <stdio.h>

#include "ast.h"

typedef struct {
	void(*begin_function)(void *backend, seadragon_function_t *func);
	void* (*register_allocate)(void *backend, char *identifier);
	void (*set_long)(void *backend, void *reg, seadragon_value_t *val);
	void (*ret)(void *backend);
} seadragon_backend_t;

#endif // SEADRAGON_BACKEND_H_

