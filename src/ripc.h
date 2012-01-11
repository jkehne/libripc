#ifndef RIPC_H_
#define RIPC_H_

#include <stdint.h>
#include <sys/types.h>

#define RECV_BUF_SIZE 2000
#define NUM_RECV_BUFFERS 10

#ifdef __cplusplus
extern "C" {
#endif

uint8_t init(void);

uint16_t ripc_register_random_service_id(void);
uint8_t ripc_register_service_id(int);

void *ripc_buf_alloc(size_t size);
void ripc_buf_free(void *buf);
uint8_t ripc_reg_recv_window(void *base, size_t size);
uint8_t ripc_buf_register(void *buf, size_t size);

uint8_t ripc_send_short(
		uint16_t src,
		uint16_t dest,
		void **buf,
		size_t *length,
		uint32_t num_items,
		void **return_bufs,
		size_t *return_buf_lengths,
		uint32_t num_return_bufs
		);

uint8_t ripc_send_long(
		uint16_t src,
		uint16_t dest,
		void **buf,
		size_t *length,
		uint32_t num_items,
		void **return_bufs,
		size_t *return_buf_lengths,
		uint32_t num_return_bufs
		);

uint8_t ripc_receive(
		uint16_t service_id,
		uint16_t *from_service_id,
		void ***short_items,
		uint32_t **short_item_sizes,
		uint16_t *num_short_items,
		void ***long_items,
		uint32_t **long_item_sizes,
		uint16_t *num_long_items
		);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* RIPC_H_ */
