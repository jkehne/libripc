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
#include "common.h"
#include "memory.h"
#include "ripc.h"
#include <common.h>
#include <errno.h>
#include <memory.h>
#include <pthread.h>
#include <resolver.h>
#include <resources.h>
#include <ripc.h>
#include <string.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

mem_buf_t ripc_alloc_recv_buf(size_t size) {
        DEBUG("ripc_alloc_recv_buf %zu", size);
        return invalid_mem_buf;

}

uint8_t ripc_buf_register(void *buf, size_t size) {
        DEBUG("ripc_alloc_recv_buf %p size %zu", buf, size);
	return 0;
}
