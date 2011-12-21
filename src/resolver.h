#ifndef RESOLVER_H_
#define RESOLVER_H_

#include "config.h"
#include "common.h"

struct resolver_msg {
	enum msg_type type;
	uint16_t dest_service_id;
	uint16_t src_service_id;
	uint16_t lid;
	uint32_t service_qpn;
	uint32_t response_qpn;
	uint32_t resolver_qpn;
};

void dispatch_responder(void);
void resolve(uint16_t src, uint16_t dest);

#endif /* RESOLVER_H_ */
