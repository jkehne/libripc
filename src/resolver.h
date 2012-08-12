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
#ifndef RESOLVER_H_
#define RESOLVER_H_

#include "config.h"
#include "common.h"
#ifdef NETARCH_INFINIBAND
#include <infiniband/resolver.h>
#endif
#ifdef NETARCH_BGP
#include <bgp/resolver.h>
#endif
#ifdef NETARCH_LOCAL
#include <local/resolver.h>
#endif

struct resolver_msg {
	enum msg_type type;
	uint16_t dest_service_id;
	uint16_t src_service_id;
	struct netarch_resolver_msg na;
};

extern pthread_mutex_t resolver_mutex;

void resolver_init(void);
void resolve(uint16_t src, uint16_t dest);

#endif /* RESOLVER_H_ */
