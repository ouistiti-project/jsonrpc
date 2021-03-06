#include <string.h>
#include <assert.h>
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

static int method_test_foreach(json_t *json_params, json_t **result, void *userdata)
{
	if (json_is_array(json_params)) {
#if JANSSON_VERSION_HEX >= 0x020500
		size_t index;
		json_t *value;
		int count = 0;
		json_array_foreach(json_params, index, value) {
			char *str = json_value_as_string(value);
			printf("%ld: %s\n", index, str);
			count++;
			free(str);
		}
		*result = json_pack("{s:i}", "count", count);
#else
		printf("JSON_ARRAY_FOREACH unsupported\n");
#endif
	} else if (json_is_object(json_params)) {
		const char *key;
		json_t *value;
		int count = 0;
		json_object_foreach(json_params, key, value) {
			char *str = json_value_as_string(value);
			printf("%s: %s\n", key, str);
			count++;
			free(str);
		}
		*result = json_pack("{s:i}", "count", count);
	} else {
		assert(0);
	}

	return 0;
}

static int method_test_iter(json_t *json_params, json_t **result, void *userdata)
{
	if (json_is_array(json_params)) {
		size_t len = json_array_size(json_params);
		size_t idx;
		for (idx = 0; idx < len; idx++) {
			json_t *value = json_array_get(json_params, idx);

			char *str = json_value_as_string(value);
			printf("%ld: %s\n", idx, str);
			free(str);
		}
		*result = json_pack("{s:i}", "count", len);
	} else if (json_is_object(json_params)) {
		void *iter = json_object_iter(json_params);
		int count = 0;
		while (iter)
		{
			const char *key = json_object_iter_key(iter);
			json_t *value = json_object_iter_value(iter);

			char *str = json_value_as_string(value);
			printf("%s: %s\n", key, str);
			count++;
			free(str);

			iter = json_object_iter_next(json_params, iter);
		}
		*result = json_pack("{s:i}", "count", count);
	} else {
		assert(0);
	}

	return 0;
}

static int method_test_apperror(json_t *json_params, json_t **result, void *userdata)
{
	/* on error, you can do one of the following: 
		return -1 and leave result unset
		return -1 and set result to a proper jsonrpc_error_object
	*/

	/* example of how to return an application-defined error */
	*result = jsonrpc_error_object(-12345, "application defined error",
		json_string("additional information"));

	return -1;
}

static int method_echo(json_t *json_params, json_t **result, void *userdata)
{
	json_incref(json_params);
	*result = json_params;
	return 0;
}

static int method_counter(json_t *json_params, json_t **result, void *userdata)
{
	struct user_context_t *userctx = (struct user_context_t *)userdata;
	userctx->count++;
	*result = json_pack("{s:i}", "counter", userctx->count);
	printf("count %d\n", userctx->count);
	return 0;
}

static int method_subtract(json_t *json_params, json_t **result, void *userdata)
{
	size_t flags = 0;
	json_error_t error;
	double x, y;
	int rc;
	
	if (json_is_array(json_params)) {
		rc = json_unpack_ex(json_params, &error, flags, "[FF!]", &x, &y);
	} else if (json_is_object(json_params)) {
		rc = json_unpack_ex(json_params, &error, flags, "{s:F,s:F}",
			"minuend", &x, "subtrahend", &y
		);
	} else {
		assert(0);
	}

	if (rc==-1) {
		*result = jsonrpc_error_object_predefined(JSONRPC_INVALID_PARAMS, json_string(error.text));
		return -1;
	}
	
	*result = json_pack("{s:f}", "subtract", (x - y));
	return 0;
}

static int method_sum(json_t *json_params, json_t **result, void *userdata)
{
	double total = 0;
	size_t len = json_array_size(json_params);
	int k;
	for (k=0; k < len; k++) {
		double value = json_number_value(json_array_get(json_params, k));
		total += value;
	}
	*result = json_pack("{s:f}", "sum", total);
	return 0;
}

struct jsonrpc_method_entry_t method_table[] = {
	{ 'r', "foreach", method_test_foreach, "o" },
	{ 'r', "iterate", method_test_iter, "o" },
	{ 'r', "apperror", method_test_apperror, "" },
	{ 'r', "echo", method_echo, "o" },
	{ 'n', "counter", method_counter, "o" },
	{ 'r', "counter", method_counter, "" },
	{ 'r', "subtract", method_subtract, "o" },
	{ 'r', "sum", method_sum, "[]" },
	{ 0, NULL },
};
