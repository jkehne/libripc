#ifndef RIPC_H_
#define RIPC_H_

#include <stdint.h>

#define RECV_BUF_SIZE 65536
#define NUM_RECV_BUFFERS 10

#ifdef __cplusplus
extern "C" {
#endif

uint16_t ripc_register_random_service_id(void);
uint8_t ripc_register_service_id(int);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* RIPC_H_ */
