/* -------------------------------------------------------------------------
//	FileName		：	D:\yx_code\yx\skynet\lualib-src\skynet_proto.c
//	Creator			：	(zc) <zcnet4@gmail.com>
//	CreateTime	：	2016-4-8 14:10
//	Description	：	
//
// -----------------------------------------------------------------------*/
#include "skynet_proto.h"
#include "lua-seri.h"
#include "lauxlib.h"
#include <stdlib.h>
#include <memory.h>
#include "skynet_malloc.h"
#ifdef _MSC_VER
#define random rand
#include <WinSock2.h>
#else
#include <netinet/in.h>
#endif
// -------------------------------------------------------------------------
#define KEY_SIZE 256

void rc4_init(unsigned char*s, unsigned char*key, unsigned long Len) {
  int i = 0, j = 0;
  char k[KEY_SIZE] = { 0 };
  unsigned char tmp = 0;
  for (i = 0; i < KEY_SIZE; i++) {
    s[i] = i;
    k[i] = key[i%Len];
  }
  for (i = 0; i < KEY_SIZE; i++) {
    j = (j + s[i] + k[i]) % KEY_SIZE;
    tmp = s[i];
    s[i] = s[j];//交换s[i]和s[j]
    s[j] = tmp;
  }
}

void rc4_crypt(unsigned char*s, unsigned char*Data, unsigned long Len) {
  int i = 0, j = 0, t = 0;
  unsigned long k = 0;
  unsigned char tmp;
  for (k = 0; k < Len; k++) {
    i = (i + 1) % KEY_SIZE;
    j = (j + s[i]) % KEY_SIZE;
    tmp = s[i];
    s[i] = s[j];//交换s[x]和s[y]
    s[j] = tmp;
    t = (s[i] + s[j]) % KEY_SIZE;
    Data[k] ^= s[t];
  }
}

static unsigned char crypt_box[KEY_SIZE] = { 0 };
static unsigned char key[] = "loveMMbyZC 2016-04-15 11:41";
void proto_crypt(uint8_t* buf, uint32_t buf_size) {
  if (!crypt_box[0]) {
    rc4_init(crypt_box, key, sizeof(key));
  }
  unsigned char sbox[KEY_SIZE] = { 0 };
  memcpy(sbox, crypt_box, KEY_SIZE);
  rc4_crypt(sbox, buf, buf_size);
}

void _skynet_proto_pack_header(uint8_t** buf, int* buf_size, uint16_t cmd, uint32_t session, uint32_t uid) {
  *((uint16_t*)*buf) = htons(cmd);
  *buf += sizeof(uint16_t);
  *buf_size -= sizeof(uint16_t);
  //
  *((uint32_t*)*buf) = htonl(session);
  *buf += sizeof(uint32_t);
  *buf_size -= sizeof(uint32_t);
  //
  *((uint32_t*)*buf) = htonl(uid);
  *buf += sizeof(uint32_t);
  *buf_size -= sizeof(uint32_t);
}

void _skynet_proto_unpack_header(const uint8_t** buf, int* buf_size, uint16_t* cmd, uint32_t* session, uint32_t* uid) {
  //[len][cmd][session][uid][content]:包长度 + 命令 + 会话 + uid + 内容。
  //sizeof(uint16_t), sizeof(uint16_t), sizeof(uint32_t), sizeof(uint32_t)
  *cmd = ntohs(*((uint16_t*)*buf));
  (*buf) += sizeof(uint16_t);
  *buf_size -= sizeof(uint16_t);
  //
  *session = ntohl(*((uint32_t*)*buf));
  (*buf) += sizeof(uint32_t);
  *buf_size -= sizeof(uint32_t);
  //
  *uid = ntohl(*((uint32_t*)*buf));
  (*buf) += sizeof(uint32_t);
  *buf_size -= sizeof(uint32_t);
}

static int _luaseri_pack_impl_protected(lua_State *L) {
  int top = lua_gettop(L);             //此时top是传入参数个数。
  int to_form = lua_tointeger(L, top); //指明pack的个数
  lua_pop(L, 1);                       //因为是新增的，故要删除。
  //
  int proto_buf_size = 0;
  char* proto_buf = _luaseri_pack_impl(L, 0, to_form, &proto_buf_size);

  lua_pushlightuserdata(L, proto_buf);  // index:-2
  lua_pushinteger(L, proto_buf_size);   // index:-1

  return lua_gettop(L);                 //返回所有栈。
}

uint8_t* _skynet_proto_content_offset(unsigned char* proto_buf, int proto_buf_size, int* proto_content_size) {
  *proto_content_size = proto_buf_size - (sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t));
  return (unsigned char*)proto_buf + (sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t));;
}

uint8_t* _skynet_proto_pack_content(lua_State* L, int from, int to, int* buf_size) {
  int proto_buf_size;
  uint8_t* proto_buf;
  int top = lua_gettop(L);
  //http://blog.codingnow.com/2015/05/lua_c_api.html
  // 为保护_luaseri_pack_impl、减少栈间值复制与保持原来栈不变。
  //方案:将要pack的参数作为_luaseri_pack_impl_protected参数传入。
  lua_pushcfunction(L, _luaseri_pack_impl_protected);
  lua_insert(L, from+1);                // 将cfunction插入到pack参数前面。
  lua_pushinteger(L, to - from);        // 新增个参数指明pack的个数。
  int args = lua_gettop(L) - (from + 1);
  if (0 == lua_pcall(L, args, LUA_MULTRET, 0)) {
    proto_buf_size = lua_tointeger(L, -1);
    proto_buf = lua_touserdata(L, -2);
    // 只加密内容。
    int en_buf_size = 0;
    uint8_t* en_buf = _skynet_proto_content_offset(proto_buf, proto_buf_size, &en_buf_size);
    proto_crypt(en_buf, en_buf_size);
  } else {
    proto_buf_size = 2 + 2 + 4 + 4;
    proto_buf = skynet_malloc(proto_buf_size);
  }
  //
  lua_settop(L, top);
  //
  *buf_size = proto_buf_size;
  return proto_buf;
}

static int _luaseri_unpack_impl_protected(lua_State *L) {
  const char* buf = lua_touserdata(L, 1);
  int buf_size = lua_tointeger(L, 2);
  //
  int top = lua_gettop(L);
  _luaseri_unpack_impl(L, buf, buf_size);
  //
  int n = lua_gettop(L) - top;
  return n;
}

int _skynet_proto_unpack_content(lua_State* L, const unsigned char* buf, int buf_size) {
  // 只解密内容。
  unsigned char* de_buf = (unsigned char*)buf;
  int de_buf_size = buf_size;
  proto_crypt(de_buf, de_buf_size);
  //
  int top = lua_gettop(L);
  //http://blog.codingnow.com/2015/05/lua_c_api.html
  lua_pushcfunction(L, _luaseri_unpack_impl_protected);
  lua_pushlightuserdata(L, (void*)buf);
  lua_pushinteger(L, buf_size);
  if (0 == lua_pcall(L, 2, LUA_MULTRET, 0)) {
    int n = lua_gettop(L) - top;
    return n;
  }
  return -1;
}

int skynet_proto_pack(lua_State* L) {
  uint16_t cmd = 0;
  uint32_t session = 0, uid = 0;
  if (!lua_isnil(L, 1)) {
    cmd = luaL_checkinteger(L, 1);
  }
  if (!lua_isnil(L, 2))
    session = luaL_checkinteger(L, 2);
  if (!lua_isnil(L, 3))
    uid = luaL_checkinteger(L, 3);
  //
  int proto_buf_size = 0;
  uint8_t* proto_buf = _skynet_proto_pack_content(L, 3, lua_gettop(L), &proto_buf_size);
	//
  int buf_size = proto_buf_size - sizeof(uint16_t);
  uint8_t* buf = proto_buf + sizeof(uint16_t);
  //[len]
  *((uint16_t*)proto_buf) = htons(buf_size);
  //[cmd][session][uid][content]
  _skynet_proto_pack_header(&buf, &buf_size, cmd, session, uid);
	//
  lua_pushlightuserdata(L, proto_buf);
	lua_pushinteger(L, proto_buf_size);
	return 2;
}

int skynet_proto_pack2(lua_State* L) {
  uint16_t cmd = 0;
  uint32_t session = 0, uid = 0;
  if (!lua_isnil(L, 1)) {
    cmd = luaL_checkinteger(L, 1);
  }
  if (!lua_isnoneornil(L, 2))
    session = luaL_checkinteger(L, 2);
  if (!lua_isnoneornil(L, 3))
    uid = luaL_checkinteger(L, 3);
  //
  size_t sz = 0;
  const char* str = NULL;
  if (!lua_isnoneornil(L, 4))
    str = luaL_checklstring(L, 4, &sz);
  //
  int proto_buf_size = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sz;
  uint8_t* proto_buf = skynet_malloc(proto_buf_size);
  //
  int buf_size = proto_buf_size - sizeof(uint16_t);
  uint8_t* buf = proto_buf + sizeof(uint16_t);
  //[len]
  *((uint16_t*)proto_buf) = htons(buf_size);
  //[cmd][session][uid][content]
  _skynet_proto_pack_header(&buf, &buf_size, cmd, session, uid);
  if (sz > 0)
    memcpy(buf, str, sz);
  //
  lua_pushlightuserdata(L, proto_buf);
  lua_pushinteger(L, proto_buf_size);
  return 2;
}


int skynet_proto_unpack(lua_State *L) {
  if (lua_isnoneornil(L, 1)) {
    return 0;
  }
  const uint8_t* buf;
  int buf_size;
  if (lua_type(L, 1) == LUA_TSTRING) {
    size_t sz;
    buf = (const uint8_t*)lua_tolstring(L, 1, &sz);
    buf_size = (int)sz;
  } else {
    buf = (const uint8_t*)lua_touserdata(L, 1);
    buf_size = luaL_checkinteger(L, 2);
  }
  if (buf == NULL) {
    return luaL_error(L, "deserialize null pointer");
  }
  if (buf_size < sizeof(uint32_t)) {
    return 0;
  }
  //
  lua_settop(L, 1);
  //
  uint16_t cmd = 0;
  uint32_t session = 0, uid = 0;
  _skynet_proto_unpack_header(&buf, &buf_size, &cmd, &session, &uid);
  lua_pushinteger(L, cmd);
  lua_pushinteger(L, session);
  lua_pushinteger(L, uid);
  if (buf_size > 0) {
    int args = _skynet_proto_unpack_content(L, buf, buf_size);
    if (args < 0) {
      lua_settop(L, 4);
    }
  }
  //
  return lua_gettop(L) - 1;
}


int skynet_proto_unpack2(lua_State *L) {
  if (lua_isnoneornil(L, 1)) {
    return 0;
  }
  const uint8_t* buf;
  int buf_size;
  if (lua_type(L, 1) == LUA_TSTRING) {
    size_t sz;
    buf = (const uint8_t*)lua_tolstring(L, 1, &sz);
    buf_size = (int)sz;
  } else {
    buf = (const uint8_t*)lua_touserdata(L, 1);
    buf_size = luaL_checkinteger(L, 2);
  }
  if (buf == NULL) {
    return luaL_error(L, "deserialize null pointer");
  }
  if (buf_size < sizeof(uint32_t)) {
    return 0;
  }
  //
  lua_settop(L, 1);
  //
  uint16_t cmd = 0;
  uint32_t session = 0, uid = 0;
  _skynet_proto_unpack_header(&buf, &buf_size, &cmd, &session, &uid);
  lua_pushinteger(L, cmd);
  lua_pushinteger(L, session);
  lua_pushinteger(L, uid);
  if (buf_size > 0) {
    lua_pushlstring(L, (const char*)buf, buf_size);
  }
  //
  return lua_gettop(L) - 1;
}

//static unsigned char auth_key[] = "fe3f88fa709e949f6455f335cce141b75f";
//#define _RANDOM_SIZE 8

//unsigned char* _skynet_proto_gen_auth(lua_State* L, int* auth_size) {
//  int top = lua_gettop(L);
//  //
//  unsigned char random_key[_RANDOM_SIZE] = { 0 };
//  int i;
//  char x = 0;
//  for (i = 0; i < _RANDOM_SIZE; ++i) {
//    random_key[i] = random() & 0xFF;
//    x ^= random_key[i];
//  }
//  if (x == 0) {
//    random_key[0] |= 1;	// avoid 0
//  }
//  lua_pushlstring(L, (char*)random_key, _RANDOM_SIZE);
//  //
//  unsigned char auth_buf[_RANDOM_SIZE] = { 0 };
//  memcpy(auth_buf, random_key, _RANDOM_SIZE);
//  unsigned char sbox[KEY_SIZE] = { 0 };
//  rc4_init(sbox, auth_key, sizeof(auth_key));
//  rc4_crypt(sbox, auth_buf, _RANDOM_SIZE);
//  lua_pushlstring(L, (char*)auth_buf, _RANDOM_SIZE);
//  //
//  int proto_buf_size = 0;
//  unsigned char* proto_buf = _skynet_proto_pack_content(L, top, lua_gettop(L), &proto_buf_size);
//  //
//  int buf_size = proto_buf_size - sizeof(uint16_t);
//  unsigned char* buf = proto_buf + sizeof(uint16_t);
//  //_skynet_proto_pack_header(0x80000000, &buf, &buf_size);
//  //
//  lua_settop(L, top);
//  //
//  *auth_size = proto_buf_size;
//  return proto_buf;
//}
//
//
//int _skynet_proto_auth(lua_State* L) {
//#define _FAILED(err_msg) { lua_settop(L, top); lua_pushboolean(L, 0); lua_writestringerror("%s", err_msg);return 1; }
//  int top = lua_gettop(L);
//  //
//  int n = skynet_proto_unpack(L);
//  if (3 != n) _FAILED("auth:1");
//  //
//  uint32_t session = lua_tointeger(L, top + 1);
//  if ((session & 0x80000000) != 0x80000000) _FAILED("auth:2");
//  //
//  size_t random_key_size = 0;
//  const unsigned char* random_key = (const unsigned char*)lua_tolstring(L, top + 2, &random_key_size);
//  if (_RANDOM_SIZE != random_key_size) _FAILED("auth:3");
//  //
//  size_t auth_size = 0;
//  const unsigned char* auth = (const unsigned char*)lua_tolstring(L, top + 3, &auth_size);
//  if (_RANDOM_SIZE != auth_size) _FAILED("auth:4");
//  //
//  unsigned char auth_decrypt[_RANDOM_SIZE] = { 0 };
//  memcpy(auth_decrypt, auth, _RANDOM_SIZE);
//  unsigned char sbox[KEY_SIZE] = { 0 };
//  rc4_init(sbox, auth_key, sizeof(auth_key));
//  rc4_crypt(sbox, auth_decrypt, _RANDOM_SIZE);
//
//  int ret = memcmp(random_key, auth_decrypt, _RANDOM_SIZE);
//  lua_settop(L, top);
//  lua_pushboolean(L, !ret);
//  //
//  return 1;
//}
//
//int _skynet_proto_tochat(lua_State* L) {
//  if (lua_isnoneornil(L, 1)) {
//    return 0;
//  }
//  char * buf = (char *)lua_touserdata(L, 1);
//  int buf_size = luaL_checkinteger(L, 2);
//  if (buf == NULL) {
//    return luaL_error(L, "_skynet_proto_to_chat");
//  }
//  uint16_t  len = ntohs(*(uint16_t*)buf);
//  memcpy((void*)buf, &len, sizeof(uint16_t));
//  //
//  uint16_t* cmd = ((uint16_t*)buf) + 1;
//  *cmd = lua_tointeger(L, 3);
//  //
//  lua_pushlstring(L, (const char*)buf, buf_size);
//  skynet_free(buf);
//  return 1;
//}

// -------------------------------------------------------------------------
