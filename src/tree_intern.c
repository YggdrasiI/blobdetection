#include <assert.h>
#include <string.h>
#include <stddef.h>

// This is the only file where INLINE is
// defined as 'extern inline'. This made the function
// externaly available in other object files in the linking process,
// if needed (if calls in other modules where not inlined.
// This is normally the case in debug builds)
//
// As noted in https://en.wikipedia.org/wiki/Inline_function#C99
// the functions in this file could lead to unreachable code
// in the final binary, but this problem is ignored here. 
#define INLINE extern inline
#include "tree_intern.h"

// Emptyness here is intentional.
