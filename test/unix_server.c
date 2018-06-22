#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "jsonrpc.h"
#include "test.h"

struct thread_info_s
{
	int sock;
	struct user_context_t *userctx;
	struct thread_info_s *next;
};
struct thread_info_s firstinfo;

typedef int (*jsonrpc_server_t)(struct thread_info_s *info);

int jsonrpc_notification(struct thread_info_s *info)
{
	int run = 1;
	struct user_context_t *userctx = info->userctx;

	while (run) {
		sleep(1);
		char *notification;
		notification = jsonrpc_request("counter", strlen("counter"), method_table, (char *)userctx, NULL);
		if (notification) {
			int ret;
			ret = strlen(notification);
			if (ret > 0) {
				struct thread_info_s *it = info;
				while (it->next)
				{
					int sock = it->next->sock;
					ret = send(sock, notification, ret, MSG_DONTWAIT | MSG_NOSIGNAL);
					it = it->next;
				}
			}
		}
	}
	return 0;
}

int jsonrpc_command(struct thread_info_s *info)
{
	int ret = 0;
	int sock = info->sock;
	struct user_context_t *userctx = info->userctx;

	while (sock > 0)
	{
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);

		ret = select(sock + 1, &rfds, NULL, NULL, NULL);
		if (ret > 0 && FD_ISSET(sock, &rfds))
		{
			char buffer[1500];
			ret = recv(sock, buffer, 1500, MSG_NOSIGNAL);
			if (ret > 0)
			{
				char *out = jsonrpc_handler(buffer, ret, method_table, userctx);
				ret = strlen(out);
				ret = send(sock, out, ret, MSG_DONTWAIT | MSG_NOSIGNAL);
			}
		}
		if (ret == 0)
		{
			close(sock);
			struct thread_info_s *it = &firstinfo;
			while (it->next) {
				struct thread_info_s *old = it->next;
				if (old->sock == sock) {
					it->next = old->next;
					free(old);
				}
				it = it->next;
				if (it == NULL)
					break;
			}
			sock = -1;
		}
		if (ret < 0)
		{
			if (errno != EAGAIN)
			{
				close(sock);
				struct thread_info_s *it = &firstinfo;
				while (it->next) {
					struct thread_info_s *old = it->next;
					if (old->sock == sock) {
						it->next = old->next;
						free(old);
					}
					it = it->next;
					if (it == NULL)
						break;
				}
				sock = -1;
			}
		}
	}
	return ret;
}

typedef void *(*start_routine_t)(void*);
int start(jsonrpc_server_t service, struct thread_info_s *info)
{
	pthread_t thread;
	pthread_create(&thread, NULL, (start_routine_t)service, (void *)info);
	pthread_detach(thread);
}

int main(int argc, char *argv)
{
	const char *root = "/tmp";
	const char *name = "jsonrpc";
	int sock;
	int ret;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock > 0) {
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(struct sockaddr_un));
		addr.sun_family = AF_UNIX;
		snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s/%s", root, name);
		unlink(addr.sun_path);

		struct user_context_t userctx = {.count = 0, .sock = (void *)sock};
		firstinfo.sock = sock;
		firstinfo.userctx = &userctx;
		firstinfo.next = NULL;
		start(jsonrpc_notification, &firstinfo);

		ret = bind(sock, (struct sockaddr *) &addr, sizeof(addr));
		if (ret == 0) {
			ret = listen(sock, 10);
		}
		if (ret == 0) {
			int newsock = 0;
			do {
				newsock = accept(sock, NULL, NULL);
				if (newsock > 0) {
					struct thread_info_s *info = calloc(1, sizeof(*info));
					info->sock = newsock;
					info->userctx = &userctx;
					info->next = firstinfo.next;
					firstinfo.next = info;
					start(jsonrpc_command, info);
				}
			} while(newsock > 0);
		}
	}
	if (ret) {
		fprintf(stderr, "error : %s\n", strerror(errno));
	}
	return 0;
}
