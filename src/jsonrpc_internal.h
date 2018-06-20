#ifndef __JSONRPC_INTERNAL_H__
#define __JSONRPC_INTERNAL_H__

#include <jansson.h>

json_t *jsonrpc_error_object(int code, const char *message, json_t *data);
json_t *jsonrpc_error_object_predefined(int code, json_t *data);
json_t *jsonrpc_ignore_error_response(json_t *json_id, json_t *json_error);
json_t *jsonrpc_request_error_response(json_t *json_id, json_t *json_error);

json_t *jsonrpc_response(json_t *json_request,
	struct jsonrpc_method_entry_t method_table[],
	void *userdata);

#endif
