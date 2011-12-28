#ifndef COMMON_H_
#define COMMON_H_

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_ROUNDS 1
#define PACKET_SIZE 2000
#define WORDS_PER_PACKET 1
#define CLIENT_SERVICE_ID 4
#define SERVER_SERVICE_ID 1

#define ERROR(...) fprintf(stderr, "%s() (%s, line %u): ", __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n")
#define panic(...) fprintf(stderr, "%s() (%s, line %u): FATAL: ", __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); exit(EXIT_FAILURE)

#ifdef HAVE_DEBUG
#define DEBUG(...) ERROR(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifndef HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
#   define _Bool signed char
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif



#endif /* COMMON_H_ */
