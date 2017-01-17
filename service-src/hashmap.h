/* -------------------------------------------------------------------------
//	FileName		:	d:\yx_code\yx\skynet\service-src\hashmap.h
//	Creator			:	(zc)
//	CreateTime	:	2016-9-18 9:20
//	Description	:	
//
// -----------------------------------------------------------------------*/
#ifndef HASHMAP_H_
#define HASHMAP_H_

// -------------------------------------------------------------------------
#define HASH_SIZE 4096

struct hashnode {
  char key[32];
  union {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    void*    ptr;
  } value;
  struct hashnode* next;
};

struct hashcell {
  struct hashnode* node;
  struct hashcell* next;
};

struct hashmap {
  uint32_t size;
  struct hashcell* list;
  struct hashcell cells[HASH_SIZE];
  struct hashnode* (*alloc_fn)();
  void(*free_fn)(struct hashnode* p);
};


uint32_t hash_string(const char* str) {
  uint32_t hash = 5381;
  int c;

  while (c = *str++) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

void hashmap_init(struct hashmap* map, struct hashnode* (*alloc_fn)(), void(*free_fn)(struct hashnode* p)) {
  memset(map, 0, sizeof(*map));
  map->alloc_fn = alloc_fn;
  map->free_fn = free_fn;
}

struct hashnode* hashmap_insert(struct hashmap* map, const char* key) {
  uint32_t hash = hash_string(key);
  int pos = hash % HASH_SIZE;
  //
  struct hashcell* cell = &map->cells[pos];
  struct hashnode* node = cell->node;
  while (node) {
    if (0 == strcmp(node->key, key))
      return node;
    // next
    node = node->next;
  }
  //
  if (NULL == cell->node) {
    cell->next = map->list;
    map->list = cell;
  }
  //
  struct hashnode * node_new = map->alloc_fn();
  int key_size = strlen(key);
  if (key_size >= sizeof(node_new->key)) key_size = sizeof(node_new->key) - 1;
  memcpy(node_new->key, key, key_size);
  node_new->key[key_size] = '\0';
  //
  node_new->next = cell->node;
  cell->node = node_new;
  ++map->size;
  //
  return node_new;
}

struct hashnode* hashmap_lookup(struct hashmap* map, const char* key) {
  uint32_t hash = hash_string(key);
  int pos = hash % HASH_SIZE;
  //
  struct hashnode* node = map->cells[pos].node;
  while (node) {
    if (0 == strcmp(node->key, key))
      return node;
    // next
    node = node->next;
  }
  //
  return NULL;
}

void hashmap_foreach(struct hashmap* map, void(*fn)(struct hashnode* n, void* ud), void* ud) {
  for (struct hashcell* c = map->list; c; c = c->next) {
    struct hashnode* node = c->node;
    while (node) {
      fn(node, ud);
      // next
      node = node->next;
    }
  }
}

void hashmap_remove(struct hashmap* map, const char* key) {
  uint32_t hash = hash_string(key);
  int pos = hash % HASH_SIZE;
  //
  struct hashcell* cell = &map->cells[pos];
  struct hashnode* node = cell->node;
  if (NULL == node) return;
  //
  if (0 == strcmp(node->key, key)) {
    cell->node = node->next;
  } else {
    while (node->next) {
      if (0 == strcmp(node->next->key, key)) {
        node->next = node->next->next;
        break;
      }
      // next
      node = node->next;
    } // while
  }
  //
  if (NULL == cell->node) {
    if (cell == map->list) {
      map->list = cell->next;
    } else {
      for (struct hashcell* c = map->list; c && c->next;) {
        if (c->next == cell) {
          c->next = cell->next;
          break;
        }
        c = c->next;
      } // for
    }
  }
  map->free_fn(node);
  --map->size;
}

//void hashmap_remove2(struct hashmap* map, bool(*fn)(struct hashnode* n, void* ud), void* ud) {
//  for (int i = 0; i < HASH_SIZE; ++i) {
//    struct hashnode* node = map->nodes[i];
//    if (node) {
//      while (node->next) {
//        if (fn(node->next, ud)) {
//          struct hashnode* node_free = node->next;
//          node->next = node->next->next;
//          map->free_fn(node_free);
//          --map->size;
//        } else {
//          node = node->next;
//        }
//      }
//      node = map->nodes[i];
//      if (fn(node, ud)) {
//        map->nodes[i] = node->next;
//        map->free_fn(node);
//        --map->size;
//      }
//    } // if node
//  } // for
//}

void hashmap_clear(struct hashmap* map) {
  for (struct hashcell* c = map->list; c;) {
    struct hashnode* node = c->node;
    while (node) {
      struct hashnode* node_free = node;
      node = node->next;
      map->free_fn(node_free);
    }
    --map->size;
    c->node = NULL;
    //
    struct hashcell* c_pre = c;
    c = c->next;
    c_pre->next = NULL;
  } // for
  map->list = NULL;
}

// -------------------------------------------------------------------------
#endif /* HASHMAP_H_ */
