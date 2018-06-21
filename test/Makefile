bin-y+=jsonrpc_server
jsonrpc_server_LIBRARY+=pthread jansson jsonrpc zmq
jsonrpc_server_LDFLAGS+=-L../src
jsonrpc_server_CFLAGS+=-I../src
jsonrpc_server_CFLAGS+=-g -DDEBUG

bin-y+=jsonrpc_client
jsonrpc_client_LIBRARY+=pthread jansson jsonrpc zmq
jsonrpc_client_LDFLAGS+=-L../src
jsonrpc_client_CFLAGS+=-I../src
jsonrpc_client_CFLAGS+=-g -DDEBUG
