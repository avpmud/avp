#ifndef __NETWORK_PROTOCOL_PROTOCOL_H__
#define __NETWORK_PROTOCOL_PROTOCOL_H__


#include <stdlib.h>


class Protocol
{
public:
	virtual ~Protocol() {}
	virtual size_t		ProcessBuffer(char *in, char *out, size_t size, size_t maxsize) = 0;
};


class ProtocolClient
{
public:
	virtual ~ProtocolClient() {}
};

#endif // __NETWORK_PROTOCOL_PROTOCOL_H__
