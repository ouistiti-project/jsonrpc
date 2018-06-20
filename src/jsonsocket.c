#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>

#include <jansson.h>
#include "jsonrpc.h"
#include "jsonsocket.h"
#include "jsonrpc_internal.h"

typedef pthread_cond_t json_cond_t;
typedef pthread_mutex_t json_mutex_t;

typedef void *(*json_malloc_t)(size_t);
typedef void (*json_free_t)(void *);
typedef int (*json_cond_destroy)(json_cond_t *);
typedef int (*json_cond_init)(json_cond_t * ,
              const pthread_condattr_t * );
typedef int (*json_cond_wait_t)(json_cond_t * ,
              json_mutex_t * );
typedef int (*json_cond_signal_t)(json_cond_t *);
typedef int (*json_mutex_destroy)(json_mutex_t *);
typedef int (*json_mutex_init)(json_mutex_t * ,
              const pthread_mutexattr_t * );
typedef int (*json_mutex_lock_t)(json_mutex_t *);
typedef int (*json_mutex_unlock_t)(json_mutex_t *);
typedef void * (*routine_t)(void *);
/* C89 allows these to be macros */
#undef malloc
#undef free

/* memory function pointers */
static json_malloc_t do_malloc = malloc;
static json_free_t do_free = free;

static json_cond_wait_t do_wait = pthread_cond_wait;
static json_cond_signal_t do_signal = pthread_cond_signal;
static json_mutex_lock_t do_lock = pthread_mutex_lock;
static json_mutex_unlock_t do_unlock = pthread_mutex_unlock;

struct jsonsocket_s
{
	int socket;
	json_t *requests;
	json_cond_t cond;
	json_mutex_t lock;
	pthread_t thread;
	void *userdata;
	struct jsonrpc_method_entry_t *method_table;
};

int jsonsocket_loop(jsonsocket_t *jsocket);

jsonsocket_t *jsonsocket(int socket, struct jsonrpc_method_entry_t method_table[], void *userdata)
{
	jsonsocket_t *jsocket;

	jsocket = do_malloc(sizeof(*jsocket));
	jsocket->socket = socket;
	jsocket->requests = json_array();
	jsocket->userdata = userdata;
	jsocket->method_table = method_table;
	pthread_mutex_init(&jsocket->lock, NULL);
	pthread_cond_init(&jsocket->cond, NULL);
	pthread_create(&jsocket->thread, NULL, (routine_t)jsonsocket_loop, jsocket);
	return jsocket;
}

void _jsonsocket(jsonsocket_t *jsocket)
{
	int status;

	json_decref(jsocket->requests);
	jsocket->requests = NULL;
	do_signal(&jsocket->cond);

	pthread_join(jsocket->thread, (void **)&status);
	pthread_cond_destroy(&jsocket->cond);
	pthread_mutex_destroy(&jsocket->lock);
	do_free(jsocket);
}

static void jsonsocket_push(jsonsocket_t *jsocket, json_t *json)
{
	do_lock(&jsocket->lock);
	json_array_append(jsocket->requests, json);
	do_signal(&jsocket->cond);
	do_unlock(&jsocket->lock);
}

static json_t *jsonsocket_pop(jsonsocket_t *jsocket)
{
	json_t *json;
	do_lock(&jsocket->lock);
	json = json_array_get(jsocket->requests, 0);
	json_incref(json);
	json_array_remove(jsocket->requests, 0);
	do_unlock(&jsocket->lock);
	return json;
}

int jsonsocket_handler(jsonsocket_t *jsocket)
{
	json_t *request, *response;
	json_error_t error;
	int ret = 0;

	request = json_loadfd(jsocket->socket, JSON_DISABLE_EOF_CHECK, &error);

	if (!request) {
		fprintf(stdout, "Syntax error: line %d col %d: %s\n", error.line, error.column, error.text);
		ret = -1;
	}
	else
	{
		response = jsonrpc_response(request, jsocket->method_table, jsocket->userdata);
		if (response) {
			jsonsocket_push(jsocket, response);
		}
		json_decref(request);
		json_decref(response);
	}

	return ret;
}

int jsonsocket_request(jsonsocket_t *jsocket, const char *method, json_t *params)
{
	int ret = -1;
	json_t *request;
	unsigned long id;
	struct jsonrpc_method_entry_t *entry;

	request = jsonrpc_request(method, params, jsocket->method_table, &id, &entry);
	if (request) {
		jsonsocket_push(jsocket, request);
		if (id > 0) {
			struct jsonrpc_method_entry_t *new = calloc(1, sizeof(*new));
			if (new) {
				memcpy(new, entry, sizeof(*new));
				new->id = id;
				entry->next = new;
			}
		}
		ret = 0;
	}
	return ret;
}

int jsonsocket_loop(jsonsocket_t *jsocket)
{
	int ret = -1;
	json_t *json_message;

	while (jsocket->requests != NULL)
	{
		do_lock(&jsocket->lock);
		do {
			do_wait(&jsocket->cond, &jsocket->lock);
		} while (jsocket->requests && json_array_size(jsocket->requests) == 0);
		do_unlock(&jsocket->lock);

		while (jsocket->requests && json_array_size(jsocket->requests) > 0) {
			ret++;
			json_message = jsonsocket_pop(jsocket);

			if (json_message) {
				json_dumpfd(json_message, jsocket->socket, 0);
				json_decref(json_message);
			}
		}
	}
	return ret;
}
