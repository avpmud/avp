#ifndef __SCRIPTCOMPILER_H__
#define __SCRIPTCOMPILER_H__

#include "types.h"
#include "interpreter.h"

#include "SharedPtr.h"


typedef unsigned int bytecode_t;

class CompiledScript;

int ScriptEngineRunScript(Entity *go, TrigData *trig, char *output = NULL, TrigThread *thread = NULL);
int ScriptEngineRunScript(TrigThread *pThread);

void ScriptCompilerParseScript(const char *script, CompiledScript *bytecode, char *errorBuffer, bool bDebugMode);
void ScriptCompilerDumpCompiledScript(CompiledScript *bytecode, char *buf, size_t buflen);

const char * ScriptEngineParseVariable(const char *var, char *out, int &context);
const char * ScriptEngineParseVariableWithEntity(const char *var, char *out, int &context, Entity *&e);

const char *ScriptEngineGetArgument(const char *src, char *dst);
const char *ScriptEngineGetParameter(const char *src, char *dst);
const char *ScriptEngineGetParameter(const char *src, char *dst, int num);
const char * ScriptParserMatchQuote(const char *p);
const char * ScriptParserMatchCurlyBracket(const char *p);
const char * ScriptParserMatchBracket(const char *p);
const char * ScriptParserMatchParenthesis(const char *p);
const char * ScriptParserMatchPercent(const char *p);
const char * ScriptParserExtractContents(const char *p, const char *pp, char *buf);

void ScriptEngineReportError(class TrigThread *pThread, bool toFile, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

struct DereferenceData
{
	enum ResultType
	{
		ResultString,
		ResultConstCharPtr,
		ResultScriptVariable
	};
	
	DereferenceData() : var(NULL), context(0), subscript(0), field(NULL), fieldcontext(0), subfield(NULL), resultType(ResultString), result(NULL), scriptVarResult(defaultScriptVar) {}
	
	const char *		var;
	int					context;
	int					subscript;
	const char *		field;
	int					fieldcontext;
	const char *		subfield;
	
	ResultType			resultType;
	char *				result;
//	const char *		constCharPtrResult;
	ScriptVariable		scriptVarResult;
	
	static ScriptVariable	defaultScriptVar;

//	void				SetResult(const char **var)
//	{
//		scriptVar = *var;
//		resultType = ResultScriptVariable;
//	}
	
	void				SetResult(ScriptVariable *var)
	{
		scriptVarResult = *var;
		resultType = ResultScriptVariable;
	}
	
	const char *		GetResult()
	{
		if (resultType == ResultScriptVariable)
			return scriptVarResult.GetValue();
//		else if (resultType == ResultTypeConstcharPtr)
//			return constCharPtrResult;
		return result;
	}
};

void ScriptEngineDereference(Scriptable *sc, TrigThread *thread, DereferenceData &data, bool continuation);
ScriptVariable *ScriptEngineFindVariable(Scriptable *sc, TrigThread *thread, const char *var, Hash hash, int context, unsigned scope /*= Scope::All*/);
const char * ScriptEngineParseVariable(const char *var, char *out, int &context);

void ScriptEngineOperation(TrigThread *thread, const char *lhs, const char *rhs, int operation, char *result);
bool ScriptEngineOperationEvaluateAsBoolean(const char *value, STR_TYPE type);

enum Bytecodes
{
	SBC_FINAL,					//	0
	SBC_START,					//	First operand; necessary
	SBC_WHILE,
	SBC_CASE,
	SBC_DEFAULT,
	SBC_SCOPE,	//	5
	SBC_HALT,
	SBC_RETURN,					//	DELETEME
	SBC_THREAD,					//	Thread a function onto another object
	SBC_SET,
	SBC_UNSET,	//	10
	SBC_WAIT,
	SBC_PUSHFRONT,
	SBC_POPFRONT,
	SBC_PUSHBACK,
	SBC_POPBACK,	//	15
	SBC_DELAY,
	SBC_PRINT,
	SBC_FASTMATH,
	SBC_COMMAND,
	SBC_PCCOMMAND,	//	20
	
//	Stack Manipulation
	SBC_PUSH,					//	table #
	SBC_OPERATION,				//	operation
	SBC_DEREFERENCE,
	SBC_STRING,			//	count
	SBC_BRANCH,		//	25
	SBC_BRANCH_FALSE,
	SBC_BRANCH_LAZY_FALSE,
	SBC_BRANCH_LAZY_TRUE,
	
	SBC_LINE,					//	Debugging purposes
};


#define SBC_MASK		0xFF
#define	SBC_SHIFT		8


enum Operation
{
	OP_OR,
	OP_XOR,
	OP_AND,
	OP_BIT_OR,
	OP_BIT_XOR,
	OP_BIT_AND,
	OP_EQ,
	OP_NEQ,
	OP_LEEQ,
	OP_GREQ,
	OP_LE,
	OP_GR,
	OP_CONTAINS,
	OP_BITSHIFT_LEFT,
	OP_BITSHIFT_RIGHT,
	OP_SUB,
	OP_ADD,
	OP_DIV,
	OP_MULT,
	OP_MOD,
	OP_NOT,
	OP_BITWISE_COMPLEMENT,
	OP_COMMA,
	OP_NONE,
	
	OP_SPECIAL_FOREACH,
	
	OP_COUNT = OP_NONE
};

enum DereferenceFlags
{
	DEREF_FIELD			= 1 << 0,	//	Mutually exclusive with SUBFIELD
	DEREF_SUBFIELD		= 1 << 1,	//	Mutually exclusive with CONTEXT and SUBFIELD
	DEREF_CONTEXT		= 1 << 2,	//	Mutually exclusive with SUBVAR
	DEREF_FIELDCONTEXT	= 1 << 3,	//	Mutually exclusive with SUBVAR
	DEREF_SUBSCRIPT		= 1 << 4,	//	Mutually exclusive with SUBVAR
	DEREF_SUBSCRIPT_ONLY= 1 << 5,	//	Mutually exclusive with SUBVAR
	DEREF_CONTINUED		= 1 << 6	//	Mutually exclusive with FIELD, FIELD_CONTEXT
};


//	%var%			
//	%var()%			Subvar
//	%var{}%			Context
//	%var{}()%		NOT LEGAL
//	%var.field%		Field
//	%var{}.field%	Context, Field
//	%var.field{}%	Field, Field-Context
//	%var{}.field{}%	Context, Field, Field-Context	-	SPLIT?
//	%var().field%	Subvar, Field	 				-	SPLIT?... or this may be good!
//	%var().field{}%	Subvar, Field, Field-Context	-	SPLIT?... or this may be good!
//	%var.field()%	Subfield
//	%var{}.field()%	Context, Subfield
//	%var().field()%	Subvar, Subfield				-	SPLIT?... or this may be good!
//	%var.subfield{}()%	NOT LEGAL

//	%var%			
//	%var()%			Subvar
//	%var{}%			Context
//	%var{}()%		NOT LEGAL
//	%var.field%		Field
//	%var{}.field%	Context, Field
//	%var.field{}%	Field, Field-Context
//	%var{}.field{}%	Context, Field, Field-Context	-	SPLIT?
//	%var().field%	Subvar, Field	 				-	SPLIT?... or this may be good!
//	%var().field{}%	Subvar, Field, Field-Context	-	SPLIT?... or this may be good!
//	%var.field()%	Subfield
//	%var{}.field()%	Context, Subfield
//	%var().field()%	Subvar, Subfield				-	SPLIT?... or this may be good!
//	%var.subfield{}()%	NOT LEGAL


class CompiledScript : public Lexi::Shareable
{
public:
						CompiledScript(ScriptPrototype *prototype);
						~CompiledScript();

public:
	struct Function
	{
		Function(const char *nm, unsigned int off) : name(nm), hash(GetStringFastHash(nm)), hasArguments(false), offset(off) {}
		
		Lexi::String		name;
		Hash				hash;
		
		Lexi::StringList	arguments;
		bool				hasArguments;
		
		unsigned int		offset;
	
	private:
		Function();
	};
	
	typedef std::vector<Function>	FunctionList;
	
	Function *			FindFunction(const char *functionName, Hash hash = 0);
	Function *			FindFunction(const char *functionName, CompiledScript *&pMatchingScript);
	void				SortFunctions();

	typedef std::vector<ScriptPrototype *>	ImportList;
	
	bytecode_t *		m_Bytecode;
	unsigned int		m_BytecodeCount;
	
	ScriptPrototype *	m_Prototype;
	Lexi::SharedString	m_Script;
	
	char *				m_Table;
	unsigned int		m_TableSize;
	unsigned int		m_TableCount;
	
	FunctionList		m_Functions;
	ImportList			m_Imports;
	ScriptVariable::List	m_Constants;
};



const char	PARAM_SEPARATOR			= '\t';

extern const char *RECORD_START_DELIMITER;
extern const char *RECORD_END_DELIMITER;
extern const char *RECORD_ITEM_DELIMITER;
const char	RECORD_FIELD_DELIMITER		= ':';

const char	RECORD_START_DELIMITER_1	= '[';
const char	RECORD_START_DELIMITER_2	= '#';

const char	RECORD_END_DELIMITER_1		= '#';
const char	RECORD_END_DELIMITER_2		= ']';

const char	RECORD_ITEM_DELIMITER_1		= '#';
const char	RECORD_ITEM_DELIMITER_2		= '#';

inline bool ScriptEngineRecordIsRecordStart(const char *src)
{
	return src[0] == RECORD_START_DELIMITER_1
		&& src[1] == RECORD_START_DELIMITER_2;
}

inline bool ScriptEngineRecordIsRecordEnd(const char *src)
{
	return src[0] == RECORD_END_DELIMITER_1
		&& src[1] == RECORD_END_DELIMITER_2;
}

inline bool ScriptEngineRecordIsRecordItem(const char *src)
{
	return src[0] == RECORD_ITEM_DELIMITER_1
		&& src[1] == RECORD_ITEM_DELIMITER_2;
}


inline const char *ScriptEngineRecordSkipDelimiter(const char *src)
{
	return src + 2;
}


bool ScriptEngineRecordGetField(const char *src, const char *field, char *dst);

double ScriptEngineParseDouble(const char *str);
void ScriptEnginePrintDouble(char *result, double d);

long long ScriptEngineParseInteger(const char *str);
int ScriptEnginePrintInteger(char *result, long long d);

const char *ScriptEngineGetUIDString(Entity *e);	//	WARNING: uses static internal buffer; volatile!

#endif
