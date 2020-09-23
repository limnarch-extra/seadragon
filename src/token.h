#ifndef SEADRAGON_TOKEN_H_
#define SEADRAGON_TOKEN_H_

#include <stdint.h>
#include <stddef.h>

typedef struct seadragon_source_pos
{
    uint32_t line, col;
} seadragon_source_pos_t;
typedef struct seadragon_source_range
{
    seadragon_source_pos_t head, tail;
} seadragon_source_range_t;

// sorry, but we need all the debugging aid we can get!
#define SEADRAGON_ENUM_token_kind(ITEM,ITEMI,VAL,VLAST)    \
    ITEM(EOF),              \
    \
    ITEM(SUB),            \
    ITEM(ADD),            \
    ITEM(DIV),            \
    ITEM(MUL),            \
    ITEM(CGT),            \
    ITEM(CLT),            \
    \
    ITEM(LPAREN),         \
    ITEM(RPAREN),         \
    ITEM(LBRACE),         \
    ITEM(RBRACE),         \
    \
    ITEM(SLONG),        \
    ITEM(SINT),         \
    ITEM(SBYTE),        \
    ITEM(GLONG),         \
    ITEM(GINT),          \
    ITEM(GBYTE),         \
    ITEM(DDASH),          \
    \
    ITEM(INTEGER),        \
    ITEM(IDENT),          \
    \
    ITEM(FN),             \
    ITEM(END),            \
    ITEM(IF),             \
    ITEM(RETURN),         \
    ITEM(WHILE),          \
    \
    ITEM(BUFFER),         \
    ITEM(VAR),            \
    ITEM(AUTO),           \
    \
    ITEM(DROP),           \
    \
    ITEMI(WSPACE),        \
    ITEMI(COMMENT),       \
    \
    ITEM(ERROR)VAL(0xFF)  \
    VLAST(0xFF)

typedef enum seadragon_token_kind
{
#define SEADRAGON_ITEM_(NAME)    SEADRAGON_TK_##NAME
#define SEADRAGON_ITEMI_(NAME)   SEADRAGON_TKI_##NAME
#define SEADRAGON_VAL_(V)        = (V)
#define SEADRAGON_VLAST_(V)      ,SEADRAGON_TK_LAST_ = (V)
    SEADRAGON_ENUM_token_kind(SEADRAGON_ITEM_,SEADRAGON_ITEMI_,SEADRAGON_VAL_,SEADRAGON_VLAST_)
#undef SEADRAGON_ITEM_
#undef SEADRAGON_ITEMI_
#undef SEADRAGON_VAL_
#undef SEADRAGON_VLAST_
} seadragon_token_kind_t;

typedef struct seadragon_token
{
    seadragon_token_kind_t kind;
    uint32_t len;   // really don't need 64 bits!
    char* ptr;
    seadragon_source_range_t range; // it's a bit wasteful to keep an entire range, but it'll aid debugging
} seadragon_token_t;

// debugging
void seadragon_dumpstr_escaped_DBG(const char* str, size_t len);
void seadragon_token_dump_DBG(const seadragon_token_t* token);
void seadragon_token_dump_simple_DBG(const seadragon_token_t* token, int color);
const char* seadragon_token_kind_tostr_DBG(seadragon_token_kind_t kind);

char *seadragon_token_read(seadragon_token_t token);
uint64_t seadragon_token_read_number(seadragon_token_t token);

#endif /* SEADRAGON_TOKEN_H_ */
