/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_rand.h directly! Include yu_common.h instead."
#endif

struct yu_rand_funcs;

typedef void (* yu_rand_free_fn)(struct yu_rand_funcs *rng);
typedef s32 (* yu_rand_int_fn)(struct yu_rand_funcs *rng);
typedef u8 (* yu_rand_byte_fn)(struct yu_rand_funcs *rng);
typedef void (* yu_rand_byte_array_fn)(struct yu_rand_funcs *rng, u8 *bytes_out, size_t count);

typedef struct yu_rand_funcs {
  yu_rand_free_fn rand_free;
  yu_rand_int_fn rand_int;
  yu_rand_byte_fn rand_byte;
  yu_rand_byte_array_fn rand_byte_array;
} yu_rand;

// Take `rng` as a void pointer to avoid warnings about incompatible pointer
// types, since these functions will be passed ‘subclasses’ of yu_rand.

YU_INLINE
void yu_rand_free(void *rng) {
  ((yu_rand *)rng)->rand_free(rng);
}

YU_INLINE
s32 yu_rand_int(void *rng) {
  return ((yu_rand *)rng)->rand_int(rng);
}

YU_INLINE
u8 yu_rand_byte(void *rng) {
  return ((yu_rand *)rng)->rand_byte(rng);
}

YU_INLINE
void yu_rand_byte_array(void *rng, u8 *bytes_out, size_t count) {
  return ((yu_rand *)rng)->rand_byte_array(rng, bytes_out, count);
}
