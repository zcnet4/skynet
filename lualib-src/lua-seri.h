#ifndef LUA_SERIALIZE_H
#define LUA_SERIALIZE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

int luaseri_pack(lua_State *L);
int luaseri_unpack(lua_State *L);

char* _luaseri_pack_impl(lua_State* L, int from, int to, int* buf_size);
void _luaseri_unpack_impl(lua_State* L, const char* buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif
