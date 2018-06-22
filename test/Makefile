bin-y+=jsonrpc_server
jsonrpc_server_SOURCES+=jsonrpc_server.c
jsonrpc_server_SOURCES+=$(TESTSERVICE)_server.c
jsonrpc_server_LIBRARY+=pthread jansson jsonrpc
ifeq ($(TESTSERVICE),zmq)
jsonrpc_server_LIBRARY+=zmq
endif
jsonrpc_server_LDFLAGS+=-L../src
jsonrpc_server_CFLAGS+=-I../src
jsonrpc_server_CFLAGS+=-g -DDEBUG

bin-y+=jsonrpc_client
jsonrpc_client_SOURCES+=jsonrpc_client.c
jsonrpc_client_SOURCES+=$(TESTSERVICE)_client.c
jsonrpc_client_LIBRARY+=pthread jansson jsonrpc
ifeq ($(TESTSERVICE),zmq)
jsonrpc_client_LIBRARY+=zmq
endif
jsonrpc_client_LDFLAGS+=-L../src
jsonrpc_client_CFLAGS+=-I../src
jsonrpc_client_CFLAGS+=-g -DDEBUG
