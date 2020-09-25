#ifndef SEADRAGON_BACKEND_LIMN2K_H_
#define SEADRAGON_BACKEND_LIMN2K_H_

#include "../backend.h"
#include <setjmp.h>

seadragon_backend_t *seadragon_backend_limn2k(jmp_buf *env, FILE *out);
seadragon_backend_t *seadragon_backend_limn2k();
void seadragon_backend_limn2k_deinit(seadragon_backend_t *backend);

#endif // SEADRAGON_BACKEND_LIMN2K_H_

