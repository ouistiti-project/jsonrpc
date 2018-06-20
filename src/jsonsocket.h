#ifndef __JSONSOCKET_H__
#define __JSONSOCKET_H__
#include "jsonrpc.h"

typedef struct jsonsocket_s jsonsocket_t;

jsonsocket_t *jsonsocket(int socket, struct jsonrpc_method_entry_t method_table[], void *userdata);
void _jsonsocket(jsonsocket_t *jsocket);
int jsonsocket_handler(jsonsocket_t *jsocket);
int jsonsocket_request(jsonsocket_t *jsocket, const char *method, json_t *params);

#endif
