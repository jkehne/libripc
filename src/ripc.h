#ifndef RIPC_H_
#define RIPC_H_

#include <stdint.h>
#include <sys/types.h>

#define RECV_BUF_SIZE 2100
#define NUM_RECV_BUFFERS 10

#ifdef __cplusplus
extern "C" {
#endif

uint16_t ripc_register_random_service_id(void);
uint8_t ripc_register_service_id(int);

void *ripc_buf_alloc(size_t size);
void ripc_buf_free(void *buf);

uint8_t ripc_send_short(
		uint16_t src,
		uint16_t dest,
		void **buf,
		uint32_t *length,
		uint32_t num_items
		);

uint8_t ripc_send_long(
		uint16_t src,
		uint16_t dest,
		void *buf,
		uint32_t length
		);

uint8_t ripc_receive(
		uint16_t service_id,
		uint16_t *from_service_id,
		void ***short_items,
		void ***long_items
		);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* RIPC_H_ */
