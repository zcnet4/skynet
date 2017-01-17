/* -------------------------------------------------------------------------
//	FileName		:	skynet\service-src\service_harbor2.c
//	Creator			:	(zc) <zcnet4@gmail.com>
//	CreateTime	:	2016-9-13 11:00
//	Description	:	
//
// -----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include "skynet.h"
#include "skynet_harbor.h"   // for REMOTE_MAX
#include "skynet_socket.h"
#include "skynet_handle.h"
#include "skynet_env.h"
#include "databuffer.h"
#include "hashid.h"
#include "hashmap.h"
// -------------------------------------------------------------------------
#define BACKLOG                         32
#define HARBOR_HEADER_SIZE              2
#define HARBOR_MESSSAGE_MAX_SIZE        65536
#define HARBOR_ADDRESS_SIZE             32

#define STATUS_DISCONNECT               0
#define STATUS_CONNECTING               1
#define STATUS_CONNECTED                2
#define STATUS_ACTION                   3

#define GET_REMOTE_FD(r)  (r->accept_fd != -1 ? r->accept_fd : r->connect_fd)

typedef struct hashnode server_t;

struct remote_harbor {
  int connect_fd;
  int accept_fd;
  uint16_t remote_harbor_id;
  char remote_harbor_host[HARBOR_ADDRESS_SIZE];
  int status;
  struct remote_harbor* next;
  struct databuffer buffer;
  struct hashmap remote_servers;                         //记录远程注册服务。
};

struct harbor {
  struct skynet_context *ctx;
  int listen_fd;
  uint16_t local_harbor_id;
  const char* local_harbor_host;
  const char* harbor_hub;                       //harbor接入点(AP)
	struct remote_harbor* remote;                          //remote_harbor列表头
  struct remote_harbor* remote_ids[REMOTE_MAX];          //harbor_id索引remote_harbor
  struct remote_harbor remote_fds[REMOTE_MAX];           //fd索引remote_harbor
	//
  struct hashmap local_servers;                          //记录本地注册的服务。
  struct hashmap global_servers;                         //记录远程注册服务。
  struct hashmap unforward_queue;                        //记录未完转发列表消息。
  //
  struct hashid fds_hash;
  struct messagepool mp;
};

//////////////////////////////////////////////////////////////////////////
static struct hashnode* _hashmap_alloc_fn() {
  struct hashnode* node = skynet_malloc(sizeof(*node));
  memset(node, 0, sizeof(*node));
  return node;
}

static void _hashmap_free_fn(struct hashnode* node) {
	skynet_free(node);
}

//////////////////////////////////////////////////////////////////////////
//
void _init_remote_harbor(struct harbor* h) {
  for (int i = 0; i < REMOTE_MAX; ++i) {
		struct remote_harbor* r = &h->remote_fds[i];
		r->accept_fd = -1;
		r->connect_fd = -1;
    r->status = STATUS_DISCONNECT;
		r->remote_harbor_id = 0;
		r->next = NULL;
    //
    h->remote_ids[i] = NULL;
	}
	//
	h->remote = NULL;
}

struct remote_harbor* _alloc_remote_harbor(struct harbor* h, int fd) {
  // connect_fd及accept_fd在单一skynet的id是不同。故可以插入在同一张表中。by ZC. 2016-9-14 10:25.
  struct remote_harbor* r = &h->remote_fds[hashid_insert(&h->fds_hash, fd)];
  r->accept_fd = -1;
  r->connect_fd = -1;
  r->status = STATUS_DISCONNECT;
  r->remote_harbor_id = 0;
  hashmap_init(&r->remote_servers, _hashmap_alloc_fn, _hashmap_free_fn);
  //
  r->next = h->remote;
  h->remote = r;
  //
	return r;
}

static void _free_remote_harbor(struct harbor* h, struct remote_harbor* r) {
  h->remote_ids[r->remote_harbor_id] = NULL;
  //
  if (r == h->remote) {
    h->remote = h->remote->next;
  } else {
    for (struct remote_harbor* rr = h->remote; rr && rr->next;) {
      if (rr->next == r) {
        rr->next = r->next;
        break;
      }
      rr = rr->next;
    } // for
  }
  //
  hashmap_clear(&r->remote_servers);
  databuffer_clear(&r->buffer, &h->mp);
  memset(r, 0, sizeof(struct remote_harbor));
	//
	r->accept_fd = -1;
	r->connect_fd = -1;
  r->status = STATUS_DISCONNECT;
	r->remote_harbor_id = 0;
}

static void _split_host_port(const char* addr, char host[HARBOR_ADDRESS_SIZE], int* port) {
  strcpy(host, addr);
  char* p = strchr(host, ':');
  if (p) {
    *p++ = '\0';
    *port = atoi(p);
  } else {
    *port = 0;
  }
}

static uint8_t* _write_uint16(uint8_t* buf, uint16_t n) {
  buf[0] = (n >> 8) & 0xff;
  buf[1] = n & 0xff;
  return buf + sizeof(uint16_t);
}

static const uint8_t* _read_uint16(const uint8_t* buf, uint16_t* n) {
  *n = buf[0] << 8 | buf[1];
  return buf + sizeof(uint16_t);
}

static uint8_t* _write_uint32(uint8_t* buf, uint32_t n) {
  buf[0] = (n >> 24) & 0xff;
  buf[1] = (n >> 16) & 0xff;
  buf[2] = (n >> 8) & 0xff;
  buf[3] = n & 0xff;
  return buf + sizeof(uint32_t);
}

static const uint8_t* _read_uint32(const uint8_t* buf, uint32_t* n) {
  *n = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
  return buf + sizeof(uint32_t);
}

static uint8_t* _write_string(uint8_t* buf, const uint8_t* str, int str_size) {
  buf = _write_uint16(buf, str_size);
  memcpy(buf, str, str_size);
  return buf + str_size;
}

static const uint8_t* _read_string(const uint8_t* buf, uint8_t* str, int* str_size) {
  uint16_t _str_size = 0;
  buf = _read_uint16(buf, &_str_size);
  memcpy(str, buf, _str_size);
  if (str_size) {
    *str_size = _str_size;
  } else {
    str[_str_size] = '\0';
  }
  return buf + _str_size;
}

/*
@func			: harbor_socket_send
@brief		: 发送harbor数据包。
*/
static void harbor_socket_send2(struct harbor* h, int fd, uint16_t cmd, 
	const uint8_t* content_buf, int content_buf_size, const uint8_t* padding_buf, int padding_buf_size) {
  //协议格式：[len][cmd][content]:包长度+命令+内容。by ZC. 2016-9-14 13:08(明天跟煊煊过中秋节)。
  int buf_size = HARBOR_HEADER_SIZE + sizeof(cmd) + content_buf_size + padding_buf_size;
  uint8_t* buf = skynet_malloc(buf_size);
  uint8_t* p = _write_uint16(buf, buf_size - HARBOR_HEADER_SIZE);
  p = _write_uint16(p, cmd);
  //
  if (content_buf && content_buf_size) {
    memcpy(p, content_buf, content_buf_size);
    p += content_buf_size;
  }
	if (padding_buf && padding_buf_size > 0) {
		memcpy(p, padding_buf, padding_buf_size);
    p += padding_buf_size;
	}
  //
  skynet_socket_send(h->ctx, fd, buf, buf_size);
}
#define harbor_socket_send(h, r, cmd, c, cs) harbor_socket_send2(h, GET_REMOTE_FD(r), cmd, (c), (cs), NULL, 0)

/*
@func			: _request_login_remote
@brief		: 请求登录远程服务。
*/
static void _request_login_remote(struct harbor* h, struct remote_harbor* r) {
  int buf_size = 0;
  uint8_t buf[HARBOR_ADDRESS_SIZE + HARBOR_ADDRESS_SIZE] = { 0 };
  uint8_t* p = _write_uint16(buf, h->local_harbor_id);
  p = _write_string(p, (uint8_t*)h->local_harbor_host, strlen(h->local_harbor_host));
  //
  harbor_socket_send(h, r, 'L', buf, p - buf);
}

/*
@func			: _serve_login_remote
@brief		: 服务登录远程。
*/
static bool _serve_login_remote(struct harbor* h, struct remote_harbor* r, uint8_t* content_buf, int content_buf_size) {
  if (sizeof(uint16_t) + sizeof(uint16_t) >= content_buf_size)
    return false;

  uint16_t remote_harbor_id = 0;
  const uint8_t* p = _read_uint16(content_buf, &remote_harbor_id);
  uint16_t remote_harbor_host_size = 0;
  p = _read_uint16(p, &remote_harbor_host_size);
  if (remote_harbor_host_size >= HARBOR_ADDRESS_SIZE)
    return false;
  char remote_harbor_host[HARBOR_ADDRESS_SIZE] = { 0 };
  memcpy((uint8_t*)remote_harbor_host, p, remote_harbor_host_size);
  // 已存在相同的harbor_id
  if (h->remote_ids[remote_harbor_id]) {
    skynet_error(h->ctx, "harbor2[%u]: error duplicate harbor2[%u] %s", h->local_harbor_id, remote_harbor_id, remote_harbor_host);
    return false;
  } else {
    for (struct remote_harbor* r = h->remote; r; r = r->next) {
      if (0 == strcmp(r->remote_harbor_host, remote_harbor_host) || r->remote_harbor_id == remote_harbor_id) {
        skynet_error(h->ctx, "harbor2[%u]: error duplicate harbor2[%u] %s", h->local_harbor_id, remote_harbor_id, remote_harbor_host);
        return false;
      }
    } // for
  }
  //
  r->remote_harbor_id = remote_harbor_id;
  strcpy(r->remote_harbor_host, remote_harbor_host);
  r->status = STATUS_ACTION;
  h->remote_ids[r->remote_harbor_id] = r;
  //
  skynet_error(h->ctx, "harbor2[%u]: accepted harbor2[%u] %s", h->local_harbor_id, r->remote_harbor_id, r->remote_harbor_host);
  return true;
}

/*
@func			: _serve_login_remote2
@brief		: 服务登录远程2，返回远程主机列表。
*/
static void _serve_login_remote2(struct harbor* h, struct remote_harbor* r) {
  uint8_t buf[2 + HARBOR_ADDRESS_SIZE* REMOTE_MAX] = { 0 };
  uint16_t accept_count = 0;
  uint8_t* p = _write_uint16(buf, accept_count);
  //
	for (struct remote_harbor* r = h->remote; r; r = r->next) {
    if (-1 != r->accept_fd) {
      p = _write_uint16(p, r->remote_harbor_id);   // size:2
      p = _write_string(p, (uint8_t*)r->remote_harbor_host, strlen(r->remote_harbor_host)); // size:2+14
      ++accept_count;
    }
  }
  _write_uint16(buf, accept_count);
  //
  harbor_socket_send(h, r, 'H', buf, p - buf);
}

void harbor_connect(struct harbor* h, uint16_t remote_harbor_id, const char* remote_harbor_host);
/*
@func			: _response_login_remote
@brief		: 响应登录，连接已存在remote形成去中心网络。
*/
static void _response_login_remote(struct harbor* h, struct remote_harbor* r, uint8_t* content_buf, int content_buf_size) {
  uint16_t remote_count = 0;
  const uint8_t* p = _read_uint16(content_buf, &remote_count);
  for (int i = 0; i < remote_count; ++i) {
    uint16_t remote_harbor_id = -1;
    p = _read_uint16(p, &remote_harbor_id);
    char remote_harbor_host[HARBOR_ADDRESS_SIZE] = { 0 };
    p = _read_string(p, (uint8_t*)remote_harbor_host, NULL);
    //
    bool need_connect = true;
		for (struct remote_harbor* r = h->remote; r; r = r->next) {
      if (remote_harbor_id == r->remote_harbor_id) {
        need_connect = false;
        break;
      }
    }
    if (need_connect && remote_harbor_id != h->local_harbor_id) {
      harbor_connect(h, remote_harbor_id, remote_harbor_host);
    }
  }
}

static void serialize_server_info_cb(server_t* s, void* ud) {
  uint8_t** p = (uint8_t**)ud;
  *p = _write_uint32(*p, s->value.u32);
  *p = _write_string(*p, (uint8_t*)s->key, strlen(s->key));
}

static uint8_t* serialize_server_info(struct hashmap* servers, int* sz) {
  uint32_t servers_count = servers->size;
  int buf_size = sizeof(uint16_t) + servers_count * (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(((server_t*)(void*)0)->key));
  uint8_t* buf = skynet_malloc(buf_size);
  //
  uint8_t* p = _write_uint16(buf, servers_count);
  hashmap_foreach(servers, serialize_server_info_cb, &p);
  //
  *sz = p - buf;
  return buf;
}

static void unserialize_server_info(struct hashmap* servers, uint8_t* buf, int buf_size, uint16_t harbor_id) {
  uint16_t servers_count = 0;
  const uint8_t* p = _read_uint16(buf, &servers_count);
  for (int i = 0; i < servers_count; ++i) {
    uint32_t destination = 0;
    p = _read_uint32(p, &destination);
    char server_name[32] = { 0 };
    p = _read_string(p, (uint8_t*)server_name, NULL);
    uint16_t remote_harbor_id = destination >> HANDLE_REMOTE_SHIFT;
    //
    if (remote_harbor_id == harbor_id) {
      server_t* s = hashmap_insert(servers, server_name);
      s->value.u32 = destination;
    }
  }
}

/*
@func			: _request_query_remote
@brief		: 查询远程服务信息，并将自身服务信息同步到远端
*/
static void _request_query_remote(struct harbor* h, struct remote_harbor* r) {
  int buf_size = 0;
  uint8_t* buf = serialize_server_info(&h->local_servers, &buf_size);
  //
  harbor_socket_send(h, r, 'Q', buf, buf_size);
  skynet_free(buf);
}

static void _sync_remote_server(struct harbor* h) {
  for (struct remote_harbor* r = h->remote; r; r = r->next) {
    if (r->status == STATUS_ACTION) {
      _request_query_remote(h, r);
    }
  }
}

static void _serve_query_remote(struct harbor* h, struct remote_harbor* r, uint8_t* content_buf, int content_buf_size) {
  hashmap_clear(&r->remote_servers);
  unserialize_server_info(&r->remote_servers, content_buf, content_buf_size, r->remote_harbor_id);
  //                                                                                                      
  int buf_size = 0;
  uint8_t* buf = serialize_server_info(&h->local_servers, &buf_size);
  //
  harbor_socket_send(h, r, 'S', buf, buf_size);
  skynet_free(buf);
}

static void _response_query_remote(struct harbor* h, struct remote_harbor* r, uint8_t* content_buf, int content_buf_size) {
  hashmap_clear(&r->remote_servers);
  unserialize_server_info(&r->remote_servers, content_buf, content_buf_size, r->remote_harbor_id);
}

struct harbor_message {
  uint16_t type;
  uint32_t source;
  uint32_t destination;
  uint32_t session;
  const void* raw_msg;
  uint16_t raw_msg_sz;
  struct harbor_message* next;
};

static void _request_forward_remote(struct harbor* h, struct remote_harbor* r, struct harbor_message* msg, const void* raw_msg, int raw_msg_sz) {
  uint8_t buf[sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t)];
  uint8_t* p = _write_uint16(buf, msg->type);
  p = _write_uint32(p, msg->source);
  p = _write_uint32(p, msg->destination);
  p = _write_uint32(p, msg->session);
  //
  harbor_socket_send2(h, GET_REMOTE_FD(r), 'F', buf, sizeof(buf), raw_msg, raw_msg_sz);
}

static void harbor_forward_local_messsage(struct harbor* h, uint32_t destination, struct harbor_message* msg);
static void _serve_forward_remote(struct harbor* h, struct remote_harbor* r, uint8_t* content_buf, int content_buf_size) {
  struct harbor_message msg;
  const uint8_t* p = _read_uint16(content_buf, &msg.type);
  p = _read_uint32(p, &msg.source);
  p = _read_uint32(p, &msg.destination);
  p = _read_uint32(p, &msg.session);
  int raw_msg_sz = content_buf_size - (p - content_buf);
  if (raw_msg_sz <= 0) {
    skynet_error(h->ctx, "harbor2[%u]: error raw_msg from harbor2[%u] %s", h->local_harbor_id, r->remote_harbor_id, r->remote_harbor_host);
    return;
  }
  //
  msg.raw_msg = skynet_malloc(raw_msg_sz);
  memcpy((void*)msg.raw_msg, p, raw_msg_sz);
  msg.raw_msg_sz = raw_msg_sz;
  //
  harbor_forward_local_messsage(h, msg.destination, &msg);
}

static void harbor_dispatch_unforward_queue(struct harbor* h, const char* server_name, uint32_t destination);
static void _merge_global_servers_cb(server_t* server, void* ud) {
  struct harbor* h = (struct harbor*)ud;
  server_t* s = hashmap_insert(&h->global_servers, server->key);
  // 设置目标句柄
  uint32_t destination = server->value.u32;
  s->value.u32 = destination;
  //
  harbor_dispatch_unforward_queue(h, server->key, destination);
}

void harbor_merge_global_servers(struct harbor* h, struct remote_harbor* r) {
  hashmap_foreach(&r->remote_servers, _merge_global_servers_cb, h);
}

static void harbor_socket_command(struct harbor* h, struct remote_harbor* r, uint16_t command, uint8_t* content_buf, int content_buf_size) {
  //请求[request] --> 服务[serve] --> 响应[response]
  switch (command) {
  case 'L': {
    if (_serve_login_remote(h, r, content_buf, content_buf_size))
      _serve_login_remote2(h, r);
    else
      skynet_socket_close(h->ctx, r->accept_fd);
  } break;
  case 'H': {
    _response_login_remote(h, r, content_buf, content_buf_size);
    // 握手后马上查询servers的信息是不合适的，因为本地servers信息都没注册进来。
    //_request_query_remote(h, r);
  } break;
  case 'Q': {
    _serve_query_remote(h, r, content_buf, content_buf_size);
    harbor_merge_global_servers(h, r);
  } break;
  case 'S': {
    _response_query_remote(h, r, content_buf, content_buf_size);
    harbor_merge_global_servers(h, r);
  } break;
  case 'F': {
    _serve_forward_remote(h, r, content_buf, content_buf_size);
  } break;
  default:
    skynet_error(h->ctx, "harbor2[%u]: invalid socket command type %u", h->local_harbor_id, command);
    break;
  }
}

static void harbor_socket_data(struct harbor* h, int fd, char* buf, int buf_size) {
  int id = hashid_lookup(&h->fds_hash, fd);
  if (id < 0) {
    skynet_error(h->ctx, "Drop unknown connection %d message", fd);
    skynet_socket_close(h->ctx, fd);
    skynet_free(buf);
    return;
  }
  //
  uint8_t stack_memory[HARBOR_MESSSAGE_MAX_SIZE] = { 0 };
  struct remote_harbor* r = &h->remote_fds[id];
  databuffer_push(&r->buffer, &h->mp, buf, buf_size);
  for (;;) {
    int msg_buf_size = databuffer_readheader(&r->buffer, &h->mp, HARBOR_HEADER_SIZE);
    if (msg_buf_size < 0) {
      return;
    } else if (msg_buf_size > 0) {
      if (msg_buf_size >= HARBOR_MESSSAGE_MAX_SIZE) {
        struct skynet_context * ctx = h->ctx;
        databuffer_clear(&r->buffer, &h->mp);
        skynet_socket_close(ctx, id);
        skynet_error(ctx, "harbor2[%u]: Recv socket message > 64k", h->local_harbor_id);
        return;
      } else {
        //协议格式：[len][cmd][content]:包长度+命令+内容。by ZC. 2016-9-14 13:08(明天跟煊煊过中秋节)。
        uint16_t command = 0;
        databuffer_read(&r->buffer, &h->mp, &command, sizeof(command));
        _read_uint16((uint8_t*)&command, &command);
        //
        int content_buf_size = msg_buf_size - sizeof(command);
        uint8_t* content_buf = NULL;
        if (content_buf_size > 0) {
          databuffer_read(&r->buffer, &h->mp, stack_memory, content_buf_size);
          content_buf = stack_memory;
        } else {
          content_buf = NULL;
        }
        harbor_socket_command(h, r, command, content_buf, content_buf_size);
        databuffer_reset(&r->buffer);
      }
    }
  }
}

static void harbor_socket_accept(struct harbor* h, int fd, int accpet_fd, const char* buf, int buf_size) {
  if (h->listen_fd != fd) {
    skynet_error(h->ctx, "harbor2[%u]:socket accept error h->listen_fd != fd", h->local_harbor_id);
    return;
  }
  if (hashid_full(&h->fds_hash)) {
    skynet_socket_close(h->ctx, accpet_fd);
  } else {
    struct remote_harbor* r = _alloc_remote_harbor(h, accpet_fd);
    r->accept_fd = accpet_fd;
    r->status = STATUS_CONNECTED;
    //
    char connect_addr[HARBOR_ADDRESS_SIZE] = { 0 };
    if (buf_size >= sizeof(connect_addr))
      buf_size = sizeof(connect_addr) - 1;
    memcpy(connect_addr, buf, buf_size);

    skynet_socket_start(h->ctx, accpet_fd);
    skynet_error(h->ctx, "harbor2[%u]: accept %d %s", h->local_harbor_id, accpet_fd, connect_addr);
  }
}


static void harbor_socket_connected(struct harbor* h, int connect_fd, const char* buf, int buf_size) {
  static const char* connect_reason[] = {
    "start", "binding", "transfer",
  };
  for (int i = 0; i < sizeof(connect_reason) / sizeof(connect_reason[0]); ++i) {
    if (!strncmp(buf, connect_reason[i], buf_size)) {
      return;
    }
  }
  //
  int id = hashid_lookup(&h->fds_hash, connect_fd);
  if (id < 0) return;
  struct remote_harbor* r = &h->remote_fds[id];
  r->connect_fd = connect_fd;
  r->accept_fd = -1;
  r->status = STATUS_CONNECTED;
  h->remote_ids[r->remote_harbor_id] = r;
  //
  char connect_addr[HARBOR_ADDRESS_SIZE] = { 0 };
  memcpy(connect_addr, buf, buf_size >= sizeof(connect_addr) ? sizeof(connect_addr) - 1 : buf_size);
  skynet_error(h->ctx, "harbor2[%u]: connect harbor2[%u] %s", h->local_harbor_id, r->remote_harbor_id, connect_addr);
  //
  skynet_socket_start(h->ctx, connect_fd);
  _request_login_remote(h, r);
  r->status = STATUS_ACTION;
}

static void harbor_socket_close(struct harbor* h, int fd, char* buf, int buf_size) {
  int id = hashid_remove(&h->fds_hash, fd);
  if (id < 0) {
    skynet_free(buf);
    return;
  }
  struct remote_harbor* r = &h->remote_fds[id];
  skynet_error(h->ctx, "harbor2[%u]: disconnect harbor2[%u] %s", h->local_harbor_id, r->remote_harbor_id, r->remote_harbor_host);
	_free_remote_harbor(h, r);
	skynet_free(buf);
}

/*
@func			: harbor_control_message
@brief		: harbor类型控制消息
*/
static bool harbor_control_message(struct harbor* h, uint32_t source, const char* msg, size_t sz) {
	uint16_t param_count = 0;
	const uint8_t* p = _read_uint16((const uint8_t*)msg, &param_count);
	if (param_count == 0) return false;
	// 这里约定每个参数最大是32个字节，因为skynet底层对Name约定是不能超过32字节的。
	char param_cmd[32] = { 0 };
	p = _read_string(p, (uint8_t*)param_cmd, NULL);
	--param_count;
	////
	if (0 == strcmp(param_cmd, "REGISTER")) {
		if (2 != param_count) return false;
		char server_name[32] = { 0 };
		p = _read_string(p, (uint8_t*)server_name, NULL);
		char server_handle[32] = { 0 };
		p = _read_string(p, (uint8_t*)server_handle, NULL);
    //
    uint32_t destination = strtoul(server_handle, NULL, 10);
    uint16_t server_harbor_id = destination >> HANDLE_REMOTE_SHIFT;
    //不允许将远程harbor_id注册为本地harbor_id。
    if (server_harbor_id != h->local_harbor_id) return false;

    server_t* s = hashmap_insert(&h->local_servers, server_name);
    s->value.u32 = destination;

    server_t* gs = hashmap_insert(&h->global_servers, server_name);
    gs->value.u32 = destination;
    //
    _sync_remote_server(h);
	} 
	return true;
}

static void harbor_forward_local_messsage(struct harbor* h, uint32_t destination, struct harbor_message* msg) {
  msg->destination = destination;
	if (0 > skynet_send(h->ctx, msg->source, msg->destination, msg->type | PTYPE_TAG_DONTCOPY, msg->session, (void *)msg->raw_msg, msg->raw_msg_sz)) {
		if (msg->type != PTYPE_ERROR) {
			// don't need report error when type is error
			skynet_send(h->ctx, msg->destination, msg->source, msg->type, msg->session, NULL, 0);
		}
		skynet_error(h->ctx, "Unknown destination :%x from :%x type(%d)", msg->destination, msg->session, msg->type);
	}
}

static bool harbor_forward_remote_messsage(struct harbor* h, uint32_t destination, struct harbor_message* msg) {
  uint16_t server_harbor_id = destination >> HANDLE_REMOTE_SHIFT;
  struct remote_harbor* r = h->remote_ids[server_harbor_id];
  if (NULL == r || r->remote_harbor_id != server_harbor_id) {
    skynet_free((void*)msg->raw_msg);
    skynet_error(h->ctx, "harbor2[%u]: forward remote to harbor2[%u] destination:%08x error", h->local_harbor_id, server_harbor_id, destination);
    return false;
  } else {
    msg->destination = destination;
  }
  //
  _request_forward_remote(h, r, msg, msg->raw_msg, msg->raw_msg_sz);
  skynet_free((void*)msg->raw_msg);
  //
  return true;
}

static void harbor_push_unforward_queue(struct harbor* h, const struct remote_name* dest, struct harbor_message* msg) {
  const char* server_name = NULL;
  char handle_name[32] = { 0 };
  if (0 == dest->handle){
    server_name = dest->name;
  } else {
    sprintf(handle_name, ":%x", dest->handle);
    server_name = handle_name;
  }
  //
  struct hashnode* queue = hashmap_lookup(&h->unforward_queue, server_name);
  if (!queue) {
    queue = hashmap_insert(&h->unforward_queue, server_name);
    queue->value.ptr = NULL;
  }
  struct harbor_message* m = skynet_malloc(sizeof(struct harbor_message));
  memcpy(m, msg, sizeof(struct harbor_message));
  m->next = queue->value.ptr;
  queue->value.ptr = m;
  //
  _sync_remote_server(h);
}

static void harbor_dispatch_unforward_queue(struct harbor* h, const char* server_name, uint32_t destination) {
  struct hashnode* queue = hashmap_lookup(&h->unforward_queue, server_name);
  if (!queue || !queue->value.ptr) {
    char handle_name[32] = { 0 };
    sprintf(handle_name, ":%x", destination);
    queue = hashmap_lookup(&h->unforward_queue, handle_name);
    if (!queue || !queue->value.ptr)
      return;
  }
  //
  struct harbor_message* m = (struct harbor_message*)queue->value.ptr;
  while (m) {
    struct harbor_message* m0 = m;
    m = m->next;
    //
    harbor_forward_remote_messsage(h, destination, m0);
    skynet_free(m0);
  }
  queue->value.ptr = NULL;
}

static bool _verify_remote_messsage(struct harbor* h, struct harbor_message* msg) {
  if (msg->raw_msg_sz >= HARBOR_MESSSAGE_MAX_SIZE - 16) { //远程消息不能超过64k。by ZC.
    skynet_error(h->ctx, "harbor2[%u]: remote message from :%08x to :%08x is too large.", h->local_harbor_id, msg->source, msg->destination);
    skynet_free((void*)msg->raw_msg);
    return false;
  }
  //
  return true;
}

static void harbor_forward_messsage(struct harbor *h, const struct remote_name* dest, struct harbor_message* msg) {
	//转发远程消息有四种情况：
  if (dest->handle == 0) {
    //2.目的是本地已注册
    server_t* s = hashmap_lookup(&h->global_servers, dest->name);
    if (s) {
      //1.已注册的本地和远程
      uint16_t server_harbor_id = s->value.u32 >> HANDLE_REMOTE_SHIFT;
      if (server_harbor_id == h->local_harbor_id) {
        harbor_forward_local_messsage(h, s->value.u32, msg);
      } else {
        if (_verify_remote_messsage(h, msg))
          harbor_forward_remote_messsage(h, s->value.u32, msg);
      }
    } else {
      //4.目的是未注册的远程
      if (_verify_remote_messsage(h, msg))
        harbor_push_unforward_queue(h, dest, msg);
    }
  } else {
    uint16_t server_harbor_id = dest->handle >> HANDLE_REMOTE_SHIFT;
    //1.目的是本地句柄
    if (server_harbor_id == h->local_harbor_id) {
      harbor_forward_local_messsage(h, dest->handle, msg);
    } else {
      //3.目的是已注册的远程
      if (_verify_remote_messsage(h, msg)) {
        struct remote_harbor* r = h->remote_ids[server_harbor_id];
        if (r && r->remote_harbor_id == server_harbor_id && r->status == STATUS_ACTION) {
            harbor_forward_remote_messsage(h, dest->handle, msg);
        } else {
          //4.目的是未注册的远程
          harbor_push_unforward_queue(h, dest, msg);
        }
      }
    }
  }
}

static int harbor_loop(struct skynet_context* context, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz) {
  struct harbor* h = ud;
  switch (type) {
  case PTYPE_SOCKET: {
    const struct skynet_socket_message* smsg = (const struct skynet_socket_message*)msg;
    int smsg_size = sz - sizeof(struct skynet_socket_message);
    int fd = smsg->id;
    //
    switch (smsg->type) {
    case SKYNET_SOCKET_TYPE_CONNECT:
      harbor_socket_connected(h, fd, (const char*)(smsg + 1), smsg_size);
      break;
    case SKYNET_SOCKET_TYPE_ACCEPT:
      harbor_socket_accept(h, fd, smsg->ud, (const char*)(smsg + 1), smsg_size);
      break;
    case SKYNET_SOCKET_TYPE_DATA:
      harbor_socket_data(h, fd, smsg->buffer, smsg->ud);
      break;
		case SKYNET_SOCKET_TYPE_ERROR:
		case SKYNET_SOCKET_TYPE_CLOSE:
			harbor_socket_close(h, fd, smsg->buffer, smsg_size);
			break;
    default:
      skynet_error(h->ctx, "harbor2[%u]: recv invalid socket message type %d", h->local_harbor_id, smsg->type);
      break;
    }
  } break;
	case PTYPE_HARBOR:
		harbor_control_message(h, source, msg, sz);
		break;
	default: {
    struct harbor_message rmsg;
    rmsg.type = type;
    rmsg.source = source;
    rmsg.destination = 0;
    rmsg.session = session;
    rmsg.raw_msg = ((const struct remote_message*)msg)->message;
    rmsg.raw_msg_sz = ((const struct remote_message*)msg)->sz;
    harbor_forward_messsage(h, &(((const struct remote_message*)msg)->destination), &rmsg);
	} break;
  }
  return 0;
}

/*
@func			: harbor2_start
@brief		: 启动逻辑。
*/
void harbor_start(struct harbor* h) {
  skynet_callback(h->ctx, h, harbor_loop);
  skynet_command(h->ctx, "REG", ".harbor");
	skynet_harbor_start(h->ctx);
  //
  int harbor_prot = 0;
  char harbor_host[HARBOR_ADDRESS_SIZE] = { 0 };
  _split_host_port(h->local_harbor_host, harbor_host, &harbor_prot);
  //
  h->listen_fd = skynet_socket_listen(h->ctx, harbor_host, harbor_prot, BACKLOG);
  skynet_socket_start(h->ctx, h->listen_fd);
  skynet_error(h->ctx, "harbor2[%u]: listen %s access_point:%s", h->local_harbor_id, h->local_harbor_host, h->harbor_hub);
}

void harbor_connect(struct harbor* h, uint16_t remote_harbor_id, const char* remote_harbor_host) {
  int harbor_prot = 0;
  char harbor_host[HARBOR_ADDRESS_SIZE] = { 0 };
  _split_host_port(remote_harbor_host, harbor_host, &harbor_prot);
  //
  int connect_fd = skynet_socket_connect(h->ctx, harbor_host, harbor_prot);
  if (-1 != connect_fd) {
    struct remote_harbor* r = _alloc_remote_harbor(h, connect_fd);
    r->connect_fd = connect_fd;
    r->remote_harbor_id = remote_harbor_id;
    r->status = STATUS_CONNECTING;
    strcpy(r->remote_harbor_host, remote_harbor_host);
  }
}
//////////////////////////////////////////////////////////////////////////

struct harbor* harbor2_create(void) {
  struct harbor* h = skynet_malloc(sizeof(struct harbor));
  memset(h, 0, sizeof(struct harbor));
  //
	h->listen_fd = -1;
  hashid_init(&h->fds_hash, REMOTE_MAX);
	_init_remote_harbor(h);
  hashmap_init(&h->local_servers, _hashmap_alloc_fn, _hashmap_free_fn);
	hashmap_init(&h->global_servers, _hashmap_alloc_fn, _hashmap_free_fn);
  hashmap_init(&h->unforward_queue, _hashmap_alloc_fn, _hashmap_free_fn);
	//
  return h;
}


static void free_unforward_queue_cb(struct hashnode* n, void* ud) {
  if (n->value.ptr) {
    struct harbor_message* m = (struct harbor_message*)n->value.ptr;
    while (m) {
      struct harbor_message* m0 = m;
      m = m->next;
      skynet_free(m0);
    }
    n->value.ptr = NULL;
  }
}

void harbor2_release(struct harbor* h) {
  hashmap_foreach(&h->unforward_queue, free_unforward_queue_cb, NULL);
  hashmap_clear(&h->unforward_queue);

  hashmap_clear(&h->global_servers);
  hashmap_clear(&h->local_servers);

  messagepool_free(&h->mp);
  hashid_clear(&h->fds_hash);

  skynet_free(h);
}

int harbor2_init(struct harbor* h, struct skynet_context* ctx, char* parm) {
  h->local_harbor_id = atoi(skynet_getenv("harbor"));
  h->local_harbor_host = skynet_getenv("harbor_host");
  h->harbor_hub = skynet_getenv("harbor_hub");
  h->ctx = ctx;
  //
  if (NULL == h->local_harbor_host) {
    skynet_error(h->ctx, "harbor2[%u]:not found harbor_host environment variable", h->local_harbor_id);
    return -1;
  } else if (strlen(h->local_harbor_host) >= HARBOR_ADDRESS_SIZE) {
    skynet_error(h->ctx, "harbor2[%u]:harbor_host environment variable size > %d", h->local_harbor_id, HARBOR_ADDRESS_SIZE);
    return -1;
  }
  // 规定harbor_hub_id为255。by ZC. 2017-1-10 13:48.
  if (h->harbor_hub && 0 == strcmp(h->harbor_hub, h->local_harbor_host) && h->local_harbor_id != REMOTE_MAX - 1) {
    skynet_error(h->ctx, "harbor2[%u]:The harbor_hub_id must be 255", h->local_harbor_id);
    return -1;
  }
  //
  harbor_start(h);
  //
  if (h->harbor_hub && strcmp(h->harbor_hub, h->local_harbor_host)) {
    harbor_connect(h, REMOTE_MAX - 1, h->harbor_hub);
  }
  //
  return 0;
}
// -------------------------------------------------------------------------
