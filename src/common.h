#ifndef COMMON_H_
#define COMMON_H_

#include "config.h"
#include <assert.h>

#define eprintf(x) fprintf(2, "%s (%s, line %u): %s\n", __PRETTY_FUNCTION__, __FILE__, __LINE__, x)

#ifdef DEBUG
#define dprintf(x) eprintf(x)
#else
#define dprintf(...)
#endif

#endif /* COMMON_H_ */
