#include "token.h"

#include <stdlib.h>
#include <string.h>
// for debug
#include <stdio.h>
#include <inttypes.h>

void seadragon_dumpstr_escaped_DBG(const char* str, size_t len)
{
    putchar('`');
    while(len--)
    {
        uint8_t c = *str++;
        switch(c)
        {
        case '\r': fputs("\\r", stdout); break;
        case '\n': fputs("\\n", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        case '\v': fputs("\\v", stdout); break;
        case '\f': fputs("\\f", stdout); break;
        case '\\': fputs("\\\\", stdout); break;
        case '`': fputs("\\`", stdout); break;
        default:
            if(c < ' ' || 0x80 <= c)
                printf("\\x%.2X", c);
            else
                putchar(c);
            break;
        }
    }
    putchar('`');
}
void seadragon_token_dump_DBG(const seadragon_token_t* token)
{
    printf("%3" PRIu32 ":%s(%" PRIu32 ":%" PRIu32 "): ", token->kind, seadragon_token_kind_tostr_DBG(token->kind), token->range.head.line, token->range.head.col);
    seadragon_dumpstr_escaped_DBG(token->ptr, token->len);
}
void seadragon_token_dump_simple_DBG(const seadragon_token_t* token, int color)
{
    if(color) fputs("\033[97m", stdout);
    fputs(seadragon_token_kind_tostr_DBG(token->kind), stdout);
    if(color) fputs("\033[0m", stdout);
    putchar(':');
    if(color) fputs("\033[94m", stdout);
    seadragon_dumpstr_escaped_DBG(token->ptr, token->len);
    if(color) fputs("\033[0m", stdout);
}
const char* seadragon_token_kind_tostr_DBG(seadragon_token_kind_t kind)
{
    static const char* KindNames[SEADRAGON_TK_LAST_+1] = {
#define SEADRAGON_ITEM_(NAME)    [SEADRAGON_TK_##NAME] = #NAME
#define SEADRAGON_ITEMI_(NAME)   [SEADRAGON_TKI_##NAME] = "!" #NAME
#define SEADRAGON_VAL_(V)
#define SEADRAGON_VLAST_(V)
        SEADRAGON_ENUM_token_kind(SEADRAGON_ITEM_,SEADRAGON_ITEMI_,SEADRAGON_VAL_,SEADRAGON_VLAST_)
#undef SEADRAGON_ITEM_
#undef SEADRAGON_ITEMI_
#undef SEADRAGON_VAL_
#undef SEADRAGON_VLAST_
    };
    if(kind < 0 || sizeof(KindNames) / sizeof(*KindNames) <= kind)
        return "<unknown>";
    return KindNames[kind];
}

char *seadragon_token_read(seadragon_token_t token)
{
    char *buf = malloc(token.len + 1);
    strncpy(buf, token.ptr, token.len);
    buf[token.len] = 0;
    return buf;
}

uint64_t seadragon_token_read_number(seadragon_token_t token) {
	char *buf = seadragon_token_read(token);
	char *ret;
	unsigned long result = strtoul(buf, &ret, 10);
	if (ret != buf + token.len) {
		return UINT64_MAX;
	}
	free(buf);
	return result;
}
