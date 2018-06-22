#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include <zmq.h>

#include "jsonrpc.h"
#include "test.h"

void wait_msg(struct user_context_t *userctx)
{
	unsigned int rc;
	zmq_msg_t msg;
	zmq_msg_init(&msg);

	rc = zmq_msg_recv(&msg, userctx->sock, 0);

	char *output = jsonrpc_handler((char *)zmq_msg_data(&msg), 
			zmq_msg_size(&msg), method_table, (char *)userctx);
	if (output != NULL) {
		printf("error: %s\n", output);
		free(output);
	}

	zmq_msg_close(&msg);
}

void *_thread(void *arg)
{
	int run = 1;
	struct user_context_t *userctx = (struct user_context_t *)arg;

	while (run) {
		wait_msg(userctx);
	}
	return NULL;
}

int start_event(struct user_context_t * userctx)
{
	pthread_t pthread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_create(&pthread, &attr, _thread, userctx);
	pthread_detach(pthread);

	return 0;
}

int treatmsg(struct user_context_t *userctx, char *request)
{
	int rc;

	rc = zmq_send(userctx->sock, request, strlen(request), 0);
	wait_msg(userctx);
	return rc;
}

typedef int (*treatmsg_t)(struct user_context_t *userctx, char *request);

int client_loop(treatmsg_t treatmsg, struct user_context_t *userctx)
{
	int ret;
	while (1) 
	{
		fd_set rfds;
		int maxfd = 0;

		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (ret > 0)
		{
			if (FD_ISSET(0, &rfds))
			{
				char buff[256];
				ret = read(0, buff, sizeof(buff));
				if (ret > 0)
				{
					char *request = NULL;
					json_t *params = NULL;
					unsigned long id;
					buff[ret] = 0;
					if (buff[ret-1] = '\n')
						buff[ret-1] = 0;
					if (!strncmp(buff, "quit", 4)) {
						break;
					}
					request = jsonrpc_request(buff, strlen(buff), method_table,
							(char *)userctx, &id);
					if (request != NULL) {
						treatmsg(userctx, request);
						free(request);
						request = NULL;
					}
				}
			}
		}
	}
	return ret;
}

json_t *client_error_handler(json_t *json_id, json_t *json_error)
{
	json_t *data = json_object_get(json_error, "data");
	if (json_is_string(data))
		printf("error :%s\n", json_string_value(data));
	return NULL;
}

int main()
{
	void *ctx = zmq_ctx_new();
	void *sock = zmq_socket(ctx, ZMQ_PAIR);
	int rc = zmq_connect(sock, "tcp://127.0.0.1:10000");
	assert(rc!=-1);

	struct user_context_t userctx = {.count = 0, .sock = sock};
	jsonrpc_set_errorhandler(client_error_handler);

	client_loop(treatmsg, &userctx);
	zmq_close(sock);
	zmq_ctx_destroy(ctx);

	return 0;
}
