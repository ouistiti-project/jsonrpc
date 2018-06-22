#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "jsonrpc.h"
#include "test.h"

int treatmsg(struct user_context_t *userctx, char *request)
{
	int ret;
	int sock = (int)userctx->sock;

	ret = send(sock, request, strlen(request), 0);
	return ret;
}

typedef int (*treatmsg_t)(struct user_context_t *userctx, char *request);

int client_loop(treatmsg_t treatmsg, struct user_context_t *userctx)
{
	int ret;
	while (1) 
	{
		int sock = (int)userctx->sock;
		fd_set rfds;
		int maxfd = sock;

		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		FD_SET(sock, &rfds);
		ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (ret > 0) {
			if (FD_ISSET(0, &rfds)) {
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
			if (FD_ISSET(sock, &rfds)) {
				char buffer[1500];
				ret = recv(sock, buffer, 1500, 0);
				char *output = jsonrpc_handler(buffer, ret, method_table, (char *)userctx);
				if (output != NULL) {
					printf("error: %s\n", output);
					free(output);
				}
				if (ret < 0)
					break;
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

int main(int argc, char *argv)
{
	const char *root = "/tmp";
	const char *name = "jsonrpc";
	int sock;
	int ret;

	jsonrpc_set_errorhandler(client_error_handler);

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock > 0) {
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(struct sockaddr_un));
		addr.sun_family = AF_UNIX;
		snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s/%s", root, name);

		ret = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
		if (ret == 0) {
			struct user_context_t userctx = {.count = 0, .sock = (void *)sock};
			client_loop(treatmsg, &userctx);		
		}
	}
	if (ret) {
		fprintf(stderr, "error : %s\n", strerror(errno));
	}
	return 0;
}
