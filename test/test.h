#ifndef __TEST_H__
#define __TEST_H__

struct user_context_t
{
	int count;
	void *sock;
};

extern struct jsonrpc_method_entry_t method_table[];

#endif
