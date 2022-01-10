/**************************************************************************
*  File: scripts.c                                                        *
*  Usage: contains general functions for using scripts.                   *
**************************************************************************/

#include <memory>
 
#include "structs.h"
#include "scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "handler.h"
#include "event.h"
#include "olc.h"
#include "clans.h"
#include "buffer.h"
#include "db.h"
#include "spells.h"
#include "scriptcompiler.h"
#include "constants.h"
#include "affects.h"
#include "track.h"

#include <pcrecpp.h>

#include <map>
#include <math.h>

//	External Functions
char *scriptedit_get_class(TrigData *trig);
RoomData * find_target_room(CharData * ch, char *rawroomstr);

//	External Variables
/* function protos from this file */

void find_uid_name(const char *uid, char *name);
void script_stat (CharData *ch, Scriptable *sc);
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_compile);


bool handle_literals(Scriptable *sc,
	TrigThread *thread,
	const char *var, Hash varhash,
	const char *field, Hash fieldhash,
	const char *subfield,
	char *result);
bool text_processed(Scriptable *sc,
	TrigThread *thread,
	const char *var, Hash varhash,
	const char *field, Hash fieldhash,
	const char *subfield,
	char *result,
	bool literal);
template<class T>
bool handle_member(TrigThread *thread, T *e, const char *field, Hash fieldhash, const char *subfield, char *result);

int find_eq_pos(CharData * ch, ObjData * obj, char *arg);

void find_obj_in_equipment(TrigThread *thread, CharData *ch, const char *str, char *output, bool deep = false);
void find_obj_in_contents(TrigThread *thread, ObjectList &contents, const char *str, char *output, bool deep = false);

void find_char_people(TrigThread *thread, RoomData *room, std::list<CharData *> &lst, const char *str);

CharData * scan_target(CharData *ch, char *arg, int range, int &dir, int *distance_result, bool isGun);
CharData * scan_target(CharData *ch, CharData *victim, int range, int &dir, int *distance_result, bool isGun);

extern struct TimeInfoData time_info;

const char * ScriptEngineParseVariable(const char *var, char *out, int &context);


//	Local functions
const char *ScriptEngineParamsSkipSpaces(const char *src);
const char *ScriptEngineParamsCopyNextParam(const char *src, char *&dst);
const char *ScriptEngineParamsSkipParam(const char *src);
const char *ScriptEngineParamsSkipToParam(const char *src, int num);
int ScriptEngineParamsCount(const char *src);

const char *ScriptEngineRecordSkipRecord(const char *p);
const char *ScriptEngineRecordCopyRecord(const char *p, char *&dst);
const char *ScriptEngineRecordSkipToField(const char *src, const char *field, bool &found);
const char *ScriptEngineRecordSkipField(const char *src);
const char *ScriptEngineRecordCopyField(const char *src, char *dst);
const char *ScriptEngineRecordCopyToField(const char *src, const char *field, char *&dst, bool &found);
const char *ScriptEngineRecordSkipListWord(const char *src);
const char *ScriptEngineRecordCopyUntilListWord(const char *src, char *&dst, int num);
const char *ScriptEngineSkipSpaces(const char *src);
const char *ScriptEngineListSkipWord(const char *src);
const char *ScriptEngineListSkipToWord(const char *src, int num);
const char *ScriptEngineListCopyNextWord(const char *src, char *&dst);
const char *ScriptEngineListCopyWord(const char *src, char *&dst, int num);
const char *ScriptEngineListCopyUntilWord(const char *src, char *&dst, int num);
void		ScriptEngineListCopyRemaining(const char *src, char *&dst);
char *		ScriptEngineListAppend(char *dst, const char *src);
char *		ScriptEngineListAppendSpace(char *dst);
void		ScriptEngineListExtractWord(const char *src, char *&dst, int num);
const char *ScriptEngineListCompareWords(const char *src, const char *match, bool &bIsMatch);
int			ScriptEngineListFindWord(const char *src, const char *match);
int			ScriptEngineListCount(const char *src);
int			ScriptEngineListCountMatching(const char *src, const char *match);
void		ScriptEngineListInsert(const char *src, char *&dst, const char *insert, int num);

int			ScriptEngineListCountParameters(const char *src);
const char *ScriptEngineListCompareParameters(const char *src, const char *match, bool &bIsMatch);
int			ScriptEngineListFindParameter(const char *src, const char *match);



class ScriptEngineBuiltin
{
public:
	ScriptEngineBuiltin(const char *name) : m_Name(name), m_Hash(GetStringFastHash(name)) {}
	virtual			~ScriptEngineBuiltin() {}

	const char *		m_Name;
	Hash				m_Hash;
};



class ScriptEngineBuiltinLiteral : public ScriptEngineBuiltin
{
public:
	enum Result
	{
		LiteralNotHandled,
		LiteralHandled,
		LiteralHandledWithField
	};
	typedef Result (*Func)(TrigThread *thread, const char *field, Hash fieldhash, const char *subfield, char *result);
	ScriptEngineBuiltinLiteral(const char *name, Func f) : ScriptEngineBuiltin(name), m_Func(f) { GetBuiltins()[m_Hash] = this; }
	
//	virtual void		get(TrigThread *thread, const char *field, Hash fieldhash, const char *subfield, char *result) = 0;
	Func				m_Func;
private:
	
	typedef std::map<Hash, ScriptEngineBuiltinLiteral *> Map;
	static Map &		GetBuiltins() { static Map ms_Builtins; return ms_Builtins; }

public:
	static ScriptEngineBuiltinLiteral *	Find(Hash varhash) 
	{
		const Map &builtins = GetBuiltins();
		Map::const_iterator iter = builtins.find(varhash);
		if (iter != builtins.end())
			return iter->second;
		
		return NULL;
	}
};


class ScriptEngineBuiltinTextFunction : public ScriptEngineBuiltin
{
public:

	typedef void (*Func)(TrigThread *thread, const char *var, const char *subfield, char *result);
	ScriptEngineBuiltinTextFunction(const char *name, Func f) : ScriptEngineBuiltin(name), m_Func(f) { GetBuiltins()[m_Hash] = this; }
	
//	virtual void		get(TrigThread *thread, const char *var, const char *subfield, char *result) = 0;
	Func				m_Func;
private:
	typedef std::map<Hash, ScriptEngineBuiltinTextFunction *> Map;
	static Map &		GetBuiltins() { static Map ms_Builtins; return ms_Builtins; }

public:
	static ScriptEngineBuiltinTextFunction *	Find(Hash fieldhash) 
	{
		const Map &builtins = GetBuiltins();
		Map::const_iterator iter = builtins.find(fieldhash);
		if (iter != builtins.end())
			return iter->second;
		
		return NULL;
	}
};


template<class T>
class ScriptEngineBuiltinMemberValue : public ScriptEngineBuiltin
{
public:

	typedef void (*Func)(TrigThread *thread, T *entity, /*const char *field, Hash fieldhash,*/ const char *subfield, char *result);
	ScriptEngineBuiltinMemberValue(const char *name, Func f) : ScriptEngineBuiltin(name), m_Func(f) { GetBuiltins()[m_Hash] = this; }
	
//	virtual void		get(TrigThread *thread, T *entity, const char *field, Hash fieldhash, const char *subfield, char *result) = 0;
	Func				m_Func;

private:
	typedef std::map<Hash, ScriptEngineBuiltinMemberValue *> Map;
	static Map &		GetBuiltins() { static Map ms_Builtins; return ms_Builtins; }
	
public:
	static ScriptEngineBuiltinMemberValue *Find(Hash fieldhash) 
	{
		const Map &builtins = GetBuiltins();
		typename Map::const_iterator iter = builtins.find(fieldhash);
		if (iter != builtins.end())
			return iter->second;
		
		return NULL;
	}
};




//	Extended Data ID Strings
//	Format:
//	EDATA_START_CHAR type:data EDATA_END_CHAR

const char EDATA_START_CHAR	= 0xAB;	//	high ascii looks-like '<<'
const char EDATA_END_CHAR	= 0xBB;	//	high ascii looks-like '>>'
const char EDATA_PARAM_SEP	= 0xB8;	//	high ascii looks-like '|'

namespace ExtendedData
{
	class Handler
	{
	public:
		class Params
		{
		public:
			Params(const char *p) : m_Params(p), m_Cursor(p) {}
			
			Params &			operator>>(IDNum & n) { n = strtoul(m_Cursor, NULL, 0); SkipToNext(); return *this; }
			Params &			operator>>(int & n) { n = *m_Cursor ? atoi(m_Cursor) : 0; SkipToNext(); return *this; }
			Params &			operator>>(VirtualID & n) { n = *m_Cursor ? VirtualID(m_Cursor) : VirtualID(); SkipToNext(); return *this; }
			
			Params &			operator>>(CharData *& ch) { IDNum id; *this >> id; ch = CharData::Find(id); return *this; }
			Params &			operator>>(ObjData *& obj) { IDNum id; *this >> id; obj = ObjData::Find(id); return *this; }
			Params &			operator>>(RoomData *& room) { IDNum id; *this >> id; room = RoomData::Find(id); return *this; }

		private:
			void				SkipToNext()
			{
				char c = *m_Cursor;
				while (c && c != EDATA_PARAM_SEP) c = *++m_Cursor;
				if (c) ++m_Cursor;
			}
			
			const char * const	m_Params;
			const char *		m_Cursor;
		};
		
		
		class ParamWriter
		{
		public:
			ParamWriter() : m_Params(NULL), m_bParamWritten(false), m_ParamBuffer(MAX_STRING_LENGTH, "m_Params", __PRETTY_FUNCTION__, __LINE__)
			{
				m_Params = m_ParamBuffer.Get();
				m_Cursor = m_Params;
			}
			
			ParamWriter &		operator<<(IDNum n) { WriteSeparator(); m_Cursor += sprintf(m_Cursor, "%u", n); return *this; }
			ParamWriter &		operator<<(Entity *e) { WriteSeparator(); m_Cursor += sprintf(m_Cursor, "%u", e->GetID()); return *this; }
			ParamWriter &		operator<<(int n) { WriteSeparator(); m_Cursor += sprintf(m_Cursor, "%d", n); return *this; }
			ParamWriter &		operator<<(VirtualID vid) { WriteSeparator(); m_Cursor += sprintf(m_Cursor, "%s", vid.Print().c_str()); return *this; }
		
			const char *		GetString() const { return m_Params; }
		
		private:
			void WriteSeparator()
			{
				if (m_bParamWritten)	*m_Cursor++ = EDATA_PARAM_SEP;	//	The sprintf will write the NUL
				else					m_bParamWritten = true;
			}
			
			char *				m_Params;
			char *				m_Cursor;
			bool				m_bParamWritten;
			BufferWrapper		m_ParamBuffer;
		};
				
		typedef bool		(*Handle)(TrigThread *thread, const char *params, const char *field, Hash fieldhash, const char *subfield, char *result);

		static void			MakeDataString(char *result, const char *name, const ParamWriter &params)
		{
			sprintf(result, "%c%s:%s%c", EDATA_START_CHAR, name, params.GetString(), EDATA_END_CHAR);
		}
	};
	
	
	template<class T, const char *name>
	class StandardHandler : public Handler
	{
	public:
		//virtual const char *GetName() { return name; }
		static const char *GetName() { return name; }
		
		virtual bool		Parse(Params &params) = 0;
		
		//	Takes a full ExtendedData string
		bool				Construct(const char *buf);
	
		static bool			Handle(TrigThread *thread, const char *params, const char *field, Hash fieldhash, const char *subfield, char *result)
		{
			ScriptEngineBuiltinMemberValue<T> *match = ScriptEngineBuiltinMemberValue<T>::Find(fieldhash);
			
			if (!match)
				return false;
			
			if (str_cmp(match->m_Name, field))
			{
				ReportHashCollision(fieldhash, match->m_Name, field);
				return false;
			}
			
			T handler;
			Params p(params);
			if (handler.Parse(p))
				match->m_Func(thread, &handler, /*field, fieldhash,*/ subfield ? subfield : "", result);
//			else
//				ScriptEngineReportError(thread, false, "unparseable %s field: '%s'", GetName(), field);
			return true;
		}
		
		static void			MakeDataStringInternal(char *result, const ParamWriter &params)
		{
			Handler::MakeDataString(result, GetName(), params);
		}
	
	protected:
		virtual ~StandardHandler() {}
	};
	
	
	enum HandleResult
	{
		HandleNoError,
		HandleHandlerNotFound,
		HandleUnknownField
	};
	
	ExtendedData::HandleResult	Handle(TrigThread *thread, const char *str, const char *field, Hash fieldhash, const char *subfield, char *result);
	
	void				RegisterHandler(const char *name, Handler::Handle handler);
	Handler::Handle		GetHandler(const char *dataclass);
	Handler::Handle		GetHandler(const char *str, char *data);
	
	Lexi::String		GetHandlerName(const char *str);

	typedef std::map<Hash, Handler::Handle> HandlerMap;
	HandlerMap			handlers;
	
	
	
	template<class T>
	class HandlerRegistrar
	{
	public:
		HandlerRegistrar()
		{
			RegisterHandler(T::GetName(), &T::Handle);
		}
	};
}


inline bool			IsExtendedDataString(const char *str) { return *str == EDATA_START_CHAR; }

template<class T>
inline bool			IsExtendedDataClass(const char *str, char *params)
{
	if (!IsExtendedDataString(str))
		return false;
	
	ExtendedData::Handler::Handle handler = ExtendedData::GetHandler(str, params);
	return (handler == &T::Handle);
}


template<class T, const char *name>
bool ExtendedData::StandardHandler<T,name>::Construct(const char *str)
{
	BUFFER(params, MAX_SCRIPT_STRING_LENGTH);
	
	if (!IsExtendedDataClass<T>(str, params))
		return false;
	
	Handler::Params p(params);
	Parse(p);
	
	return true;
}




ExtendedData::HandleResult ExtendedData::Handle(TrigThread *thread, const char *str, const char *field, Hash fieldhash, const char *subfield, char *result)
{
	BUFFER(data, MAX_SCRIPT_STRING_LENGTH);
	
	Handler::Handle handler = GetHandler(str, data);
	
	if (!handler)	return HandleHandlerNotFound;
	
	if (!handler(thread, data, field, fieldhash, subfield, result))
		return HandleUnknownField;
	
	return HandleNoError;
}


ExtendedData::Handler::Handle ExtendedData::GetHandler(const char *str, char *data)
{
	BUFFER(dataclass, MAX_SCRIPT_STRING_LENGTH);
	
	if (!IsExtendedDataString(str))
		return NULL;
	
	++str;

	char *p = dataclass;
	char c = *str++;
	while (c && c != ':' && c != EDATA_END_CHAR)
	{
		*p++ = c;
		c = *str++;
	}
	*p = '\0';
	
	if (c != ':' || !*dataclass)
		return NULL;

	p = data;
	c = *str++;
	int depth = 1;
	while (c)
	{
		if (c == EDATA_START_CHAR)
			++depth;
		else if (c == EDATA_END_CHAR && --depth == 0)
			break;
	
		*p++ = c;
		
		c = *str++;
	}
	*p = '\0';
	
	if (c != EDATA_END_CHAR || !*data)
		return NULL;
	
	Handler::Handle handler = GetHandler(dataclass);
	
	return handler;
}


Lexi::String ExtendedData::GetHandlerName(const char *str)
{
	BUFFER(dataclass, MAX_SCRIPT_STRING_LENGTH);
	
	if (!IsExtendedDataString(str))
		return NULL;
	
	char *p = dataclass;
	const char *s = str;
	
	++s;
	
	char c = *s++;
	while (c && c != ':' && c != EDATA_END_CHAR)
	{
		*p++ = c;
		c = *s++;
	}
	*p = '\0';
	
	if (c != ':' || !*dataclass)
		return str;

	return dataclass;
}


void ExtendedData::RegisterHandler(const char *name, Handler::Handle handler)
{
	Hash hash = GetStringFastHash(name);
	assert(handlers.find(hash) == handlers.end());
	handlers[hash] = handler;
}


ExtendedData::Handler::Handle ExtendedData::GetHandler(const char *dataclass)
{
	HandlerMap::iterator iter = handlers.find(GetStringFastHash(dataclass));
	if (iter == handlers.end())	return NULL;
	return iter->second;
}




bool handle_literals(Scriptable *sc,
	TrigThread *thread,
	const char *var, Hash varhash,
	const char *field, Hash fieldhash,
	const char *subfield,
	char *str)
{
	ScriptEngineBuiltinLiteral *match = ScriptEngineBuiltinLiteral::Find(varhash);
	
	if (!match)
		return false;

	if (str_cmp(match->m_Name, var) != 0)
	{
		ReportHashCollision(varhash, match->m_Name, var);
		return false;
	}
	
	ScriptEngineBuiltinLiteral::Result handled = match->m_Func(thread, field, fieldhash, !fieldhash && subfield ? subfield : "", str);
	if (handled != ScriptEngineBuiltinLiteral::LiteralHandledWithField && fieldhash)
	{
		BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
		
		DereferenceData data;
		
		data.var = buf;
		data.field = field;
		data.subfield = subfield;
		data.result = str;
		
		strcpy(buf, str);
		
		ScriptEngineDereference(sc, thread, data, true);
		
		if (data.resultType != DereferenceData::ResultString)
		{
			strcpy(str, data.GetResult());
		}
	}
	
	return true;
}



bool text_processed(Scriptable *sc, TrigThread *thread, const char *var, Hash varhash, const char *field, Hash fieldhash, const char *subfield, char *str, bool literal)
{
	if (!literal && is_number(field))
 	{
		int	num = atoi(field);
	
		if (num > 0)
			ScriptEngineListCopyWord(var, str, num);
		
		return true;
	}
	
	ScriptEngineBuiltinTextFunction *match = ScriptEngineBuiltinTextFunction::Find(fieldhash);
	
	if (!match)
		return false;
	
	if (str_cmp(match->m_Name, field) != 0)
	{
		ReportHashCollision(fieldhash, match->m_Name, field);
		return false;
	}
		
	
	if (literal)	strcpy(str, "");
	else			match->m_Func(thread, var, subfield ? subfield : "", str);
	return true;
}


template<class T>
bool handle_member(TrigThread *thread, T *e,
	const char *field, Hash fieldhash,
	const char *subfield, char *result)
{
	ScriptEngineBuiltinMemberValue<T> *match = ScriptEngineBuiltinMemberValue<T>::Find(fieldhash);
	
	if (!match)
		return false;
	
	if (str_cmp(match->m_Name, field) != 0)
	{
		ReportHashCollision(fieldhash, match->m_Name, field);
		return false;
	}
	
	match->m_Func(thread, e, /*field, fieldhash,*/ subfield ? subfield : "", result);
	return true;
}




/* checks every PLUSE_SCRIPT for random triggers */
void script_trigger_check(void) {
	FOREACH(CharacterList, character_list, iter)
	{
		CharData *ch = *iter;
		Scriptable *sc = SCRIPT(ch);
		
		if (ch->InRoom()
			&& IS_SET(SCRIPT_TYPES(sc), MTRIG_RANDOM)
			&& (IS_SET(SCRIPT_TYPES(sc), MTRIG_GLOBAL) || ch->InRoom()->GetZone()->ArePlayersPresent()))
			random_mtrigger(ch);
	}
  
	FOREACH(ObjectList, object_list, iter)
	{
		ObjData *obj = *iter;
		Scriptable *sc = SCRIPT(obj);
		RoomData *room = obj->Room();
		
		if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM)
			&& room != NULL
			&& (IS_SET(SCRIPT_TYPES(sc), OTRIG_GLOBAL) || room->GetZone()->ArePlayersPresent()))
			random_otrigger(obj);
	}

	//	TODO: Dynamic rooms covered here, too!
	FOREACH(RoomMap, world, iter)
	{
		RoomData *room = *iter;
		Scriptable *sc = SCRIPT(room);
		
		if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
				(IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL) || room->GetZone()->ArePlayersPresent()))
			random_wtrigger(room);
	}
}



void do_stat_trigger(CharData *ch, TrigData *trig, bool bShowLineNums) {
	char **trig_types;
	
	if (!trig) {
		log("SYSERR: NULL trigger passed to do_stat_trigger.");
		return;
	}
	
	BUFFER(string, MAX_STRING_LENGTH * 6);
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(bitBuf, MAX_STRING_LENGTH);

	sprintf(string, "Name: '`y%s`n',  Virtual: [`g%10s`n]\n",
			trig->GetName(), trig->GetVirtualID().Print().c_str());

	switch (GET_TRIG_DATA_TYPE(trig)) {
		case WLD_TRIGGER:	trig_types = wtrig_types;	break;
		case OBJ_TRIGGER:	trig_types = otrig_types;	break;
		case MOB_TRIGGER:	trig_types = mtrig_types;	break;
		case GLOBAL_TRIGGER:trig_types = gtrig_types;	break;
		default:			trig_types = wtrig_types;	break;
	}

	sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);

	sprintf_cat(string,"Trigger Class: %s %s%s%s\n"
				"Trigger Type: %s, N Arg: %d, Arg list: %s\n",
			scriptedit_get_class(trig),
			trig->m_Flags.any() ? " (" : "", trig->m_Flags.any() ? trig->m_Flags.print().c_str() : "", trig->m_Flags.any() ? ")" : "",
			buf, GET_TRIG_NARG(trig),
			trig->m_Arguments.c_str());

	if (trig->GetPrototype())
	{
		BUFFER(name, MAX_STRING_LENGTH);
		for (ScriptVariable::List::iterator var = trig->GetPrototype()->m_SharedVariables.begin();
			var != trig->GetPrototype()->m_SharedVariables.end(); ++var)
		{
			strcpy(buf, (*var)->GetName());
			if ((*var)->GetContext())			sprintf_cat(buf, "{%d}", (*var)->GetContext());
			
			const char *value = (*var)->GetValue();
			
			if (IsIDString(value))
			{
				find_uid_name(value, name);
				value = name;
			}
			
			sprintf_cat(string, "  %-15s:  %s\n", buf, value);
		}
	}

	strcat(string, "Commands:\n");

	const char *str = trig->GetScript();
	
	if (bShowLineNums)
	{
		int	line = 0;
		
		while (*str)
		{
			sprintf_cat(string, "%4d: ", ++line);
			
			const char *newline = strchr(str, '\n');
			
			if (newline)
			{
				++newline;	//	Skip to include newline
				strncat(string, str, newline - str);	//	Output everything up to that point.
			}
			else
			{
				strcat(string, str);	//	Last line, just output it!
				break;					//	then we're done
			}
			
			str = newline;
		}
	}
	else
		strcat(string, trig->GetScript());

//	/*if (trig->GetScript())*/	strcat(string, trig->GetScript());
	//else						strcat(string, "* Empty\n");
	
	if (*trig->GetErrors()) {
		strcat(string, "*** Compiler Errors\n");
		strcat(string, trig->GetErrors());
	}
	
	page_string(ch->desc, string);
}


/* find the name of what the uid points to */
void find_uid_name(const char *uid, char *name) {
	CharData *ch;
	ObjData *obj;

	if ((ch = get_char(uid)))		sprintf(name, "uid = %s, \"%s\"", uid + 1, ch->GetName());
	else if ((obj = get_obj(uid)))	sprintf(name, "uid = %s, \"%s\"", uid + 1, obj->GetName());
	else							sprintf(name, "uid = %s, (not found)", uid + 1);
}


/* general function to display stats on script sc */
void script_stat (CharData *ch, Scriptable *sc) {
	BUFFER(name, MAX_STRING_LENGTH);
	BUFFER(buf, MAX_STRING_LENGTH);
	
	char **trig_types;
		
	ch->Send("Global Variables: %s\n", !sc->m_Globals.empty() ? "" : "None");
	
	FOREACH(ScriptVariable::List, sc->m_Globals, var)
	{
		strcpy(name, (*var)->GetName());
		if ((*var)->GetContext())	sprintf_cat(name, "{%d}", (*var)->GetContext());
		
		const char *value = (*var)->GetValue();
		if (IsIDString(value))
		{
			find_uid_name(value, buf);
			value = buf;
		}
		
		ch->Send("    %-20s:  %s\n", name, value);
	}
	
	FOREACH(TrigData::List, TRIGGERS(sc), iter)
	{
		TrigData *t = *iter;
		
		ch->Send("\n"
				 "  Trigger: `y%s`n, Virtual: [`g%10s`n]\n",
				t->GetName(), t->GetVirtualID().Print().c_str());

		switch (GET_TRIG_DATA_TYPE(t)) {
			case WLD_TRIGGER:	trig_types = wtrig_types;	break;
			case OBJ_TRIGGER:	trig_types = otrig_types;	break;
			case MOB_TRIGGER:	trig_types = mtrig_types;	break;
			case GLOBAL_TRIGGER:trig_types = gtrig_types;	break;
			default:			trig_types = wtrig_types;	break;
		}
		sprintbit(GET_TRIG_TYPE(t), trig_types, buf);

		ch->Send("Script Type: %s %s%s%s\n"
				 "  Trigger Type: %s, N Arg: %d, Arg list: %s\n", 
				scriptedit_get_class(t),
				t->m_Flags.any() ? " (" : "", t->m_Flags.any() ? t->m_Flags.print().c_str() : "", t->m_Flags.any() ? ")" : "",
				buf, GET_TRIG_NARG(t), 
				t->m_Arguments.c_str());

//		if (GET_TRIG_WAIT(t) && t->curr_state)
		//int count = 0;
		FOREACH(std::list<TrigThread *>, t->m_Threads, iter)
		{
			TrigThread *thread = *iter;
			ch->Send("  Thread %d:\n", /*++count*/ thread->m_ID);
			
			int line = thread->GetCurrentLine();
			
			CompiledScript *pScript = thread->GetCurrentCompiledScript();
			if (pScript != pScript->m_Prototype->m_pProto->m_pCompiledScript)
				strcpy(buf, "<OUT OF DATE>");
			else if (line > 0)
			{
				const char *str = pScript->m_Prototype->m_pProto->GetScript();
				
				for (int i = 1; i < line; ++i)	//	line is 1-indexed
				{
					str = strchr(str, '\n');
					if (str)	++str;
				}
				int length = strcspn(str, "\n");
				strncpy(buf, str, length);
				buf[length] = 0;
			}
			else
				strcpy(buf, "<UNAVAILABLE>");
			
			if (thread->m_pWaitEvent)
				ch->Send("    Wait: %d, Current line [%d]: %s\n",
						thread->m_pWaitEvent->GetTimeRemaining(), line, buf);
			else if (thread->m_pDelay)
				ch->Send("    Wait: %d, Current line [%d]: %s\n",
						((TimerCommand *)thread->m_pDelay)->m_pEvent->GetTimeRemaining(), line, buf);
		
			ch->Send("  Variables: %s\n", !GET_THREAD_VARS(thread).empty() ? "" : "None");
			
			for (ScriptVariable::List::iterator var = GET_THREAD_VARS(thread).begin(); var != GET_THREAD_VARS(thread).end(); ++var)
			{
				strcpy(name, (*var)->GetName());
				if ((*var)->GetContext())	sprintf_cat(name, "{%d}", (*var)->GetContext());
				
				const char *value = (*var)->GetValue();
				if (IsIDString(value))
				{
					find_uid_name(value, buf);
					value = buf;
				}
				
				ch->Send("    %-20s:  %s\n", name, value);
			}
		}
		
		if (t->GetPrototype() && !t->GetPrototype()->m_SharedVariables.empty())
		{
			ch->Send("  Shared Variables:\n");
		
			FOREACH(ScriptVariable::List, t->GetPrototype()->m_SharedVariables, var)
			{
				strcpy(name, (*var)->GetName());
				if ((*var)->GetContext())	sprintf_cat(name, "{%d}", (*var)->GetContext());
				
				const char *value = (*var)->GetValue();
				if (IsIDString(value))
				{
					find_uid_name(value, buf);
					value = buf;
				}
				
				ch->Send("    %-20s:  %s (shared)\n", name, value);
			}
		}
	}
}


void do_sstat_room(CharData * ch, RoomData *rm)
{
	ch->Send("Room name: `c%s`n\n"
			 "Virtual: [`g%10s`n]\n",
			 rm->GetName(),
			 rm->GetVirtualID().Print().c_str());

	script_stat(ch, SCRIPT(rm));
}


void do_sstat_object(CharData *ch, ObjData *j) {
	ch->Send("Name   : '`y%s`n', Aliases: %s\n"
			 "Virtual: [`g%10s`n], Type: %s\n",
		j->m_Name.empty() ? "<None>" : j->GetName(), j->GetAlias(),
		j->GetVirtualID().Print().c_str(), GetEnumName(GET_OBJ_TYPE(j)));

	script_stat(ch, SCRIPT(j));
}


void do_sstat_character(CharData *ch, CharData *k) {
	ch->Send("Name   : '`y%s`n', Alias: %s\n"
			 "Virtual: [`g%10s`n], In room [%10s]\n",
			k->GetName(), k->GetAlias(),
			k->GetVirtualID().Print().c_str(), k->InRoom()->GetVirtualID().Print().c_str());
	
	script_stat(ch, SCRIPT(k));
}




ACMD(do_compile)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(errors, MAX_STRING_LENGTH);
	BUFFER(buf, MAX_BUFFER_LENGTH);
	
	one_argument(argument, arg);
	
	if (!IsVirtualID(arg))
		ch->Send("Usage: compile <vnum>\n");
	else
	{
		ScriptPrototype *proto = trig_index.Find(arg);
		
		if (!proto)
		{
			ch->Send("Unknown script.\n");
			return;
		}
		
		CompiledScript	bytecode(proto);
		ScriptCompilerParseScript(proto->m_pProto->GetScript(), &bytecode, errors, proto->m_pProto->m_Flags.test(TrigData::Script_DebugMode));
		ScriptCompilerDumpCompiledScript(&bytecode, buf, MAX_BUFFER_LENGTH - 1024);
		if (*errors)
		{
			strcat("\n\n\n**** COMPILER ERRORS REPORTED\n"
					 "%s", errors);
		}
		page_string(ch->desc, buf);
	}
}


ACMD(do_attach)
{
	BUFFER(trig_name, MAX_STRING_LENGTH);
	BUFFER(targ_name, MAX_STRING_LENGTH);
	BUFFER(loc_name, MAX_STRING_LENGTH);
	BUFFER(arg, MAX_STRING_LENGTH);
	TrigData *trig;
	int loc;

	argument = two_arguments(argument, arg, trig_name);
	two_arguments(argument, targ_name, loc_name);

	if (!*arg || !*targ_name || !*trig_name)
		send_to_char("Usage: attach { mtr | otr | wtr } { trigger } { name } [ location ]\n", ch);
	else {  
		loc = (*loc_name) ? atoi(loc_name) : -1;
  
		ScriptPrototype *proto = trig_index.Find(trig_name);
		if (!proto)
			send_to_char("That trigger does not exist.\n", ch);
		else if (is_abbrev(arg, "mtr"))
		{
			CharData *victim;
			if ((victim = get_char_vis(ch, targ_name, FIND_CHAR_WORLD)))
			{
				if (!IS_STAFF(victim))
				{
					trig = TrigData::Create(proto);
					victim->GetScript()->AddTrigger(trig);

					ch->Send("Trigger %s (%s) attached to %s.\n",
							proto->GetVirtualID().Print().c_str(), trig->GetName(), victim->GetName());
				}
				else
					send_to_char("Staff can't have scripts.\n", ch);
			}
			else
				send_to_char("That mob does not exist.\n", ch);
		}
		else if (is_abbrev(arg, "otr"))
		{
			ObjData *object;
			if ((object = get_obj_vis(ch, targ_name)))
			{
				trig = TrigData::Create(proto);
				object->GetScript()->AddTrigger(trig);
  
				ch->Send("Trigger %s (%s) attached to %s.\n", proto->GetVirtualID().Print().c_str(), trig->GetName(), 
						(!object->m_Name.empty() ? object->GetName() : object->GetAlias()));
			}
			else
				send_to_char("That object does not exist.\n", ch);
		}
		else if (is_abbrev(arg, "wtr"))
		{
			RoomData *room;
			if (IsVirtualID(targ_name) /*&& !strchr(targ_name, '.')*/)
			{
				if ((room = find_target_room(ch, targ_name)))
				{
					trig = TrigData::Create(proto);
					room->GetScript()->AddTrigger(trig);

					ch->Send("Trigger %s (%s) attached to room %s.\n",
							proto->GetVirtualID().Print().c_str(), trig->GetName(), room->GetVirtualID().Print().c_str());
				}
	    	}
	    	else
				send_to_char("You need to supply a room number.\n", ch);
		} else
			send_to_char("Please specify 'mtr', otr', or 'wtr'.\n", ch);
    }
}



ACMD(do_detach)
{
	BUFFER(arg1, MAX_STRING_LENGTH);
	BUFFER(arg2, MAX_STRING_LENGTH);
	BUFFER(arg3, MAX_STRING_LENGTH);
	CharData *victim = NULL;
	ObjData *object = NULL;
	RoomData *room;
	char *trigger = 0;
	int tmp;

	
	three_arguments(argument, arg1, arg2, arg3);

	if (!*arg1 || !*arg2) {
		send_to_char("Usage: detach [ mob | object ] { target } { trigger | 'all' }\n", ch);
	} else {
		if (!str_cmp(arg1, "room")) {
			room = ch->InRoom();
			if (!str_cmp(arg2, "all")) {
				room->GetScript()->Extract();
				send_to_char("All triggers removed from room.\n", ch);
			} else if (room->GetScript()->RemoveTrigger(arg2)) {
				send_to_char("Trigger removed.\n", ch);
			} else
				send_to_char("That trigger was not found.\n", ch);
		} else {
			if (is_abbrev(arg1, "mob")) {
				if (!(victim = get_char_vis(ch, arg2, FIND_CHAR_WORLD)))
					send_to_char("No such mobile around.\n", ch);
				else if (!*arg3)
					send_to_char("You must specify a trigger to remove.\n", ch);
				else
					trigger = arg3;
			} else if (is_abbrev(arg1, "object")) {
				if (!(object = get_obj_vis(ch, arg2)))
					send_to_char("No such object around.\n", ch);
				else if (!*arg3)
					send_to_char("You must specify a trigger to remove.\n", ch);
				else
					trigger = arg3;
			} else  {
				if ((object = get_obj_in_equip_vis(ch, arg1, ch->equipment, &tmp))) ;
				else if ((object = get_obj_in_list_vis(ch, arg1, ch->carrying))) ;
				else if ((victim = get_char_vis(ch, arg1, FIND_CHAR_ROOM))) ;
				else if ((object = get_obj_in_list_vis(ch, arg1, ch->InRoom()->contents))) ;
				else if ((victim = get_char_vis(ch, arg1, FIND_CHAR_WORLD))) ;
				else if ((object = get_obj_vis(ch, arg1))) ;
				else send_to_char("Nothing around by that name.\n", ch);
				trigger = arg2;
			}
			
			if (!trigger) { /* handled already */ }
			else if (victim) {
				if (!str_cmp(arg2, "all")) {
					victim->GetScript()->Extract();
					act("All triggers removed from $N.", TRUE, ch, 0, victim, TO_CHAR);
				} else if (victim->GetScript()->RemoveTrigger(trigger)) {
					send_to_char("Trigger removed.\n", ch);
					/*if (!TRIGGERS(SCRIPT(victim)).Top()) {
						victim->DeleteScript();
					}*/
				} else
					send_to_char("That trigger was not found.\n", ch);
			} else if (object) {
				if (!str_cmp(arg2, "all")) {
					object->GetScript()->Extract();
					act("All triggers removed from $p.", TRUE, ch, 0, object, TO_CHAR);
				} else if (object->GetScript()->RemoveTrigger(trigger)) {
					send_to_char("Trigger removed.\n", ch);
					/*if (!TRIGGERS(SCRIPT(object)).Top()) {
						object->DeleteScript();
					}*/
				} else
					send_to_char("That trigger was not found.\n", ch);
			}
		}
	}
}




/* same as one_argument except that it doesn't ignore fill words */
const char * ScriptEngineGetArgument(const char *src, char *dst)
{
	char c = *src;
	
	while (isspace(c))
		c = *(++src);

	while (c && !isspace(c)) {
		*(dst++) = c;
		c = *(++src);
	}

	*dst = '\0';

	while (isspace(c))
		c = *(++src);

	return src;
}




const char *ScriptEngineParamsSkipSpaces(const char *src)
{
	while (*src == ' ')
		++src;
	
	return src;
}


const char *ScriptEngineParamsCopyNextParam(const char *src, char *&dst)
{
	src = ScriptEngineParamsSkipSpaces(src);
	
	char c = *src;
		
	while (c && c != PARAM_SEPARATOR)
	{
		if (ScriptEngineRecordIsRecordStart(src))
		{
			src = ScriptEngineRecordCopyRecord(src, dst);
			c = *src;
		}
		else
		{
			*dst++ = c;
			c = *(++src);
		}
	}
	
	*dst = '\0';
	
	if (c)	++src;	//	c == PARAM_SEPARATOR
	
	return src;
}


const char *ScriptEngineParamsSkipParam(const char *src)
{
	char c = *src;
	
	while (c && c != PARAM_SEPARATOR)
	{
		if (ScriptEngineRecordIsRecordStart(src))
		{
			src = ScriptEngineRecordSkipRecord(src);
			c = *src;
		}
		else
			c = *(++src);
	}
	
	if (c)	++src;	//	c == PARAM_SEPARATOR
	
	return src;
}


const char *ScriptEngineParamsSkipToParam(const char *src, int num)
{
	while (*src && --num > 0)
	{
		src = ScriptEngineParamsSkipParam(src);
	}
	return ScriptEngineParamsSkipSpaces(src);
}


int ScriptEngineParamsCount(const char *src)
{
	int count = 0;
	
	src = ScriptEngineParamsSkipSpaces(src);
	while (*src)
	{
		++count;
		src = ScriptEngineParamsSkipParam(src);
	}
	return count;
}


const char *ScriptEngineGetParameter(const char *src, char *dst, int num)
{
	src = ScriptEngineParamsSkipToParam(src, num);
	src = ScriptEngineParamsCopyNextParam(src, dst);
	return src;
}


const char *ScriptEngineGetParameter(const char *src, char *dst)
{
	return ScriptEngineParamsCopyNextParam(src, dst);
}


#if 0
const char * ScriptEngineGetParameter(const char *src, char *dst)
{
	char c = *src;
	
	while (isspace(c) && c != PARAM_SEPARATOR)
		c = *(++src);

	while (c && c != PARAM_SEPARATOR)
	{
		*(dst++) = c;
		c = *(++src);
	}

	*dst = '\0';
	
	if (c)	c = *(++src);	//	c == PARAM_SEPARATOR

	while (isspace(c) && c != PARAM_SEPARATOR)
		c = *(++src);

	return src;
}


const char * ScriptEngineGetParameter(const char *src, char *dst, int num)
{
	char c = *src;
	
	while (c && --num > 0)
	{
		while (c && c != PARAM_SEPARATOR)
			c = *(++src);
			
		if (c)	c = *(++src);	//	c == PARAM_SEPARATOR
	}
	
	return ScriptEngineGetParameter(src, dst);
}
#endif

int ScriptEngineListCountParameters(const char *src)
{
	int count = 0;
	
	char c = *src;
	
	while (c)
	{
		++count;
		
		while (c && c != PARAM_SEPARATOR)
			c = *(++src);
		
		if (c)	//	it's a param separator
		{
			c = *(++src);	//	Skip the separator
			
			if (!c)		//	There is a blank parameter, the loop will exit, so it needs to be counted
				++count;
		}
	}
	
	return count;
}


const char * ScriptEngineListCompareParameters(const char *src, const char *match, bool &bIsMatch)
{
	src = ScriptEngineSkipSpaces(src);
	
	char c = *src;
	char m = *match;
	
	while (m && tolower(c) == tolower(m))
	{
		c = *(++src);
		m = *(++match);
	}
	
	if (!m)
	{
		//	We matched everything, now skip any non-ParamSep spaces!
		while (c && isspace(c) && c != PARAM_SEPARATOR)
			c = *(++src);
	}
		
	if (!m && (!c || c == PARAM_SEPARATOR))	//	Matched everything!
		bIsMatch = true;
	else
	{
		while (c && c != PARAM_SEPARATOR)	c = *(++src);
		if (c)	++src;
	}
	
	return src;
}

int ScriptEngineListFindParameter(const char *src, const char *match)
{
	bool bIsMatch = false;
	int	num = 0;

	match = ScriptEngineSkipSpaces(match);
	
	while (*src)
	{
		//	Skip spaces to start of parameter
		char c = *src;
		while (isspace(c) && c != PARAM_SEPARATOR)
			c = *(++src);
		
		++num;
		
		if (!c)	break;	//	End of the line; abort
		else if (c != PARAM_SEPARATOR)	//	Small optimization
		{
			src = ScriptEngineListCompareParameters(src, match, bIsMatch);
			if (bIsMatch)
				return num;
		}
	}

	return 0;
}


const char *RECORD_START_DELIMITER		= "[#";
const char *RECORD_END_DELIMITER		= "#]";
const char *RECORD_ITEM_DELIMITER		= "##";

//	Should be at Record Start Char, returns char after Record End or \0
const char * ScriptEngineRecordSkipRecord(const char *p)
{
	int		i;

	p = ScriptEngineRecordSkipDelimiter(p);
	
	for (i = 1; *p && i; ++p)
	{
		if (ScriptEngineRecordIsRecordStart(p))		{ ++i; ++p; }
		else if (ScriptEngineRecordIsRecordEnd(p))	{ --i; ++p; }
	}
	return p;
}

//	Should be at Record Start Char, returns char after Record End or \0
const char * ScriptEngineRecordCopyRecord(const char *p, char *&dst)
{
	int i;
	char c, prevc;
	
	*dst++ = *p++;	//	Copy first char - should be Record Start
	*dst++ = *p++;	//	Copy second char - should be Record Start

	//	Copy chars, up to and including the Record End
	prevc = 0;
	for (i = 1; (c = *p) && i; ++p)
	{
		*dst++ = c;
		
		if (prevc == RECORD_START_DELIMITER_1 && c == RECORD_START_DELIMITER_2)
		{
			++i;
		}
		else if (prevc == RECORD_END_DELIMITER_1 && c == RECORD_END_DELIMITER_2)
		{
			--i;
		}
		
		prevc = c;
	}
	
	*dst = '\0';
	return p;	//	p COULD be on the \0
}

//	Should be at Record Start Char, returns char after Item Start
const char *ScriptEngineRecordSkipField(const char *src)
{
	char c = *src;
	
	while (c)
	{
		if (ScriptEngineRecordIsRecordItem(src))
		{
			src = ScriptEngineRecordSkipDelimiter(src);
			break;
		}
		else if (ScriptEngineRecordIsRecordEnd(src))
		{
			break;
		}
		else if (ScriptEngineRecordIsRecordStart(src))
		{
			src = ScriptEngineRecordSkipRecord(src);
			c = *src;
			while (isspace(c))	c = *(++src);
		}
		else
			c = *(++src);
	}
	
	return src;
}

//	Should be at Record Start Char, returns char after Item Start, or End of Record or \0
const char *ScriptEngineRecordSkipToField(const char *src, const char *field, bool &found)
{
	int	fieldlen = strlen(field);
	
	if (ScriptEngineRecordIsRecordStart(src))
		src = ScriptEngineRecordSkipDelimiter(src);
	
	found = false;
	while (*src && !ScriptEngineRecordIsRecordEnd(src))
	{
		if (!strn_cmp(src, field, fieldlen) && src[fieldlen] == RECORD_FIELD_DELIMITER)
		{
			src += fieldlen + 1;	//	+1 for the ':'
			found = true;
			break;
		}
		
		src = ScriptEngineRecordSkipField(src);
	}

	return src;
}

//	Should be at Record Start Char, returns char after Item Start, or End of Record or \0
const char *ScriptEngineRecordCopyToField(const char *src, const char *field, char *&dst, bool &found)
{
	const char *start = src;
	
	src = ScriptEngineRecordSkipToField(src, field, found);
	
	//	Copy from Start to Src
	int len = src - start;
	strncpy(dst, start, len);
	dst[len] = '\0';
	dst += len;
	
	return src;
}

//	Should be at first char of item (past Item Separator or Record Start)
//	Returns char * at next item delimiter, end of record char, or \0
const char *ScriptEngineRecordCopyField(const char *src, char *dst)
{
	char c = *src;

	//	Copy until next item delimiter, or end of record
	int i = 0;
	while (c)
	{
		if (i == 0 && (ScriptEngineRecordIsRecordItem(src) || ScriptEngineRecordIsRecordEnd(src)))
		{
			break;
		}
		
		if (ScriptEngineRecordIsRecordStart(src))
		{
			++i;
		}
		else if (ScriptEngineRecordIsRecordEnd(src))
		{
			--i;
		}
		
		*dst++ = c;
		c = *(++src);
	}
	*dst = '\0';
	
	return src;
}


bool ScriptEngineRecordGetField(const char *src, const char *field, char *dst)
{
	bool found;
	src = ScriptEngineRecordSkipToField(src, field, found);
	
	*dst = 0;
	if (found)
	{
		ScriptEngineRecordCopyField(src, dst);
	}
	
	return found;
}


const char *ScriptEngineRecordSkipListWord(const char *src)
{
	src = ScriptEngineSkipSpaces(src);
	
	char c = *src;
	
	if (ScriptEngineRecordIsRecordStart(src))
	{
		src = ScriptEngineRecordSkipRecord(src);
		c = *src;
	}
	else
	{
		while (c && !isspace(c) && !ScriptEngineRecordIsRecordStart(src) &&
			!ScriptEngineRecordIsRecordItem(src) && !ScriptEngineRecordIsRecordEnd(src))
		{
			c = *(++src);
		}
	}
	
	while (isspace(c))			c = *(++src);
	
	return src;
}


const char *ScriptEngineRecordCopyUntilListWord(const char *src, char *&dst, int num)
{
	char c = *src;

	while (isspace(c))
	{
		*dst++ = c;
		c = *(++src);
	}
	
	while (c && --num > 0)
	{
		if (ScriptEngineRecordIsRecordStart(src))
		{
			src = ScriptEngineRecordCopyRecord(src, dst);
			c = *src;
		}
		else
		{
			while (c && !isspace(c) && !ScriptEngineRecordIsRecordStart(src))
			{
				if (ScriptEngineRecordIsRecordItem(src) || ScriptEngineRecordIsRecordEnd(src))
				{
					num = 0;
					break;
				}
				
				*dst++ = c;
				c = *(++src);
			}
		}
	
		while (isspace(c))
		{
			*dst++ = c;
			c = *(++src);
		}
	}
	
	*dst = '\0';
	
	return src;
}


const char *ScriptEngineSkipSpaces(const char *src)
{
	char c = *src;
	
	while (isspace(c))		c = *(++src);
	
	return src;
}

const char *ScriptEngineListSkipWord(const char *src)
{
	src = ScriptEngineSkipSpaces(src);
	
	char c = *src;
	
	if (ScriptEngineRecordIsRecordStart(src))
	{
		src = ScriptEngineRecordSkipRecord(src);
		c = *src;
	}
	else
	{
		while (c && !isspace(c) && !ScriptEngineRecordIsRecordStart(src))
		{
			c = *(++src);
		}
	}
	
	while (isspace(c))			c = *(++src);
	
	return src;
}


const char *ScriptEngineListSkipToWord(const char *src, int num)
{
	src = ScriptEngineSkipSpaces(src);
	
	char c = *src;
	
	while (c && --num > 0)
	{
		src = ScriptEngineListSkipWord(src);
		c = *src;
	}
	return src;
}

const char *ScriptEngineListCopyNextWord(const char *src, char *&dst)
{
	src = ScriptEngineSkipSpaces(src);
		
	if (ScriptEngineRecordIsRecordStart(src))
	{
		src = ScriptEngineRecordCopyRecord(src, dst);
	}
	else
	{
		char c = *src;
		while (c && !isspace(c) && !ScriptEngineRecordIsRecordStart(src))
		{
			*dst++ = c;
			c = *(++src);
		}
		*dst = '\0';
	}
	
//	while (isspace(c))			c = *(++src);

	return src;
}

const char *ScriptEngineListCopyWord(const char *src, char *&dst, int num)
{
	src = ScriptEngineListSkipToWord(src, num);
	src = ScriptEngineListCopyNextWord(src, dst);
	return src;
}

const char *ScriptEngineListCopyUntilWord(const char *src, char *&dst, int num)
{
	char c = *src;
	
	while (--num > 0 && c)
	{
		while (isspace(c))
		{
			*dst++ = c;
			c = *(++src);
		}
		
		src = ScriptEngineListCopyNextWord(src, dst);
	
		c = *src;
	}
	
	while (isspace(c))
	{
		*dst++ = c;
		c = *(++src);
	}
	
	*dst = '\0';
	
	return src;
}

void ScriptEngineListCopyRemaining(const char *src, char *&dst)
{
	char c = *src;
	while (c)
	{
		*dst++ = c;
		c = *(++src);
	}
	
	*dst = '\0';
}


char *ScriptEngineListAppend(char *dst, const char *src)
{
	char c = *src;
	while (c)
	{
		*dst++ = c;
		c = *(++src);
	}
	*dst = '\0';
	
	return dst;
}


char *ScriptEngineListAppendSpace(char *dst)
{
	*dst++ = ' ';
	*dst = '\0';
	
	return dst;
}

void ScriptEngineListExtractWord(const char *src, char *&dst, int num)
{
	src = ScriptEngineListCopyUntilWord(src, dst, num);
	src = ScriptEngineListSkipWord(src);
	ScriptEngineListCopyRemaining(src, dst);
}

void ScriptEngineListInsert(const char *src, char *&dst, const char *insert, int num)
{
	src = ScriptEngineListCopyUntilWord(src, dst, num);
	dst = ScriptEngineListAppend(dst, insert);
	if (*src)
	{
		dst = ScriptEngineListAppendSpace(dst);
		ScriptEngineListCopyRemaining(src, dst);
	}
}

const char * ScriptEngineListCompareWords(const char *src, const char *match, bool &bIsMatch)
{
	src = ScriptEngineSkipSpaces(src);
	
	char c = *src;
	char m = *match;
	
	bool	bAtLeastOneCharacterMatched = false;
	
	bIsMatch = false;
	
/*	while (m && tolower(c) == tolower(m))
	{
		c = *(++src);
		m = *(++match);
	}
*/

	while (m)
	{
		while (c == '`')
		{
			c = *++src;
			if (c && c != '`')
			{
				if (c == '^')	c = *++src;
				if (c)			c = *++src;
			}
		}

		while (m == '`')
		{
			m = *++match;
			if (m && m != '`')
			{
				if (m == '^')	m = *++match;
				if (m)			m = *++match;
			}
		}
		
		if (tolower(c) != tolower(m))
			break;
		
		bAtLeastOneCharacterMatched = true;
		
		if (c)	c = *++src;
		if (m)	m = *++match;
	}
	
	while (c == '`')
	{
		c = *++src;
		if (c && c != '`')
		{
			if (c == '^')	c = *++src;
			if (c)			c = *++src;
		}
	}

	while (m == '`')
	{
		m = *++match;
		if (m && m != '`')
		{
			if (m == '^')	m = *++match;
			if (m)			m = *++match;
		}
	}
	
	if (!m && (!c || isspace(c)))
		bIsMatch = bAtLeastOneCharacterMatched;
	else
	{
		while (c && !isspace(c))	c = *(++src);
	}
	
	return src;
}

int ScriptEngineListFindWord(const char *src, const char *match)
{
	bool bIsMatch = false;
	int	num = 0;
	
	src = ScriptEngineSkipSpaces(src);
	match = ScriptEngineSkipSpaces(match);
	
	while (*src)
	{
		src = ScriptEngineListCompareWords(src, match, bIsMatch);
		++num;
		if (bIsMatch)
			return num;
	}

	return 0;
}


int ScriptEngineListCount(const char *src)
{
	int count = 0;
	
	src = ScriptEngineSkipSpaces(src);
	while (*src)
	{
		++count;
		src = ScriptEngineListSkipWord(src);
		src = ScriptEngineSkipSpaces(src);
	}
	return count;
}


int ScriptEngineListCountMatching(const char *src, const char *match)
{
	int count = 0;
	
	src = ScriptEngineSkipSpaces(src);
	match = ScriptEngineSkipSpaces(match);
	
	while (*src)
	{
		bool	bIsMatch = false;
		
		src = ScriptEngineListCompareWords(src, match, bIsMatch);
		src = ScriptEngineSkipSpaces(src);
		
		if (bIsMatch)
			++count;
	}
	return count;
}


class ScriptEngineListConstructor
{
public:
	ScriptEngineListConstructor(char *dst) : m_Length(0), m_Destination(dst) {}
	
	char *				GetCursor() { return m_Destination + m_Length; }
	
	void				Add(const char *item)
	{
		if (IsOverflowing())	return;
		
		AddSpaceIfAppropriate();
		m_Length += sprintf(GetCursor(), "%s", item);
	}
	
	void				AddParam(const char *item)
	{
		if (IsOverflowing())	return;
		
		//	Special case: uses a tab instead of normal space
		m_Length += sprintf(GetCursor(), ShouldAddSpace() ? "\t%s" : "%s", item);
	}
	
	void				Add(int item)
	{
		if (IsOverflowing())	return;
		
		AddSpaceIfAppropriate();
		m_Length += sprintf(GetCursor(), "%d", item);
	}
	
	void				Add(IDNum item)
	{
		if (IsOverflowing())	return;
		
		AddSpaceIfAppropriate();
		m_Length += sprintf(GetCursor(), "%u", item);
	}
	
	void				Add(Entity *e) { Add(ScriptEngineGetUIDString(e)); }
	
	void				AddFormat(const char *format, ...) __attribute__ ((format (printf, 2, 3)))
	{
		if (IsOverflowing())	return;
		
		va_list args;
		
		va_start(args, format);
		AddSpaceIfAppropriate();
		m_Length += vsnprintf(GetCursor(), MAX_SCRIPT_STRING_LENGTH - m_Length, format, args);
		va_end(args);
	}
	
	template<class T, typename Arg1>
	void				AddExtendedData(Arg1 a1) 
	{
		if (IsOverflowing())	return;
		
		AddSpaceIfAppropriate();
		T::MakeDataString(GetCursor(), a1);
		Update();
	}
	
	template<class T, typename Arg1, typename Arg2>
	void				AddExtendedData(Arg1 a1, Arg2 a2) 
	{
		if (IsOverflowing())	return;
		
		AddSpaceIfAppropriate();
		T::MakeDataString(GetCursor(), a1, a2);
		Update();
	}
	
	template<class T, typename Arg1, typename Arg2, typename Arg3>
	void				AddExtendedData(Arg1 a1, Arg2 a2, Arg3 a3)
	{
		if (IsOverflowing())	return;
		
		AddSpaceIfAppropriate();
		T::MakeDataString(GetCursor(), a1, a2, a3);
		Update();
	}
	
	bool				IsOverflowing()
	{
		return (m_Length > MAX_SCRIPT_STRING_LENGTH - 32);
	}
	
private:
	unsigned int		m_Length;
	char *				m_Destination;
	
	bool				ShouldAddSpace() { return m_Length > 0; }
	void				AddSpaceIfAppropriate()
	{
		if (ShouldAddSpace())
		{
			m_Destination[m_Length++] = ' ';
			m_Destination[m_Length] = '\0';
		}
	}
	void				Update()
	{
		m_Length += strlen(GetCursor());
	}
	
	friend class ScriptEngineRecordConstructor;
};


class ScriptEngineRecordConstructor
{
public:
	ScriptEngineRecordConstructor(char *dst) : m_Length(0), m_Destination(dst), m_bFinalized(false), m_ListConstructor(NULL)
	{
		Initialize();
	}
	
	ScriptEngineRecordConstructor(ScriptEngineListConstructor &listCon) : m_Length(0), m_Destination(NULL), m_bFinalized(false), m_ListConstructor(&listCon)
	{
		m_ListConstructor->AddSpaceIfAppropriate();
		
		m_Destination = listCon.GetCursor();
		
		Initialize();
	}
	
	~ScriptEngineRecordConstructor()
	{
		Finalize();
	}
	
	char *				GetCursor() { return m_Destination + m_Length; }
	void				Update()
	{
		m_Length += strlen(GetCursor());
	}
	
	void				Add(const char *field, const char *data)
	{
		if (IsOverflowing() || m_bFinalized)
			return;
		
		m_Length += sprintf(GetCursor(), "#%s:%s#", field, data);
	}
	
	void				Add(const char *field, int i)
	{
		if (IsOverflowing() || m_bFinalized)
			return;
		
		m_Length += sprintf(GetCursor(), "#%s:%d#", field, i);
	}
	
	void				Add(const char *field, IDNum i)
	{
		if (IsOverflowing() || m_bFinalized)
			return;
		
		m_Length += sprintf(GetCursor(), "#%s:%u#", field, i);
	}
	
	void				Add(const char *field, Entity *e) { Add(field, ScriptEngineGetUIDString(e)); }
	
	void				StartField(const char *field)
	{
		if (IsOverflowing())
			return;
		
		m_Length += sprintf(GetCursor(), "#%s:", field);
	}

	void				EndField()
	{
		if (IsOverflowing())
			return;
		
		Update();
		
		strcpy(GetCursor(), "#");
		m_Length += strlen(GetCursor());
	}
	
	bool				IsOverflowing()
	{
		return (m_Length > MAX_SCRIPT_STRING_LENGTH - 32);
	}
	
	void				Finished()
	{
		Finalize();
	}
	
private:
	unsigned int		m_Length;
	char *				m_Destination;
	bool				m_bFinalized;
	ScriptEngineListConstructor	*m_ListConstructor;
	
	void				Initialize()
	{
		strcpy(m_Destination, "[");
		Update();
	}
	
	void				Finalize()
	{
		if (!m_bFinalized)
		{
			strcpy(GetCursor(), m_Length > 1 ? "]" : "##]");
			Update();
			
			m_bFinalized = true;
		}
		
		if (m_ListConstructor)
			m_ListConstructor->Update();
	}
};


double ScriptEngineParseDouble(const char *str)
{
	return atof(str);
}


long long ScriptEngineParseInteger(const char *str)
{
	return strtoll(str, NULL, 0);
}


void ScriptEnginePrintDouble(char *result, double d)
{
	sprintf(result, "%.12f", d);	//	13 is the most we can reasonably do; 14+ introduces problems
	fix_float(result);
}


int ScriptEnginePrintInteger(char *result, long long l)
{
	return sprintf(result, "%lld", l);
}


class ObjectMatcher 
{
public:
	virtual ~ObjectMatcher() {}

	void Find(ObjData ** vec, int num, std::list<ObjData *> &found, bool recursive);
	void Find(ObjectList &list, std::list<ObjData *> &found, bool recursive);

private:
	virtual bool match(ObjData *obj) const = 0;
};


void ObjectMatcher::Find(ObjData ** vec, int num, std::list<ObjData *> &found, bool recursive)
{
	for (int i = 0; i < num; ++i)
	{
		ObjData *	obj = vec[i];
		
		if (!obj)		continue;
		if (match(obj))	found.push_back(obj);
		
		if (recursive && !obj->contents.empty())
			Find(obj->contents, found, recursive);
	}
}

void ObjectMatcher::Find(ObjectList &list, std::list<ObjData *> &found, bool recursive)
{
	FOREACH(ObjectList, list, iter)
	{
		ObjData *obj = *iter;
		
		if (match(obj))	found.push_back(obj);
		
		if (recursive && !obj->contents.empty())
			Find(obj->contents, found, recursive);
	}
}


class ObjectMatcherIDNum : public ObjectMatcher
{
public:
	ObjectMatcherIDNum(IDNum idnum) : m_IDNum(idnum) {}

private:
	virtual bool match(ObjData *obj) const { return obj->GetID() == m_IDNum; }
	
	IDNum		m_IDNum;
};


class ObjectMatcherVirtual : public ObjectMatcher
{
public:
	ObjectMatcherVirtual(VirtualID vnum) : m_VNum(vnum) {}

private:
	virtual bool match(ObjData *obj) const { return (obj->GetVirtualID() == m_VNum); }
	
	VirtualID	m_VNum;
};


class ObjectMatcherName : public ObjectMatcher
{
public:
	ObjectMatcherName(const char *name) : m_Name(name) {}

private:
	virtual bool match(ObjData *obj) const { return silly_isname(m_Name, obj->GetAlias()); }
	
	const char *m_Name;
};



void find_obj_in_equipment(TrigThread *thread, CharData *ch, const char *str, char *output, bool deep)
{
	if (!str || !*str)	return;
	
	skip_spaces(str);
	
	std::auto_ptr<ObjectMatcher>	matcher;
	if (IsIDString(str))			matcher.reset(new ObjectMatcherIDNum(ParseIDString(str)));
	else if (IsVirtualID(str))		matcher.reset(new ObjectMatcherVirtual( VirtualID(str, thread->GetCurrentVirtualID().zone) ));
	else							matcher.reset(new ObjectMatcherName(str));
	
	std::list<ObjData *> found;
	matcher->Find(ch->equipment, NUM_WEARS, found, deep);
	
	ScriptEngineListConstructor	lc(output);
	FOREACH(std::list<ObjData *>, found, obj)
	{
		lc.Add(*obj);
		if (lc.IsOverflowing())
			break;
	}
}


void find_obj_in_contents(TrigThread *thread, ObjectList &contents, const char *str, char *output, bool deep)
{
	if (!str || !*str)	return;
	
	skip_spaces(str);
	
	std::auto_ptr<ObjectMatcher>	matcher;
	if (IsIDString(str))			matcher.reset(new ObjectMatcherIDNum(ParseIDString(str)));
	else if (IsVirtualID(str))		matcher.reset(new ObjectMatcherVirtual( VirtualID(str, thread->GetCurrentVirtualID().zone) ));
	else							matcher.reset(new ObjectMatcherName(str));
	
	std::list<ObjData *> found;
	matcher->Find(contents, found, deep);
	
	ScriptEngineListConstructor	lc(output);
	FOREACH(std::list<ObjData *>, found, obj)
	{
		lc.Add(*obj);
		if (lc.IsOverflowing())
			break;
	}
}



void find_char_people(TrigThread *thread, RoomData *room, std::list<CharData *> &lst, const char *str)
{
	if (!str || !*str)	return;
	
	skip_spaces(str);
	if (IsIDString(str))
	{
		IDNum idnum = ParseIDString(str);
		FOREACH(CharacterList, room->people, iter)
		{
			CharData *ch = *iter;
			if (idnum == GET_ID(ch))
				lst.push_back(ch);
		}
	}
	else if (IsVirtualID(str))
	{
		VirtualID vid(str);
		FOREACH(CharacterList, room->people, iter)
		{
			CharData *ch = *iter;
			if (!vid.IsValid() && !IS_NPC(ch))
				lst.push_back(ch);
			else if (vid.IsValid() && IS_NPC(ch) && vid == ch->GetVirtualID())
				lst.push_back(ch);
		}
	}
	else
	{
		FOREACH(CharacterList, room->people, iter)
		{
			CharData *ch = *iter;
			if (silly_isname(str, ch->GetAlias()))
				lst.push_back(ch);
		}
	}
}


/* sets str to be the value of var.field */
void ScriptEngineDereference(Scriptable *sc, TrigThread *thread, DereferenceData &data, bool continuation)
{
	static const Hash SelfHash = GetStringFastHash("self");
	const char *var = data.var;
	const char *field = data.field;
	const char *subfield = data.subfield;
	char *str = data.result;
	
	*str = '\0';
	
	Hash	varhash = !continuation ? GetStringFastHash(var) : 0;
	
	ScriptVariable *vd = NULL;
	
	if (!continuation && (field || !subfield)
		&& (varhash != SelfHash || str_cmp(var, "self"))	//	Don't look for a variable named %self...%
		&& *var != '@'
		&& !IsIDString(var) && !IsExtendedDataString(var)
		&& !is_number(var))	//	foo.bar.baz - foo.bar will evaluate, and then the result evaluated with field '.baz'.  This is called a 'continuation',
	{
		vd = ScriptEngineFindVariable(sc, thread, var, varhash, data.context, Scope::All | Scope::Constant);	//	if it was a foo{c}.var{c} then it's a continuation
	}
	
	if ((!field || !*field) && *var != '@')
	{
//		RoomData *	room;
		
		if (vd)	
		{
			//strcpy(str, vd->GetValue());
			data.SetResult(vd);
		}
		else if (!continuation && varhash == SelfHash && !str_cmp(var, "self"))	strcpy(str, ScriptEngineGetUIDString(GET_THREAD_ENTITY(thread)));
		else if (!continuation && handle_literals(sc, thread, var, varhash, "", 0, subfield, str)) {}	//	Continuations are never literals
//		else if ((room = get_room(var)))	sprintf(str, "%s", room->GetVirtualID().Print().c_str());
		else								*str = '\0';
		
		return;
	}

	Hash		fieldhash = GetStringFastHash(field);
	CharData *	c = NULL;
	ObjData *	o = NULL;
	RoomData *	r = NULL;
	
	if (vd)
	{
		const char *name = vd->GetValue();

		if (IsIDString(name))
		{
			Entity *e = IDManager::Instance()->Find(name);
			if (e)
			{
				switch (e->GetEntityType())
				{
					case Entity::TypeCharacter:	c = (CharData *)e;	break;
					case Entity::TypeObject:	o = (ObjData *)e;	break;
					case Entity::TypeRoom:		r = (RoomData *)e;	break;
				}
			}
		}
		else
		{
			r = get_room(name, thread->GetCurrentVirtualID().zone);
		}
	}
	else if (!continuation && varhash == SelfHash && !str_cmp(var, "self"))
	{
		switch (GET_THREAD_ENTITY(thread)->GetEntityType())
		{
			case Entity::TypeCharacter:	c = (CharData *) GET_THREAD_ENTITY(thread);	break;
			case Entity::TypeObject:	o = (ObjData *) GET_THREAD_ENTITY(thread);	break;
			case Entity::TypeRoom:		r = (RoomData *) GET_THREAD_ENTITY(thread);	break;
			default:	break;
		}
	}
	else if (IsIDString(var))
	{
		Entity *e = IDManager::Instance()->Find(var);
		if (e)
		{
			switch (e->GetEntityType())
			{
				case Entity::TypeCharacter:	c = (CharData *)e;	break;
				case Entity::TypeObject:	o = (ObjData *)e;	break;
				case Entity::TypeRoom:		r = (RoomData *)e;	break;
			}
		}
	}
	else if (IsExtendedDataString(var))
	{}
	else if ((r = get_room(var, thread->GetCurrentVirtualID().zone)))
	{ }
	else if (*var == '@' && !continuation)
	{
		BUFFER(vnum, MAX_STRING_LENGTH);
		
		field = ScriptEngineGetParameter(subfield ? subfield : "", vnum);
		fieldhash = GetStringFastHash(field);
		subfield = "";
		
		if (!str_cmp(var + 1, "mob"))
		{
			CharacterPrototype *proto = mob_index.Find(VirtualID(vnum, thread->GetCurrentVirtualID().zone));
			if (proto)
				c = proto->m_pProto;				
		}
		else if (!str_cmp(var + 1, "obj"))
		{
			ObjectPrototype *proto = obj_index.Find(VirtualID(vnum, thread->GetCurrentVirtualID().zone));
			if (proto)
				o = proto->m_pProto;
		}
	}
	
	
	if (vd)	//	instead of a bunch of: vd ? vd->GetValue() : var
	{
		var = vd->GetValue();
	}

	int num;
	if (vd && is_number(field) && (num = atoi(field)))
	{
		// GET WORD - recycle field buffer!
		BUFFER(buf, MAX_STRING_LENGTH);
		const char *name = vd->GetValue();
		int count;
		for (count = 0; (count < num) && (*name); count++)
			name = ScriptEngineGetArgument(name, buf);
		if (count == num)	strcpy(str, buf);
	}
	else if (vd == NULL && !continuation && handle_literals(sc, thread, var, varhash, field, fieldhash, subfield, str)) {}
	else if (*field && text_processed(sc, thread, var, 0, field, fieldhash, subfield, str, (vd == NULL && !continuation))) {}
	else if (ScriptEngineRecordIsRecordStart(var))
	{
		//	Retrieve the field
		bool	found;
		const char *data = ScriptEngineRecordSkipToField(var, field, found);
		ScriptEngineRecordCopyField(data, str);
	}
	else
	{
		Entity *e = c ? static_cast<Entity *>(c)
				:  (o ? static_cast<Entity *>(o)
				:		static_cast<Entity *>(r));
		
		if (!e)
		{
			//	Here, because we now know its not an Entity at least, or its a missing entity
			if (IsExtendedDataString(var))
			{
				ExtendedData::HandleResult result = ExtendedData::Handle(thread, var, field, fieldhash, subfield, str);
				if (result != ExtendedData::HandleNoError)
				{
					switch (result)
					{
						case ExtendedData::HandleHandlerNotFound:
							ScriptEngineReportError(thread, false, "unhandled extended data: %s", var);
							break;
						case ExtendedData::HandleUnknownField:
							{
								BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
								ExtendedData::Handler::Handle handler = ExtendedData::GetHandler(var);
								ScriptEngineReportError(thread, false, "unknown %s field: '%s'", ExtendedData::GetHandlerName(var).c_str(), field);
								break;
							}
					}
				}
			} else
				ScriptEngineReportError(thread, false, "invalid entity, field: %s", field);
		}
		else
		{
			extern bool ScriptEngineRunScriptAsFunctionOnEntity(Scriptable *sc, TrigThread *thread, Entity *e, const char *functionName, const char *functionArgs, char *output);

			if (!subfield)
			{
				vd = ScriptEngineFindVariable(e->GetScript(), thread, field, fieldhash, data.fieldcontext, Scope::Global | Scope::Shared);
				
				if (vd)
				{
					strcpy(str, vd->GetValue());
					return;
				}
			}
			
			bool found = handle_member(thread, e, field, fieldhash, subfield, str);
			if (!found && c)	found = handle_member(thread, c, field, fieldhash, subfield, str);
			if (!found && o)	found = handle_member(thread, o, field, fieldhash, subfield, str);
			if (!found && r)	found = handle_member(thread, r, field, fieldhash, subfield, str);
			
			if (!found && subfield)	found = ScriptEngineRunScriptAsFunctionOnEntity(sc, thread, e, field, subfield, str);
			
			if (!found)
			{
				ScriptEngineReportError(thread, false, "unknown %s field: '%s'",
					  (c ? "char"
					: (o ? "object"
					: (r ? "room"
					: "entity"))), field);
			}
		}
	}
}


//	Literal:
//		literal
//		literal(subfield)
//		literal.field
//		literal.field(subfield)

//	Text process:
//		<var>.field
//		<var>.field(subfield)

//	Entity value:
//		<entity>.field
//		<entity>.field(subfield)



#define BUILTIN_LITERAL_FINAL(SUFFIX, NAME) \
ScriptEngineBuiltinLiteral::Result BUILTIN_LITERAL_##SUFFIX(TrigThread *thread, const char *field, Hash fieldhash, const char *subfield, char *result); \
static ScriptEngineBuiltinLiteral g_BUILTING_LITERAL_##SUFFIX( NAME, BUILTIN_LITERAL_##SUFFIX ); \
ScriptEngineBuiltinLiteral::Result BUILTIN_LITERAL_##SUFFIX(TrigThread *thread, const char *field, Hash fieldhash, const char *subfield, char *result)

#define BUILTIN_LITERAL(N)			BUILTIN_LITERAL_FINAL( N, #N )
#define BUILTIN_LITERAL_NS(NS, N)	BUILTIN_LITERAL_FINAL( NS##_##N , #NS ":" #N )


#define BUILTIN_TEXT(N) \
void BUILTIN_TEXT_##N(TrigThread *thread, const char *value, const char *subfield, char *result); \
static ScriptEngineBuiltinTextFunction g_BUILTING_TEXT_##N( #N, BUILTIN_TEXT_##N ); \
void BUILTIN_TEXT_##N(TrigThread *thread, const char *value, const char *subfield, char *result)


#define BUILTIN_MEMBER(T, N) \
void BUILTIN_MEMBER_##T##_##N(TrigThread *thread, T *entity, /*const char *field, Hash fieldhash,*/ const char *subfield, char *result); \
static ScriptEngineBuiltinMemberValue<T> g_BUILTIN_MEMBER_##T##_##N( #N, BUILTIN_MEMBER_##T##_##N ); \
void BUILTIN_MEMBER_##T##_##N(TrigThread *thread, T *entity, /*const char *field, Hash fieldhash,*/ const char *subfield, char *result)


#define EXTENDEDDATA_HANDLER(T, N) \
/*namespace ExtendedData { class T; }*/ \
char g_EDCLASS_NAME_##T[] = /*#T*/ N; \
class /*ExtendedData::*/T : public ExtendedData::StandardHandler</*ExtendedData::*/T, g_EDCLASS_NAME_##T>

#define REGISTER_EXTENDEDDATA_HANDLER(T) static ExtendedData::HandlerRegistrar</*ExtendedData::*/T> g_EDCLASS_##T

#define BUILTIN_EXTENDEDDATA_MEMBER(T, N) \
void BUILTIN_MEMBER_ED_##T##_##N(TrigThread *thread, /*ExtendedData::*/T *handler, const char *subfield, char *result); \
static ScriptEngineBuiltinMemberValue</*ExtendedData::*/T> g_BUILTIN_MEMBER_ED_##T##_##N( #N, BUILTIN_MEMBER_ED_##T##_##N ); \
void BUILTIN_MEMBER_ED_##T##_##N(TrigThread *thread, /*ExtendedData::*/T *handler, const char *subfield, char *result)





BUILTIN_LITERAL(Character)
{
	CharData *ch = NULL;
	
	if (IsVirtualID(subfield))
	{
		VirtualID vid(VirtualID(subfield, thread->GetCurrentVirtualID().zone));
		
		if (mob_index.Find(vid))
		{
			ch = get_char_num(vid);
		}
	}
	else
	{
		ch = get_char(subfield);	
	}
	
	strcpy(result, ScriptEngineGetUIDString(ch));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(ClanMembersOn)
{
	Clan *clan = Clan::GetClan(atoi(subfield));
		
	if (!clan)	strcpy(result, "0");
	else
	{
		*result = 0;
		
		ScriptEngineListConstructor	lc(result);
		FOREACH(Clan::MemberList, clan->m_Members, iter)
		{
			if ((*iter)->m_Character)
				lc.Add((*iter)->GetName());
		}
	}
	if (!*result)	strcpy(result, "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}



EXTENDEDDATA_HANDLER(ClanData, "Clan")
{
public:
	ClanData() : clan(NULL) {}
	
	bool Parse(Params &params)
	{
		IDNum id;
		
		params >> id;
		clan = Clan::GetClan(id);
		
		return (clan != NULL);
	}
	
	Clan *			clan;
	
	static void			MakeDataString(char *result, Clan *clan)
	{
		MakeDataStringInternal(result, ParamWriter() << clan->GetID());
	}
};
REGISTER_EXTENDEDDATA_HANDLER(ClanData);


EXTENDEDDATA_HANDLER(ClanMemberData, "ClanMember")
{
public:
	ClanMemberData() : clan(NULL), memberId(0), member(NULL) {}
	
	bool Parse(Params &params)
	{
		IDNum id;
		
		params >> id;
		params >> memberId;
		
		clan = Clan::GetClan(id);
		member = clan ? clan->GetMember(memberId) : NULL;
		return (member != NULL);
	}
	
	Clan *				clan;
	IDNum				memberId;
	Clan::Member *		member;
	
	static void			MakeDataString(char *result, Clan *clan, Clan::Member *member)
	{
		MakeDataStringInternal(result, ParamWriter() << clan->GetID() << member->GetID());
	}
};
REGISTER_EXTENDEDDATA_HANDLER(ClanMemberData);


EXTENDEDDATA_HANDLER(ClanRankData, "ClanRank")
{
public:
	ClanRankData() : clan(NULL), rank(NULL) {}
	
	bool Parse(Params &params)
	{
		IDNum id;
		int rankNum;
		
		params >> id;
		params >> rankNum;
		
		clan = Clan::GetClan(id);
		rank = clan ? clan->GetRank(rankNum) : NULL;
		return (rank != NULL);
	}
	
	Clan *				clan;
	Clan::Rank *		rank;
	
	static void			MakeDataString(char *result, Clan *clan, Clan::Rank *rank)
	{
		MakeDataStringInternal(result, ParamWriter() << clan->GetID() << rank->m_Order);
	}
};
REGISTER_EXTENDEDDATA_HANDLER(ClanRankData);



BUILTIN_LITERAL(Clan)
{
	Clan *	clan = Clan::GetClan(atoi(subfield));
	
	if (!clan)	strcpy(result, "0");
	else		ClanData::MakeDataString(result, clan);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}



BUILTIN_LITERAL(Clans)
{
	ScriptEngineListConstructor	lc(result);
	for (int i = 0; i < Clan::IndexSize; ++i)
	{
		lc.AddExtendedData<ClanData>(Clan::Index[i]);
	}

	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_EXTENDEDDATA_MEMBER(ClanData, vnum)				{ sprintf(result, "%u", handler->clan->GetID()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, id)				{ sprintf(result, "%u", handler->clan->GetID()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, name)				{ strcpy(result, handler->clan->GetName()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, tag)				{ strcpy(result, handler->clan->GetTag()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, leader)
{
	if (handler->clan->m_Leader)
	{
		ScriptEngineRecordConstructor record(result);
		
		record.Add("name", get_name_by_id(handler->clan->m_Leader, ""));
		record.Add("ref", CharData::Find(handler->clan->m_Leader));
		record.Finished();
		
//		sprintf(result,
//			"[#name:%s"
//			"##ref:%s"
//			"#]",
//			get_name_by_id(handler->clan->m_Leader, ""),
//			ScriptEngineGetUIDString(CharData::Find(handler->clan->m_Leader)));	
	}
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, races)			{ sprintbit(handler->clan->m_Races, race_types, result); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, room)				{ ScriptEngineGetUIDString(world.Find(handler->clan->m_Room)); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, savings)			{ ScriptEnginePrintInteger(result, handler->clan->m_Savings); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, ranks)
{
	ScriptEngineListConstructor	lc(result);
	FOREACH(Clan::RankList, handler->clan->m_Ranks, rank)
	{
		lc.AddExtendedData<ClanRankData>(handler->clan, *rank);
	}
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, members)
{
	ScriptEngineListConstructor	lc(result);
	FOREACH(Clan::MemberList, handler->clan->m_Members, member)
	{
		lc.AddExtendedData<ClanMemberData>(handler->clan, *member);
	}
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, membersOnline)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(Clan::MemberList, handler->clan->m_Members, iter)
	{
		Clan::Member *member = *iter;
		if (member->m_Character)
		{
			lc.Add(member->m_Character);
		}
	}
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, enemies)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(Clan::EnemyList, handler->clan->m_Enemies, iter)
	{
		lc.Add(*iter);
	}
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, allies)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(Clan::AllyList, handler->clan->m_Allies, iter)
	{
		lc.Add(*iter);
	}
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, IsMember)
{
	strcpy(result, handler->clan->IsMember(IsIDString(subfield) ? ParseIDString(subfield)
		: (is_number(subfield) ? atoi(subfield) : get_id_by_name(subfield))) ? "1" : "0");
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, IsApplicant)
{
	strcpy(result, handler->clan->IsApplicant(IsIDString(subfield) ? ParseIDString(subfield)
		: (is_number(subfield) ? atoi(subfield) : get_id_by_name(subfield))) ? "1" : "0");
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, IsBanned)
{
	strcpy(result, handler->clan->IsBanned(IsIDString(subfield) ? ParseIDString(subfield)
		: (is_number(subfield) ? atoi(subfield) : get_id_by_name(subfield))) ? "1" : "0");
}

static Clan *GetClanByScriptString(const char *str)
{
	if (!IsExtendedDataString(str))
		return Clan::GetClan(atoi(str));

	ClanData c;
	return c.Construct(str) ? c.clan : NULL;
}
	

BUILTIN_EXTENDEDDATA_MEMBER(ClanData, IsEnemy)
{
	Clan *otherClan = GetClanByScriptString(subfield);
	if (otherClan)	strcpy(result, Clan::AreEnemies(handler->clan, otherClan) ? "1" : "0");
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, IsAlly)
{
	Clan *otherClan = GetClanByScriptString(subfield);
	if (otherClan)	strcpy(result, Clan::AreAllies(handler->clan, otherClan) ? "1" : "0");
}
BUILTIN_EXTENDEDDATA_MEMBER(ClanData, Relation)
{
	Clan *otherClan = GetClanByScriptString(subfield);
	if (otherClan)	strcpy(result, GetEnumName(handler->clan->GetRelation(otherClan)));
}


BUILTIN_EXTENDEDDATA_MEMBER(ClanMemberData, id)			{ sprintf(result, "%u", handler->member->GetID()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanMemberData, name)		{ strcpy(result, handler->member->GetName()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanMemberData, ref)		{ strcpy(result, ScriptEngineGetUIDString(handler->member->m_Character)); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanMemberData, rank)		{ ClanRankData::MakeDataString(result, handler->clan, handler->member->m_pRank); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanMemberData, privileges)	{ strcpy(result, handler->member->m_Privileges.print().c_str()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanMemberData, laston)		{ sprintf(result, "%ld", handler->member->m_LastLogin); }

BUILTIN_EXTENDEDDATA_MEMBER(ClanRankData, name)			{ strcpy(result, handler->rank->GetName()); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanRankData, order)		{ ScriptEnginePrintInteger(result, handler->rank->m_Order); }
BUILTIN_EXTENDEDDATA_MEMBER(ClanRankData, privileges)	{ strcpy(result, handler->rank->m_Privileges.print().c_str()); }


BUILTIN_LITERAL(ExpandColorCodes)
{
	char *dest = result;
	const char *src = subfield;
	int sz = 0;
	
	while (1)
	{
		char c = *src++;
		
		*dest++ = c;
		
		if (!c)				break;
		else if (c == '`')	
		{
			*dest++ = '`';
			++sz;
		}
		++sz;
		
		if (sz >= (MAX_SCRIPT_STRING_LENGTH - 256))
		{
			*dest++ = '\0';
			break;
		}
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(CRC)
{
	sprintf(result, "%#.8x", (unsigned int)GetStringCRC32(subfield));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Hash)
{
	sprintf(result, "%#.8x", (unsigned int)GetStringFastHash(subfield));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}




BUILTIN_LITERAL(Date)
{
	time_t mytime = time(0);
	
	if (!fieldhash)			sprintf(result,"%u", (unsigned)mytime);
	else
	{
		struct tm *tm = localtime(&mytime);
		
		static const Hash Hour		= GetStringFastHash("Hour");
		static const Hash Day		= GetStringFastHash("Day");
		static const Hash Minute	= GetStringFastHash("Minute");
		static const Hash Second	= GetStringFastHash("Second");
		static const Hash Month		= GetStringFastHash("Month");
		static const Hash Weekday	= GetStringFastHash("Weekday");
		static const Hash Year		= GetStringFastHash("Year");
		static const Hash Yearday	= GetStringFastHash("Yearday");
		
		if (fieldhash == Hour)			sprintf(result,"%d", tm->tm_hour);
		else if (fieldhash == Day)		sprintf(result,"%d", tm->tm_mday);
		else if (fieldhash == Minute)	sprintf(result,"%d", tm->tm_min);
		else if (fieldhash == Second)	sprintf(result,"%d", tm->tm_sec);
		else if (fieldhash == Month)	strcpy(result, month_name[tm->tm_mon]);
		else if (fieldhash == Weekday)	strcpy(result, weekdays[tm->tm_wday]);
		else if (fieldhash == Year)		sprintf(result,"%d", tm->tm_year + 1900);
		else if (fieldhash == Yearday)	sprintf(result,"%d", tm->tm_yday + 1);
		else							strcpy(result, "0");
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandledWithField;
}



BUILTIN_LITERAL(Pulse)
{
	extern UInt32 pulse;	
	sprintf(result, "%u", pulse);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Eval)
{
	strcpy(result, subfield);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Float)
{
	switch (str_type(subfield))
	{ 
		case STR_TYPE_STRING:	strcpy(result, "0.0");			break;
		case STR_TYPE_INTEGER:	sprintf(result, "%lld.0", ScriptEngineParseInteger(subfield));	break;
		case STR_TYPE_FLOAT:	strcpy(result, subfield);		break;
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(FindRoom)
{
	strcpy(result, ScriptEngineGetUIDString(world.Find(VirtualID(subfield, thread->GetCurrentVirtualID().zone))));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Integer)
{
	switch (str_type(subfield))
	{ 
		case STR_TYPE_STRING:	strcpy(result, "0");			break;
//		case STR_TYPE_INTEGER:	strcpy(result, subfield);		break;
		case STR_TYPE_INTEGER:	ScriptEnginePrintInteger(result, strtoull(subfield, NULL, 0));	break;
		case STR_TYPE_FLOAT:	ScriptEnginePrintInteger(result, (long long)atof(subfield));	break;
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(IsNumber)
{
	strcpy(result, str_type(subfield) != STR_TYPE_STRING ? "1" : "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(IsInteger)
{
	strcpy(result, str_type(subfield) == STR_TYPE_INTEGER ? "1" : "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(IsFloat)
{
	strcpy(result, str_type(subfield) == STR_TYPE_FLOAT ? "1" : "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(IsDir)
{
	int dir = search_block(subfield, dirs, false);
	
	if (dir == -1)					strcpy(result, "0");
	else							strcpy(result, dirs[dir]);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(MakeUID)
{
	strcpy(result, MakeIDString(atoi(subfield)));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(MudCommand)
{
	extern struct command_info *complete_cmd_info;
	int length, cmd;

	for (length = strlen(subfield), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
		if (!strncmp(complete_cmd_info[cmd].command, subfield, length) && (length >= strlen(complete_cmd_info[cmd].sort_as)))
			if (!IS_STAFFCMD(cmd))
				break;
	
	if (*complete_cmd_info[cmd].command == '\n')	strcpy(result, "");
	else								strcpy(result, complete_cmd_info[cmd].command);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(MobExists)
{
	strcpy(result, mob_index.Find(VirtualID(subfield, thread->GetCurrentVirtualID().zone)) ? "1" : "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Object)
{
	ObjData *obj = NULL;
	
	if (IsVirtualID(subfield))
	{
		VirtualID	vid(subfield, thread->GetCurrentVirtualID().zone);
		
		if (obj_index.Find(vid))
		{
			obj = get_obj_num(vid);
		}
	}
	else
	{
		obj = get_obj(subfield);
	}
	
	strcpy(result, ScriptEngineGetUIDString(obj));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(ObjExists)
{
	strcpy(result, obj_index.Find(VirtualID(subfield, thread->GetCurrentVirtualID().zone)) ? "1" : "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Player)
{
	strcpy(result, ScriptEngineGetUIDString(get_player(subfield)));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Players)
{
	bool bLinkdead = (str_str(subfield, "linkdead") != NULL);
	
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(CharacterList, character_list, iter)
	{
		CharData *ch = *iter;
		
		if (IS_NPC(ch))					continue;
		if (!bLinkdead && !ch->desc)	continue;
		
		lc.Add(ch);
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


#if 0

BUILTIN_LITERAL(Print2)
{
	char buf[512];
	
	enum JustificationMode
	{
		JustifyRight,
		JustifyLeft,
		JustifyCenter,
		JustifyZeroFill
	};
	
	enum SignMode
	{
		SignOnlyMinus,
		SignAlways,
		SignSpaceHolder
	};
	
	const int CONVERSION_MAX = sizeof(buf) - 3;
	
	if (!subfield)
		return ScriptEngineBuiltinLiteral::LiteralHandled;
	
	BUFFER(format, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(vararg, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, format);

	char *dst = result;
	
	while (*format)
	{
		char c = *format++;
		
		if (c != '%')
		{
			*dst++ = c;
			continue;
		}
		
		//	Handle %%
		c = *format++;
		
		if (c == '%')
		{
			*dst++ = c;
			continue;
		}
		
		
		JustificationMode	justification		= JustifyRight;
		SignMode			signMode			= SignOnlyMinus;
		bool				bPrecisionSpecified	= false;
		
		int					nFieldWidth			= 0;
		int					nPrecision			= 0;
		
		int					nCurrentWidth		= 0;
		int					nVisualWidth		= 0;
		
		
		//	Flags
		while (1)
		{
			bool flagFound = true;
			
			switch (c)
			{
				case '-':	justification = JustifyLeft;	break;
				case '|':	justification = JustifyCenter;	break;
				case '+':	signMode = SignAlways;			break;
				case ' ':	if (signMode == SignOnlyMinus) signMode = SignSpaceHolder; break;
				case '0':	if (justification == JustifyRight) justification = JustifyZeroFill; break;
				default:	flagFound = false;
			}
			
			if (!flagFound)	break;
			
			c = *format++;
		}
		
		//	Field Width
		if (c == '*')
		{
			//	TODO: *m$ where m is the numbered argument
			
			//	Dynamic width
			subfield = ScriptEngineGetParameter(subfield, vararg);
			
			nFieldWidth = atoi(vararg);
			c = *format++;
		}
		else
		{
			//	Specified width
			while (isdigit(c))
			{
				nFieldWidth = (nFieldWidth * 10) + (c - '0');
				c = *format++;
			}
		}
		
		nFieldWidth = RANGE(nFieldWidth, 0, CONVERSION_MAX);
		
		//	Precision
		if (c == '.')
		{
			bPrecisionSpecified = true;
			
			c = *format++;
			if (c == '*')
			{
				//	TODO: *m$ where m is the numbered argument
				subfield = ScriptEngineGetParameter(subfield, vararg);
				nPrecision = atoi(vararg);
				c = *format++;
			}
			else
			{
				while (isdigit(c))
				{
					nPrecision = (nPrecision * 10) + (c - '0');
					c = *format++;
				}
			}
		}
		
		char *str = "";
		buf[0] = 0;
		
		//	If have a parameter on the stack, lets handle it here
		switch (c)
		{
			case 'd':
			case 'D':
				{
					if (!bPrecisionSpecified)
						nPrecision = 1;
					else if (justification == JustifyZeroFill)
						justification = JustifyRight;
					
					subfield = ScriptEngineGetParameter(subfield, vararg);

					//	Start writing to the buf, from the end backwards
					str = buf + sizeof(buf);
					*--str = '\0';
										
					long long		num = ScriptEngineParseInteger(vararg);
					
					bool			bMinus = num < 0;
					unsigned long	unsignedNum = bMinus ? -num : num;
					unsigned long	base = 10;
					
					int				digits = 0;
					
					do
					{
						//	TODO: Handle base other than 10
						*--str = (char)(unsignedNum % base) + '0';
						unsignedNum /= base;
						++digits;
					}
					while (unsignedNum != 0);
					
					//	If we zero fill, change our precision to the total width
					//	but exclude 1 character for the sign
					if (justification == JustifyZeroFill)
					{
						nPrecision = nFieldWidth;
						if (bMinus || signMode != SignOnlyMinus)
							--nPrecision;
					}
					
					for (; digits < nPrecision; ++digits)
						*--str = '0';
						
					if (bMinus)								*--str = '-';
					else if (signMode == SignAlways)		*--str = '+';
					else if (signMode == SignSpaceHolder)	*--str = ' ';
					
					//	Visual width - length between current position and end of buffer
					nVisualWidth = buf + sizeof(buf) - 1 - str;
				}
				break;
#error FINISH ME
		}
	}
	
	*dst = '\0';
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}

#endif


BUILTIN_LITERAL(Print)
{
	char buf[512];

	/* JUSTIFICATION_OPTIONS: formats output to left justified, right justified and zero fill (no justification) */
	enum justification_options
	{
		left_justification,
		right_justification,
		center_justification,
		zero_fill
	};

	/* SIGN OPTIONS: Sign of what? */
	enum sign_options
	{
		only_minus,
		sign_always,
		space_holder
	};
	
	enum
	{
		conversion_max = 509
	};

	if (!subfield)
		return ScriptEngineBuiltinLiteral::LiteralHandled;
	
	BUFFER(format, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(vararg, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, format);
	
	char *p = result;
		
	while (*format)
	{
		char c = *(format++);
		
		if (c == '%')
		{
			bool	flagFound;
			int		justification			= right_justification;
			int		sign					= only_minus;
			bool	precision_specified		= false;
			int		field_width				= 0;
			int		precision				= 0;
			
			int		real_field_width		= 0;
			int		num_chars				= 0;
			
			c = *(format++);
			
			if (c == '%')
			{
				*(p++) = c;
				continue;
			}
			
			//	Flags
			while (1)
			{
				flagFound = true;
				
				switch (c)
				{
					case '-':
						justification = left_justification;
						break;
					
					case '|':
						justification = center_justification;
						break;
						
					case '+':
						sign = sign_always;
						break;
						
					case ' ':
						if (sign != sign_always)
							sign = space_holder;
						break;
						
					case '0':
						if (justification == right_justification)
							justification = zero_fill;
						break;
						
					default:
						flagFound = false;
				}
				
				if (flagFound)
					c = *(format++);
				else
					break;
			}
			
			if (c == '*')
			{
				subfield = ScriptEngineGetParameter(subfield, vararg);	
				field_width = atoi(vararg);
				c = *(format++);
			}
			else
			{
				while (isdigit(c))
				{
					field_width = (field_width * 10) + (c - '0');
					c = *(format++);
				}
			}
			if (field_width > conversion_max)
				field_width = conversion_max;
			
			if (c == '.')
			{
				precision_specified = true;
				
				c = *(format++);
				if (c == '*')
				{
					subfield = ScriptEngineGetParameter(subfield, vararg);	
					precision = atoi(vararg);
					c = *(format++);
				}
				else
				{
					while (isdigit(c))
					{
						precision = (precision * 10) + (c - '0');
						c = *(format++);
					}
				}
			}
			
			flagFound = true;
			
			char *s = "";
			num_chars = 0;
			buf[0] = 0;
			if (*subfield)
			{
				switch (c)
				{
					case 'd':
					case 'D':
						{
							if (!precision_specified)
								precision = 1;
							else if (justification == zero_fill)
								justification = right_justification;
							
							subfield = ScriptEngineGetParameter(subfield, vararg);							
							
							long long num = ScriptEngineParseInteger(vararg);
							
							s = buf + sizeof(buf);
							*--s = 0;
							
							unsigned long unsigned_num = num, base;
							int n, digits = 0;
							bool minus = false;
							
							base = 10;
							if (num < 0)
							{
								unsigned_num = -num;
								minus = true;
							}
							
							do
							{
								n = unsigned_num % base;
								unsigned_num /= base;
								n += '0';
								
								*--s = n;
								++digits;
							}
							while (unsigned_num != 0);
							
							if (justification == zero_fill)
							{
								precision = field_width;
								if (minus || sign != only_minus)
									--precision;
							}
							
							while (digits < precision)
							{
								*--s = '0';
								++digits;
							}
							
							if (minus)
								*--s = '-';
							else if (sign == sign_always)
								*--s = '+';
							else if (sign == space_holder)
								*--s = ' ';
								
							num_chars = buf + sizeof(buf) - 1 - s;
						}
						break;
					
					case 'f':
					case 'F':
						{
							if (!precision_specified)
								precision = 6;
							else if (precision > conversion_max)
								precision = conversion_max;
							
							subfield = ScriptEngineGetParameter(subfield, vararg);
							
							double num = atof(vararg);
							
							s = buf + sizeof(buf);
							*--s = 0;
							
							double unsigned_num;
							double unsigned_num_frac;
							int n, digits = 0;
							bool minus = false;
							
							if (num < 0.0)
							{
								num = -num;
								minus = true;
							}
							
							unsigned_num_frac = modf(num, &unsigned_num);
							
							if (precision > 0)
							{
								s -= precision;

								do
								{
									++digits;
									
									unsigned_num_frac *= 10.0;
									n = (int)fmod(digits < precision ?
											floor(unsigned_num_frac) :
											rint(unsigned_num_frac), 10.0);
									
									*s++ = n + '0';
								}
								while (digits < precision);
								
								s -= precision;

								*--s = '.';
								++digits;
							}
							else
								unsigned_num = rint(num);
							
							do
							{
								n = (int)fmod(unsigned_num, 10.0);
								unsigned_num /= 10.0;
								
								*--s = n + '0';
								++digits;
							}
							while (unsigned_num >= 1.0f && digits < conversion_max);
							
							if (justification == zero_fill)
							{
								precision = field_width;
								if (minus || sign != only_minus)
									--precision;
								
								while (digits < precision)
								{
									*--s = '0';
									++digits;
								}
							}
							
							if (minus)
								*--s = '-';
							else if (sign == sign_always)
								*--s = '+';
							else if (sign == space_holder)
								*--s = ' ';
								
							num_chars = buf + sizeof(buf) - 1 - s;
						}
						break;



					case 's':
					case 'S':
						{
							subfield = ScriptEngineGetParameter(subfield, vararg);

							num_chars = cstrlen(vararg);
							if (precision_specified && precision < num_chars)
							{
								num_chars = precision;
							}
							
							s = vararg;
						}
						break;
					
					default:
						continue;
				}
			}
			
			real_field_width = num_chars;
			
			if (justification != left_justification)
			{
				char fill_char = (justification == zero_fill) ? '0' : ' ';
				if (((*s == '+') || (*s == '-') || (*s == ' ')) &&
						(fill_char == '0'))
				{
					*(p++) = *s;
					++s;
					--num_chars;
				}
				while (real_field_width < field_width)
				{
					*(p++) = fill_char;
					++real_field_width;
				}
			}
			
			while (*s && num_chars > 0)
			{
				char c = (*s++);
				char nc = *s;

				*(p++) = c;
				
				if ((nc != 0) && (c == '`'))
				{
					//	Special case: Color Code.  We need to copy the remaining color code chars...
					
					*(p++) = nc;	//	Copy next char, regardless...
					++s;
					
					if (nc == '`')		//	Special case: ``
						--num_chars;
					else if (nc == '^')	//	Special case: `^ background color - 1 more color code character coming!
					{
						nc = *s;
						
						if (nc)
						{
							*(p++) = nc;
							++s;
						}
					}
				}
				else
					--num_chars;
			}
			
			
			//	Copy trailing color codes... except ``!
			while (*s == '`')
			{
				char nc = *++s;
				
				if (nc == 0 || nc == '`')
					break;
				
				*p++ = '`';
				*p++ = nc;
				
				++s;
				
				if (nc == '^')	//	Special case: `^ background color code
				{
					nc = *s;
					if (nc)
					{
						*p++ = nc;
						++s;	
					}
				}
			}
			
			if (justification == left_justification)
			{
				while (real_field_width < field_width)
				{
					*(p++) = ' ';
					++real_field_width;
				}
			}
		}
		else
			*(p++) = c;
	}
	*p = '\0';
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}



BUILTIN_LITERAL(Random)
{
	if (field && *field)
	{
		if (!str_cmp(field, "char"))
		{
			int count = 0;
			
			CharData *rndm = NULL;

			if (GET_THREAD_ENTITY(thread)->GetEntityType() == Entity::TypeCharacter)
			{
				CharData *ch = (CharData *) GET_THREAD_ENTITY(thread);
				FOREACH(CharacterList, ch->InRoom()->people, iter)
				{
					CharData *c = *iter;
					if (!PRF_FLAGGED(c, PRF_NOHASSLE) && (c != ch) && ch->CanSee(c) == VISIBLE_FULL)
						if (!Number(0, count++))	rndm = c;
				}
			}
			else if (GET_THREAD_ENTITY(thread)->GetEntityType() == Entity::TypeObject)
			{
				ObjData *obj = (ObjData *) GET_THREAD_ENTITY(thread);
				FOREACH(CharacterList, obj->Room()->people, iter)
				{
					CharData *c = *iter;
					if (!PRF_FLAGGED(c, PRF_NOHASSLE) && c->GetPlayer()->m_StaffInvisLevel == 0)
						if (!Number(0, count++))	rndm = c;
				}
			}
			else if (GET_THREAD_ENTITY(thread)->GetEntityType() == Entity::TypeRoom)
			{
				RoomData *room = (RoomData *) GET_THREAD_ENTITY(thread);
				FOREACH(CharacterList, room->people, iter)
				{
					CharData *c = *iter;
					if (!PRF_FLAGGED(c, PRF_NOHASSLE) && c->GetPlayer()->m_StaffInvisLevel == 0)
						if (!Number(0, count++))	rndm = c;
				}
			}
			strcpy(result, ScriptEngineGetUIDString(rndm));
		}
		else
		{
			int num;
			sprintf(result, "%d", ((num = atoi(field)) > 0) ? Number(1, num) : 0);
		}
	
		return ScriptEngineBuiltinLiteral::LiteralHandledWithField;
	}
	else if (is_number(subfield))
		sprintf(result, "%d", Number(1, atoi(subfield)));
	else
		strcpy(result, "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Revdir)
{
	int dir = search_block(subfield, dirs, false);
	
	if (dir == -1)					strcpy(result, "0");
	else							strcpy(result, dirs[rev_dir[dir]]);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(RoomExists)
{
	sprintf(result, "%d", world.Find(VirtualID(subfield, thread->GetCurrentVirtualID().zone)) != NULL);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Select)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, buf);
	
	if (!ScriptEngineOperationEvaluateAsBoolean(subfield, str_type(subfield)))
	{
		subfield = ScriptEngineParamsSkipParam(subfield);
	}
	
	ScriptEngineGetParameter(subfield, result);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(String)
{
	strcpy(result, subfield);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Time)
{
	static const Hash Hour		= GetStringFastHash("Hour");
	static const Hash Day		= GetStringFastHash("Day");
	static const Hash Minute	= GetStringFastHash("Minute");
	static const Hash Month		= GetStringFastHash("Month");
	static const Hash Weekday	= GetStringFastHash("Weekday");
	static const Hash Year		= GetStringFastHash("Year");
	
	if (fieldhash == Hour)			sprintf(result,"%d", time_info.hours);
	else if (fieldhash == Day)		sprintf(result,"%d", time_info.day + 1);
	else if (fieldhash == Minute)	sprintf(result,"%d", time_info.minutes);
	else if (fieldhash == Month)	strcpy(result, month_name[time_info.month]);
	else if (fieldhash == Weekday)	strcpy(result, weekdays[time_info.weekday]);
	else if (fieldhash == Year)		sprintf(result,"%d", time_info.year);
	else							strcpy(result, "");
	
	return ScriptEngineBuiltinLiteral::LiteralHandledWithField;
}


BUILTIN_LITERAL(ThreadID)
{
	ScriptEngineRecordConstructor record(result);
	
	record.Add("entity", thread->m_pEntity);
	record.Add("thread", thread->m_ID);
	
	record.Finished();
		
//	sprintf(result, "[#entity:%s##thread:%u#]", ScriptEngineGetUIDString(thread->m_pEntity), thread->m_ID);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Valid)
{
	strcpy(result, IDManager::Instance()->Find(subfield) ? "1" : "0");
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}

BUILTIN_LITERAL(Variable)
{
	if (*subfield)
	{
		BUFFER(variable, MAX_SCRIPT_STRING_LENGTH);
		int context;
		ScriptEngineParseVariable(subfield, variable, context);
		
		if (*variable)
		{
			ScriptVariable *vd = ScriptEngineFindVariable(thread->m_pEntity->GetScript(), thread, variable, 0, context, Scope::All | Scope::ContextSpecific | Scope::Constant);
			strcpy(result, vd ? vd->GetValue() : "");
		}
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Tiers)
{
	ScriptEnginePrintDouble(result, GET_EFFECTIVE(atoi(subfield)));
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


EXTENDEDDATA_HANDLER(ScriptData, "Script")
{
public:
	ScriptData() : script(NULL) {}
	
	bool Parse(Params &params)
	{
		VirtualID vid;
		
		params >> vid;
		
		script = trig_index.Find(vid);
		return (script != NULL);
	}
	
	ScriptPrototype *	script;
	
	static void			MakeDataString(char *result, ScriptPrototype *script)
	{
		MakeDataStringInternal(result, ParamWriter() << script->GetVirtualID());
	}
};
REGISTER_EXTENDEDDATA_HANDLER(ScriptData);


BUILTIN_LITERAL(Script)
{
	ScriptPrototype *script = trig_index.Find(subfield);
	
	if (!script)strcpy(result, "0");
	else		ScriptData::MakeDataString(result, script);
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_EXTENDEDDATA_MEMBER(ScriptData, constant)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	int context;
	
	ScriptEngineParseVariable(subfield, buf, context);
	
	ScriptVariable *var = handler->script->m_pProto->m_pCompiledScript->m_Constants.Find(buf, GetStringFastHash(buf), context);
	
	strcpy(result, var ? var->GetValue() : "");
}


BUILTIN_EXTENDEDDATA_MEMBER(ScriptData, shared)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	int context;
	
	ScriptEngineParseVariable(subfield, buf, context);
	
	ScriptVariable *var = handler->script->m_SharedVariables.Find(buf, GetStringFastHash(buf), context);
	
	strcpy(result, var ? var->GetValue() : "");
}



BUILTIN_LITERAL(Skillroll)
{
	if (*subfield)	ScriptEnginePrintDouble(result, SkillRoll(atoi(subfield)));
	else			ScriptEnginePrintDouble(result, BellCurve());
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(Simplepath)
{
	if (!*subfield)
		return ScriptEngineBuiltinLiteral::LiteralHandled;
	
	BUFFER(start, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(end, MAX_SCRIPT_STRING_LENGTH);
	
	RoomData* startRoom = NULL;
	RoomData* endRoom = NULL;
	
	subfield = ScriptEngineGetParameter(subfield, start);
	subfield = ScriptEngineGetParameter(subfield, end);
	
	if (IsIDString(start))
	{
		IDNum id = ParseIDString(start);
		Entity *e = IDManager::Instance()->Find(id);
		if (e)
		{
			switch (e->GetEntityType())
			{
				case Entity::TypeCharacter:	startRoom = static_cast<CharData *>(e)->InRoom();	break;
				case Entity::TypeObject:	startRoom = static_cast<ObjData *>(e)->Room();		break;
				case Entity::TypeRoom:		startRoom = static_cast<RoomData *>(e);				break;
			}
		}
	}
	else
	{
		startRoom = get_room(start, thread->GetCurrentVirtualID().zone);
	}
	
	
	if (IsIDString(end))
	{
		Entity *e = IDManager::Instance()->Find(end);
		if (e)
		{
			switch (e->GetEntityType())
			{
				case Entity::TypeCharacter:	endRoom = static_cast<CharData *>(e)->InRoom();		break;
				case Entity::TypeObject:	endRoom = static_cast<ObjData *>(e)->Room();		break;
				case Entity::TypeRoom:		endRoom = static_cast<RoomData *>(e);				break;
			}
		}
	}
	else
	{
		endRoom = get_room(end, thread->GetCurrentVirtualID().zone);
	}
	
	if (!startRoom || !endRoom)
		return ScriptEngineBuiltinLiteral::LiteralHandled;
	
	Path *path = Path2Room(startRoom, endRoom, 99, HUNT_GLOBAL | HUNT_THRU_DOORS | HUNT_THRU_GRAVITY);
	
	ScriptEngineListConstructor	lc(result);
	
	if (path)
	{
		for (std::vector<TrackStep>::reverse_iterator iter = path->m_Moves.rbegin(),
			end = path->m_Moves.rend(); iter != end; ++iter)
		{
//			sprintf_cat(result, "[#room:%s##dir:%s#] ",
//				ScriptEngineGetUIDString(iter->room),
//				dirs[iter->dir]);
			lc.Add(dirs[iter->dir]);
		}
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL(zonerooms)
{
	ZoneData *zone = *subfield ? zone_table.Find(subfield) : NULL;
	if (!zone)
	{
		strcpy(result, "0");
		return ScriptEngineBuiltinLiteral::LiteralHandled;
	}
	
	ScriptEngineListConstructor	lc(result);
	
	FOREACH_BOUNDED(RoomMap, world, zone->GetHash(), room)
	{
		lc.Add(*room);
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


#define BUILTIN_MATH(FUNC) \
	BUILTIN_LITERAL_NS(Math, FUNC )	{ ScriptEnginePrintDouble(result, FUNC(atof(subfield))); return ScriptEngineBuiltinLiteral::LiteralHandled; }

#define BUILTIN_MATH2(FUNC) \
	BUILTIN_LITERAL_NS(Math, FUNC )	{ \
		BUFFER(x, MAX_SCRIPT_STRING_LENGTH); \
		subfield = ScriptEngineGetParameter(subfield, x); \
		ScriptEnginePrintDouble(result, FUNC(atof(x), atof(subfield))); \
		return ScriptEngineBuiltinLiteral::LiteralHandled; \
	}


BUILTIN_LITERAL_NS(Math, abs)
{
	switch (str_type(subfield))
	{
		case STR_TYPE_INTEGER:	ScriptEnginePrintInteger(result, llabs(atoll(subfield)));	break;
		case STR_TYPE_FLOAT:	ScriptEnginePrintDouble(result, fabs(atof(subfield)));		break;
		default:		strcpy(result, "0");
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL_NS(Math, factorial)
{
	switch  (str_type(subfield))
	{
		case STR_TYPE_INTEGER:
			{
				unsigned long long	v = 1;
				long			count = atoi(subfield);
				
				for (int i = 1; i <= count; ++i)
					v *= i;
				
				ScriptEnginePrintInteger(result, v);
			}
			break;
		case STR_TYPE_FLOAT:
			{
				double				v = 1.0f;
				long			count = atoi(subfield);
				
				for (int i = 1; i <= count; ++i)
					v = v * i;
				
				ScriptEnginePrintDouble(result, v);
			}
			break;
		default:		strcpy(result, "0");
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_MATH(acos);
BUILTIN_MATH(acosh);
BUILTIN_MATH(asin);
BUILTIN_MATH(asinh);
BUILTIN_MATH(atan);
BUILTIN_MATH(atanh);
BUILTIN_MATH2(atan2);
BUILTIN_MATH(cbrt);
BUILTIN_MATH(ceil);
BUILTIN_MATH2(copysign);
BUILTIN_MATH(cos);
BUILTIN_MATH(cosh);
BUILTIN_MATH(erf);
BUILTIN_MATH(erfc);
BUILTIN_MATH(exp);
BUILTIN_MATH(exp2);
BUILTIN_MATH(expm1);
BUILTIN_MATH2(fdim);
BUILTIN_MATH(floor);
BUILTIN_MATH2(hypot);
BUILTIN_MATH(lgamma);
BUILTIN_MATH(log);
BUILTIN_MATH(log10);
BUILTIN_MATH(log1p);
BUILTIN_MATH(log2);
BUILTIN_MATH(nearbyint);
BUILTIN_MATH2(nextafter);
BUILTIN_MATH2(pow);
BUILTIN_MATH(round);
BUILTIN_MATH(sin);
BUILTIN_MATH(sinh);
BUILTIN_MATH(sqrt);
BUILTIN_MATH(tan);
BUILTIN_MATH(tanh);
BUILTIN_MATH(tgamma);
BUILTIN_MATH(trunc);


BUILTIN_LITERAL_NS(Math, min)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, buf);
	
	STR_TYPE lhs_type = str_type(buf);
	STR_TYPE rhs_type = str_type(subfield);
	
	STR_TYPE dominant_type = (STR_TYPE)MAX(lhs_type, rhs_type);	//	Simple: the higher the value, the more dominant
	
	switch (dominant_type)
	{
		case STR_TYPE_INTEGER:
		{
			long long lhs = atoi(buf);
			long long rhs = atoi(subfield);
			ScriptEnginePrintInteger(result, std::min(lhs, rhs));
		}
		break;
		
		case STR_TYPE_FLOAT:
		{
			double lhs = atoi(buf);
			double rhs = atoi(subfield);
			ScriptEnginePrintDouble(result, std::min(lhs, rhs));
		}
		break;
		
		default:		strcpy(result, "0");
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}


BUILTIN_LITERAL_NS(Math, max)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, buf);
	
	STR_TYPE lhs_type = str_type(buf);
	STR_TYPE rhs_type = str_type(subfield);
	
	STR_TYPE dominant_type = (STR_TYPE)MAX(lhs_type, rhs_type);	//	Simple: the higher the value, the more dominant
	
	switch (dominant_type)
	{
		case STR_TYPE_INTEGER:
		{
			long long lhs = ScriptEngineParseInteger(buf);
			long long rhs = ScriptEngineParseInteger(subfield);
			ScriptEnginePrintInteger(result, std::max(lhs, rhs));
		}
		break;
		
		case STR_TYPE_FLOAT:
		{
			double lhs = atof(buf);
			double rhs = atof(subfield);
			ScriptEnginePrintDouble(result, std::max(lhs, rhs));
		}
		break;
		
		default:		strcpy(result, "0");
	}
	
	return ScriptEngineBuiltinLiteral::LiteralHandled;
}




BUILTIN_TEXT(An)
{
	sprintf(result, "%s %s", AN(value), value);
}


BUILTIN_TEXT(IsAbbrev)
{
	strcpy(result, is_abbrev(subfield, value) ? "1" : "0");
}


BUILTIN_TEXT(IsName)
{
//	strcpy(result, isname(subfield, value) ? "1" : "0");
	
	
	BUFFER(newlist, MAX_SCRIPT_STRING_LENGTH);

	strcpy(newlist, value);

	long long i = 1;
	while (char *curtok = strsep(&newlist, " \t"))
	{
		if(is_abbrev(subfield, curtok))
		{
			
			ScriptEnginePrintInteger(result, i);
			return;
		}
		
		++i;
	}
	
	strcpy(result, "0");
}


BUILTIN_TEXT(Count)
{
	int num = 0;
	
	if (*subfield)		num = ScriptEngineListCountMatching(value, subfield);
	else				num = ScriptEngineListCount(value);
	
	sprintf(result, "%d", num);
}


BUILTIN_TEXT(ParamCount)
{
	sprintf(result, "%d", ScriptEngineListCountParameters(value));
}


BUILTIN_TEXT(ParamFind)
{
	sprintf(result, "%d", *subfield ? ScriptEngineListFindParameter(value, subfield) : 0);
}


BUILTIN_TEXT(Cap)
{
	char *s;
	char c;
	
	strcpy(result, value);
	s = result;
	
	while ((c = s[0]))
	{
		if (c == '`')
		{
			s+=1;					//	Skip the `
			if(s[0] == '^') s+=1;	//	Skip the ^ after a `
			if(s[0])		s+=1;	//	Skip the color code
		}
		else if (!isalnum(c))
			s += 1;
		else
		{
			*s = toupper(c);
			break;
		}
	}
}


BUILTIN_TEXT(Contains)
{
	strcpy(result, str_str(value, subfield) ? "1" : "0");
}


BUILTIN_TEXT(BeginsWith)
{
	int subLength = strlen(subfield);
	
	strcpy(result, subLength && !strn_cmp(value, subfield, subLength) ? "1" : "0");
}


BUILTIN_TEXT(EndsWith)
{
	int subLength = strlen(subfield);
	int valLength = strlen(value);
	int start = valLength - subLength;
	
	strcpy(result, subLength && start >= 0 && !str_cmp(value + start, subfield) ? "1" : "0");
}


BUILTIN_TEXT(Extract)
{
	int num = atoi(subfield);
	
	ScriptEngineListExtractWord(value, result, num);
}


BUILTIN_TEXT(ExtractParam)
{
	int num = atoi(subfield);
	
	const char *src = value;
	char *dst = result;
	while (*src)
	{
		if (--num == 0)
		{
			src = ScriptEngineParamsCopyNextParam(src, dst);
			if (*src && num > 1)	//	More to copy yet before skip?
				*dst++ = '\t';
		}
		else
		{
			src = ScriptEngineParamsSkipParam(src);
			if (*src)	//	More to copy yet?
				*dst++ = '\t';
		}
	}

	*dst = '\0';
}


BUILTIN_TEXT(Find)
{
	sprintf(result, "%d", *subfield ? ScriptEngineListFindWord(value, subfield) : 0);
}


BUILTIN_TEXT(FindByField)
{
	BUFFER(fieldName, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(fieldMatch, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(fieldValue, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, fieldName);
	subfield = ScriptEngineGetParameter(subfield, fieldMatch);
	
	const char *src = ScriptEngineSkipSpaces(value);
	
	int num = 0;
	
	while (*src)
	{
		++num;
		
		if (!ScriptEngineRecordIsRecordStart(src))
		{
			src = ScriptEngineListSkipWord(src);
			continue;
		}
		
		bool bFound = false;
		src = ScriptEngineRecordSkipToField(src, fieldName, bFound);
		
		if (bFound)
		{
			src = ScriptEngineRecordCopyField(src, fieldValue);
			
			bool bIsMatch = false;
			ScriptEngineListCompareWords(fieldMatch, fieldValue, bIsMatch);
			
			if (bIsMatch)
			{
				sprintf(result, "%d", num);
				return;
			}
		}
		
//		src = ScriptEngineRecordSkipDelimiter(src);
//		while (*src && !ScriptEngineRecordIsRecordEnd(src))
//		{
//			src = ScriptEngineRecordSkipField(src);
//		}
//		src = ScriptEngineRecordSkipDelimiter(src);

		if (!ScriptEngineRecordIsRecordEnd(src))
			src = ScriptEngineRecordSkipRecord(src);
		
		while (isspace(*src))	++src;
	}
	
	strcpy(result, "0");
}


BUILTIN_TEXT(Head)
{
	ScriptEngineListCopyNextWord(value, result);
}


BUILTIN_TEXT(Insert)
{
	BUFFER(where, MAX_SCRIPT_STRING_LENGTH);
	int	position;
	
	subfield = ScriptEngineGetParameter(subfield, where);
	position = atoi(where);
	
	ScriptEngineListInsert(value, result, subfield, position);
}


BUILTIN_TEXT(Param)
{
	int num = atoi(subfield);
	if (num > 0)
		ScriptEngineGetParameter(value, result, num);
}


typedef std::list<pcrecpp::RE *> RegexList;

static pcrecpp::RE *GetCachedRegex(const char *pat)
{
	static RegexList regexCache;

	/*	This is meant to help counter regex parsing costs, if they are significant */
	/*	... I don't really know if they are.  Therefore I'm erring on the side of caution. */
	const int			MAX_REGEX_CACHE = 25;
	pcrecpp::RE *		regex = NULL;
	
	FOREACH(RegexList, regexCache, iter)
	{
		if ((*iter)->pattern() == pat)
		{
			regex = *iter;
			regexCache.erase(iter);
			break;
		}
	}
	
	if (!regex)
	{
		if (regexCache.size() >= MAX_REGEX_CACHE)
		{
			delete regexCache.front();
			regexCache.pop_front();
		}
		
		regex = new pcrecpp::RE(pat, PCRE_CASELESS);
	}
	
	regexCache.push_back(regex);
	
	return regex;
}



BUILTIN_TEXT(Regex)
{
	if (!*subfield)
		return;
	
	static std::string			regexStrings[16];
	static pcrecpp::Arg			regexCaptures[16] = 
	{
		&regexStrings[0],&regexStrings[1],&regexStrings[2],&regexStrings[3],
		&regexStrings[4],&regexStrings[5],&regexStrings[6],&regexStrings[7],
		&regexStrings[8],&regexStrings[9],&regexStrings[10],&regexStrings[11],
		&regexStrings[12],&regexStrings[13],&regexStrings[14],&regexStrings[15]
	};
	static const pcrecpp::Arg*	regexCapturesArray[16] =
	{
		&regexCaptures[0],&regexCaptures[1],&regexCaptures[2],&regexCaptures[3],
		&regexCaptures[4],&regexCaptures[5],&regexCaptures[6],&regexCaptures[7],
		&regexCaptures[8],&regexCaptures[9],&regexCaptures[10],&regexCaptures[11],
		&regexCaptures[12],&regexCaptures[13],&regexCaptures[14],&regexCaptures[15]
	};
	
	pcrecpp::RE *		regex = GetCachedRegex(subfield);
	int					consumed;
	
	int	numArgs = MIN(regex->NumberOfCapturingGroups(), 16);
	
	ScriptEngineRecordConstructor record(result);
	
	if (numArgs < 0)
	{
		ScriptEngineReportError(thread, false, "bad regular expression: 'regex(%s)'", subfield);
		
		record.Add("success", 0);
		record.Add("matches", "");
//		strcpy(result, "[#success:0##matches:#]");
	}
	else if (!regex->DoMatch(value, pcrecpp::RE::UNANCHORED, &consumed, regexCapturesArray, numArgs))
	{
		record.Add("success", 0);
		record.Add("matches", "");
//		strcpy(result, "[#success:0##matches:#]");
	}
	else
	{
		record.Add("success", 1);
		record.StartField("matches");
		
//		strcpy(result, "[#success:1##matches:");
		
//		ScriptEngineListConstructor	lc(result + strlen(result));
		ScriptEngineListConstructor	lc(record.GetCursor());
	
		for (int i = 0; i < numArgs; ++i)
		{
			lc.AddParam(regexStrings[i].c_str());
		}
		
		record.EndField();
		
//		strcat(result, "#]");
	}
	
	record.Finished();
}



BUILTIN_TEXT(Replace)
{
	if (!*subfield)
		return;
	
	BUFFER(pat, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, pat);
	
	std::string out = value;
	GetCachedRegex(pat)->Replace(subfield, &out);
	strcpy(result, out.c_str());
}


BUILTIN_TEXT(ReplaceAll)
{
	if (!*subfield)
		return;
	
	BUFFER(pat, MAX_SCRIPT_STRING_LENGTH);
	
	subfield = ScriptEngineGetParameter(subfield, pat);
	
	std::string out = value;
	GetCachedRegex(pat)->GlobalReplace(subfield, &out);
	strcpy(result, out.c_str());
}


BUILTIN_TEXT(Strlen)
{
	sprintf(result, "%d", strlen(value));	
}


BUILTIN_TEXT(String)
{
	strcpy(result, value);
}


BUILTIN_TEXT(Splat)
{
	if (!*subfield)	return;
	
	int numTimes = atoi(subfield);
	
	if (numTimes < 0)
		ScriptEngineReportError(thread, false, "can't splat a negative number of times: '%s.splat(%d)'", value, numTimes);
	else if ((numTimes * strlen(value)) > 2048)
		ScriptEngineReportError(thread, false, "can't splat more than 2K of data: '%s.splat(%d)'", value, numTimes);
	else
	{
		for (; numTimes > 0; --numTimes)
		{
			strcat(result, value);
		}
	}
}


BUILTIN_TEXT(Trim)			//	trim whitespace
{
	const char *p = value;
	const char *p2 = value + strlen(value) - 1;
	skip_spaces(p);
	while ((p < p2) && isspace(*p2)) --p2;
	if (p == p2)	//	Nothing left
	{
		if (isspace(*p2))
			*result = '\0';
	}
	else
	{
		while (p <= p2)
			*result++ = *p++;
		*result = '\0';
	}
}


BUILTIN_TEXT(Tail)			//	Remainder
{
	const char *remainder = ScriptEngineListSkipWord(value);
	ScriptEngineListCopyRemaining(remainder, result);
}


BUILTIN_TEXT(Pop)			//	Skip one word
{
	const char *remainder = ScriptEngineListSkipWord(value);
	ScriptEngineListCopyRemaining(remainder, result);
}


BUILTIN_TEXT(Word)
{
	int num = atoi(subfield);
	if (num > 0)
		ScriptEngineListCopyWord(value, result, num);
}


BUILTIN_TEXT(Intersection)
{
	if (!*subfield)	return;
	
	ScriptEngineListConstructor	lc(result);
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
	while (*subfield)
	{
		char *dst = buf;
		subfield = ScriptEngineListCopyNextWord(subfield, dst);
		
		if (ScriptEngineListFindWord(value, buf))
		{
			lc.Add(buf);
		}
	}
}


BUILTIN_TEXT(Field)
{
	if (ScriptEngineRecordIsRecordStart(value))
	{
		//	Retrieve the field
		bool	found;
		const char *data = ScriptEngineRecordSkipToField(value, subfield, found);
		ScriptEngineRecordCopyField(data, result);
	}
}


BUILTIN_TEXT(StripColor)
{
	extern void ProcessColor(const char *src, char *dest, size_t len, bool bColor);
    ProcessColor(value, result, MAX_SCRIPT_STRING_LENGTH / 2, false);	//	This strips color
}


BUILTIN_TEXT(DoubleDollar)
{
	int len = MAX_SCRIPT_STRING_LENGTH / 2;
	char c = *value++;
	while (c)
	{
		if (c == '$')	*result++ = c;
		*result++ = c;
		c = *value++;
	}
	*result = '\0';
}


//	$
//	$ op Result
//	Field op Result
//	Field

BUILTIN_TEXT(Filter)
{
	const char *listToFilter = value;
	const char *filter = subfield;
	
	enum
	{
		FilterValues,
		FilterRecords,
		FilterEntities
	} filterType;
	bool bFilterWithOperation = false;
	int op = OP_NONE;
	
	BUFFER(filterField, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(filterSubfield, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(filterOperator, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(filterComparison, MAX_SCRIPT_STRING_LENGTH);
	
//	filter = ScriptParserParseEvaluationStatement(filter, field);
	
	//	Basic Filter parsing
	char *dst;
	char c = *filter++;
	if (c == '$')
	{
		filterType = FilterValues;	
		c = *filter++;
	}
	else
	{
		if (c == '#')
		{
			filterType = FilterRecords;
			c = *filter++;
		}
		else
			filterType = FilterEntities;
		
		dst = filterField;
		while (c && !isspace(c) && !ispunct(c))	{ *dst++ = c; c = *filter++; }
		*dst = 0;
			
		//	Field->subfield
		if (c == '(')
		{
			--filter;	//	Back up one
			const char *pp = ScriptParserMatchParenthesis(filter);
			filter = ScriptParserExtractContents(filter, pp, filterSubfield);
			c = *filter++;
		}
		else
		{
			filterSubfield = NULL;
		}
	}
	
	//	Skip spaces after the field
	while (c && isspace(c))					{ c = *filter++; }
	
	if (c)
	{
		dst = filterOperator;
		while (c && !isspace(c) && ispunct(c))	{ *dst++ = c; c = *filter++; }
		while (c && isspace(c))					{ c = *filter++; }
		*dst = 0;
		dst = filterComparison;
		while (c /* && !isspace(c)*/)				{ *dst++ = c; c = *filter++; }
		*dst = 0;
		
		extern char *ops[];
		op = search_block(filterOperator, ops, true);
		bFilterWithOperation = true;
		
		if (op == -1)
		{
			ScriptEngineReportError(thread, false, "bad filter: 'filter(%s)'", subfield);
			strcpy(result, "0");
			return;
		}
	}
	
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(derefResult, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(opResult, MAX_SCRIPT_STRING_LENGTH);
	
	listToFilter = ScriptEngineSkipSpaces(listToFilter);
	
	DereferenceData data;
	data.var		= buf;
	data.field		= filterField;
	data.subfield	= filterSubfield;
	data.result		= derefResult;
	
	*result = '\0';
	while (*listToFilter)
	{
		const char *filterValue = NULL;
		const char *filterResult;
		
		dst = buf;
		listToFilter = ScriptEngineListCopyNextWord(listToFilter, dst);
		listToFilter = ScriptEngineSkipSpaces(listToFilter);
	
		if (filterType == FilterValues)
		{
			//	Filter the value directly
			filterValue = buf;
		}
		else if (filterType == FilterRecords)
		{
			if (!ScriptEngineRecordIsRecordStart(buf))
				continue;
			
			ScriptEngineRecordGetField(buf, filterField, derefResult);
			filterValue = derefResult;
		}
		else if (filterType == FilterEntities)
		{
			if (ScriptEngineRecordIsRecordStart(buf))
			{
				ScriptEngineRecordGetField(buf, filterField, derefResult);
				filterValue = derefResult;
			}
			else if (IsIDString(buf) || IsExtendedDataString(buf))
			{
				ScriptEngineDereference(thread->m_pEntity->GetScript(), thread, data, true);
				filterValue = data.GetResult();
			}
			else
				continue;
		}

		if (bFilterWithOperation)
		{
			ScriptEngineOperation(thread, filterValue, filterComparison, op, opResult);
			filterResult = opResult;
		}
		else
		{
			filterResult = filterValue;
		}
		
		if (ScriptEngineOperationEvaluateAsBoolean(filterResult, str_type(filterResult)))
		{
			if (*result)	strcat(result, " ");
			strcat(result, buf);
		}
	}
}


BUILTIN_MEMBER(Entity, HasAttached)
{
	strcpy(result, entity->GetScript()->HasTrigger(subfield) ? "1" : "0");
}


BUILTIN_MEMBER(Entity, HasThread)
{
	BUFFER(target, MAX_STRING_LENGTH);
	BUFFER(threadid, MAX_STRING_LENGTH);
	
	ScriptEngineRecordGetField(subfield, "entity", target);
	ScriptEngineRecordGetField(subfield, "thread", threadid);
	
	strcpy(result, entity->GetID() == ParseIDString(target) && entity->GetScript()->GetThread(atoi(threadid)) ? "1" : "0");
}


BUILTIN_MEMBER(Entity, Hasfunction)
{
	bool bFound = false;
	
	Hash hash = GetStringFastHash(subfield);
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(entity)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGERS(t) == 0 && t->GetPrototype() != NULL)
		{
			if ( t->m_pCompiledScript->FindFunction(subfield, hash) )
			{
				bFound = true;
				break;
			}
		}
	}
	
	strcpy(result, bFound ? "1" : "0");
}


BUILTIN_MEMBER(Entity, Hasvariable)
{
	BUFFER(variable, MAX_SCRIPT_STRING_LENGTH);
	int context;

	ScriptEngineParseVariable(subfield, variable, context);
	
	strcpy(result, *subfield && ScriptEngineFindVariable(entity->GetScript(), thread, variable, 0, context, Scope::Global) != NULL ? "1" : "0");
}


BUILTIN_MEMBER(Entity, Id)
{
	sprintf(result, "%d", GET_ID(entity));
}


BUILTIN_MEMBER(Entity, Variable)
{
	BUFFER(variable, MAX_SCRIPT_STRING_LENGTH);
	int context;
	ScriptEngineParseVariable(subfield, variable, context);
	
	if (*variable)
	{
		ScriptVariable *vd = ScriptEngineFindVariable(entity->GetScript(), thread, variable, 0, context, Scope::Global | Scope::ContextSpecific);
		strcpy(result, vd ? vd->GetValue() : "");
	}
}





BUILTIN_MEMBER(CharData, Agi)
{
	sprintf(result, "%d", GET_AGI(entity));
}


BUILTIN_MEMBER(CharData, Coo)
{
	sprintf(result, "%d", GET_COO(entity));
}

BUILTIN_MEMBER(CharData, Hea)
{
	sprintf(result, "%d", GET_HEA(entity));
}

BUILTIN_MEMBER(CharData, Kno)
{
	sprintf(result, "%d", GET_KNO(entity));
}

BUILTIN_MEMBER(CharData, Per)
{
	sprintf(result, "%d", GET_PER(entity));
}


BUILTIN_MEMBER(CharData, Str)
{
	sprintf(result, "%d", GET_STR(entity));
}



BUILTIN_MEMBER(CharData, UnmodAgi)
{
	sprintf(result, "%d", GET_REAL_AGI(entity));
}


BUILTIN_MEMBER(CharData, UnmodCoo)
{
	sprintf(result, "%d", GET_REAL_COO(entity));
}

BUILTIN_MEMBER(CharData, UnmodHea)
{
	sprintf(result, "%d", GET_REAL_HEA(entity));
}

BUILTIN_MEMBER(CharData, UnmodKno)
{
	sprintf(result, "%d", GET_REAL_KNO(entity));
}

BUILTIN_MEMBER(CharData, UnmodPer)
{
	sprintf(result, "%d", GET_REAL_PER(entity));
}


BUILTIN_MEMBER(CharData, UnmodStr)
{
	sprintf(result, "%d", GET_REAL_STR(entity));
}

BUILTIN_MEMBER(CharData, Age)
{
	sprintf(result, "%d", GET_AGE(entity));
}


EXTENDEDDATA_HANDLER(CharacterAIData, "AI")
{
public:
	CharacterAIData() : ch(NULL) {}
	
	bool Parse(Params &params)
	{
		params >> ch;
		return (ch != NULL);
	}
	
	CharData *			ch;
	
	static void			MakeDataString(char *result, CharData *ch)
	{
		MakeDataStringInternal(result, ParamWriter() << ch);
	}
};
REGISTER_EXTENDEDDATA_HANDLER(CharacterAIData);


BUILTIN_MEMBER(CharData, AI)
{
	if (MOB_FLAGGED(entity, MOB_AI))
	{
		CharacterAIData::MakeDataString(result, entity);
	}
	else
		strcpy(result, "0");
}

BUILTIN_EXTENDEDDATA_MEMBER(CharacterAIData, aggrroom)		{ ScriptEnginePrintInteger(result, handler->ch->mob->m_AIAggrRoom); }
BUILTIN_EXTENDEDDATA_MEMBER(CharacterAIData, aggrranged)	{ ScriptEnginePrintInteger(result, handler->ch->mob->m_AIAggrRanged); }
BUILTIN_EXTENDEDDATA_MEMBER(CharacterAIData, aggrloyalty)	{ ScriptEnginePrintInteger(result, handler->ch->mob->m_AIAggrLoyalty); }
BUILTIN_EXTENDEDDATA_MEMBER(CharacterAIData, awarerange)	{ ScriptEnginePrintInteger(result, handler->ch->mob->m_AIAwareRange); }
BUILTIN_EXTENDEDDATA_MEMBER(CharacterAIData, moverate)		{ ScriptEnginePrintInteger(result, handler->ch->mob->m_AIMoveRate); }




BUILTIN_MEMBER(CharData, Alias)
{
	strcpy(result, entity->GetAlias());
}


BUILTIN_MEMBER(CharData, Affects)
{
	strcpy(result, AFF_FLAGS(entity).print().c_str());
}


BUILTIN_MEMBER(CharData, Affecttypes)
{
	*result = 0;
	
	FOREACH(Lexi::List<Affect *>, entity->m_Affects, aff)
	{
		const char *	type = (*aff)->GetType();
		
		if (*result)
			strcat(result, " ");
		strcat(result, type);
	}
}

BUILTIN_MEMBER(CharData, BonusArmor)
{
	sprintf(result, "%d", GET_EXTRA_ARMOR(entity));
}


BUILTIN_MEMBER(CharData, Cansee)
{
	CharData *ch = NULL;
	ObjData *obj = NULL;
	
	if ((ch = get_char_by_char(entity, subfield)))		sprintf(result, "%d", entity->CanSee(ch));
	else if ((obj = get_obj_by_char(entity, subfield)))	strcpy(result, entity->CanSee(obj) ? "1" : "0");
	else											strcpy(result, "0");
}


BUILTIN_MEMBER(CharData, Canuse)
{
	ObjData *obj = NULL;
	
	if ((obj = get_obj_by_char(entity, subfield)))		strcpy(result, entity->CanUse(obj) == NotRestricted ? "1" : "0");
	else											strcpy(result, "0");
}


BUILTIN_MEMBER(CharData, Cango)
{
	int dir = search_block(subfield, dirs, false);
	
	strcpy(result, dir != -1 && RoomData::CanGo(entity, dir) ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Carrying)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(ObjectList, entity->carrying, obj)
	{
		lc.Add(*obj);
	}
}


BUILTIN_MEMBER(CharData, Channels)
{
	sprintbit(entity->GetPlayer() ? entity->GetPlayer()->m_Channels : 0, channel_bits, result);
}


BUILTIN_MEMBER(CharData, Clan)
{
	if (GET_CLAN(entity) && (IS_NPC(entity) || Clan::GetMember(entity)))
		sprintf(result, "%d", GET_CLAN(entity)->GetID());
//		ClanData::MakeDataString(result, GET_CLAN(entity));
}


BUILTIN_MEMBER(CharData, hasClanPermission)
{
	int bit = GetEnumByName<Clan::RankPermissions>(subfield);
	if (bit != -1 && !IS_NPC(entity) && GET_CLAN(entity) && Clan::GetMember(entity))
		strcpy(result, Clan::HasPermission(entity, (Clan::RankPermissions)bit) ? "1" : "0");
}

BUILTIN_MEMBER(CharData, ClanRank)
{
	if (!IS_NPC(entity) && GET_CLAN(entity) && Clan::GetMember(entity))
		ClanRankData::MakeDataString(result, GET_CLAN(entity), Clan::GetMember(entity)->m_pRank);
}


BUILTIN_MEMBER(CharData, Condition)
{
	const char *cond;
	int percent = -1;
	
	if (GET_MAX_HIT(entity) > 0)
		percent = (100 * GET_HIT(entity)) / GET_MAX_HIT(entity);

	if (MOB_FLAGGED(entity, MOB_MECHANICAL))
	{
		if (percent >= 100)			cond = "excellent condition";
		else if (percent >= 90)		cond = "a little bit of damage";
		else if (percent >= 75)		cond = "some small holes and tears";
		else if (percent >= 50)		cond = "quite a bit of damage";
		else if (percent >= 30)		cond = "big nasty holes and tears";
		else if (percent >= 15)		cond = "pretty beaten up";
		else if (percent >= 0)		cond = "awful condition";
		else						cond = "broken and short circuiting";
	}
	else
	{
		if (percent >= 100)			cond = "excellent condition";
		else if (percent >= 90)		cond = "a few scratches";
		else if (percent >= 75)		cond = "some small wounds and bruises";
		else if (percent >= 50)		cond = "quite a few wounds";
		else if (percent >= 30)		cond = "big nasty wounds and scratches";
		else if (percent >= 15)		cond = "pretty hurt";
		else if (percent >= 0)		cond = "awful condition";
		else						cond = "bleeding awfully from big wounds";
	}
	
	strcpy(result, result);
}


BUILTIN_MEMBER(CharData, Description)
{
	strcpy(result, entity->GetDescription());
}


//BUILTIN_MEMBER(CharData, En)		sprintf(result, "%d", GET_ENDURANCE(entity));			break;

BUILTIN_MEMBER(CharData, Enemy)
{
	strcpy(result, ((GET_THREAD_ENTITY(thread)->GetEntityType() == Entity::TypeCharacter) &&
				(entity->GetRelation((CharData *)GET_THREAD_ENTITY(thread)) == RELATION_ENEMY)) ? "1" : "0");
}

BUILTIN_MEMBER(CharData, Eq)
{
	ScriptEngineListConstructor	lc(result);
	
	for (int i = 0; i < NUM_WEARS; ++i)
	{
		if (GET_EQ(entity, i))
			lc.Add(GET_EQ(entity, i));
	}
}


BUILTIN_MEMBER(CharData, Faction)
{
	sprinttype(entity->GetFaction(), faction_types, result);
}


BUILTIN_MEMBER(CharData, Friend)
{
	strcpy(result, ((GET_THREAD_ENTITY(thread)->GetEntityType() == Entity::TypeCharacter) &&
				(entity->GetRelation((CharData *)GET_THREAD_ENTITY(thread)) == RELATION_FRIEND)) ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Fighting)
{
	strcpy(result, ScriptEngineGetUIDString(FIGHTING(entity)));
}


BUILTIN_MEMBER(CharData, Findtarget)
{
	if (subfield && *subfield)
	{
		BUFFER(name, MAX_STRING_LENGTH);
		BUFFER(rangeOrWeapon, MAX_STRING_LENGTH);
		int			range = 0;
		ObjData *	weapon = NULL;
		bool		isGun = false;
		int			direction = -1;
		CharData *	target;
		int			distance;
		
		subfield = ScriptEngineGetParameter(subfield, name);
		subfield = ScriptEngineGetParameter(subfield, rangeOrWeapon);
		
		if (IsIDString(rangeOrWeapon))
		{
			weapon = ObjData::Find(ParseIDString(rangeOrWeapon));
			if (weapon && IS_GUN(weapon))
			{
				range = GET_GUN_RANGE(weapon);
				isGun = true;
			}
		}
		else
		{
			range = atoi(rangeOrWeapon);
			if (range < 0)
			{
				range = -range;
				isGun = true;
			}
		}
		
		if (*subfield)
		{
			if (is_number(subfield))	direction = atoi(subfield);
			else						direction = search_block(subfield, dirs, false);
		}
		
		if (IsIDString(name))
		{
			target = CharData::Find(ParseIDString(name));
			
			if (target)
				target = scan_target(entity, target, range, direction, &distance, isGun);
		}
		else
		{
			target = scan_target(entity, name, range, direction, &distance, isGun);									
		}
		
		if (target)
		{
			sprintf(result, "%s %s %d", ScriptEngineGetUIDString(target), dirs[direction], distance);
		}
		else
		{
			strcpy(result, "0");
		}
	}							
}


BUILTIN_MEMBER(CharData, Followers)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(CharacterList, entity->m_Followers, follower)
	{
		lc.Add(*follower);
	}
}


BUILTIN_MEMBER(CharData, Group)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(CharacterList, entity->m_Followers, follower)
	{
		if (AFF_FLAGGED(*follower, AFF_GROUP))
			lc.Add(*follower);
	}
}


BUILTIN_MEMBER(CharData, Height)
{
	sprintf(result, "%d", entity->GetHeight());
}


BUILTIN_MEMBER(CharData, Heshe)
{
	strcpy(result, HSSH(entity));
}


BUILTIN_MEMBER(CharData, Himher)
{
	strcpy(result, HMHR(entity));
}


BUILTIN_MEMBER(CharData, Hisher)
{
	strcpy(result, HSHR(entity));
}


BUILTIN_MEMBER(CharData, Hitpercent)
{
	sprintf(result, "%d", GET_HIT(entity) * 100 / GET_MAX_HIT(entity));
}


BUILTIN_MEMBER(CharData, Hp)
{
	sprintf(result, "%d", GET_HIT(entity));
}


BUILTIN_MEMBER(CharData, Hasskill)
{
	if (subfield && *subfield)
	{
		int num = find_skill_abbrev(subfield);
		if (num)
			strcpy(result, skill_info[num].min_level[entity->GetRace()] <= entity->GetLevel() ?
				skill_info[num].name : "0");
		else
		{
//										ScriptEngineReportError(thread, false, "unknown skill name: 'hasskill(%s)'", subfield);
			strcpy(result, "0");
		}
	}
	else
		strcpy(result, "0");
}


BUILTIN_MEMBER(CharData, Idle)
{
	sprintf(result, "%d", IS_NPC(entity) ? 0 : entity->GetPlayer()->m_IdleTime);
}


BUILTIN_MEMBER(CharData, Immobilized)
{
	strcpy(result, GET_POS(entity) < POS_RESTING ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Invis)
{
	int invisValue = (AFF_FLAGGED(entity, AFF_INVISIBLE) ? 1 : 0) 
				   + (AFF_FLAGGED(entity, AFF_INVISIBLE_2) ? 2 : 0);
		
	sprintf(result, "%d", invisValue);
}


BUILTIN_MEMBER(CharData, Iscarrying)
{
	find_obj_in_contents(thread, entity->carrying, subfield, result);
}


BUILTIN_MEMBER(CharData, Iscarryingdeep)
{
	find_obj_in_contents(thread, entity->carrying, subfield, result, true);
}


BUILTIN_MEMBER(CharData, Iswearing)
{
	find_obj_in_equipment(thread, entity, subfield, result);
}


BUILTIN_MEMBER(CharData, Iswearingdeep)
{
	find_obj_in_equipment(thread, entity, subfield, result, true);
}


BUILTIN_MEMBER(CharData, IsName)
{
	strcpy(result, silly_isname(subfield, entity->GetAlias()) ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Ismarine)
{
	strcpy(result, entity->GetFaction() == RACE_HUMAN ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Karma)
{
	sprintf(result, "%d", entity->GetPlayer()->m_Karma);
}


BUILTIN_MEMBER(CharData, Level)
{
	sprintf(result, "%d", entity->GetLevel());
}


BUILTIN_MEMBER(CharData, Lifemp)
{
	sprintf(result, "%d", GET_LIFEMP(entity));
}


BUILTIN_MEMBER(CharData, LoadRoom)
{
	if (IS_NPC(entity))	strcpy(result, "0");
	else	
	{
		RoomData *room = entity->GetPlayer()->m_LoadRoom.IsValid() ? world.Find(entity->GetPlayer()->m_LoadRoom) : NULL;
		strcpy(result, ScriptEngineGetUIDString(room));
	}	
}


BUILTIN_MEMBER(CharData, StartRoom)
{
	if (IS_NPC(entity))	strcpy(result, "0");
	else				strcpy(result, ScriptEngineGetUIDString(entity->StartRoom()));
}


BUILTIN_MEMBER(CharData, Maxhp)
{
	sprintf(result, "%d", GET_MAX_HIT(entity));
}


BUILTIN_MEMBER(CharData, Maxmv)
{
	sprintf(result, "%d", GET_MAX_MOVE(entity));
}


//BUILTIN_MEMBER(CharData, Maxen)	sprintf(result, "%d", GET_MAX_ENDURANCE(c));		break;

BUILTIN_MEMBER(CharData, Master)
{
	strcpy(result, ScriptEngineGetUIDString(entity->m_Following));
}


BUILTIN_MEMBER(CharData, Mdeaths)
{
	sprintf(result, "%d", IS_NPC(entity) ? 0 : entity->GetPlayer()->m_MobDeaths);
}


BUILTIN_MEMBER(CharData, Mkills)
{
	sprintf(result, "%d", IS_NPC(entity) ? 0 : entity->GetPlayer()->m_MobKills);
}
	
	
BUILTIN_MEMBER(CharData, Mobbits)
{
	sprintbit(IS_NPC(entity) ? MOB_FLAGS(entity) : 0, action_bits, result);
}


BUILTIN_MEMBER(CharData, Mp)
{
	sprintf(result, "%d", GET_MISSION_POINTS(entity));
}


BUILTIN_MEMBER(CharData, Mv)
{
	sprintf(result, "%d", GET_MOVE(entity));
}


BUILTIN_MEMBER(CharData, Name)
{
	strcpy(result, entity->GetName());
}


BUILTIN_MEMBER(CharData, Npc)
{
	strcpy(result, IS_NPC(entity) ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Neutral)
{
	CharData *	threadChar = (GET_THREAD_ENTITY(thread)->GetEntityType() == Entity::TypeCharacter)
		? (CharData *)GET_THREAD_ENTITY(thread)
		: NULL;
		
	strcpy(result, (threadChar && (entity->GetRelation(threadChar) == RELATION_NEUTRAL)) ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Prefs)
{
	sprintbit(IS_NPC(entity) ? 0 : PRF_FLAGS(entity), preference_bits, result);
}


BUILTIN_MEMBER(CharData, Playerbits)
{
	sprintbit(IS_NPC(entity) ? 0 : PLR_FLAGS(entity), player_bits, result);
}


BUILTIN_MEMBER(CharData, Pdeaths)
{
	sprintf(result, "%d", IS_NPC(entity) ? 0 : entity->GetPlayer()->m_PlayerDeaths);
}


BUILTIN_MEMBER(CharData, Pkills)
{
	sprintf(result, "%d", IS_NPC(entity) ? 0 : entity->GetPlayer()->m_PlayerKills);
}


BUILTIN_MEMBER(CharData, Position)
{
	strcpy(result, GetEnumName(GET_POS(entity)));
}


BUILTIN_MEMBER(CharData, Pretitle)
{
	strcpy(result, IS_NPC(entity) ? "" : entity->GetPlayer()->m_Pretitle.c_str());
}


BUILTIN_MEMBER(CharData, Race)
{
	sprinttype(entity->GetRace(), race_types, result);
}


BUILTIN_MEMBER(CharData, Realskill)
{
	if (subfield && *subfield)
	{
		int num = find_skill_abbrev(subfield);
		if (num)
			sprintf(result, "%d", GET_SKILL(entity, num));
		else
		{
			ScriptEngineReportError(thread, false, "unknown skill name: 'realskill(%s)'", subfield);
			strcpy(result, "0");
		}
	}
	else
		strcpy(result, "0");
}


BUILTIN_MEMBER(CharData, Realunmodskill)
{
	if (subfield && *subfield)
	{
		int num = find_skill_abbrev(subfield);
		if (num)
			sprintf(result, "%d", GET_REAL_SKILL(entity, num));
		else
		{
			ScriptEngineReportError(thread, false, "unknown skill name: 'realunmodskill(%s)'", subfield);
			strcpy(result, "0");
		}
	}
	else
		strcpy(result, "0");
}



BUILTIN_MEMBER(CharData, Relation)
{
	if (subfield && *subfield)
	{
		CharData *ch = NULL;
	
		if ((ch = get_char_by_char(entity, subfield)))
			strcpy(result, GetEnumName(entity->GetRelation(ch)));
		else
			strcpy(result, "0");
	}
	else
		strcpy(result, (GET_THREAD_ENTITY(thread)->GetEntityType() == Entity::TypeCharacter) ?
			GetEnumName(entity->GetRelation((CharData *)GET_THREAD_ENTITY(thread))) : "0");
}


BUILTIN_MEMBER(CharData, Room)
{
/*	if (entity->InRoom()->GetVirtualID().IsValid())	strcpy(result, entity->InRoom()->GetVirtualID().Print().c_str());
	else*/								strcpy(result, ScriptEngineGetUIDString(entity->InRoom()));
}


BUILTIN_MEMBER(CharData, Sex)
{
	strcpy(result, GetEnumName(entity->GetSex()));
}
	
	
BUILTIN_MEMBER(CharData, Skill)
{
	if (subfield && *subfield)
	{
		int num = find_skill_abbrev(subfield);
		if (num)
			ScriptEnginePrintDouble(result, entity->GetEffectiveSkill(num));
		else
		{
			ScriptEngineReportError(thread, false, "unknown skill name: 'skill(%s)'", subfield);
			strcpy(result, "0");
		}
	}
	else
		strcpy(result, "0");
}


BUILTIN_MEMBER(CharData, Skillroll)
{
	if (subfield && *subfield)
	{
		int num = find_skill_abbrev(subfield);
		if (num)
			ScriptEnginePrintDouble(result, SkillRoll(entity->GetEffectiveSkill(num)));
		else
		{
			ScriptEngineReportError(thread, false, "unknown skill name: 'skillroll(%s)'", subfield);
			strcpy(result, "-100");
		}
	}
	else
		strcpy(result, "0");
}


BUILTIN_MEMBER(CharData, Staff)
{
	strcpy(result, NO_STAFF_HASSLE(entity) ? "1" : "0");
}

	
BUILTIN_MEMBER(CharData, Sitting)
{
	strcpy(result, ScriptEngineGetUIDString(entity->sitting_on));
}


BUILTIN_MEMBER(CharData, Targetcharacter)
{
	CharData *ch = NULL;
		
	if (subfield && *subfield)
	{
		ch = get_char_vis(entity, subfield, FIND_CHAR_ROOM);
//		generic_find(subfield, FIND_CHAR_ROOM, entity, &ch, NULL);
	}
	
	strcpy(result, ScriptEngineGetUIDString(ch));
}


BUILTIN_MEMBER(CharData, Targetobject)
{
	ObjData *obj = NULL;
	
	if (subfield && *subfield)
	{
		generic_find(subfield, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, entity, NULL, &obj);
	}

	strcpy(result, ScriptEngineGetUIDString(obj));
}


BUILTIN_MEMBER(CharData, Targetobjectequipment)
{
	ObjData *obj = NULL;
		
	if (subfield && *subfield)
	{
		generic_find(subfield, FIND_OBJ_EQUIP, entity, NULL, &obj);
	}
	
	strcpy(result, ScriptEngineGetUIDString(obj));
}


BUILTIN_MEMBER(CharData, Targetobjectinventory)
{
	ObjData *obj = NULL;
		
	if (subfield && *subfield)
	{
		generic_find(subfield, FIND_OBJ_INV, entity, NULL, &obj);
	}
	
	strcpy(result, ScriptEngineGetUIDString(obj));
}


BUILTIN_MEMBER(CharData, Targetobjectroom)
{
	ObjData *obj = NULL;
		
	if (subfield && *subfield)
	{
		generic_find(subfield, FIND_OBJ_ROOM, entity, NULL, &obj);
	}
	
	strcpy(result, ScriptEngineGetUIDString(obj));
}

	
BUILTIN_MEMBER(CharData, Team)
{
	strcpy(result, GET_TEAM(entity) == 0 ? "0" : team_names[GET_TEAM(entity)]);
}


BUILTIN_MEMBER(CharData, Title)
{
	strcpy(result, entity->m_Title.c_str());
}


BUILTIN_MEMBER(CharData, TotalInGame)
{
	sprintf(result, "%d", entity->GetPrototype() ? entity->GetPrototype()->m_Count : 1);
}


BUILTIN_MEMBER(CharData, Traitor)
{
	strcpy(result, AFF_FLAGGED(entity, AFF_TRAITOR) ? "1" : "0");
}


BUILTIN_MEMBER(CharData, Varclass)
{
	strcpy(result, "character");
}


BUILTIN_MEMBER(CharData, Vnum)
{
	sprintf(result, "%d", VNumLegacy::GetVirtualNumber(entity->GetVirtualID()));
}


BUILTIN_MEMBER(CharData, vid)
{
	strcpy(result, entity->GetVirtualID().IsValid() ? entity->GetVirtualID().Print().c_str() : "0");
}


BUILTIN_MEMBER(CharData, Wearing)
{
	if (subfield && *subfield)
	{
		int pos = search_block(subfield, eqpos, 0);
		if ((pos == -1) && is_number(subfield))
			pos = atoi(subfield);

		if (pos >= NUM_WEARS) {
			ObjData *weapon = NULL;
			
			if (pos == (NUM_WEARS + 0))			weapon = entity->GetWeapon();
			else if (pos == (NUM_WEARS + 1))	weapon = entity->GetSecondaryWeapon();
			else if (pos == (NUM_WEARS + 2))	weapon = entity->GetGun();
			
			if (weapon)		pos = weapon->WornOn();
			else			pos = -1;
		}
		
		if ((pos >= 0) && (pos < NUM_WEARS) && GET_EQ(entity, pos))
			strcpy(result, ScriptEngineGetUIDString(GET_EQ(entity, pos)));
	}
	else
	{
	
		ScriptEngineListConstructor	lc(result);
	
		for (int i = 0; i < NUM_WEARS; ++i)
			if (GET_EQ(entity, i))
				lc.Add(GET_EQ(entity, i));
	}
}


BUILTIN_MEMBER(CharData, Weight)
{
	sprintf(result, "%d", entity->GetWeight());
}





BUILTIN_MEMBER(ObjData, Actiondesc)
{
	strcpy(result, entity->GetDescription());
}


BUILTIN_MEMBER(ObjData, Affects)
{
	strcpy(result, entity->m_AffectFlags.print().c_str());
}


BUILTIN_MEMBER(ObjData, AffectMods)
{
	ScriptEngineListConstructor lc(result);
	
	for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
	{
		const ObjAffectedType &aff = entity->affect[i];
		if (!aff.modifier)
			continue;
		
		ScriptEngineRecordConstructor record(lc);
		
//		lc.AddFormat(
//			"[#type:%s"
//			"##amount:%d"
//			"#]",
//			aff.location < 0
//				? skill_name(-aff.location)
//				: GetEnumName((AffectLoc)aff.location),
//			aff.modifier);
		record.Add("type",
			aff.location < 0
				? skill_name(-aff.location)
				: GetEnumName((AffectLoc)aff.location));
		record.Add("amount", aff.modifier);

		record.Finished();
	}
}


BUILTIN_MEMBER(ObjData, Buyer)
{
	strcpy(result, entity->buyer ? MakeIDString(entity->buyer) : "0");
}


BUILTIN_MEMBER(ObjData, Bought)
{
	sprintf(result, "%u", (unsigned)entity->bought);
}


BUILTIN_MEMBER(ObjData, Contents)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(ObjectList, entity->contents, obj)
		lc.Add(*obj);
}


BUILTIN_MEMBER(ObjData, Clan)
{
	sprintf(result, "%d", entity->GetClanRestriction());
}


BUILTIN_MEMBER(ObjData, Cost)
{
	sprintf(result, "%d", GET_OBJ_COST(entity));
}


BUILTIN_MEMBER(ObjData, Description)
{
	strcpy(result, entity->GetRoomDescription());
}


BUILTIN_MEMBER(ObjData, Extra)
{
	sprintbit(GET_OBJ_EXTRA(entity), extra_bits, result);
}


BUILTIN_MEMBER(ObjData, Inside)
{
	strcpy(result, ScriptEngineGetUIDString(entity->Inside()));
}



EXTENDEDDATA_HANDLER(ObjectGunData, "Gun")
{
public:
	ObjectGunData() : obj(NULL) {}
	
	bool Parse(Params &params)
	{
		params >> obj;
		return (obj != NULL) && IS_GUN(obj);
	}
	
	ObjData *			obj;
	
	static void			MakeDataString(char *result, ObjData *obj)
	{
		MakeDataStringInternal(result, ParamWriter() << obj);
	}
};
REGISTER_EXTENDEDDATA_HANDLER(ObjectGunData);


EXTENDEDDATA_HANDLER(ObjectGunAmmoData, "GunAmmo")
{
public:
	ObjectGunAmmoData() : obj(NULL) {}
	
	bool Parse(Params &params)
	{
		params >> obj;
		return (obj != NULL) && IS_GUN(obj);
	}
	
	ObjData *			obj;
	
	static void			MakeDataString(char *result, ObjData *obj)
	{
		MakeDataStringInternal(result, ParamWriter() << obj);
	}
};
REGISTER_EXTENDEDDATA_HANDLER(ObjectGunAmmoData);



BUILTIN_MEMBER(ObjData, Gun)
{
	if (IS_GUN(entity))
	{
		ObjectGunData::MakeDataString(result, entity);
	}
}


BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, skill)
{
	if (GET_GUN_SKILL(handler->obj) > 0 && GET_GUN_SKILL(handler->obj) < NUM_SKILLS)
		strcpy(result, skill_name(GET_GUN_SKILL(handler->obj)));
}

BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, skill2)
{
	if (GET_GUN_SKILL2(handler->obj) > 0 && GET_GUN_SKILL2(handler->obj) < NUM_SKILLS)
		strcpy(result, skill_name(GET_GUN_SKILL2(handler->obj)));
}

BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, attack)		{ strcpy(result, findtype(GET_GUN_ATTACK_TYPE(handler->obj), attack_types)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, damage)		{ sprintf(result, "%d", GET_GUN_DAMAGE(handler->obj)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, damagetype)	{ strcpy(result, findtype(GET_GUN_DAMAGE_TYPE(handler->obj), damage_types)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, rate)		{ sprintf(result, "%d", GET_GUN_RATE(handler->obj)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, range)		{ sprintf(result, "%d", GET_GUN_RANGE(handler->obj)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, optimalrange){ sprintf(result, "%d", GET_GUN_OPTIMALRANGE(handler->obj)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, ammotype)	{ strcpy(result, findtype(GET_GUN_AMMO_TYPE(handler->obj), ammo_types)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, ammovnum)	{ strcpy(result, GET_GUN_AMMO(handler->obj) > 0 ? GET_GUN_AMMO_VID(handler->obj).Print().c_str() : ""); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, ammovid)		{ strcpy(result, GET_GUN_AMMO(handler->obj) > 0 ? GET_GUN_AMMO_VID(handler->obj).Print().c_str() : ""); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, ammoamount)	{ sprintf(result, "%d", GET_GUN_AMMO(handler->obj)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunData, ammo)		{ ObjectGunAmmoData::MakeDataString(result, handler->obj); }

BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunAmmoData, type)	{ strcpy(result, findtype(GET_GUN_AMMO_TYPE(handler->obj), ammo_types)); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunAmmoData, vid)		{ strcpy(result, GET_GUN_AMMO(handler->obj) > 0 ? GET_GUN_AMMO_VID(handler->obj).Print().c_str() : ""); }
BUILTIN_EXTENDEDDATA_MEMBER(ObjectGunAmmoData, amount)	{ sprintf(result, "%d", GET_GUN_AMMO(handler->obj)); }


BUILTIN_MEMBER(ObjData, Iscontents)
{
	find_obj_in_contents(thread, entity->contents, subfield, result);
}


BUILTIN_MEMBER(ObjData, Iscontentsdeep)
{
	find_obj_in_contents(thread, entity->contents, subfield, result, true);
}


BUILTIN_MEMBER(ObjData, IsName)
{
	strcpy(result, silly_isname(subfield, entity->GetAlias()) ? "1" : "0");
}


BUILTIN_MEMBER(ObjData, Level)
{
	sprintf(result, "%d", entity->GetMinimumLevelRestriction());
}


BUILTIN_MEMBER(ObjData, Name)
{
	strcpy(result, entity->GetAlias());
}


BUILTIN_MEMBER(ObjData, Owner)
{
	while (entity->Inside())
		entity = entity->Inside();

	strcpy(result, ScriptEngineGetUIDString(entity->CarriedBy() ? entity->CarriedBy() : entity->WornBy()));
}


BUILTIN_MEMBER(ObjData, Reloaded)
{
	strcpy(result, OBJ_FLAGGED(entity, ITEM_RELOADED) ? "1" : "0");
}


BUILTIN_MEMBER(ObjData, Room)
{
	RoomData *room = entity->Room();
/*	if (room && room->GetVirtualID().IsValid())	strcpy(result, room->GetVirtualID().Print().c_str());
	else*/							strcpy(result, ScriptEngineGetUIDString(room));
}


BUILTIN_MEMBER(ObjData, Racerestriction)
{
	strcpy(result, GetDisplayRaceRestrictionFlags(entity->GetRaceRestriction()).c_str());
}


BUILTIN_MEMBER(ObjData, Shortdesc)
{
	strcpy(result, entity->GetName());
}


BUILTIN_MEMBER(ObjData, Type)
{
	strcpy(result, GetEnumName(GET_OBJ_TYPE(entity)));
}


BUILTIN_MEMBER(ObjData, Timer)
{
	sprintf(result, "%d", GET_OBJ_TIMER(entity));
}


BUILTIN_MEMBER(ObjData, TotalInGame)
{
	sprintf(result, "%d", entity->GetPrototype() ? entity->GetPrototype()->m_Count : 1);
}


BUILTIN_MEMBER(ObjData, Val0)	//	LEGACY SUPPORT
{
//	const ObjectDefinition * definition = ObjectDefinition::Get(GET_OBJ_TYPE(entity));
//	ObjectDefinition::ValueMap::const_iterator i = definition->values.find((ObjectTypeValue)0);
	
//	if (i != definition->values.end() && i->second.GetType() == ObjectDefinition::Value::TypeVirtualID)
//	{
//		strcpy(result, VirtualID::from_pair(entity->m_Value[0], entity->m_Value[1]).Print().c_str());
//		return;	
//	}
	
	if (GET_OBJ_TYPE(entity) == ITEM_VEHICLE)
		strcpy(result, entity->GetVIDValue((ObjectTypeValue)0).Print().c_str());
	else
		sprintf(result, "%d", entity->GetValue((ObjectTypeValue)0));
}


BUILTIN_MEMBER(ObjData, Value)
{
	if (subfield && *subfield)
	{
		const ObjectDefinition * definition = ObjectDefinition::Get(GET_OBJ_TYPE(entity));
		
		FOREACH_CONST(ObjectDefinition::ValueMap, definition->values, iter)
		{
			const ObjectDefinition::Value &value = *iter;
			ObjectTypeValue	slot = value.GetSlot();
			
			if (str_cmp(value.GetName(), subfield))
				continue;
			
			switch (value.GetType())
			{
				case ObjectDefinition::Value::TypeInteger:
					sprintf(result, "%d", entity->GetValue(slot));
					break;
				case ObjectDefinition::Value::TypeEnum:
					sprinttype(entity->GetValue(slot), const_cast<char **>(value.nametable), result);
					break;
				case ObjectDefinition::Value::TypeFlags:
					sprintbit(entity->GetValue(slot), const_cast<char **>(value.nametable), result);
					break;
				case ObjectDefinition::Value::TypeSkill:
					strcpy(result, skill_name(entity->GetValue(slot)));
					break;
				case ObjectDefinition::Value::TypeVirtualID:
					strcpy(result, entity->m_VIDValue[slot].Print().c_str());
					break;
				case ObjectDefinition::Value::TypeFloat:
					ScriptEnginePrintDouble(result, entity->GetFloatValue(slot));
					break;
			}
			
			break;
		}
	}
}

BUILTIN_MEMBER(ObjData, Varclass)
{
	strcpy(result, "object");
}


BUILTIN_MEMBER(ObjData, Vnum)
{
	sprintf(result, "%d", VNumLegacy::GetVirtualNumber(entity->GetVirtualID()));
}


BUILTIN_MEMBER(ObjData, vid)
{
	strcpy(result, entity->GetVirtualID().IsValid() ? entity->GetVirtualID().Print().c_str() : "0");
}


BUILTIN_MEMBER(ObjData, Wear)
{
	sprintbit(GET_OBJ_WEAR(entity), wear_bits, result);
}


BUILTIN_MEMBER(ObjData, Weight)
{
	sprintf(result, "%d", GET_OBJ_WEIGHT(entity));
}



BUILTIN_MEMBER(ObjData, TotalWeight)
{
	sprintf(result, "%d", GET_OBJ_TOTAL_WEIGHT(entity));
}


BUILTIN_MEMBER(ObjData, Worn)
{
	strcpy(result, entity->WornBy() ? eqpos[entity->WornOn()] : "0");
}



BUILTIN_MEMBER(RoomData, Contents)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(ObjectList, entity->contents, obj)
		lc.Add(*obj);
}

EXTENDEDDATA_HANDLER(ExitData, "Exit")
{
public:
	ExitData() : room(NULL), dir(INVALID_DIRECTION), exit(NULL) {}
	
	bool Parse(Params &params)
	{
		params >> room;
		params >> dir;
		exit = room ? Exit::Get(room, dir) : NULL;
		
		return (exit != NULL);
	}
	
	RoomData *			room;
	int					dir;
	RoomExit *		exit;
	
	static void			MakeDataString(char *result, RoomData *room, Direction dir)
	{
		MakeDataStringInternal(result, ParamWriter() << room->GetID() << dir);
	}
};
REGISTER_EXTENDEDDATA_HANDLER(ExitData);


BUILTIN_MEMBER(RoomData, NewExit)
{
	if (subfield && *subfield)
	{
		int dir = search_block(subfield, dirs, false);
		RoomExit *exit = Exit::Get(entity, dir);
		
		if (exit)		ExitData::MakeDataString(result, entity, (Direction)dir);
		else			strcpy(result, "0");
	}
}


BUILTIN_MEMBER(RoomData, Exits)
{
	ScriptEngineListConstructor	lc(result);
		
	for (int i = 0; i < NUM_OF_DIRS; ++i)
	{
		if (!Exit::Get(entity, i))	continue;
		
		lc.AddExtendedData<ExitData>(entity, (Direction)i);
	}
}


BUILTIN_EXTENDEDDATA_MEMBER(ExitData, room)					{ strcpy(result, ScriptEngineGetUIDString(handler->exit->ToRoom())); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, vnum)
{
	if (handler->exit->ToRoom() && handler->exit->ToRoom()->GetVirtualID().IsValid())
		strcpy(result, handler->exit->ToRoom()->GetVirtualID().Print().c_str());
}
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, vid)
{
	if (handler->exit->ToRoom() && handler->exit->ToRoom()->GetVirtualID().IsValid())
		strcpy(result, handler->exit->ToRoom()->GetVirtualID().Print().c_str());
}
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, key)
{
	if (handler->exit->key.IsValid())
		strcpy(result, handler->exit->key.Print().c_str());
}
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, name)					{ strcpy(result, handler->exit->GetKeyword()); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, description)			{ strcpy(result, handler->exit->GetDescription()); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, direction)			{ strcpy(result, GetEnumName<Direction>(handler->dir)); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, flags)				{ strcpy(result, handler->exit->GetFlags().print().c_str()); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, states)				{ strcpy(result, handler->exit->GetStates().print().c_str()); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, isOpen)				{ strcpy(result, Exit::IsOpen(handler->exit) ? "1" : "0"); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, isDoorClosed)			{ strcpy(result, Exit::IsDoorClosed(handler->exit) ? "1" : "0"); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, isPassable)			{ strcpy(result, Exit::IsPassable(handler->exit) ? "1" : "0"); }
BUILTIN_EXTENDEDDATA_MEMBER(ExitData, isViewable)			{ strcpy(result, Exit::IsViewable(handler->exit) ? "1" : "0"); }


BUILTIN_MEMBER(RoomData, Flags)
{
	strcpy(result, entity->GetFlags().print().c_str());
}


BUILTIN_MEMBER(RoomData, Iscontents)
{
	find_obj_in_contents(thread, entity->contents, subfield, result);
}


BUILTIN_MEMBER(RoomData, Iscontentsdeep)
{
	find_obj_in_contents(thread, entity->contents, subfield, result, true);
}


BUILTIN_MEMBER(RoomData, Ispeople)
{
	if (subfield && *subfield)
	{
		std::list<CharData *>	lst;
		find_char_people(thread, entity, lst, subfield);
		
		ScriptEngineListConstructor	lc(result);
		
		FOREACH(std::list<CharData *>, lst, ch)
		{
			lc.Add(*ch);
		}
	}
	//if (lst.empty())	strcpy(str, "0");
}


BUILTIN_MEMBER(RoomData, Ispeoplerace)
{
	if (subfield && *subfield)
	{
		ScriptEngineListConstructor	lc(result);
	
		int race = search_block(subfield, race_types, 0);
		FOREACH(CharacterList, entity->people, ch)
		{
			if ((*ch)->GetRace() == race)
				lc.Add(*ch);
		}
	}
	//if (lst.empty())	strcpy(str, "0");
}


BUILTIN_MEMBER(RoomData, Ispeoplenotrace)
{
	if (subfield && *subfield)
	{
		ScriptEngineListConstructor	lc(result);
	
		int race = search_block(subfield, race_types, 0);
		FOREACH(CharacterList, entity->people, ch)
		{
			if ((*ch)->GetRace() != race)
				lc.Add(*ch);
		}
	}
	//if (lst.empty())	strcpy(str, "0");
}


BUILTIN_MEMBER(RoomData, Light)
{
	sprintf(result, "%d", entity->GetLight());
}


BUILTIN_MEMBER(RoomData, Name)
{
	strcpy(result, entity->GetName());
}


BUILTIN_MEMBER(RoomData, Description)
{
	strcpy(result, entity->GetDescription());
}



BUILTIN_MEMBER(RoomData, People)
{
	ScriptEngineListConstructor	lc(result);
	
	FOREACH(CharacterList, entity->people, ch)
		lc.Add(*ch);
}


BUILTIN_MEMBER(RoomData, Sector)
{
	strcpy(result, GetEnumName(entity->GetSector()));
}


BUILTIN_MEMBER(RoomData, Varclass)
{
	strcpy(result, "room");
}


BUILTIN_MEMBER(RoomData, Vnum)
{
	sprintf(result, "%d", VNumLegacy::GetVirtualNumber(entity->GetVirtualID()));
}


BUILTIN_MEMBER(RoomData, vid)
{
	strcpy(result, entity->GetVirtualID().IsValid() ? entity->GetVirtualID().Print().c_str() : "0");
}



BUILTIN_MEMBER(RoomData, Zone)
{
	sprintf(result, "%s", entity->GetZone() ? entity->GetZone()->GetTag() : "-1");
}




BUILTIN_MEMBER(RoomData, Exit)
{
	if (subfield && *subfield)
	{
		int dir = search_block(subfield, dirs, false);
		
		const RoomExit *exit = (dir != -1) ? (RoomExit *)entity->GetExit(dir) : NULL;
		
		if (exit)
		{
			sprintf(result, "%s %s %s",
				ScriptEngineGetUIDString(exit->ToRoom()),
				exit->GetFlags().print().c_str(),
				exit->GetStates().print().c_str());
		}
		else
			strcpy(result, "0");
	}
}


#if 0



template <class Type>
struct VariableDefinition
{
	const char *	strName;
	unsigned int	nameHash;
	
	void			(*pGetFunction)(Type *pEntity, TrigThread *pThread, char *str, const char *subfield);
};

#define DEFINE_GET_VALUE(Type, Value) \
	static void Get##Type##Value ( Type##Data *pEntity, TrigThread *pThread, char *str, const char *subfield)

#define GET_VALUE(Type, Value) \
	Get##Type##Value

#define VALUE(Type, Value) \
	{ #Value, 0x00000000, GET_VALUE(Type, Value) }


#endif
