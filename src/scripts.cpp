/*************************************************************************
*   File: scripts.cp                 Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for scripts and triggers                          *
*************************************************************************/

#include "scripts.h"
#include "scriptcompiler.h"
#include "buffer.h"
#include "utils.h"
#include "find.h"
#include "characters.h"
#include "db.h"
#include "comm.h"
#include "rooms.h"
#include "handler.h"
#include "constants.h"
#include "lexifile.h"
#include "lexiparser.h"

// Internal functions
ACMD(do_gscript);

void LoadGlobalTriggers();
void SaveGlobalTriggers();

LexiScriptCommands::Commands lookup_script_line(const char *p);


TrigData::List			trig_list;
TrigData::List			purged_trigs;		   /* list of mtrigs to be deleted  */
ScriptPrototypeMap		trig_index;


LexiScriptCommands::Commands lookup_script_line(const char *p) {
	LexiScriptCommands::Commands retval = LexiScriptCommands::None;
	
	const char *p2 = p;
	while (*p2 && !isspace(*p2))
		++p2;
	bool singleWord = (!*p2 || isspace(*p2));
	
	switch (tolower(*p)) {
		case '*':
		case '#':
			retval = LexiScriptCommands::Comment;
			break;

		case 'b':
			if (!strn_cmp(p, "break", 5) && singleWord)	retval = LexiScriptCommands::Break;
			break;
		
		case 'c':
			if (!strn_cmp(p, "case ", 5))				retval = LexiScriptCommands::Case;
			else if (!strn_cmp(p, "constant ", 9))		retval = LexiScriptCommands::Constant;
			else if (!strn_cmp(p, "continue", 8))		retval = LexiScriptCommands::Continue;
			break;
		
		case 'd':
			if (!strn_cmp(p, "done", 4) && singleWord)	retval = LexiScriptCommands::Done;
			else if (!strn_cmp(p, "delay ", 6))			retval = LexiScriptCommands::Delay;
			else if (!strn_cmp(p, "default", 7) && singleWord)	retval = LexiScriptCommands::Default;
			else if (!strn_cmp(p, "dec ", 4) || !strn_cmp(p, "decrement ", 10))	retval = LexiScriptCommands::Decrement;
			break;
		
		case 'e':
			if (!strn_cmp(p, "end", 3) && singleWord)	retval = LexiScriptCommands::End;
			else if (!strn_cmp(p, "elseif ", 7))		retval = LexiScriptCommands::ElseIf;
			else if (!strn_cmp(p, "else if ", 8))		retval = LexiScriptCommands::ElseIf;
			else if (!strn_cmp(p, "else", 4) && singleWord)	retval = LexiScriptCommands::Else;
			else if (!strn_cmp(p, "eval ", 5))			retval = LexiScriptCommands::Eval;
			else if (!strn_cmp(p, "evalglobal ", 11))	retval = LexiScriptCommands::EvalGlobal;
			else if (!strn_cmp(p, "evalshared ", 11))	retval = LexiScriptCommands::EvalShared;
			break;
		
		case 'f':
			if (!strn_cmp(p, "foreach ", 8))			retval = LexiScriptCommands::Foreach;
			else if (!strn_cmp(p, "function ", 9))		retval = LexiScriptCommands::Function;
			break;
		
		case 'g':
			if (!strn_cmp(p, "global ", 7))				retval = LexiScriptCommands::Global;
			break;
		
		case 'h':
			if (!strn_cmp(p, "halt", 4) && singleWord)	retval = LexiScriptCommands::Halt;
			break;
		
		case 'i':
			if (!strn_cmp(p, "if ", 3))					retval = LexiScriptCommands::If;
			else if (!strn_cmp(p, "import ", 7))		retval = LexiScriptCommands::Import;
			else if (!strn_cmp(p, "inc ", 4) || !strn_cmp(p, "increment ", 10))	retval = LexiScriptCommands::Increment;
			break;
		
		case 'j':
			if (!strn_cmp(p, "jump ", 5))				retval = LexiScriptCommands::Jump;
//			else if (!strn_cmp(p, "jumpsub ", 8))		retval = LexiScriptCommands::JumpSub;
			break;
		
		case 'l':
			if (!strn_cmp(p, "label ", 6))				retval = LexiScriptCommands::Label;
			break;
			
		case 'p':
			if (!strn_cmp(p, "popback ", 8))			retval = LexiScriptCommands::Popback;
			else if (!strn_cmp(p, "popfront ", 9))		retval = LexiScriptCommands::Popfront;
			else if (!strn_cmp(p, "print ", 6))			retval = LexiScriptCommands::Print;
			else if (!strn_cmp(p, "pushback ", 9))		retval = LexiScriptCommands::Pushback;
			else if (!strn_cmp(p, "pushfront ", 10))	retval = LexiScriptCommands::Pushfront;
			break;
		
		case 'r':
			if (!strn_cmp(p, "return ", 7))				retval = LexiScriptCommands::Return;
			break;
		
		case 's':
			if (!strn_cmp(p, "set ", 4))				retval = LexiScriptCommands::Set;
			else if (!strn_cmp(p, "setglobal ", 10))	retval = LexiScriptCommands::SetGlobal;
			else if (!strn_cmp(p, "setshared ", 10))	retval = LexiScriptCommands::SetShared;
			else if (!strn_cmp(p, "shared ", 7))		retval = LexiScriptCommands::Shared;
			else if (!strn_cmp(p, "switch ", 7))		retval = LexiScriptCommands::Switch;
			break;
		
		case 't':
			/*if (!strn_cmp(p, "then ", 5))				retval = LexiScriptCommands::Then;
			else*/ if (!strn_cmp(p, "thread ", 7))		retval = LexiScriptCommands::Thread;
			break;
		
		case 'u':
			if (!strn_cmp(p, "unset ", 6))				retval = LexiScriptCommands::Unset;
			break;
		
		case 'w':
			if (!strn_cmp(p, "wait ", 5))				retval = LexiScriptCommands::Wait;
			else if (!strn_cmp(p, "while ", 6))			retval = LexiScriptCommands::While;
			break;
	}
	
	return retval;
}


Scriptable::Scriptable()
 :	m_Types(0),
 	m_LastThreadID(0)
{
}



/* release memory allocated for a script */
Scriptable::~Scriptable()
{
	FOREACH(TrigData::List, m_Triggers, trig)
	{
		(*trig)->Extract();
	}
}


TrigData *Scriptable::HasTrigger(const char *name)
{
	int num = 0;
	char *cname;
  
	if (!*name)
		return 0;
		
	if (IsVirtualID(name))	return HasTrigger(VirtualID(name));
	
	BUFFER(buf, MAX_STRING_LENGTH);
	strcpy(buf, name);
	
	cname = strstr(buf,".");
	if (cname) {
		*cname = '\0';
		num = atoi(buf);
		name = cname + 1;
	}
	else
		name = buf;
  
	FOREACH(TrigData::List, m_Triggers, iter)
	{
		if (silly_isname(name, (*iter)->GetName()) && --num <= 0)
				return *iter;
	}
	
	return NULL;
}


TrigData *Scriptable::HasTrigger(VirtualID vid)
{
	FOREACH(TrigData::List, m_Triggers, iter)
	{
		if ((*iter)->GetVirtualID() == vid)
			return *iter;
	}
	
	return NULL;
}


/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void Scriptable::AddTrigger(TrigData *t)
{
	m_Triggers.push_front(t);
	m_Types |= GET_TRIG_TYPE(t);
}


/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
bool Scriptable::RemoveTrigger(const char *name) {
	TrigData *i = HasTrigger(name);
	
	if (i)
	{
		RemoveTrigger(i);
		return true;
	}

	return false;
}


void Scriptable::RemoveTrigger(TrigData *trig)
{
	m_Triggers.remove(trig);
	trig->Extract();
	UpdateTriggerTypes();
}


void Scriptable::Extract()
{
	FOREACH(TrigData::List, m_Triggers, trig)
	{
		(*trig)->Extract();
	}
	
	m_Triggers.clear();
	m_Globals.Clear();
	
	m_Types = 0;
}

		
void Scriptable::UpdateTriggerTypes()
{
	//	Update the script type bitvector
	m_Types = 0;

	FOREACH(TrigData::List, m_Triggers, trig)
	{
		m_Types |= GET_TRIG_TYPE(*trig);
	}
}


TrigThread *Scriptable::GetThread(IDNum id)
{
	FOREACH(TrigData::List, m_Triggers, trig)
	{
		FOREACH(std::list<TrigThread *>, (*trig)->m_Threads, thread)
		{
			TrigThread *pThread = *thread;
			
			if (pThread->m_ID == id)
				return pThread;
		}
	}
	
	return NULL;
}



ScriptVariable::List TrigData::ms_InitialVariables;

TrigData::TrigData(void)
:	m_Prototype(NULL)
,	m_bPurged(false)
,	m_Type(0)
,	m_Triggers(0)
,	m_NumArgument(0)
,	m_TotalTimeUsed(0)
{
}


TrigData::TrigData(const TrigData &trig)
:	m_Prototype(trig.m_Prototype)
,	m_bPurged(false)
,	m_Name(trig.m_Name)
,	m_Script(trig.m_Script)
,	m_Errors(trig.m_Errors)
,	m_Type(trig.m_Type)
,	m_Triggers(trig.m_Triggers)
,	m_Arguments(trig.m_Arguments)
,	m_NumArgument(trig.m_NumArgument)
,	m_Flags(trig.m_Flags)
,	m_pCompiledScript(trig.m_pCompiledScript)
//,	m_Threads - don't copy
,	m_TotalTimeUsed(0)
{
}


void TrigData::operator=(const TrigData &trig)
{	
	if (&trig == this)
		return;

	m_Prototype = trig.m_Prototype;
//	m_bPurged - don't copy
	
	m_Name = trig.m_Name;
	m_Script = trig.m_Script;
	m_Errors = trig.m_Errors;
	
	m_Type = trig.m_Type;
	m_Triggers = trig.m_Triggers;
	
	m_Arguments = trig.m_Arguments;
	m_NumArgument = trig.m_NumArgument;
	
	m_Flags = trig.m_Flags;
	
	m_pCompiledScript = trig.m_pCompiledScript;
	
//	m_Threads - don't copy
}


TrigData::~TrigData(void)
{
	m_bPurged = true;
}


void TrigData::Extract(void)
{
	if (IsPurged()) 
		return;

	SetPurged();

//	TrigData::List::iterator iter = Lexi::Find(trig_list, this);
//	if (iter != trig_list.end())
//		trig_list.erase(iter);
	trig_list.remove_one(this);	//	Faster with the optimized Lexi::List::remove
	
	purged_trigs.push_back(this);
	
	if (m_Prototype)
		--m_Prototype->m_Count;
	
	FOREACH(std::list<TrigThread *>, m_Threads, thread)
	{
		(*thread)->m_pTrigger = NULL;
		delete *thread;
	}
	m_Threads.clear();
}


TrigData *TrigData::Create(TrigData *t)
{
	t->Compile();
	
	TrigData *trig = new TrigData(*t);
	
	trig_list.push_front(trig);
	
	if (trig->GetPrototype())
		++trig->GetPrototype()->m_Count;

	return trig;	
}


TrigData *TrigData::Create(ScriptPrototype *proto)
{
	return proto ? Create(proto->m_pProto) : NULL;
}


void TrigData::Compile()
{
	if (!m_pCompiledScript)
	{
		m_pCompiledScript = new CompiledScript(GetPrototype());

		BUFFER(errors, MAX_STRING_LENGTH);
		*errors = '\0';
		ScriptCompilerParseScript(m_Script.c_str(), m_pCompiledScript, errors, m_Flags.test(Script_DebugMode));
		m_Errors = errors;
	
		FOREACH(CompiledScript::ImportList, m_pCompiledScript->m_Imports, import)
		{
			(*import)->m_pProto->Compile();
		}
	}
}

#if 0
void TrigData::CompileAllScripts()
{
	FOREACH(ScriptPrototypeMap, trig_index, proto)
	{
		(*proto)->m_pProto->Compile();
	}
}
#endif

void TrigData::Parse(Lexi::Parser &parser, VirtualID nr)
{
	TrigData *trig = new TrigData;
	
//	ScriptPrototype *proto = new ScriptPrototype(trig, nr);
//	trig_index.push_back(proto);
	ScriptPrototype *proto = trig_index.insert(nr, trig);
	trig->SetPrototype(proto);
	
	trig->m_Name				= parser.GetString("Name", "invalid");
	trig->m_Type				= parser.GetEnum("Type", trig_data_types, -1);
	if (trig->m_Type != -1)
	{
		char **table = NULL;
		
		switch (trig->m_Type)
		{
			default:
			case MOB_TRIGGER:	table = mtrig_types;	break;
			case OBJ_TRIGGER:	table = otrig_types;	break;
			case WLD_TRIGGER:	table = wtrig_types;	break;
			case GLOBAL_TRIGGER:table = gtrig_types;	break;
		}
		
		trig->m_Triggers			= parser.GetFlags("Triggers", table);
	}
	trig->m_Arguments			= parser.GetString("Argument");
	trig->m_NumArgument			= parser.GetInteger("NumArg");
	trig->m_Flags				= parser.GetBitFlags<ScriptFlag>("Flags");
	if (parser.DoesFieldExist("Multithreaded"))
	{
		if (!str_cmp(parser.GetString("Multithreaded", "NO"), "YES"))
			trig->m_Flags.set(Script_Threadable);
	}

	trig->m_Script				= parser.GetString("Script");
	
//	BUFFER(errors, MAX_STRING_LENGTH);
//	ScriptCompilerParseScript(trig->m_Commands.c_str(), trig->m_pCompiledScript, errors);
//	trig->m_Errors = errors;
}


void ScriptVariable::SetValue(const char *val)
{
	//if (!str_cmp(val, GetValue()))	return;
	
	if (m_Impl->m_RefCount > 1)
	{
		Implementation *oldImp = m_Impl;
		m_Impl->Release();
		m_Impl = new Implementation(*oldImp);
	}
	m_Impl->m_Value = val;
}



ScriptVariable *ScriptVariable::Parse(Lexi::Parser &parser)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	const char *name	= parser.GetString("Name");
	int			context = 0;
	
	if (!*name)
		return NULL;
	
	ScriptEngineParseVariable(name, buf, context);

	context = parser.GetInteger("Context", context);	//	To handle old format
	
	ScriptVariable *var = new ScriptVariable(buf, context);
	var->SetValue(parser.GetString("Value"));
	
	return var;
}


//	New format for variables combines name and context together
void ScriptVariable::Write(Lexi::OutputParserFile &file, const Lexi::String &label, bool bSaving)
{
	if (GetNameString().empty())	return;
	
	BUFFER(buf, MAX_STRING_LENGTH);
	
	file.BeginSection(label);
		strcpy(buf, GetName());
		
		if (GetContext())
		{
			sprintf_cat(buf, "{%d}", GetContext());
		}
		
		file.WriteString("Name", buf);
		file.WritePossiblyLongString("Value", GetValueString());
		
		if (!bSaving)
		{
			//	TODO: Write out flags...
		}
	file.EndSection();
}


void ScriptVariable::List::Parse(Lexi::Parser &parser)
{
	int numVariables = parser.NumSections("Variables");
	for (int v = 0; v < numVariables; ++v)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Variables", v);
		
		ScriptVariable *var = ScriptVariable::Parse(parser);
		
		if (var)		Add(var);
	}
}


void ScriptVariable::List::Write(Lexi::OutputParserFile &file)
{
	file.BeginSection("Variables");
	{
		int count = 0;
		FOREACH(ScriptVariable::List, *this, var)
		{
			(*var)->Write(file, Lexi::Format("Variable %d", ++count), false);
		}
	}
	file.EndSection();
}


void ScriptVariable::List::Clear()
{
	for (List::iterator var = begin(); var != end(); ++var)
	{
		delete *var;
	}
	
	clear();
}


ScriptVariable::List::List(const List &list)
{
	for (const_iterator var = list.begin(); var != list.end(); ++var)
	{
		push_back(new ScriptVariable(**var));
	}
}


ScriptVariable::List &ScriptVariable::List::operator=(const List &list)
{
	Clear();
	
	for (const_iterator var = list.begin(); var != list.end(); ++var)
	{
		push_back(new ScriptVariable(**var));
	}
	
	return *this;
}


struct ScriptVariableListCompareFuncHelper
{
	ScriptVariableListCompareFuncHelper(const char *n, Hash h, int c) : name(n), hash(h), context(c), found(NULL) {}
	ScriptVariableListCompareFuncHelper(const ScriptVariable &v) : name(v.GetName()), hash(v.GetNameHash()), context(v.GetContext()), found(NULL) {}
	
	const char *name;
	Hash hash;
	int context;
	
	mutable ScriptVariable *found;
};


static bool ScriptVariableListCompareFunc(ScriptVariable *lhs, const ScriptVariableListCompareFuncHelper &rhs)
{
	//	False if (helper) comes before (i)
	Hash lhsHash = lhs->GetNameHash(),
		 rhsHash = rhs.hash;
	
	if (lhsHash != rhsHash)
		return lhsHash < rhsHash;

	//	Slight optimization - compare context before name
	int lhsContext = lhs->GetContext();
	int rhsContext = rhs.context;
	
	if (lhsContext != rhsContext)
		return lhsContext < rhsContext;
	
	int compResult = lhs->GetNameString().compare(rhs.name);

	if (compResult == 0)
		rhs.found = lhs;
	else
		ReportHashCollision(lhsHash, lhs->GetName(), rhs.name);

	return compResult < 0;
}


static bool ScriptVariableListSortFunc(const ScriptVariable *lhs, const ScriptVariable *rhs)
{
	//	False if (helper) comes before (i)
	Hash lhsHash = lhs->GetNameHash(),
		 rhsHash = rhs->GetNameHash();
	
	if (lhsHash != rhsHash)
		return lhsHash < rhsHash;

	//	Slight optimization - compare context before name
	int lhsContext = lhs->GetContext();
	int rhsContext = rhs->GetContext();
	
	if (lhsContext != rhsContext)
		return lhsContext < rhsContext;
	
	int compResult = lhs->GetNameString().compare(rhs->GetNameString());

	if (compResult != 0)
		ReportHashCollision(lhsHash, lhs->GetName(), rhs->GetName());
	
	return compResult < 0;
}


//return std::lower_bound(begin(), end(), ScriptVariableListCompareFuncHelper(name, hash, context), CompareFunc);


/* adds a variable with given name and value to trigger */
ScriptVariable::List &ScriptVariable::List::Add(const char *name, const char *value, int context)
{
	ScriptVariable *var = NULL;
	Hash hash = GetStringFastHash(name);

	ScriptVariableListCompareFuncHelper helper(name, hash, context);
	iterator iter = std::lower_bound(begin(), end(), helper, ScriptVariableListCompareFunc);
	
	//if (iter != end() && (*iter)->IsMatch(name, hash, context))
	if (helper.found)
	{
		var = helper.found; //*iter;
	}
	else
	{
		var = new ScriptVariable(name, context);
		insert(iter, var);
	}

	
//	var = Find(name, hash, context);
	
//	if (!var)
//	{
//		var = new ScriptVariable(name, context);
//		push_back(var);
//	}

	var->SetValue(value);
	
	return *this;
}


/* adds a variable with given name and value to trigger */
ScriptVariable::List &ScriptVariable::List::Add(ScriptVariable *add)
{
	ScriptVariableListCompareFuncHelper helper(*add);
	iterator iter = std::lower_bound(begin(), end(), helper, ScriptVariableListCompareFunc);
	
//	if (iter != end() && **iter == *add)
	if (helper.found)
	{
//		**iter = *add;
		*helper.found = *add;
		delete add;
	}
	else
	{
		//	Not found or not a match
		insert(iter, add);
	}
	
//	ScriptVariable *var = Find(add->GetName(), add->GetNameHash(), add->GetContext());
	
//	if (!var)
//	{
//		push_back(add);
//	}
//	else
//	{
//		*var = *add;
//		delete add;
//	}

	return *this;
}


ScriptVariable::List &ScriptVariable::List::Add(const char *name, Entity *e, int context)
{
//	if (!e || e->IsPurged())
//		return Add(name, "0", context);
	
//	BUFFER(buf, MAX_INPUT_LENGTH);
//	sprintf(buf, "%s", ch->GetIDString());
	return Add(name, ScriptEngineGetUIDString(e), context);
}


ScriptVariable::List &ScriptVariable::List::Add(const char *name, int i, int context)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	sprintf(buf, "%d", i);
	return Add(name, buf, context);
}


ScriptVariable::List &ScriptVariable::List::Add(const char *name, float f, int context)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	sprintf(buf, "%f", f);
	return Add(name, buf, context);
}


ScriptVariable::List &ScriptVariable::List::Add(const List &list)
{
	for (const_iterator var = list.begin(); var != list.end(); ++var)
	{
//		iterator iter = std::lower_bound(begin(), end(), ScriptVariableListCompareFuncHelper(**var), ScriptVariableListCompareFunc);
//		
//		if (iter != end() && **iter == **var)
//		{
//			**iter = **var;
//		}
//		else
//		{
//			//	Not found or not a match
//			insert(iter, new ScriptVariable(**var));
//		}
		
		
		Add(new ScriptVariable(**var));
	}
	
	return *this;
}


void ScriptVariable::List::Sort()
{
	std::sort(begin(), end(), ScriptVariableListSortFunc);
}


ScriptVariable *ScriptVariable::List::Find(const char *name, Hash hash, int context, bool extract)
{
	ScriptVariableListCompareFuncHelper helper(name, hash, context);
	iterator iter = std::lower_bound(begin(), end(), helper, ScriptVariableListCompareFunc);

	//if (iter == end() || !(*iter)->IsMatch(name, hash, context))
	if (!helper.found)
		return NULL;
	
	//ScriptVariable *var = *iter;
	
	if (extract)
		erase(iter);
	
	//return var;
	return helper.found;

//	if (empty())
//		return NULL;
	
//	for (iterator iter = begin(), endIter = end(); iter != endIter; ++iter)
//	{
//		ScriptVariable *var = *iter;
//		if (var->IsMatch(name, hash, context))
//		{
//			if (extract)
//			{
//				erase(iter);
//			}
//			return var;
//		}
//	}
//	
//	return NULL;
}


#if defined(SCRIPTVAR_TRACKING)
unsigned int ScriptVariable::Implementation::ms_Count = 0;
unsigned int ScriptVariable::Implementation::ms_TotalRefs = 0;
#endif


TrigData::List	globalTriggers;


void LoadGlobalTriggers()
{
	Lexi::BufferedFile	file(GLOBALSCRIPTS_FILE);

	if (file.bad())
	{
		mudlogf( NRM, LVL_STAFF, TRUE,  "SYSERR: Couldn't open global scripts file " GLOBALSCRIPTS_FILE);
		return;
	}
	
	Lexi::Parser	parser;
	parser.Parse(file);
	
	int numScripts = parser.NumFields("Scripts");
	for (int i = 0; i < numScripts; ++i)
	{
		VirtualID vid = parser.GetIndexedVirtualID("Scripts", i);
		ScriptPrototype *proto = trig_index.Find(vid);
		
		if (proto)
		{
			globalTriggers.push_back(TrigData::Create(proto));
		}
	}
}


void SaveGlobalTriggers()
{
	Lexi::OutputParserFile file(GLOBALSCRIPTS_FILE ".new");
	
	if (file.fail())
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write global scripts list to disk!");
		return;
	}
	
	file.BeginParser();
	file.BeginSection("Scripts");
		int i = 0;
		FOREACH(TrigData::List, globalTriggers, iter)
		{
			TrigData *t = *iter;
			if (t->m_Type == GLOBAL_TRIGGER && t->GetVirtualID().IsValid())
				file.WriteVirtualID(Lexi::Format("Script %d", ++i).c_str(), t->GetVirtualID());
		}
	file.EndSection();
	file.EndParser();
	
	file.close();
	
	//	We're fubar'd if we crash between the two lines below.
	remove(GLOBALSCRIPTS_FILE);
	rename(GLOBALSCRIPTS_FILE ".new", GLOBALSCRIPTS_FILE);
}


ACMD(do_gscript)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	argument = one_argument(argument, arg);
	
	if (!str_cmp(arg, "show"))
	{
		BUFFER(string, MAX_STRING_LENGTH * 4);
		
		strcpy(string, "Global Scripts:\n");
		
		int i = 0;
		FOREACH(TrigData::List, globalTriggers, iter)
		{
			TrigData *t = *iter;
			
			sprintf_cat(string, "%5d. [%10s] %s\n", ++i, t->GetVirtualID().Print().c_str(), t->GetName());
			
			if (strlen(string) > ((MAX_STRING_LENGTH * 4) - 1024))
			{
				strcat(string, "*** OVERFLOW ***\n");
				break;
			}
		}
		
		if (i == 0)	strcat(string, "*** None found.\n");
		
		page_string(ch->desc, string);
	}
	else if (!str_cmp(arg, "remove"))
	{
		VirtualID vid(argument);
		
		if (!vid.IsValid())
			ch->Send("Usage: gscript remove <vnum>\n");
		else
		{
			FOREACH(TrigData::List, globalTriggers, iter)
			{
				TrigData *trig = *iter;
				
				if (trig->GetVirtualID() == vid)
				{
					ch->Send("Script %s removed from the global script list.\n", vid.Print().c_str());

					mudlogf( NRM, LVL_STAFF, TRUE,  "%s removed script %s (%s) from the global script list.",
						ch->GetName(), trig->GetVirtualID().Print().c_str(), trig->GetName() );
					
					globalTriggers.remove(trig);
					
					trig->Extract();
					
					SaveGlobalTriggers();
					
					return;
				}
			}
			
			ch->Send("Script %s is not in the global script list.\n", vid.Print().c_str());
		}
	}
	else if (!str_cmp(arg, "add"))
	{
		ScriptPrototype *proto = trig_index.Find(argument);

		if (!proto)
			ch->Send("Uknown script '%s'.\n", argument);
		else if (proto->m_pProto->m_Type != GLOBAL_TRIGGER)
			ch->Send("Script %s type is not global.\n", proto->GetVirtualID().Print().c_str());
		else
		{
			FOREACH(TrigData::List, globalTriggers, iter)
			{
				if ((*iter)->GetPrototype() == proto)
				{
					ch->Send("Script %s is already in the global script list.\n", proto->GetVirtualID().Print().c_str());
					return;
				}
			}
			
			TrigData *trig = TrigData::Create(proto);
			
			ch->Send("Script %s added to the global script list.\n", trig->GetVirtualID().Print().c_str());

			mudlogf( NRM, LVL_STAFF, TRUE,  "%s added script %s (%s) to the global script list.",
				ch->GetName(), trig->GetVirtualID().Print().c_str(), trig->GetName() );
			
			globalTriggers.push_back(trig);
			SaveGlobalTriggers();
		}
	}
	else
	{
		ch->Send("Usage:\n"
			"gscript show\n"
			"gscript add <vnum>\n"
			"gscript remove <vnum>\n");
	}
}


BehaviorSet::BehaviorMap	BehaviorSet::ms_Sets;


BehaviorSet::BehaviorSet(const char *name) :
	m_Name(name)
//,	m_NameHash(GetStringFastHash(name))
{
	
}


void BehaviorSet::Apply(Scriptable *sc) const
{
	FOREACH_CONST(ScriptVector, m_Scripts, vnum)
	{
		if (!sc->HasTrigger(*vnum))
		{
			ScriptPrototype *proto = trig_index.Find(*vnum);
			
			if (proto)
			{
				sc->AddTrigger(TrigData::Create(proto));
			}
		}
	}
	
	sc->m_Globals.Add(m_Variables);	
//	for (ScriptVariable::List::const_iterator var = m_Variables.begin(); var != m_Variables.end(); ++var)
//	{
//		if (!sc->m_Globals.Find(*var))
//			sc->m_Globals.Add(new ScriptVariable(**var));
//	}
}


void BehaviorSet::ApplySets(const BehaviorSets &sets, Scriptable *sc)
{
	if (sets.empty())	return;
	
	for (BehaviorSets::const_iterator s = sets.begin(), e = sets.end(); s != e; ++s)
	{
		//BehaviorSet *pSet = BehaviorSet::Find(*s);
		const BehaviorSet *	pSet = *s;
		if (pSet)	pSet->Apply(sc);
	}
}


BehaviorSet *BehaviorSet::Find(/*Hash hash*/ const char *name)
{
	Hash hash = GetStringFastHash(name);
	
	BehaviorMap::iterator iter = ms_Sets.find(name);
	
	return (iter != ms_Sets.end()) ? iter->second : NULL;
}


void BehaviorSet::SaveSet(const BehaviorSet &behavior)
{
	BehaviorSet *set = Find(behavior.GetName());	//	Heh, that's evil.  I like it!
	if (!set)	ms_Sets[behavior.m_Name] = new BehaviorSet(behavior);
	else		*set = behavior;
	Save();
}



#define BEHAVIORS_FILE "etc/behaviors"

void BehaviorSet::Load()
{
	Lexi::BufferedFile	file(BEHAVIORS_FILE);
	Lexi::Parser	parser;

	parser.Parse(file);
	
	int numBehaviors = parser.NumSections("Behaviors");
	for (int i = 0; i < numBehaviors; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Behaviors", i);
		
		BehaviorSet *behavior = new BehaviorSet(parser.GetString("Name"));
		
		int numScripts = parser.NumFields("Scripts");
		for (int s = 0; s < numScripts; ++s)
		{
			VirtualID script = parser.GetIndexedVirtualID("Scripts", s);
			
//			if (FindScript(script))
			if (script.IsValid())
				behavior->m_Scripts.push_back(script);
		}
		
		int numVariables = parser.NumSections("Variables");
		for (int v = 0; v < numVariables; ++v)
		{
			PARSER_SET_ROOT_INDEXED(parser, "Variables", v);
			
			ScriptVariable *var = ScriptVariable::Parse(parser);
			
			if (var)		behavior->m_Variables.Add(var);
		}
		
		ms_Sets[behavior->m_Name] = behavior;
	}
}


void BehaviorSet::Save()
{
	Lexi::OutputParserFile	file(BEHAVIORS_FILE ".new");
	
	file.BeginParser();
	
	int count = 0;
	FOREACH(BehaviorMap, ms_Sets, iter)
	{
		BehaviorSet *behavior = iter->second;
		
		file.BeginSection(Lexi::Format("Behaviors/%d", ++count));
		{
			file.WriteString("Name", behavior->m_Name.c_str());

			file.BeginSection("Scripts");
			{
				int count = 0;
				FOREACH(ScriptVector, behavior->m_Scripts, vnum)
				{
					file.WriteVirtualID(Lexi::Format("Script %d", ++count).c_str(), *vnum);
				}
			}
			file.EndSection();
			
			file.BeginSection("Variables");
			{
				ScriptVariable::List &variables = behavior->m_Variables;
				int count = 0;
				FOREACH(ScriptVariable::List, variables, var)
				{
					(*var)->Write(file, Lexi::Format("Variable %d", ++count), false);
				}
			}
			file.EndSection();
		}
		file.EndSection();
	}

	file.EndParser();
	
	file.close();
	
	remove(BEHAVIORS_FILE);
	rename(BEHAVIORS_FILE ".new", BEHAVIORS_FILE);
}


static bool BehaviorSetSorter(const BehaviorSet *lhs, const BehaviorSet *rhs)
{
	return str_cmp(lhs->GetName(), rhs->GetName()) < 0;	
}


void BehaviorSet::Show(CharData *ch)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	ch->Send("Behavior Set: %s\n", GetName());
	
	int counter = 0;
	if (!GetScripts().empty())
	{
		ch->Send("`y-- Scripts (%d):`n\n", GetScripts().size());

		FOREACH(ScriptVector, GetScripts(), i)
		{
			ScriptPrototype *proto = trig_index.Find(*i);
			
			ch->Send("  [`c%10s`n] %s\n", i->Print().c_str(), proto ? proto->m_pProto->GetName() : "(Uncreated)");
		}
	}
	else
		ch->Send("`y-- No scripts`n\n");
	
	if (!GetVariables().empty())
	{
		ch->Send("`y-- Global Variables (%d):`n\n", GetVariables().size());
		FOREACH (ScriptVariable::List, GetVariables(), iter)
		{
			ScriptVariable *var = *iter;
			strcpy(buf, var->GetName());
			if (var->GetContext())		sprintf_cat(buf, " {%d}", var->GetContext());
			ch->Send("  %-30.30s: %s\n", buf, var->GetValue());
		}
	}
	else
		ch->Send("`y-- No Variables`n\n");
}


void BehaviorSet::List(CharData *ch)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	strcpy(buf, "Behavior Sets\n"
				"-------------\n");
	
	std::vector<BehaviorSet *> setArray;
	setArray.reserve(sizeof(ms_Sets));
	FOREACH(BehaviorMap, ms_Sets, iter)
		setArray.push_back(iter->second);
	
	std::sort(setArray.begin(), setArray.end(), BehaviorSetSorter);
	
	int counter = 1;
	FOREACH(std::vector<BehaviorSet *>, setArray, iter)
	{
		BehaviorSet *set = *iter;
		snprintf_cat(buf, MAX_STRING_LENGTH, "`g%2d`n) %-30.30s %s",
			counter, set->GetName(), (counter % 2) == 0 ? "\n" : "");
		++counter;
	}
	
	page_string(ch->desc, buf);
}
