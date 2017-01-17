/* -------------------------------------------------------------------------
//	FileName		：		D:\yx_code\yx\skynet\lualib-src\skynet_proto.h
//	Creator			：	(zc) <zcnet4@gmail.com>
//	CreateTime	：	2016-4-8 14:10
//	Description	：	
//
// -----------------------------------------------------------------------*/
#ifndef SKYNET_PROTO_H_
#define SKYNET_PROTO_H_
#include <stdint.h>
// -------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

typedef struct lua_State lua_State;
int skynet_proto_pack(lua_State* L);
int skynet_proto_pack2(lua_State* L);
int skynet_proto_unpack(lua_State *L);
int skynet_proto_unpack2(lua_State *L);

void _skynet_proto_pack_header(uint8_t** buf, int* buf_size, uint16_t cmd, uint32_t session, uint32_t uid);
void _skynet_proto_unpack_header(const uint8_t** buf, int* buf_size, uint16_t* cmd, uint32_t* session, uint32_t* uid);

// return proto_buf
uint8_t* _skynet_proto_pack_content(lua_State* L, int from, int to, int* buf_size);
int _skynet_proto_unpack_content(lua_State* L, const unsigned char* buf, int buf_size);

uint8_t* _skynet_proto_content_offset(unsigned char* proto_buf, int proto_buf_size, int* proto_content_size);
#ifdef __cplusplus
}
#endif
// -------------------------------------------------------------------------
#endif /* SKYNET_PROTO_H_ */
