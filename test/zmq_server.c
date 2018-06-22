#include <pthread.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include <zmq.h>

#include "jsonrpc.h"
#include "test.h"

void _zfree(void *data, void *hint)
{
	free(data);
}

int main()
{
	void *ctx = zmq_ctx_new();
	void *sock = zmq_socket(ctx, ZMQ_PAIR);
	int rc = zmq_bind(sock, "tcp://127.0.0.1:10000");
	assert(rc!=-1);

	struct user_context_t userctx = {.count = 0, .sock = sock};
	while (1) 
	{
		zmq_msg_t msg;
		zmq_msg_init(&msg);
		zmq_msg_recv(&msg, sock, 0);
		char *input = (char *)zmq_msg_data(&msg);
		int inputlen = zmq_msg_size(&msg);
		printf("receive:\n%s\n", input);
		char *output = jsonrpc_handler(input, inputlen, method_table, &userctx);
		zmq_msg_close(&msg);

		if (output) {
			printf("send:\n%s\n", output);
			zmq_msg_t msg;
			zmq_msg_init_data(&msg, output, strlen(output),_zfree, NULL);
			zmq_msg_send(&msg, sock, 0);
			zmq_msg_close(&msg);
		}
	}
	
	zmq_close(sock);
	zmq_ctx_destroy(ctx);
	
	return 0;
}
