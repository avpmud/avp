#include <typeinfo>
#include "descriptors.h"

struct IdentPrefs {
};

class IdentServer {
public:
						IdentServer(void);
						~IdentServer(void);
	
	//	Functions
	void				Lookup(DescriptorData *newd);
	char *				LookupHost(struct sockaddr_in sa);
	char *				LookupUser(struct sockaddr_in sa);
	void				Receive(void);
	void				Loop(void);
	
	//	Sub-Accessors
	bool				IsActive(void) { return (this->descriptor > 0); }

	int				descriptor;
	int				child;
	pid_t				pid, parent;
	int				port;

	typedef struct {
		enum { Error = -1, Nop, Ident, Quit, IdRep }
							type;
		struct sockaddr_in	address;
		int				fd;
		char				host[256];
		char				user[256];
	} Message;
protected:
	struct IdentPrefs {
		bool				host;
		bool				user;
	} lookup;
};
