#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <ripc.h>
#include <common.h>
#include <memory.h>
#include <resolver.h>
#include <resources.h>

struct library_context context;

pthread_mutex_t services_mutex, remotes_mutex;

uint8_t init(void) {
    	if (context.netarch.device_context != NULL)
		return true;

	srand(time(NULL));

        memset(context.services, 0, UINT16_MAX * sizeof(struct service_id *));
	memset(context.remotes, 0, UINT16_MAX * sizeof(struct remote_context *));
	context.netarch.device_context = NULL;

        pthread_mutex_init(&services_mutex, NULL);
	pthread_mutex_init(&remotes_mutex, NULL);
	pthread_mutex_init(&used_list_mutex, NULL);
	pthread_mutex_init(&free_list_mutex, NULL);
	pthread_mutex_init(&recv_window_mutex, NULL);
	pthread_mutex_init(&rdma_connect_mutex, NULL);
	pthread_mutex_init(&resolver_mutex, NULL);

        dispatch_responder();

	return netarch_init();

}
