#ifndef SEADRAGON_CODEGEN_H_
#define SEADRAGON_CODEGEN_H_

#include "ast.h"
#include <stdbool.h>
#include <stdio.h>
#include <setjmp.h>

#include "backend.h"

/// Takes in an AST - which *must* have already passed through semantic analysis
/// - and generates machine code to the given FILE for the specified backend.
bool seadragon_cg(seadragon_ast_t *ast, FILE *out, seadragon_backend_t *(*backend)(jmp_buf *env, FILE *out));

#endif // SEADRAGON_CODEGEN_H_

