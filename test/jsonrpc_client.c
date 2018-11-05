#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "jsonrpc.h"
#include "test.h"

static char *json_value_as_string(json_t *value)
{
	// caller to free the returned string

	char buffer[64];

	switch (json_typeof(value)) {
		case JSON_OBJECT:
		case JSON_ARRAY:
			return json_dumps(value, JSON_COMPACT);
		case JSON_STRING:
			return strdup(json_string_value(value));
		case JSON_INTEGER:
			snprintf(buffer, sizeof(buffer), "%" JSON_INTEGER_FORMAT, json_integer_value(value));
			return strdup(buffer);
		case JSON_REAL:
			snprintf(buffer, sizeof(buffer), "%f", json_real_value(value));
			return strdup(buffer);
		case JSON_TRUE:
			return strdup("True");
		case JSON_FALSE:
			return strdup("False");
		case JSON_NULL:
			return strdup("None");
	}
	assert(0);
}

static int method_response(json_t *json_params, json_t **result, void *userdata)
{
	*result = NULL;
	if (json_is_array(json_params)) {
		size_t len = json_array_size(json_params);
		size_t idx;
		printf("result:\n");
		for (idx = 0; idx < len; idx++) {
			json_t *value = json_array_get(json_params, idx);

			char *str = json_value_as_string(value);
			printf("%ld: %s\n", idx, str);
			free(str);
		}
	} else if (json_is_object(json_params)) {
		void *iter = json_object_iter(json_params);
		printf("result:\n");
		while (iter)
		{
			const char *key = json_object_iter_key(iter);
			json_t *value = json_object_iter_value(iter);

			char *str = json_value_as_string(value);
			printf("%s: %s\n", key, str);
			free(str);

			iter = json_object_iter_next(json_params, iter);
		}
	} else if (json_is_null(json_params)) {
		printf("result: null\n");
	}

	return 0;
}

static int method_test_foreach(json_t *json_params, json_t **params, void *userdata)
{
	*params = json_pack("[{s:s},{s:s}]",
			"hello", "world",
			"bonjour", "le monde");
	char * tempo = json_dumps(*params, 0);
	printf("foreach %s\n", tempo);
	free(tempo);
}

static int method_test_iter(json_t *json_params, json_t **params, void *userdata)
{
	*params = json_pack("{s:s,s:s}",
			"string1", "hello",
			"string2", "world");
	char * tempo = json_dumps(*params, 0);
	printf("iter of %s\n", tempo);
	free(tempo);
	return 0;
}

static int method_test_apperror(json_t *json_params, json_t **params, void *userdata)
{
	return -1;
}

static int method_echo(json_t *json_params, json_t **params, void *userdata)
{
	*params = json_pack("{s:s}",
			"hello", "world");
	char * tempo = json_dumps(*params, 0);
	printf("echo of %s\n", tempo);
	free(tempo);
	return 0;
}

/*
static int method_counter(json_t *json_params, json_t **params, void *userdata)
{
	*params = json_null();
	return 0;
}
*/
static int method_subtract(json_t *json_params, json_t **params, void *userdata)
{
	*params = json_pack("[i,i]",
			5,3);
	char * tempo = json_dumps(*params, 0);
	printf("subtract of %s\n", tempo);
	free(tempo);
	return 0;
}

static int method_sum(json_t *json_params, json_t **params, void *userdata)
{
	*params = json_pack("[i,i,i,i,i,i,i,i,i,i]",
			1,2,3,4,5,6,7, 8,9, 10);
	char * tempo = json_dumps(*params, 0);
	printf("sum of %s\n", tempo);
	free(tempo);
	return 0;
}

struct jsonrpc_method_entry_t method_table[] = {
	{ 'a', "foreach",method_response, "o" },
	{ 'a', "iterate", method_response, "o" },
	{ 'a', "apperror", method_response, "" },
	{ 'a', "echo", method_response, "o" },
	{ 'a', "counter", method_response, "o" },
	{ 'a', "subtract", method_response, "o" },
	{ 'a', "sum", method_response, "o" },
	{ 'r', "foreach",method_test_foreach, "[]" },
	{ 'r', "iterate", method_test_iter, "o" },
	{ 'r', "apperror", method_test_apperror, "" },
	{ 'r', "echo", method_echo, "o" },
	{ 'r', "counter", NULL, NULL },
	{ 'r', "subtract", method_subtract, "[]" },
	{ 'r', "sum", method_sum, "[]" },
	{ 0, NULL},
};
