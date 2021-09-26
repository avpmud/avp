
#include "types.h"

#include "boards.h"

#include "objects.h"
#include "descriptors.h"
#include "characters.h"
#include "rooms.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "files.h"

#include <map>
#include "lexifile.h"

ACMD(do_respond);


Board::BoardMap Board::boards;


Lexi::String Board::GetFilename()
{
	BUFFER(filename, MAX_INPUT_LENGTH);
	
	sprintf(filename, BOARD_DIRECTORY "%s.brd", GetVirtualID().Print().c_str());
	while (1)
	{
		char *colon = strchr(filename, ':');
		if (colon)	*colon = '_';
		else		break;
	}

	return filename;
}


Lexi::String Board::GetOldFilename()
{
	VNum oldVNum = VNumLegacy::GetVirtualNumber(GetVirtualID());
	if (oldVNum != -1)
	{
		return Lexi::Format(BOARD_DIRECTORY "%d.brd", oldVNum);
	}
	
	return Lexi::String();
}


Board *Board::GetBoard(VirtualID vid)
{
	int error=0, t[4];
	FILE *fl;
	Board *board = NULL;
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	BoardMap::iterator iter = boards.find(vid);
	
	if (iter != boards.end())
		return iter->second;
	
	board = new Board(vid);
	
	ObjectPrototype *proto = obj_index.Find(vid);
	
	if (!proto || proto->m_pProto->GetType() != ITEM_BOARD)
	{
		if (!board->GetOldFilename().empty())	remove(board->GetOldFilename().c_str());
		remove(board->GetFilename().c_str());
		
		delete board;
		return NULL;
	}
	
	board->m_readLevel = proto->m_pProto->GetValue(OBJVAL_BOARD_READLEVEL);
	board->m_writeLevel = proto->m_pProto->GetValue(OBJVAL_BOARD_WRITELEVEL);
	board->m_removeLevel = proto->m_pProto->GetValue(OBJVAL_BOARD_REMOVELEVEL);	
	
	Lexi::String filename = board->GetFilename();
	
	VNum oldVNum = VNumLegacy::GetVirtualNumber(vid);
	if (oldVNum != -1)
	{
		Lexi::String oldFilename = board->GetOldFilename();
		if (Lexi::InputFile(oldFilename.c_str()).good())
		{
			rename(oldFilename.c_str(), filename.c_str());
		}
	}
	
	
	fl = fopen(filename.c_str(), "r");
	if(!fl)
	{
		/* this is for boards which exist as an object, but don't have a file yet */
		/* Ie, new boards */

		fl = fopen(filename.c_str(),"w");
		if(!fl)
		{
			log("Hm. Error while creating new board file '%s'", filename.c_str());
			delete board;
			return NULL;
		}
		else
		{
			log("Creating new board - board %s", vid.Print().c_str());
			fprintf(fl,"Board File\n%d %d %d %d\n", board->GetReadLevel(), board->GetWriteLevel(),
					board->GetRemoveLevel(), board->GetMessages().size());
		}
	}
	else
	{
		get_line(fl,buf);
		
		if(!(str_cmp("Board File", buf)))
		{
			get_line(fl, buf); 
			if (sscanf(buf,"%d", t) != 1)
				error=1;
				
			if (error)
			{
				log("Parse error in board %s - attempting to continue", board->GetVirtualID().Print().c_str());
				/* attempting board ressurection */

				if(t[0] < 0 || t[0] > LVL_CODER)		board->m_readLevel = LVL_IMMORT;
				if(t[1] < 0 || t[1] > LVL_CODER)		board->m_writeLevel = LVL_IMMORT;
				if(t[2] < 0 || t[2] > LVL_CODER)		board->m_removeLevel = LVL_ADMIN;
			}
			
			while(get_line(fl,buf))
			{
				/* this is the loop that reads the messages and also the message read info stuff */
				if (*buf == '#')
				{
					int id = atoi(buf + 1);
					board->ParseMessage(fl, filename.c_str(), id);
				}
			}
		}
	}

	fclose(fl);

	boards[proto->GetVirtualID()] = board;

	return board;
}


void Board::Renumber(VirtualID newVID)
{
	if (!GetOldFilename().empty())	remove(GetOldFilename().c_str());
	remove(GetFilename().c_str());
	
	boards.erase(m_Virtual);
	m_Virtual = newVID;
	boards[m_Virtual] = this;
	
	Save();
}


void Board::ParseMessage( FILE *fl, const char *filename, int id) {
	Message *msg;
	BUFFER(subject, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_STRING_LENGTH);

	msg = new Message(id);
	
	fscanf(fl, "%d\n", &msg->m_Poster);
	fscanf(fl, "%ld\n", &msg->m_Time);

	get_line(fl,subject);

	msg->m_Subject = subject;
	msg->m_Message = fread_lexistring(fl,buf, filename);

	m_Messages.push_back(msg);
}


int Board::Show(ObjData *obj, CharData *ch)
{
	Board *board = Board::GetBoard(obj->GetVirtualID());

	/* board locate */
	if (!board || ch->GetLevel() < board->GetReadLevel()) {
		send_to_char("*** Security code invalid. ***\n",ch);
		return 1;
	}
	/* send the standard board boilerplate */

	BUFFER(buf, MAX_STRING_LENGTH * 4);
	BUFFER(buf2, MAX_STRING_LENGTH);
	
	Board::MessageList &messageList = board->GetMessages();
	int		numMessages = messageList.size();
		

	sprintf(buf,"This is a bulletin board.\n"
			"Usage: READ/REMOVE <messg #>, WRITE <header>.\n");
	if (numMessages == 0)
	{
		strcat(buf, "The board is empty.\n");
		send_to_char(buf, ch);
		return 1;
	}
	else
	{
		sprintf_cat(buf, "There %s %d message%s on the board.\n",
				(numMessages == 1) ? "is" : "are",
				numMessages,
				(numMessages == 1) ? "" : "s");
	}
	
	
	FOREACH(Board::MessageList, messageList, iter)
	{
		Board::Message *message = *iter;
		
		if (strlen(buf) > ((MAX_STRING_LENGTH * 4) - 1024))
		{
			sprintf_cat(buf, "   *** OVERFLOW ***\n");
			break;
		}

		sprintf(buf2, "(%s)", get_name_by_id( message->m_Poster, "<UNKNOWN>" ));
		
		sprintf_cat(buf, "[%5d] : %6.10s %-14s :: %s\n",
				message->m_ID,
				Lexi::CreateDateString(message->m_Time, "%a %b %e").c_str(),
				buf2, !message->m_Subject.empty() ? message->m_Subject.c_str() : "No Subject");
	}
	page_string(ch->desc, buf);
	return 1;
}


int Board::DisplayMessage(ObjData *obj, CharData * ch, int id)
{
	Board *board = Board::GetBoard(obj->GetVirtualID());
 
	if (!board) {
		send_to_char("Error: Your board could not be found. Please report.\n",ch);
		log("Error in Board_display_msg - board %s", obj->GetVirtualID().Print().c_str());
		return 0;
	}
	
	if (ch->GetLevel() < board->GetReadLevel()) {
		send_to_char("*** Security code invalid. ***\n", ch);
		return 1;
	}
	
	Board::MessageList &messageList = board->GetMessages();
    
	if (messageList.empty()) {
		send_to_char("The board is empty!\n", ch);
		return 1;
	}
	
	/* now we locate the message.*/  
	if (id < 1) {
		send_to_char("You must specify the (positive) number of the message to be read!\n",ch);
		return 1;
	}
	
	FOREACH(Board::MessageList, messageList, iter)
	{
		Board::Message *message = *iter;

		if ( message->m_ID == id )
		{
			BUFFER(buf, MAX_STRING_LENGTH);
	
			sprintf(buf,"Message %d : %6.10s (%s) :: %s\n\n%s\n",
					message->m_ID,
					Lexi::CreateDateString(message->m_Time, "%a %b %e").c_str(),
					get_name_by_id(message->m_Poster, "<UNKNOWN>"),
					!message->m_Subject.empty() ? message->m_Subject.c_str() : "No Subject",
					!message->m_Message.empty() ? message->m_Message.c_str() : "Looks like this message is empty.");
			page_string(ch->desc, buf);
			
			return 1;
		}
	}
	
	send_to_char("That message exists only in your imagination.\n", ch);
	return 1;
}
 
 
int Board::RemoveMessage(ObjData *obj, CharData * ch, int id)
{
	Board *board = Board::GetBoard(obj->GetVirtualID());
	
	if (!board) {
		send_to_char("Error: Your board could not be found. Please report.\n",ch);
		log("Error in Board_remove_msg - board %s", obj->GetVirtualID().Print().c_str());
		return 0;
	}
	
	if (ch->GetLevel() < board->GetReadLevel()) {
		send_to_char("*** Security code invalid. ***\n", ch);
		return 1;
	}

	Board::MessageList &messageList = board->GetMessages();
    
	if (messageList.empty()) {
		send_to_char("The board is empty!\n", ch);
		return 1;
	}
	
	FOREACH(Board::MessageList, messageList, iter)
	{
		Board::Message *message = *iter;

		if ( message->m_ID == id )
		{
			if(ch->GetID() != message->m_Poster && ch->GetLevel() < board->GetRemoveLevel()) {
				send_to_char("*** Security code invalid. ***\n",ch);
				return 1;
			}

			/* perform check for mesg in creation */
		    FOREACH(DescriptorList, descriptor_list, desc)
		    {
		    	DescriptorData *d = *desc;
				if (d->GetState()->IsPlaying()
					&& d->m_StringEditor
					&& (d->m_StringEditor->getTarget() == &message->m_Message)) {
					send_to_char("At least wait until the author is finished before removing it!\n", ch);
					return 1;
				}
			}
			
			messageList.erase(iter);
			
			send_to_char("Message removed.\n", ch);
			BUFFER(buf, MAX_INPUT_LENGTH);
			sprintf(buf, "$n just removed message %d.", id);
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			
			board->Save();
			
			return 1; 
		}
	}

	send_to_char("That message exists only in your imagination.\n", ch);
	return 1;
}


void Board::SaveBoard(VirtualID vid) {
	Board *board = GetBoard(vid);
	
	if(!board)	log("Creating new board failed - board %s", vid.Print().c_str());
	else		board->Save();
}

void Board::Save()
{
	FILE *fl;
	BUFFER(buf, MAX_STRING_LENGTH);

	/* before we save to the file, lets make sure that our board is updated */
	sprintf(buf, BOARD_DIRECTORY "%s.brd", m_Virtual.Print().c_str());
	while (1)
	{
		char *colon = strchr(buf, ':');
		if (colon)	*colon = '_';
		else		break;
	}

	ObjectPrototype *proto = obj_index.Find(m_Virtual);
	if (!proto)
	{
		remove(buf);
		return;
	}
	m_readLevel = proto->m_pProto->GetValue(OBJVAL_BOARD_READLEVEL);
	m_writeLevel = proto->m_pProto->GetValue(OBJVAL_BOARD_WRITELEVEL);
	m_removeLevel = proto->m_pProto->GetValue(OBJVAL_BOARD_REMOVELEVEL);

	if (!(fl = fopen(buf, "w"))) {
		perror("Error writing board");
		return;
	}
	/* now we write out the basic stats of the board */

	fprintf(fl,"Board File\n%d\n", m_Messages.size());

	/* now write out the saved info.. */
	FOREACH(MessageList, m_Messages, iter)
	{
		Message *message = *iter;
		
		Lexi::BufferedFile::PrepareString(buf, !message->m_Message.empty() ? message->m_Message.c_str() : "Empty!");
		fprintf(fl,	"#%d\n"
					"%d\n"
					"%ld\n"
					"%s\n"
					"%s~\n",
				message->m_ID, message->m_Poster, message->m_Time,
				!message->m_Subject.empty() ? message->m_Subject.c_str() : "No Subject", buf);
	}
	fclose(fl);
}



class BoardWriteMessageConnectionState : public PlayingConnectionState
{
public:
	BoardWriteMessageConnectionState(Lexi::String &msg, VirtualID board)
	:	m_Message(msg)
	,	m_Board(board)
	{
	}
	
	virtual void		Enter()
	{
		CharData *ch = GetDesc()->m_Character;

		send_to_char(m_Message.c_str(), ch);
		
		ch->desc->StartStringEditor(m_Message, MAX_MESSAGE_LENGTH);

		SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
	}

	virtual void		Exit()
	{
		CharData *ch = GetDesc()->m_Character;
		
		if (ch)
		{
			REMOVE_BIT(PLR_FLAGS(ch), PLR_WRITING);
			act("$n finishes writing a message.", TRUE, ch, 0, 0, TO_ROOM);
		}
	}

	virtual void		OnEditorSave() {}
	virtual void		OnEditorAbort() { GetDesc()->Write("Post not aborted, use REMOVE <post #>.\n"); }
	virtual void		OnEditorFinished() { Board::SaveBoard(m_Board); Pop(); }

private:
	Lexi::String &		m_Message;
	VirtualID			m_Board;
};



void Board::WriteMessage(ObjData *obj, CharData *ch, char *arg)
{
	Board *board = Board::GetBoard(obj->GetVirtualID());

	if(IS_NPC(ch)) {
		send_to_char("Orwellian police thwart your attempt at free speech.\n",ch);
		return;
	}
	
	if (!board) {
		send_to_char("Error: Your board could not be found. Please report.\n",ch);
		log("Error in Board_display_msg - board #%s", obj->GetVirtualID().Print().c_str());
		return;
	}

	if (ch->GetLevel() < board->GetWriteLevel()) {
		send_to_char("*** Security code invalid. ***\n",ch);
		return;
	}

	if (!*arg)
		strcpy(arg, "No Subject");
	else
	{
		skip_spaces(arg);
		delete_doubledollar(arg);
		arg[81] = '\0';
	}

	Board::MessageList &messageList = board->GetMessages();
	Board::Message *message = new Board::Message(messageList.empty() ? 1 : messageList.front()->m_ID + 1);
	message->m_Poster = ch->GetID();
	message->m_Time = time(0);
	message->m_Subject = arg;
	
	messageList.push_front(message);

	act("You are writing on \"$p\".  (/s saves /h for help)\n\n", TRUE, ch, obj, 0, TO_CHAR);
	act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

//	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

//	ch->desc->StartStringEditor(message->m_Message, MAX_MESSAGE_LENGTH);
//	ch->desc->m_PostBoard = board->GetVirtualID();
	ch->desc->GetState()->Push(new BoardWriteMessageConnectionState(message->m_Message, board->GetVirtualID()));
}


void Board::RespondMessage(ObjData *obj, CharData *ch, int id)
{
	Board *board = Board::GetBoard(obj->GetVirtualID());

	if(IS_NPC(ch)) {
		send_to_char("Orwellian police thwart your attempt at free speech.\n",ch);
		return;
	}
	
	if (!board) {
		send_to_char("Error: Your board could not be found. Please report.\n",ch);
		log("Error in Board_display_msg - board #%s", obj->GetVirtualID().Print().c_str());
		return;
	}

	if (ch->GetLevel() < board->GetWriteLevel() || ch->GetLevel() < board->GetReadLevel()) {
		send_to_char("*** Security code invalid. ***\n",ch);
		return;
	}
	
	Board::MessageList &messageList = board->GetMessages();
    
	if (messageList.empty()) {
		send_to_char("The board is empty!\n", ch);
		return;
	}
	
	FOREACH(Board::MessageList, messageList, iter)
	{
		Board::Message *otherMessage = *iter;
		
		if ( otherMessage->m_ID == id )
		{
			Board::MessageList &messageList = board->GetMessages();
			Board::Message *message = new Board::Message(messageList.empty() ? 1 : messageList.front()->m_ID + 1);
			message->m_Poster = ch->GetID();
			message->m_Time = time(0);

			message->m_Subject = "Re: ";
			message->m_Subject += otherMessage->m_Subject;

			message->m_Message = "\t------- Quoted message -------\n";
			message->m_Message += otherMessage->m_Message;
			message->m_Message += "\t------- End Quote -------\n",
			
			messageList.push_front(message);

			act("You are writing on \"$p\".  (/s saves /h for help)\n\n", TRUE, ch, obj, 0, TO_CHAR);
			act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);
			
//			send_to_char(message->m_Message.c_str(), ch);

//			SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
			
//			ch->desc->StartStringEditor(message->m_Message, MAX_MESSAGE_LENGTH);
//			ch->desc->m_PostBoard = board->GetVirtualID();
			
			ch->desc->GetState()->Push(new BoardWriteMessageConnectionState(message->m_Message, board->GetVirtualID()));
	
			return;
		}
	}

	send_to_char("That message exists only in your imagination.\n", ch);
}


ACMD(do_respond) {
	int found=0,mnum=0;
	ObjData *obj;

	if(IS_NPC(ch)) {
		send_to_char("As a mob, you never bothered to learn to read or write.\n",ch);
		return;
	}
	
	
	if(!(obj = get_obj_in_list_type(ITEM_BOARD, ch->carrying)))
		obj = get_obj_in_list_type(ITEM_BOARD, ch->InRoom()->contents);

	if (obj) {
		BUFFER(arg, MAX_INPUT_LENGTH);
		argument = one_argument(argument, arg);
		if (ch->CanUse(obj) != NotRestricted)
			ch->Send("You don't have the access privileges for that board!\n");
		else if (!*arg)
			send_to_char("Respond to what?\n",ch);
		else if (isname(arg, obj->GetAlias()))
			send_to_char("You cannot reply to an entire board!\n",ch);
		else if (!isdigit(*arg) || (!(mnum = atoi(arg))))
			send_to_char("You must reply to a message number.\n",ch);
		else
			Board::RespondMessage(obj, ch, mnum);
	} else
		send_to_char("There is no board here!\n", ch);
}
