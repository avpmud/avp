
//	Primary Includes
//#include "skills.h"

//	External Data
#include "db.h"

//	Utilities
#include "stl.llist.h"
#include "buffer.h"
#include "utils.h"


char *fread_action(FILE * fl, int nr);


class CombatMessages {
	class Message {
		public:
					Message(void) : attacker(NULL), victim(NULL), room(NULL) { };
		char *		attacker;
		char *		victim;
		char *		room;
	};
	class MessageType {
		public:
		Message		die;
		Message		miss;
		Message		hit;
		Message		staff;
	};
	class MessageList {
		public:
					MessageList(void) : attack(NULL) { };
		SInt32		attack;
		LList<MessageType *> Messages;
	};
	
	void			Load(void);
	MessageList *	Find(SInt32 attack);
	
	LList<MessageList *>	Messages;
};


class Combat {
public:
	static CombatMessages	Messages;
};



void CombatMessages::Load(void) {
	FILE *			fl;
	SInt32			type, i = 0;
	MessageType *	msg;
	MessageList *	list;
	char *			string;
	
	if (!(fl = fopen(MESS_FILE, "r"))) {
		log("Error reading combat message file " MESS_FILE ".");
		exit(1);
	}
	
	string = get_buffer(256);
	
	fgets(string, 128, fl);
	while (!feof(fl) && (*string == '\n' || *string == '*'))
		fgets(string, 128, fl);
	
	while (*string == 'M') {
		i++;							//	Action counter for error messages
		fgets(string, 128, fl);
		sscanf(string, " %d\n", &type);
		
		msg = new MessageType;
		
		msg->die.attacker	= fread_action(fl, i);
		msg->die.victim		= fread_action(fl, i);
		msg->die.room		= fread_action(fl, i);
		
		msg->miss.attacker	= fread_action(fl, i);
		msg->miss.victim	= fread_action(fl, i);
		msg->miss.room		= fread_action(fl, i);
		
		msg->hit.attacker	= fread_action(fl, i);
		msg->hit.victim		= fread_action(fl, i);
		msg->hit.room		= fread_action(fl, i);
		
		msg->staff.attacker	= fread_action(fl, i);
		msg->staff.victim	= fread_action(fl, i);
		msg->staff.room		= fread_action(fl, i);
		
		if (!(list = this->Find(type))) {
			list = new MessageList;
			list->attack = type;
			this->Messages.Add(list);
		}
		list->Messages.Add(msg);
		
		fgets(string, 128, fl);
		while (!feof(fl) && (*string == '\n' || *string == '*'))
			fgets(string, 128, fl);
	}
	
	release_buffer(string);
	fclose(fl);
}


CombatMessages::MessageList *CombatMessages::Find(SInt32 attack) {
	MessageList *		list;
	LListIterator<MessageList *>	iter(this->Messages);
	
	while ((list = iter.Next()))
		if (list->attack == attack)
			return list;
	
	return NULL;
}
