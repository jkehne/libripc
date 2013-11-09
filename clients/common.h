/*  Copyright 2011, 2012 Jens Kehne
 *  Copyright 2012 Jan Stoess, Karlsruhe Institute of Technology
 *
 *  LibRIPC is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation, either version 2.1 of the License, or (at your
 *  option) any later version.
 *
 *  LibRIPC is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with libRIPC.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CLIENTS_COMMON_H_
#define CLIENTS_COMMON_H_

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define OLD_RESOLVER 1
#define NUM_ROUNDS 10000
#define PACKET_SIZE RECV_BUF_SIZE - 100
#define WORDS_PER_PACKET 1
//#define CLIENT_SERVICE_ID 4
#define SERVER_SERVICE_ID 1
#define CLIENT_RETURN_BUFFERS 0
#define SERVER_RETURN_BUFFERS 0
#define XCHANGE_SERVICE "XChangeService"

#define ERROR(...) do { fprintf(stderr, "%s() (%s, line %u): ", __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); } while (0)
#define panic(...) do { fprintf(stderr, "%s() (%s, line %u): FATAL: ", __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); exit(EXIT_FAILURE); } while (0)

#ifdef HAVE_DEBUG
#define DEBUG(...) ERROR(__VA_ARGS__)
#else
#define DEBUG(...) do { } while (0)
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



#endif /* CLIENTS_COMMON_H_ */
