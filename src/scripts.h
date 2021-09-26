/**************************************************************************
*  File: scripts.h                                                        *
*  Usage: header file for script structures and contstants, and           *
*         function prototypes for scripts.c                               *
*                                                                         *
*                                                                         *
*  $Author: avp $
*  $Date: 2008-12-20 08:24:31 $
*  $Revision: 3.119 $
**************************************************************************/


#ifndef __SCRIPTS_H__
#define __SCRIPTS_H__

#include "types.h"
#include "index.h"
//#include "scriptcompiler.h"

#include "ManagedPtr.h"
#include "SharedPtr.h"
#include "lexilist.h"
#include "lexistring.h"
#include "lexisharedstring.h"
#include "bitflags.h"
#include <list>
#include <vector>
#include <map>
#include <set>

const int MAX_SCRIPT_STRING_LENGTH = 1 << 15; /*(8192 * 4);*/

#define MOB_TRIGGER				0
#define OBJ_TRIGGER				1
#define WLD_TRIGGER				2
#define GLOBAL_TRIGGER			3


#define TRIG_GLOBAL				(1 << 0)	//	MOW	- Random when nobody in zone, *-all,
#define TRIG_RANDOM				(1 << 1)	//	MOW
#define TRIG_COMMAND			(1 << 2)	//	MOW
#define TRIG_SPEECH				(1 << 3)	//	MOW
#define TRIG_GREET				(1 << 4)	//	MOW
#define TRIG_LEAVE				(1 << 5)	//	MOW
#define TRIG_DOOR				(1 << 6)	//	MOW
#define TRIG_DROP				(1 << 7)	//	 OW
#define TRIG_GET				(1 << 8)	//	MO	(MTRIG_RECEIVE, OTRIG_GET)
#define TRIG_ACT				(1 << 9)	//	M W
#define TRIG_DEATH				(1 << 10)	//	MO	(W later)
#define TRIG_FIGHT				(1 << 11)	//	M	(OW later)
#define TRIG_HITPRCNT			(1 << 12)	//	M	(O later)
#define TRIG_SIT				(1 << 13)	//	 O
#define	TRIG_GIVE				(1 << 14)	//	 O
#define TRIG_WEAR				(1 << 15)	//	 O
#define TRIG_CONSUME			(1 << 16)	//	 O
#define TRIG_REMOVE				(1 << 17)	//	 O
#define TRIG_LOAD				(1 << 18)	//	MO
#define TRIG_TIMER				(1 << 19)	//	 O	- Hardly used, only 5.  Still useful.
#define TRIG_START				(1 << 20)	//	 O	- Unused
#define TRIG_QUIT				(1 << 21)	//	 O	- Hardly used, only 2
#define TRIG_ATTACK				(1 << 22)	//	 O
#define TRIG_INSTALL			(1 << 23)	//	 O	- This should come out!
#define TRIG_CALL				(1 << 24)	//	MOW
#define TRIG_ENTER				(1 << 25)	//	 O
#define TRIG_DESTROYREPAIR		(1 << 26)	//	 O - This should come out
#define TRIG_ATTACKED			(1 << 27)	//	 O
#define TRIG_KILL				(1 << 28)	//	 O
#define TRIG_PUT				(1 << 29)	//	 O
#define TRIG_MOTION				(1 << 30)	//	 O
#define TRIG_RESET				(1 << 31)	//	  W	-	Merge with Load?

/* mob trigger types */
#define MTRIG_GLOBAL			(1 << 0)		/* check even if zone empty   */
#define MTRIG_RANDOM			(1 << 1)		/* checked randomly           */
#define MTRIG_COMMAND			(1 << 2)		/* character types a command  */
#define MTRIG_SPEECH			(1 << 3)		/* a char says a word/phrase  */
#define MTRIG_ACT				(1 << 4)		/* word or phrase sent to act */
#define MTRIG_DEATH				(1 << 5)		/* character dies             */
#define MTRIG_GREET				(1 << 6)		/* something enters room seen */
#define MTRIG_GREET_ALL			(1 << 7)		/* anything enters room       */
#define MTRIG_ENTRY				(1 << 8)		/* the mob enters a room      */
#define MTRIG_RECEIVE			(1 << 9)		/* character is given obj     */
#define MTRIG_FIGHT				(1 << 10)		/* each pulse while fighting  */
#define MTRIG_HITPRCNT			(1 << 11)		/* fighting and below some hp */
#define MTRIG_BRIBE				(1 << 12)		/* coins are given to mob     */
#define MTRIG_LEAVE				(1 << 13)
#define MTRIG_LEAVE_ALL			(1 << 14)
#define MTRIG_DOOR				(1 << 15)
#define MTRIG_LOAD				(1 << 16)
#define MTRIG_CALL				(1 << 17)
#define MTRIG_KILL				(1 << 18)
#define MTRIG_MOTION			(1 << 19)
#define MTRIG_ATTACK			(1 << 20)
#define MTRIG_ATTACKED			(1 << 21)

/* obj trigger types */
#define OTRIG_GLOBAL			(1 << 0)		// unused
#define OTRIG_RANDOM			(1 << 1)		//	Checked randomly
#define OTRIG_COMMAND			(1 << 2)		//	Character types a command
#define OTRIG_SIT				(1 << 3)		//	Character tries to sit on the object
#define OTRIG_LEAVE				(1 << 4)		//	Character leaves room
#define OTRIG_GREET				(1 << 5)		//	Character enters room
#define OTRIG_GET				(1 << 6)		//	Character tries to get obj
#define OTRIG_DROP				(1 << 7)		//	Character tries to drop obj
#define OTRIG_GIVE				(1 << 8)		//	Character tries to give obj
#define OTRIG_WEAR				(1 << 9)		//	character tries to wear obj
#define OTRIG_DOOR				(1 << 10)
#define OTRIG_SPEECH			(1 << 11)		//	Speech check
#define OTRIG_CONSUME			(1 << 12)		//	Eat/Drink obj
#define OTRIG_REMOVE			(1 << 13)
#define OTRIG_LOAD				(1 << 14)
#define OTRIG_TIMER				(1 << 15)
#define OTRIG_START				(1 << 16)
#define OTRIG_QUIT				(1 << 17)
#define OTRIG_ATTACK			(1 << 18)
#define OTRIG_INSTALL			(1 << 19)
#define OTRIG_CALL				(1 << 20)
#define OTRIG_ENTER				(1 << 21)
#define OTRIG_DESTROYREPAIR		(1 << 22)
#define OTRIG_DEATH				(1 << 23)
#define OTRIG_ATTACKED			(1 << 24)
#define OTRIG_KILL				(1 << 25)
#define OTRIG_PUT				(1 << 26)
#define OTRIG_MOTION			(1 << 27)
#define	OTRIG_RETRIEVE			(1 << 28)
#define OTRIG_STORE				(1 << 29)

/* wld trigger types */
#define WTRIG_GLOBAL			(1 << 0)		// check even if zone empty
#define WTRIG_RANDOM			(1 << 1)		// checked randomly
#define WTRIG_COMMAND			(1 << 2)		// character types a command
#define WTRIG_SPEECH			(1 << 3)		// a char says word/phrase
#define WTRIG_DOOR				(1 << 4)
#define WTRIG_LEAVE				(1 << 5)		// character leaves room
#define WTRIG_ENTER				(1 << 6)		// character enters room
#define WTRIG_DROP				(1 << 7)		// something dropped in room
#define WTRIG_RESET				(1 << 8)
#define WTRIG_CALL				(1 << 9)
#define WTRIG_GREET				(1 << 10)
#define WTRIG_ACT				(1 << 11)
#define WTRIG_MOTION			(1 << 12)


/* global trigger types */
#define GTRIG_GLOBAL			(1 << 0)		// check even if zone empty
#define GTRIG_COMMAND			(1 << 1)		// character types a command
#define GTRIG_SPEECH			(1 << 2)		// a char says word/phrase


#define TRIG_DELETED			(1 << 31)

#define MAX_SCRIPT_DEPTH	10	/* maximum depth triggers can recurse into each other */

namespace LexiScriptCommands {
enum Commands {
	None,
	Comment,
	If,
	ElseIf,
	Else,
	While,
	Foreach,
	Switch,
	End,
	Done,
	Continue,
	Break,
	Case,
	Default,
	Delay,
	Eval,
	EvalGlobal,
	EvalShared,
	Global,
	Shared,
	Halt,
	Return,
	Thread,
	Set,
	SetGlobal,
	SetShared,
	Unset,
	Wait,
	Pushfront,
	Popfront,
	Pushback,
	Popback,
	Print,
	Function,
	Import,
	Label,
	Jump,
	Increment,
	Decrement,
	Command,
	PCCommand,
	Constant,
	NumCommands
};
}


class CompiledScript;
typedef unsigned int bytecode_t;


class CallstackEntry
{
public:
	enum CallType
	{
		Call_Normal,
		Call_Embedded
	};

						CallstackEntry(CompiledScript *pCompiledScript,
								bytecode_t * bytecodeStart,
								const char *pFunctionName,
								CallType type);
						
						~CallstackEntry();
	
	VirtualID			GetVirtualID() const;
	
	Lexi::SharedPtr<CompiledScript>	m_pCompiledScript;
	bytecode_t *		m_BytecodePosition;
	
	CallType			m_Type;
	
	ScriptVariable::List	m_Variables;		//	list of local vars for trigger
	
	//	Debug info
	const char * const	m_FunctionName;
	unsigned int		m_Line;
	
private:
						CallstackEntry(const CallstackEntry &entry);
};


class ThreadStack
{
public:
						ThreadStack();
						~ThreadStack();
						
	void				Push(const char *entry);
	void				PushCharPtr(const char *entry);
	void				PushScriptVariable(const char *entry);
	char *				Pop();
	char *				Peek();
	inline int			Count() { return m_Count; }

private:
	enum StackEntryType
	{
		TYPE_STRING		= 0,
		TYPE_CHAR_PTR,
		TYPE_VARIABLE
	};
	
	struct StackEntry
	{
		unsigned int	size : 24;	//	2^16 (32k) is the maximum data size
		unsigned int	type : 8;	//	A StackEntryType
		char			data[4];	//	Minimum 4 bytes
	};
	
	char *				m_pStack;
	unsigned int		m_StackBufferSize;
	unsigned int		m_StackBufferUsed;
	unsigned int		m_Count;
	
	StackEntry *		Push(const void *data, unsigned int dataSize, StackEntryType type);

	static inline unsigned	Align(unsigned size, unsigned alignment) { return (size + (alignment - 1)) & ~(alignment - 1); }
	
	enum 
	{
		StackGrowSize = 512	
	};
};


class TrigThread {
public:
	enum eCreateRoot
	{
		CreateRoot,
		DontCreateRoot
	};
	
						TrigThread(TrigData *pTrigger, Entity *pEntity, eCreateRoot createRoot = CreateRoot);
						~TrigThread();
	
	//	Configuration
	TrigData *			m_pTrigger;
	Entity *			m_pEntity;
	IDNum				m_ID;
	Event *				m_pWaitEvent;		//	event to pause the trigger
	class ScriptTimerCommand *m_pDelay;
	
	//	Thread Execution State
	typedef std::vector< CallstackEntry * > Callstack;
	
	ThreadStack			m_Stack;
	Callstack			m_Callstack;
	CallstackEntry *	m_pCurrentCallstack;
	Lexi::String		m_pResult;
	
	//	Performance Monitoring
	double				m_TimeUsed;

	//	Debugging Facilities
	bool				m_bDebugPaused;
	bool				m_bHasBreakpoints;
	typedef std::set<unsigned int>	BreakpointSet;
	BreakpointSet		m_Breakpoints;

	//	Error Reporting
	int					m_NumReportedErrors;
	
	CompiledScript *	GetCurrentCompiledScript()	{ return m_pCurrentCallstack->m_pCompiledScript; }
	bytecode_t *		GetCurrentBytecodePosition(){ return m_pCurrentCallstack->m_BytecodePosition; }
	unsigned int		GetCurrentLine()			{ return m_pCurrentCallstack->m_Line; }
	VirtualID			GetCurrentVirtualID();
	TrigData *			GetCurrentScript();
	
	ScriptVariable::List &GetCurrentVariables()		{ return m_pCurrentCallstack->m_Variables; }
	
	const char *		GetCurrentFunctionName() const	{ return m_pCurrentCallstack->m_FunctionName; }
	
	void				SetCurrentBytecodePosition(bytecode_t *pos)	{ m_pCurrentCallstack->m_BytecodePosition = pos; }
	void				SetCurrentLine(unsigned int line)	{ m_pCurrentCallstack->m_Line = line; }
	
	bool				PushCallstack(CompiledScript *pCompiledScript, bytecode_t *bytecodePosition, const char *pFunctionName,
								CallstackEntry::CallType type = CallstackEntry::Call_Normal);
	bool				PopCallstack();
	
	void				Terminate();

	static int			ms_TotalThreads;
	
private:
						TrigThread(const TrigThread &);
	TrigThread &		operator=(const TrigThread &);
};


/* structure for triggers */

namespace Lexi {
	class Parser;
}

/*
class Script;
typedef IndexData<Script> *	ScriptPrototype;


//	CompiledScript - the result of compiling a script
//		compiled bytecode
//		symbol table
//		constants
//		function table
//		imports (link) table
//		ref to Prototype Script
//	TriggerDefinition - definition of what can execute a script, and what script to run
//		Type
//		trigger flags
//		argument
//		numArg
//		threadsupport flag
//		(optional)function name
//	Script - a container for a script, and trigger conditions
//		name
//		uncompiled script
//		errors
//		ref to CompiledScript
//		list of TriggerDefinitions
//	Trigger : isa TriggerDefinition - an instance of a trigger on an entity
//		(optional) ref to Script
//			OR
//		(optional) ref to CompiledScript
//		list of running threads


class Trigger
{
	//	Script Prototype that this Trigger is for
	ScriptPrototype *	m_pScript;
	
	//	
	int					m_Type;
	Flags				m_Triggers;
	
	SString				m_Arguments;
	int					m_NumArgument;
	
	bool				m_bThreadable;
};
*/

class TrigData;
typedef IndexMap<TrigData>				ScriptPrototypeMap;
typedef ScriptPrototypeMap::data_type	ScriptPrototype;


class TrigData
{
public:
						TrigData();
						TrigData(const TrigData &trig);
						~TrigData();
						
	static TrigData *	Create(TrigData *obj = NULL);
	static TrigData *	Create(ScriptPrototype *proto);
	
	void				operator=(const TrigData &trig);
						
	void				Extract(void);
	
public:
	ScriptPrototype *	GetPrototype() { return m_Prototype; }
	void				SetPrototype(ScriptPrototype *proto) { m_Prototype = proto; }

	VirtualID			GetVirtualID() { return GetPrototype() ? GetPrototype()->GetVirtualID() : VirtualID(); }

	bool				IsPurged()				{ return m_bPurged; }
	void				SetPurged()				{ m_bPurged = true; }
	
	const char *		GetName()				{ return m_Name.c_str(); }
	const char *		GetScript()				{ return m_Script.c_str(); }
	const char *		GetErrors()				{ return m_Errors.c_str(); }
	
	void				SetName(const char *name)		{ m_Name = name; }
	void				SetCommands(const char *commands)	{ m_Script = commands; }
	
	static void			Parse(Lexi::Parser &parser, VirtualID nr);
	void				Compile();
	
private:
	ScriptPrototype *	m_Prototype;
	bool				m_bPurged;				//	trigger is set to be purged

	Lexi::SharedString	m_Name;				//	Name of trigger
	Lexi::SharedString	m_Script;			//	Raw command text
	Lexi::SharedString	m_Errors;			//	Compiler error output
	
public:
	int					m_Type;
	Flags				m_Triggers;

	Lexi::SharedString	m_Arguments;		//	Argument list
	int					m_NumArgument;
	
	enum ScriptFlag
	{
		Script_Threadable,
		Script_DebugMode,
		NUM_SCRIPT_FLAGS
	};
	
	typedef Lexi::BitFlags<NUM_SCRIPT_FLAGS, ScriptFlag> ScriptFlags;
	ScriptFlags			m_Flags;
	
	//	New engine
	Lexi::SharedPtr<CompiledScript>	m_pCompiledScript;
	
	std::list<TrigThread *>	m_Threads;
	static ScriptVariable::List	ms_InitialVariables;

	//	Performance Metrics
	double				m_TotalTimeUsed;

	typedef Lexi::List<TrigData *>	List;
	
	friend class CompiledScript;
};


#define GET_THREAD_VARS(t)				((t)->GetCurrentVariables())

#define GET_THREAD_TRIG(t)		((t)->m_pTrigger)
#define GET_THREAD_ENTITY(t)	((t)->m_pEntity)
#define GET_THREAD_TYPE(t)		((t)->m_Type)



/* a complete script (composed of several triggers) */
class Scriptable
{
public:
							Scriptable();
							~Scriptable();
	
	void					AddTrigger(TrigData *t);
	TrigData *				HasTrigger(const char *name);
	TrigData *				HasTrigger(VirtualID vid);
	bool					RemoveTrigger(const char *name);
	void					RemoveTrigger(TrigData *t);
	void					UpdateTriggerTypes();
	
	TrigThread *			GetThread(IDNum id);
	
	void					Extract();
	
	Flags					m_Types;			//	bitvector of trigger types
	TrigData::List			m_Triggers;			//	list of triggers
	ScriptVariable::List	m_Globals;			//	list of global variables
	IDNum					m_LastThreadID;
	
private:
							Scriptable(const Scriptable &);
	Scriptable &			operator=(const Scriptable &);
};


class BehaviorSet : public WeakPointable
{
public:
						BehaviorSet(const char *name);
						//BehaviorSet(const BehaviorSet &set);	// default Copy constructor is fine
	
	const char *		GetName() const { return m_Name.c_str(); }
//	Hash				GetNameHash() { return m_NameHash; }
	
	void				SetName(const char *name) { m_Name = name; /*m_NameHash = GetStringFastHash(name);*/ }
	
//	void				AddScript(VirtualID script);
//	void				AddVariable(const char *name, int context, const char *value);
	
	ScriptVector &		GetScripts() { return m_Scripts; }
	ScriptVariable::List &	GetVariables() { return m_Variables; }
	
	void				Apply(Scriptable *sc) const;
	static void			ApplySets(const BehaviorSets &sets, Scriptable *sc);
	
	static BehaviorSet *Find(const char *name);
	static void			SaveSet(const BehaviorSet &set);
	
	static void			Load();
	static void			Save();
	
	void				Show(CharData *ch);
	static void			List(CharData *ch);
	
private:
	Lexi::String		m_Name;
//	Hash				m_NameHash;
	
	ScriptVector		m_Scripts;
	ScriptVariable::List	m_Variables;
	
	typedef std::map<Lexi::String, BehaviorSet *> BehaviorMap;
	static BehaviorMap	ms_Sets;
};


extern TrigData::List		trig_list;
extern TrigData::List		purged_trigs;
extern ScriptPrototypeMap	trig_index;


/* function prototypes from triggers.c */
   /* mob triggers */
void random_mtrigger(CharData *ch);

   /* object triggers */
void random_otrigger(ObjData *obj);

   /* world triggers */
void random_wtrigger(RoomData *ch);


/* function prototypes from scripts.c */
void script_trigger_check(void);

void do_stat_trigger(CharData *ch, TrigData *trig, bool bShowLineNums = false);
void do_sstat_room(CharData * ch, RoomData *room);
void do_sstat_object(CharData * ch, ObjData * j);
void do_sstat_character(CharData * ch, CharData * k);

void script_log(char *msg);

/* Macros for scripts */

#define GET_TRIG_TYPE(t)		((t)->m_Triggers)
#define GET_TRIG_DATA_TYPE(t)	((t)->m_Type)
#define GET_TRIG_NARG(t)		((t)->m_NumArgument)


//#define SCRIPT(o)				((o)->script)
#define SCRIPT(o)				((o)->GetScript())

#define SCRIPT_TYPES(s)			((s)->m_Types)
#define TRIGGERS(s)				((s)->m_Triggers)


#define SCRIPT_CHECK(go, type)	(IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type)	(IS_SET(GET_TRIG_TYPE(t), type) && (t->m_Flags.test(TrigData::Script_Threadable) || t->m_Threads.empty()))


//Lexi::String MakeIntegerString(int i);

#define SCMD(name)	void (name)(Entity * go, TrigData *trig, TrigThread *thread, const char *argument, int subcmd)


namespace Scope {
enum {
	Local		= (1 << 0),
	Global		= (1 << 1),
	Shared		= (1 << 2),
	Constant	= (1 << 3),
	ContextSpecific	= (1 << 4),
	Extract		= (1 << 5),
	All			= Local | Global | Shared
};
}	//	namespace Scope


struct ScriptCommand {
	enum {
		Mob		= (1 << 0),
		Obj		= (1 << 1),
		Room	= (1 << 2),
		All		= Mob | Obj | Room
	};

	char *	command;
	SCMD(*command_pointer);
//	void	(*command_pointer)	(Scriptable *go, char *argument, int subcmd);
	int	type;
	int	subcmd;
};

extern const struct ScriptCommand scriptCommands[];

extern void script_command_interpreter(Entity * go, TrigData * trig, TrigThread *thread, const char *argument, int cmd = -1);




#endif	// __SCRIPTS_H__
