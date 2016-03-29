// Implementation for rope library.

#include "rope.h"

// The number of bytes the rope head structure takes up
static const size_t ROPE_SIZE = sizeof(rope) + sizeof(rope_node) * ROPE_MAX_HEIGHT;

// Create a new rope with no contents
rope *rope_new(yu_allocator *mctx, sfmt_t *rng) {
  rope *r = yu_xalloc(mctx, 1, ROPE_SIZE);
  r->num_chars = r->num_bytes = 0;

  r->mem_ctx = mctx;
  r->rng = rng;

  r->head.height = 1;
  r->head.num_bytes = 0;
  r->head.nexts[0].node = NULL;
  r->head.nexts[0].skip_size = 0;
  return r;
}

// Create a new rope containing the specified string
rope *rope_new_with_utf8(yu_allocator *mctx, sfmt_t *rng, const uint8_t *str) {
  rope *r = rope_new(mctx, rng);
  ROPE_RESULT result = rope_insert(r, 0, str);

  if (result != ROPE_OK) {
    rope_free(r);
    return NULL;
  } else {
    return r;
  }
}

rope *rope_copy(const rope *other) {
  rope *r = yu_xalloc(other->mem_ctx, 1, ROPE_SIZE);

  // Just copy most of the head's data. Note this won't copy the nexts list in head.
  *r = *other;

  rope_node *nodes[ROPE_MAX_HEIGHT];

  for (int i = 0; i < other->head.height; i++) {
    nodes[i] = &r->head;
    // non-NULL next pointers will be rewritten below.
    r->head.nexts[i] = other->head.nexts[i];
  }

  for (rope_node *n = other->head.nexts[0].node; n != NULL; n = n->nexts[0].node) {
    // I wonder if it would be faster if we took this opportunity to rebalance the node list..?
    size_t h = n->height;
    rope_node *n2 = yu_xalloc(r->mem_ctx, sizeof(rope_node) + h, sizeof(rope_skip_node));

    // Would it be faster to just *n2 = *n; ?
    n2->num_bytes = n->num_bytes;
    n2->height = h;
    memcpy(n2->val.str, n->val.str, n->num_bytes);
    memcpy(n2->nexts, n->nexts, h * sizeof(rope_skip_node));

    for (int i = 0; i < h; i++) {
      nodes[i]->nexts[i].node = n2;
      nodes[i] = n2;
    }
  }

  return r;
}

// Free the specified rope
void rope_free(rope *r) {
  assert(r);
  rope_node *next;

  for (rope_node *n = r->head.nexts[0].node; n != NULL; n = next) {
    next = n->nexts[0].node;
    yu_free(r->mem_ctx, n);
  }

  yu_free(r->mem_ctx, r);
}

// Get the number of characters in a rope
size_t rope_char_count(const rope *r) {
  assert(r);
  return r->num_chars;
}

// Get the number of bytes which the rope would take up if stored as a utf8
// string
size_t rope_byte_count(const rope *r) {
  assert(r);
  return r->num_bytes;
}

// Copies the rope's contents into a utf8 encoded C string. Also copies a trailing '\0' character.
// Returns the number of bytes written, which is rope_byte_count(r) + 1.
size_t rope_write_cstr(rope *r, uint8_t * restrict dest) {
  size_t num_bytes = rope_byte_count(r);
  dest[num_bytes] = '\0';

  if (num_bytes) {
    uint8_t *p = dest;
    for (rope_node* restrict n = &r->head; n != NULL; n = n->nexts[0].node) {
      memcpy(p, n->val.str, n->num_bytes);
      p += n->num_bytes;
    }

    assert(p == &dest[num_bytes]);
  }
  return num_bytes + 1;
}

// Create a new C string which contains the rope. The string will contain
// the rope encoded as utf8.
uint8_t *rope_create_cstr(rope *r) {
  uint8_t *bytes = yu_xalloc(r->mem_ctx, rope_byte_count(r) + 1, 1); // Room for a zero.
  rope_write_cstr(r, bytes);
  return bytes;
}

static
uint8_t random_height(rope *r) {
  // This function is horribly inefficient. I'm throwing away heaps of entropy, and
  // the mod could be replaced by some clever shifting.
  //
  // However, random_height barely appears in the profiler output - so its probably
  // not worth investing the time to optimise.

  uint8_t height = 1;

  // The root node's height is the height of the largest node + 1, so the largest
  // node can only have ROPE_MAX_HEIGHT - 1.
  while(height < (ROPE_MAX_HEIGHT - 1) && (sfmt_genrand_uint32(r->rng) % 100) < ROPE_BIAS) {
    height++;
  }

  return height;
}

// Figure out how many bytes to allocate for a node with the specified height.
static YU_CONST
size_t node_size(uint8_t height) {
  return sizeof(rope_node) + height * sizeof(rope_skip_node);
}

// Allocate and return a new node. The new node will be full of junk, except
// for its height.
// This function should be replaced at some point with an object pool based version.
static YU_MALLOC_LIKE
rope_node *alloc_node(rope *r, uint8_t height) {
  rope_node *node = yu_xalloc(r->mem_ctx, 1, node_size(height));
  node->height = height;
  return node;
}

// Find out how many bytes the unicode character which starts with the specified byte
// will occupy in memory.
// Returns the number of bytes, or SIZE_MAX if the byte is invalid.
static YU_CONST
size_t codepoint_size(uint8_t byte) {
  if (byte == 0) { return SIZE_MAX; } // NULL byte.
  else if (byte <= 0x7f) { return 1; } // 0x74 = 0111 1111
  else if (byte <= 0xbf) { return SIZE_MAX; } // 1011 1111. Invalid for a starting byte.
  else if (byte <= 0xdf) { return 2; } // 1101 1111
  else if (byte <= 0xef) { return 3; } // 1110 1111
  else if (byte <= 0xf7) { return 4; } // 1111 0111
  else if (byte <= 0xfb) { return 5; } // 1111 1011
  else if (byte <= 0xfd) { return 6; } // 1111 1101
  else { return SIZE_MAX; }
}

// This little function counts how many bytes a certain number of characters take up.
static YU_PURE
size_t count_bytes_in_utf8(const uint8_t *str, size_t num_chars) {
  const uint8_t *p = str;
  for (unsigned int i = 0; i < num_chars; i++) {
    p += codepoint_size(*p);
  }
  return p - str;
}

// Count the number of characters in a string.
static YU_PURE
size_t strlen_utf8(const uint8_t *str) {
  const uint8_t *p = str;
  size_t i = 0;
  while (*p) {
    p += codepoint_size(*p);
    i++;
  }
  return i;
}

// Checks if a UTF8 string is ok. Returns the number of bytes in the string if
// it is ok, otherwise returns -1.
static YU_PURE
ssize_t bytelen_and_check_utf8(const uint8_t *str) {
  const uint8_t *p = str;
  while (*p != '\0') {
    size_t size = codepoint_size(*p);
    if (size == SIZE_MAX) return -1;
    p++; size--;
    while (size > 0) {
      // Check that any middle bytes are of the form 0x10xx xxxx
      if ((*p & 0xc0) != 0x80)
        return -1;
      p++; size--;
    }
  }

  assert(p - str == strlen((char *)str));

  return p - str;
}

typedef struct {
  // This stores the previous node at each height, and the number of characters from the start of
  // the previous node to the current iterator position.
  rope_skip_node s[ROPE_MAX_HEIGHT];
} rope_iter;

// Internal function for navigating to a particular character offset in the rope.
// The function returns the list of nodes which point past the position, as well as
// offsets of how far into their character lists the specified characters are.
static
rope_node *iter_at_char_pos(rope *r, size_t char_pos, rope_iter *iter) {
  assert(char_pos <= r->num_chars);

  rope_node *e = &r->head;
  int height = r->head.height - 1;

  // Offset stores how many characters we still need to skip in the current node.
  size_t offset = char_pos;
  size_t skip;

  while (true) {
    skip = e->nexts[height].skip_size;
    if (offset > skip) {
      // Go right.
      assert(e == &r->head || e->num_bytes);

      offset -= skip;
      e = e->nexts[height].node;
    } else {
      // Go down.
      iter->s[height].skip_size = offset;
      iter->s[height].node = e;

      if (height == 0) {
        break;
      } else {
        height--;
      }
    }
  }

  assert(offset <= ROPE_NODE_STR_SIZE);
  assert(iter->s[0].node == e);
  return e;
}

static
void update_offset_list(rope *r, rope_iter *iter, size_t num_chars) {
  for (int i = 0; i < r->head.height; i++) {
    iter->s[i].node->nexts[i].skip_size += num_chars;
  }
}


// Internal method of rope_insert.
// This function creates a new node in the rope at the specified position and fills it with the
// passed string.
static
void insert_at(rope *r, rope_iter *iter, const uint8_t *str, size_t num_bytes, size_t num_chars) {

  // This describes how many levels of the iter are filled in.
  uint8_t max_height = r->head.height;
  uint8_t new_height = random_height(r);
  rope_node *new_node = alloc_node(r, new_height);
  new_node->num_bytes = num_bytes;
  memcpy(new_node->val.str, str, num_bytes);

  assert(new_height < ROPE_MAX_HEIGHT);

  // Max height (the rope's head's height) must be 1+ the height of the largest node.
  while (max_height <= new_height) {
    r->head.height++;
    r->head.nexts[max_height] = r->head.nexts[max_height - 1];

    // This is the position (offset from the start) of the rope.
    iter->s[max_height] = iter->s[max_height - 1];
    max_height++;
  }

  // Fill in the new node's nexts array.
  int i;
  for (i = 0; i < new_height; i++) {
    rope_skip_node *prev_skip = &iter->s[i].node->nexts[i];
    new_node->nexts[i].node = prev_skip->node;
    new_node->nexts[i].skip_size = num_chars + prev_skip->skip_size - iter->s[i].skip_size;


    prev_skip->node = new_node;
    prev_skip->skip_size = iter->s[i].skip_size;

    // & move the iterator to the end of the newly inserted node.
    iter->s[i].node = new_node;
    iter->s[i].skip_size = num_chars;
  }

  for (; i < max_height; i++) {
    iter->s[i].node->nexts[i].skip_size += num_chars;
    iter->s[i].skip_size += num_chars;
  }

  r->num_chars += num_chars;
  r->num_bytes += num_bytes;
}

// Insert the given utf8 string into the rope at the specified position.
static
ROPE_RESULT rope_insert_at_iter(rope *r, rope_node *e, rope_iter *iter, const uint8_t *str) {
  // iter.offset contains how far (in characters) into the current element to skip.
  // Figure out how much that is in bytes.
  size_t offset_bytes = 0;
  // The insertion offset into the destination node.
  size_t offset = iter->s[0].skip_size;
  if (offset) {
    assert(offset <= e->nexts[0].skip_size);
    offset_bytes = count_bytes_in_utf8(e->val.str, offset);
  }

  // We might be able to insert the new data into the current node, depending on
  // how big it is. We'll count the bytes, and also check that its valid utf8.
  ssize_t num_inserted_bytes = bytelen_and_check_utf8(str);
  if (num_inserted_bytes == -1) return ROPE_INVALID_UTF8;

  // Can we insert into the current node?
  bool insert_here = e->num_bytes + num_inserted_bytes <= ROPE_NODE_STR_SIZE;

  // Can we insert into the subsequent node?
  rope_node *next = NULL;
  if (!insert_here && offset_bytes == e->num_bytes) {
    next = e->nexts[0].node;
    // We can insert into the subsequent node if:
    // - We can't insert into the current node
    // - There _is_ a next node to insert into
    // - The insert would be at the start of the next node
    // - There's room in the next node
    if (next && next->num_bytes + num_inserted_bytes <= ROPE_NODE_STR_SIZE) {
      offset = offset_bytes = 0;
      for (int i = 0; i < next->height; i++) {
        iter->s[i].node = next;
        // tree offset nodes will not be used.
      }
      e = next;

      insert_here = true;
    }
  }

  if (insert_here) {
    // First move the current bytes later on in the string.
    if (offset_bytes < e->num_bytes) {
      memmove(&e->val.str[offset_bytes + num_inserted_bytes],
              &e->val.str[offset_bytes],
              e->num_bytes - offset_bytes);
    }

    // Then copy in the string bytes
    memcpy(&e->val.str[offset_bytes], str, num_inserted_bytes);
    e->num_bytes += num_inserted_bytes;

    r->num_bytes += num_inserted_bytes;
    size_t num_inserted_chars = strlen_utf8(str);
    r->num_chars += num_inserted_chars;

    // .... aaaand update all the offset amounts.
    update_offset_list(r, iter, num_inserted_chars);

  } else {
    // There isn't room. We'll need to add at least one new node to the rope.

    // If we're not at the end of the current node, we'll need to remove
    // the end of the current node's data and reinsert it later.
    size_t num_end_chars, num_end_bytes = e->num_bytes - offset_bytes;
    if (num_end_bytes) {
      // We'll pretend like the character have been deleted from the node, while leaving
      // the bytes themselves there (for later).
      e->num_bytes = offset_bytes;
      num_end_chars = e->nexts[0].skip_size - offset;
      update_offset_list(r, iter, -num_end_chars);

      r->num_chars -= num_end_chars;
      r->num_bytes -= num_end_bytes;
    }

    // Now we insert new nodes containing the new character data. The data must be broken into
    // pieces of with a maximum size of ROPE_NODE_STR_SIZE. Node boundaries must not occur in the
    // middle of a utf8 codepoint.
    size_t str_offset = 0;
    while (str_offset < num_inserted_bytes) {
      size_t new_node_bytes = 0;
      size_t new_node_chars = 0;

      while (str_offset + new_node_bytes < num_inserted_bytes) {
        size_t cs = codepoint_size(str[str_offset + new_node_bytes]);
        if (cs + new_node_bytes > ROPE_NODE_STR_SIZE) {
          break;
        } else {
          new_node_bytes += cs;
          new_node_chars++;
        }
      }

      insert_at(r, iter, &str[str_offset], new_node_bytes, new_node_chars);
      str_offset += new_node_bytes;
    }

    if (num_end_bytes) {
      insert_at(r, iter, &e->val.str[offset_bytes], num_end_bytes, num_end_chars);
    }
  }

  return ROPE_OK;
}

ROPE_RESULT rope_insert(rope *r, size_t pos, const uint8_t * restrict str) {
  assert(r);
  assert(str);
  ROPE_CHECK(r);
  pos = min(pos, r->num_chars);

  rope_iter iter;
  // First we need to search for the node where we'll insert the string.
  rope_node *e = iter_at_char_pos(r, pos, &iter);

  ROPE_RESULT result = rope_insert_at_iter(r, e, &iter, str);

  ROPE_CHECK(r);

  return result;
}

// Delete num characters at position pos. Deleting past the end of the string
// has no effect.
static
void rope_del_at_iter(rope *r, rope_node *e, rope_iter *iter, size_t length) {
  r->num_chars -= length;
  size_t offset = iter->s[0].skip_size;
  while (length) {
    if (offset == e->nexts[0].skip_size) {
      // End of the current node. Skip to the start of the next one.
      e = iter->s[0].node->nexts[0].node;
      offset = 0;
    }

    size_t num_chars = e->nexts[0].skip_size;
    size_t removed = min(length, num_chars - offset);

    int i;
    if (removed < num_chars || e == &r->head) {
      // Just trim this node down to size.
      size_t leading_bytes = count_bytes_in_utf8(e->val.str, offset);
      size_t removed_bytes = count_bytes_in_utf8(&e->val.str[leading_bytes], removed);
      size_t trailing_bytes = e->num_bytes - leading_bytes - removed_bytes;
      if (trailing_bytes) {
        memmove(&e->val.str[leading_bytes], &e->val.str[leading_bytes + removed_bytes], trailing_bytes);
      }
      e->num_bytes -= removed_bytes;
      r->num_bytes -= removed_bytes;

      for (i = 0; i < e->height; i++) {
        e->nexts[i].skip_size -= removed;
      }
    } else {
      // Remove the node from the list
      for (i = 0; i < e->height; i++) {
        iter->s[i].node->nexts[i].node = e->nexts[i].node;
        iter->s[i].node->nexts[i].skip_size += e->nexts[i].skip_size - removed;
      }

      r->num_bytes -= e->num_bytes;
      // TODO: Recycle e.
      rope_node *next = e->nexts[0].node;
      yu_free(r->mem_ctx, e);
      e = next;
    }

    for (; i < r->head.height; i++) {
      iter->s[i].node->nexts[i].skip_size -= removed;
    }

    length -= removed;
  }
}

void rope_del(rope *r, size_t pos, size_t length) {
  assert(r);
  ROPE_CHECK(r);

  pos = min(pos, r->num_chars);
  length = min(length, r->num_chars - pos);

  rope_iter iter;

  // Search for the node where we'll insert the string.
  rope_node *e = iter_at_char_pos(r, pos, &iter);

  rope_del_at_iter(r, e, &iter, length);

  ROPE_CHECK(r);
}

void _rope_check(rope *r) {
  assert(r->head.height); // Even empty ropes have a height of 1.
  assert(r->num_bytes >= r->num_chars);

  rope_skip_node skip_over = r->head.nexts[r->head.height - 1];
  assert(skip_over.skip_size == r->num_chars);
  assert(skip_over.node == NULL);

  size_t num_bytes = 0;
  size_t num_chars = 0;

  // The offsets here are used to store the total distance travelled from the start
  // of the rope.
  rope_iter iter = {};
  for (int i = 0; i < r->head.height; i++) {
    iter.s[i].node = &r->head;
  }

  for (rope_node *n = &r->head; n != NULL; n = n->nexts[0].node) {
    assert(n == &r->head || n->num_bytes);
    assert(n->height <= ROPE_MAX_HEIGHT);
    assert(count_bytes_in_utf8(n->val.str, n->nexts[0].skip_size) == n->num_bytes);
    for (int i = 0; i < n->height; i++) {
      assert(iter.s[i].node == n);
      assert(iter.s[i].skip_size == num_chars);
      iter.s[i].node = n->nexts[i].node;
      iter.s[i].skip_size += n->nexts[i].skip_size;
    }

    num_bytes += n->num_bytes;
    num_chars += n->nexts[0].skip_size;
  }

  for (int i = 0; i < r->head.height; i++) {
    assert(iter.s[i].node == NULL);
    assert(iter.s[i].skip_size == num_chars);
  }

  assert(r->num_bytes == num_bytes);
  assert(r->num_chars == num_chars);
}

// For debugging.
void _rope_print(rope *r) {
  printf("chars: %zd\tbytes: %zd\theight: %d\n", r->num_chars, r->num_bytes, r->head.height);

  printf("HEAD");
  for (int i = 0; i < r->head.height; i++) {
    printf(" |%3zd ", r->head.nexts[i].skip_size);
  }
  printf("\n");

  int num = 0;
  for (rope_node *n = &r->head; n != NULL; n = n->nexts[0].node) {
    printf("%3d:", num++);
    for (int i = 0; i < n->height; i++) {
      printf(" |%3zd ", n->nexts[i].skip_size);
    }
    printf("        : \"");
    fwrite(n->val.str, n->num_bytes, 1, stdout);
    printf("\"\n");
  }
}
