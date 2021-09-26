/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2003-2007        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : scriptcompiler.cp                                              [*]
[*] Usage: Script compiler and virtual machine                            [*]
\***************************************************************************/


#include "scripts.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "scriptcompiler.h"
#include "characters.h"
#include "find.h"
#include "rooms.h"
#include "objects.h"
#include "event.h"

#include <math.h>


extern LexiScriptCommands::Commands lookup_script_line(const char *p);



extern const char *ScriptEngineListSkipWord(const char *src);
extern const char *ScriptEngineListCopyNextWord(const char *src, char *&dst);



struct ScriptCompilerBytecodeTable;


ScriptVariable DereferenceData::defaultScriptVar("");

//#define strtoll(a,b,c) atoll(a)
//#define strtoull(a,b,c) (unsigned long long)atoll(a)


char *ops[OP_COUNT + 2] = {	//	Reverse priority
	"||",
	"^^",
	"&&",
	"|",
	"^",
	"&",
	"==",
	"!=",
	"<=",
	">=",
	"<",
	">",
	"/=",
	"<<",
	">>",
	"-",
	"+",
	"/",
	"*",
	"@",
	"!",
	"~",
	",",
	"\n",
	"FOREACH"
};


class ScriptCompilerArgument
{
public:
	ScriptCompilerArgument() : mNext(NULL) { }
	ScriptCompilerArgument(const char *arg) : mArg(arg), mNext(NULL) { /*assert(mArg.size() < 1024);*/ }
	virtual ~ScriptCompilerArgument() { delete mNext; }

	Lexi::String			mArg;
	
	ScriptCompilerArgument *mNext;
	
	virtual bool			IsVariable() { return false; }
	virtual bool			IsOperation() { return false; }
	virtual bool			IsString() { return false; }
	
	virtual void			Compile(ScriptCompilerBytecodeTable &scbt);
};


class ScriptCompilerVariable : public ScriptCompilerArgument {
public:
	ScriptCompilerVariable() : mField(NULL), mSubfield(NULL), mContext(NULL), mSubscript(NULL) { }
	virtual ~ScriptCompilerVariable() { delete mField; delete mSubfield; delete mContext; delete mSubscript; }
	
	ScriptCompilerVariable *mField;
	ScriptCompilerArgument *mSubfield;
	ScriptCompilerArgument *mContext;
	ScriptCompilerArgument *mSubscript;

	virtual bool			IsVariable() { return true; }

	virtual void			Compile(ScriptCompilerBytecodeTable &scbt);

};


class ScriptCompilerOperation : public ScriptCompilerArgument {
public:
	ScriptCompilerOperation() : mOperation(OP_NONE), mLeft(NULL), mRight(NULL) { }
	virtual ~ScriptCompilerOperation() { delete mLeft; delete mRight; }
	
	Operation 				mOperation;
	ScriptCompilerArgument *mLeft;
	ScriptCompilerArgument *mRight;
	
	virtual bool			IsOperation() { return true; }

	virtual void			Compile(ScriptCompilerBytecodeTable &scbt);
};


class ScriptCompilerString : public ScriptCompilerArgument
{
public:
	ScriptCompilerString() : mVariables(NULL) { }
	virtual ~ScriptCompilerString() { delete mVariables; }
	
	Lexi::String			mString;
	ScriptCompilerArgument *mVariables;
	
	virtual bool			IsString() { return true; }
	
	virtual void			Compile(ScriptCompilerBytecodeTable &scbt);
};


class ScriptCompilerCommand
{
public:
	ScriptCompilerCommand() : mCommand(LexiScriptCommands::None), mCommandNum(0), mLineNum(0), mArguments(NULL), mVarArgCount(0), mPrev(NULL), mNext(NULL), mChildren(NULL), mParent(NULL), mBytecodeStart(0) { }
	virtual ~ScriptCompilerCommand()
	{
		delete mArguments;
		delete mNext;
		delete mChildren;
/*		ScriptCompilerCommand *scc = mChildren;
		while (scc)
		{
			ScriptCompilerCommand *next = scc->mNext;
			delete scc;
			scc = next;
		}
*/
	}

	ScriptCompilerCommand *	GenerateTree();
	ScriptCompilerCommand *	GenerateTreeUntil(int terminator);
	void					Compile(ScriptCompilerBytecodeTable &scbt);

	LexiScriptCommands::Commands	mCommand;
	int						mCommandNum;
	Lexi::String			mLine;
	int						mLineNum;
	
	ScriptCompilerArgument *mArguments;
	int						mVarArgCount;
	
	ScriptCompilerCommand *	mPrev;
	ScriptCompilerCommand *	mNext;
	ScriptCompilerCommand *	mChildren;
	ScriptCompilerCommand *	mParent;
	
	unsigned int			mBytecodeStart;
	std::list<unsigned int>	mClosures;

private:
	void					CompileIf(ScriptCompilerBytecodeTable &scbt);
	void					CompileWhile(ScriptCompilerBytecodeTable &scbt);
	void					CompileSwitch(ScriptCompilerBytecodeTable &scbt);
	
	void					CompileArguments(ScriptCompilerBytecodeTable &scbt);
	void					CompileBlock(ScriptCompilerBytecodeTable &scbt);
	
	void					MoveFromNextToChild(ScriptCompilerCommand *scc);
	void					MoveFromChildToNext(ScriptCompilerCommand *scc);
};


/* local structures */

//	Script Execution Core
void ScriptEngineOperation(TrigThread *thread, const char *lhs, const char *rhs, int operation, char *result);
void ScriptEngineString(const char *formatp, ThreadStack &stack, char *resultp);

//	Script Execution Commands
void ScriptEngineRunScriptThreadFunctionOntoEntity(Scriptable *sc, TrigThread *thread, const char *entity, const char *functionName, const char *functionArgs);
bool ScriptEngineRunScriptAsFunctionOnEntity(Scriptable *sc, TrigThread *thread, Entity *e, const char *functionName, const char *functionArgs, char *output);
void ScriptEngineRunScriptProcessSet(Scriptable *sc, TrigThread *thread, const char *var, const char *value, unsigned scope);
void ScriptEngineRunScriptProcessUnset(Scriptable *sc, TrigThread *thread, const char *buf);
void ScriptEngineRunScriptProcessScope(Scriptable *sc, TrigThread *thread, const char *var, unsigned scope);
void ScriptEngineRunScriptProcessPushfront(Scriptable *sc, TrigThread *thread, const char *name, const char *value);
void ScriptEngineRunScriptProcessPushback(Scriptable *sc, TrigThread *thread, const char *name, const char *value);
void ScriptEngineRunScriptProcessPopfront(Scriptable *sc, TrigThread *thread, const char *name, const char *popvar, char *result = NULL);
void ScriptEngineRunScriptProcessPopback(Scriptable *sc, TrigThread *thread, const char *name, const char *popvar);
void ScriptEngineRunScriptProcessWait(Scriptable *sc, TrigThread *thread, const char *arg);
void ScriptEngineRunScriptProcessDelay(Scriptable *sc, TrigThread *thread, const char *arg);
void ScriptEngineRunScriptProcessPrint(Scriptable *sc, TrigThread *thread, const char *varname, const char *format, int varArgCount, ThreadStack &stack );
void ScriptEngineRunScriptProcessFastMath(Scriptable *sc, TrigThread *thread, const char *name, Operation op, const char *constant = "1");
void ScriptEngineRunScriptEnterFunction(TrigThread *pThread, CompiledScript *pCompiledScript, CompiledScript::Function *pFunction, const char *functionArgs, CallstackEntry::CallType calltype = CallstackEntry::Call_Normal);

//	Compiler Core
void ScriptCompilerCompileScript(ScriptCompilerCommand *scc, ScriptCompilerBytecodeTable &scbt);
void ScriptCompilerCompileArguments(ScriptCompilerArgument *sca, ScriptCompilerBytecodeTable &scbt);

//	Parser Core
void ScriptParserParseScriptReportError(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void ScriptParserParseScriptClearErrors();
ScriptCompilerCommand *	ScriptParserParseScript(const char *scriptText, char *errorBuffer, bool bDebugMode);
ScriptCompilerArgument *ScriptParserParseVariable(const char *varString);
ScriptCompilerArgument *ScriptParserParseLiteral(const char *&varString);
ScriptCompilerArgument *ScriptParserParseOneArgument(const char *&varString);
ScriptCompilerArgument *ScriptParserParseVariableName(const char *&varString);
ScriptCompilerArgument *ScriptParserParseEvaluationStatement(const char *string);	//	Not const because we modify
ScriptCompilerArgument *ScriptParserParseString(const char *string);
ScriptCompilerArgument *ScriptParserParseFunction(const char *varString);

//	Parser Utilities
const char * ScriptParserExtractContents(const char *p, const char *pp, char *buf);

/*
class ScriptParserError {
public:
				ScriptParserError(const char *err, int ln) : error(str_dup(err)), line(ln) { }
				~ScriptParserError() { free(error); }
	char *		error_message;
	int			line;
};

int							ScriptParserCurrentLine;
LList<ScriptParserError *>	ScriptParserErrors;
*/


Lexi::String		ScriptParserError;
#define MAX_SCRIPT_ERRORS 5


void ScriptParserParseScriptReportError(const char *format, ...)
{
 	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);

	va_list args;

	if (!format || !*format || !ScriptParserError.empty() /*ScriptParserErrors.Count() > MAX_SCRIPT_ERRORS*/)
		return;
		
	va_start(args, format);
	int length = vsnprintf(buf, MAX_SCRIPT_STRING_LENGTH, format, args);
	va_end(args);
	
	ScriptParserError = buf;
}


void ScriptParserParseScriptClearErrors()
{
	ScriptParserError.clear();
}



/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
*/
const char * ScriptParserMatchQuote(const char *p) {
	char c;
	for (++p; (c = *p) && (c != '"'); ++p) {
		if (c == '\\' && *(p+1))	++p;
	}
//	if (!*p)	--p;
	return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
const char * ScriptParserMatchCurlyBracket(const char *p) {
	char c;
	for (++p; (c = *p) && (c != '}'); ++p) {
		if (c == '(')		{ p = ScriptParserMatchParenthesis(p);	if (!*p) break; }
		else if (c == '%')	{ p = ScriptParserMatchPercent(p);		if (!*p) break; }
		else if (c == '"')	{ p = ScriptParserMatchQuote(p);		if (!*p) break; }
	}
	return p;
}


/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
const char * ScriptParserMatchBracket(const char *p) {
	char c;
	for (++p; (c = *p) && (c != ']'); ++p) {
		if (c == '(')		{ p = ScriptParserMatchParenthesis(p);	if (!*p) break; }
		else if (c == '%')	{ p = ScriptParserMatchPercent(p);		if (!*p) break; }
		else if (c == '"')	{ p = ScriptParserMatchQuote(p);		if (!*p) break; }
	}
	return p;
}


/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
const char * ScriptParserMatchParenthesis(const char *p) {
	int i;
	char c;
	for (++p, i = 1; (c = *p); ++p) {
		if (c == '(')			++i;
		else if (c == ')')	{ --i; if (i == 0) break; }
		else if (c == '"')	{ p = ScriptParserMatchQuote(p);		if (!*p) break; }
	}
	return p;
}

/*
 * p points to the first percent.  returns a pointer to the
 * matching closing percent, or the last non-null char in p.
 */
const char * ScriptParserMatchPercent(const char *p) {
	char c;
	for (++p; (c = *p) && (c != '%'); ++p) {
		if (c == '(')		{ p = ScriptParserMatchParenthesis(p);	if (!*p) break; }
		else if (c == '{')	{ p = ScriptParserMatchCurlyBracket(p);	if (!*p) break; }
		else if (c == '[')	{ p = ScriptParserMatchBracket(p);		if (!*p) break; }
	}
	return p;
}


const char * ScriptParserExtractContents(const char *p, const char *pp, char *buf)
{
	char *bufp;
	
	*buf = 0;
	bufp = buf;

	++p;	//	past '('
	while (isspace(*p))	//	past spaces
		++p;
	
	//	copy it
	//	< because pp is ')' or could be the NULL term
	while (p < pp)
		*(bufp++) = *(p++);			
	*bufp = 0;
	
	--bufp;	//	drop back into string
	while (bufp >= buf && isspace(*bufp))	//	move back while space
		--bufp;
	++bufp;	//	step forward to space/null
	*bufp = 0;	//	null or re-null
	
	if (*pp)
		++pp;
	
	return pp;
}


//	Assumes it is receiving the CONTENTS (minus % and skipping spaces) of a
//	%var.var.var(var).var% type string
ScriptCompilerArgument * ScriptParserParseVariable(const char *varString)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
	ScriptCompilerVariable * scv;
	ScriptCompilerVariable * cv;
	const char *p, *pp;
	char *bufp;
	char c;
	
	p = varString;
	scv = new ScriptCompilerVariable;
	cv = scv;
	
	while (*p) {
		if (*p == '{')
		{
			if (cv->mContext)
			{
				//	Expects either '.' or end of string
				ScriptParserParseScriptReportError("Second context found in '%s'", varString);
				break;
			}
			
			pp = ScriptParserMatchCurlyBracket(p);
			if (!*pp) ScriptParserParseScriptReportError("Unmatched '{'");
			p = ScriptParserExtractContents(p, pp, buf);
			
			cv->mContext = ScriptParserParseEvaluationStatement(buf);
		}
		else if (*p == '[')
		{
			if (cv->mSubscript)
			{
				//	Expects either '.' or end of string
				ScriptParserParseScriptReportError("Second subscript found in '%s'", varString);
				break;
			}
			
			pp = ScriptParserMatchBracket(p);
			if (!*pp) ScriptParserParseScriptReportError("Unmatched '['");
			p = ScriptParserExtractContents(p, pp, buf);
			
			//	buf is now the subsript - we want to process this
			cv->mSubscript = ScriptParserParseEvaluationStatement(buf);
			//	if cv->mSubscript stays NULL, that's ok; it COULD be empty
		}
		else if (*p == '.')
		{
			cv->mField = new ScriptCompilerVariable;
			cv = cv->mField;
			
			++p;	//	step past '.'

			if (!*p) ScriptParserParseScriptReportError("Missing field on variable '%s'", varString);
		}
		else if (*p == '(')
		{
			pp = ScriptParserMatchParenthesis(p);
			if (!*pp) ScriptParserParseScriptReportError("Unmatched '('");
			p = ScriptParserExtractContents(p, pp, buf);
			
			//	buf is now the 'subfield' - we want to process this
			cv->mSubfield = ScriptParserParseEvaluationStatement(buf);
			//	if cv->subfield stays NULL, that's ok; it COULD be empty
		}
		else if (cv->mSubfield)
		{
			//	Expects either '.' or end of string
			ScriptParserParseScriptReportError("Extraneous characters after %s() in '%s'", cv->mSubfield->mArg.c_str(), varString);
			break;
		}
		else
		{
			*buf = 0;
			bufp = buf;
			
			while ((c = *p) && (c != '.') && (c != '(') && (c != '{') && (c != '['))
			{
				*(bufp++) = c;
				++p;
			}
			*bufp = 0;
			
			cv->mArg = buf;
		}
	}
	
	//	COMPILER OPTIMIZATION: eval and string are special-purpose; string should be stripped, eval should be 
	if (scv->mArg == "eval" && scv->mSubfield)
	{
		ScriptCompilerArgument *subfield = scv->mSubfield;
//		cv->mField = scv->mField;
		
		scv->mSubfield = NULL;
//		scv->mField = NULL;
		
		delete scv;
		
		return subfield;
	}
	
	return scv;
}


//	Grab one "arg"
ScriptCompilerArgument *ScriptParserParseOneArgument(const char *&varString)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);

	ScriptCompilerArgument *sca = NULL;
	const char *p, *pp;
	char *bufp;
	
	p = varString;
	while (isspace(*p))
		++p;
	
	if (*p == '%' && *(p+1) != '%')
	{
		pp = ScriptParserMatchPercent(p);
		if (!*pp) ScriptParserParseScriptReportError("Unmatched '%%'");
		p = ScriptParserExtractContents(p, pp, buf);
		sca = ScriptParserParseVariable(buf);
	}
	else if (*p == '"')
	{
		*buf = 0;
		bufp = buf;
		
		pp = ScriptParserMatchQuote(p);
		if (!*pp) ScriptParserParseScriptReportError("Unmatched '\"'");
		
		//	DO NOT use ScriptParserExtractContents as it skips leading and trailing spaces!
		
		++p;	//	Past '"'
		
		while (p != pp)
			*(bufp++) = *(p++);
		*bufp = 0;
		
		if (*p)	++p;

		sca = new ScriptCompilerArgument(buf);	
	}
	else// if (*p)
	{
		*buf = 0;
		bufp = buf;
		
		while (*p && !isspace(*p))
			*(bufp++) = *(p++);
		*bufp = 0;
		
		sca = new ScriptCompilerArgument(buf);
	}
	
	while (isspace(*p))
		++p;
	
	varString = p;
	
	return sca;
}


//	Grab one "arg"
ScriptCompilerArgument *ScriptParserParseLiteral(const char *&varString)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);

	ScriptCompilerArgument *sca = NULL;
	const char *p;
	char *bufp;
	
	p = varString;
	while (isspace(*p))
		++p;
	
	bufp = buf;
	while (*p && !isspace(*p))
	{
		*(bufp++) = *(p++);
	}
	*bufp = 0;
	
	if (*buf)
	{
		sca = new ScriptCompilerArgument(buf);
	}
	
	while (isspace(*p))
		++p;
	
	varString = p;
	
	return sca;
}


//	Grab one "arg"
ScriptCompilerArgument *ScriptParserParseVariableName(const char *&varString)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);

	ScriptCompilerArgument *sca = NULL;
	const char *p, *pp;
	
	p = varString;
	while (isspace(*p))
		++p;
	
/*	if (*p == '%' && *(p+1) != '%')
	{
		pp = ScriptParserMatchPercent(p);
		if (!*pp) ScriptParserParseScriptReportError("Unmatched '%'");
		p = ScriptParserExtractContents(p, pp, buf);
		sca = ScriptParserParseVariable(buf);
	}
	else*/
	if (*p)
	{
		*buf = 0;
		
		pp = p;
		char c = *pp;
		int i = 0;
		while (c && !isspace(c))
		{
/*			if (c == '(')
			{
				pp = ScriptParserMatchParenthesis(pp);
				if (!*pp)
				{
				 	ScriptParserParseScriptReportError("Unmatched '('");
					break;
				}
				//else		++pp;	//	Its not a space, so... we can examine it normally
			}
			else*/
			if (c == '%')
			{
				pp = ScriptParserMatchPercent(pp);
				if (!*pp)
				{
					ScriptParserParseScriptReportError("Unmatched '%%'");
					break;
				}
				else		++pp;	//	It's a % and will match the next iteration
			}
			else if (c == '{')
			{
				pp = ScriptParserMatchCurlyBracket(pp);
				if (!*pp)
				{
					ScriptParserParseScriptReportError("Unmatched '{'");
					break;
				}
				//else		++pp;	//	Its not a space, so... we can examine it normally
			}
			else if (c == '[')
			{
				pp = ScriptParserMatchBracket(pp);
				if (!*pp)
				{
					ScriptParserParseScriptReportError("Unmatched '['");
					break;
				}
				//else		++pp;	//	Its not a space, so... we can examine it normally
			}
			else
				++pp;
			
			c = *pp;
		}
		
		int length = pp - p;
		strncpy(buf, p, length);
		buf[length] = '\0';
		
		if (strchr(buf, '%'))
		{
			const char *p3 = ScriptParserMatchPercent(p);
			
			if (p3 == pp - 1)
			{
				p = ScriptParserExtractContents(p, p3, buf);
				sca = ScriptParserParseVariable(buf);
			}
			else
			{
				int length = pp - p;
				strncpy(buf, p, length);
				buf[length] = '\0';
				sca = ScriptParserParseString(buf);
			}
		}
		else
		{
			sca = new ScriptCompilerArgument(buf);
		}
		p = pp;
	}
	
	while (isspace(*p))
		++p;
	
	varString = p;
	
	return sca;
}


//	Extract FuncName(arg, arg, arg) as an Argument, Evaluation
ScriptCompilerArgument * ScriptParserParseFunction(const char *varString)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
	ScriptCompilerArgument * sca;
	const char *p, *pp;
	char *bufp;
	char c;
	
	p = varString;
	sca = new ScriptCompilerArgument;
	
	while (*p && !isspace(*p))
	{
		if (*p == '(')
		{
			pp = ScriptParserMatchParenthesis(p);
			if (!*pp)	ScriptParserParseScriptReportError("Unmatched '('");
			p = ScriptParserExtractContents(p, pp, buf);
			
			//	buf is now the 'subfield' - we want to process this
			sca->mNext = ScriptParserParseEvaluationStatement(buf);
		}
		else if (sca->mNext)
		{
			//	Expects end of string
			ScriptParserParseScriptReportError("Extraneous characters after %s(...) in '%s'", sca->mArg.c_str(), varString);
			break;
		}
		else
		{
			*buf = 0;
			bufp = buf;
			
			while ((c = *p) && (c != '(') && !isspace(c))
			{
				*(bufp++) = c;
				++p;
			}
			*bufp = 0;
			
			sca->mArg = buf;
		}
	}
	
	if (!sca->mNext)
	{
		sca->mNext = new ScriptCompilerArgument;
	}
	
	return sca;
}


//	Split the string into 'halves' based on the priority of tokens
//	Work backwards on the tokens, since it will split left/right,
//	and when generating opcodes, generate left, right, opcode....
//	thus:
//	(5 + 10) / 4 * 3 + 2
//                   ^
//	             ^
//           ^
//	   ^
//	Becomes (5 + 10) / 4 * 3    +   2
//	Left hand first... (5 + 10)   /   4 * 3
//	Left hand first... (5   +   10)
//	Left hand first... 5
//	Result would be: 5 10 + 4 / 3 * 2 +
//	Has C-style precedence

enum OperationPrecedence
{
	OP_PRECEDENCE_NONE,
	OP_PRECEDENCE_UNARY,
	OP_PRECEDENCE_MUL_DIV,
	OP_PRECEDENCE_ADD_SUB,
	OP_PRECEDENCE_BITSHIFT,
	OP_PRECEDENCE_RELATIONAL_EQUALITY,
	OP_PRECEDENCE_EQUALITY,
	OP_PRECEDENCE_BIT_AND,
	OP_PRECEDENCE_BIT_XOR,
	OP_PRECEDENCE_BIT_OR,
	OP_PRECEDENCE_AND,
	OP_PRECEDENCE_XOR,
	OP_PRECEDENCE_OR,
	OP_PRECEDENCE_COMMA,
};


OperationPrecedence opPrecedences[OP_COUNT] =
{
	OP_PRECEDENCE_OR, OP_PRECEDENCE_XOR, OP_PRECEDENCE_AND,
	OP_PRECEDENCE_BIT_OR, OP_PRECEDENCE_BIT_XOR, OP_PRECEDENCE_BIT_AND,
	OP_PRECEDENCE_EQUALITY, OP_PRECEDENCE_EQUALITY,
	OP_PRECEDENCE_RELATIONAL_EQUALITY, OP_PRECEDENCE_RELATIONAL_EQUALITY,
		OP_PRECEDENCE_RELATIONAL_EQUALITY, OP_PRECEDENCE_RELATIONAL_EQUALITY,
	OP_PRECEDENCE_EQUALITY,
	OP_PRECEDENCE_BITSHIFT, OP_PRECEDENCE_BITSHIFT,
	OP_PRECEDENCE_ADD_SUB, OP_PRECEDENCE_ADD_SUB,
	OP_PRECEDENCE_MUL_DIV, OP_PRECEDENCE_MUL_DIV, OP_PRECEDENCE_MUL_DIV,
	OP_PRECEDENCE_UNARY, OP_PRECEDENCE_UNARY, OP_PRECEDENCE_COMMA
};

Operation opsSortedByLength[OP_COUNT] =
{
	OP_OR, OP_XOR, OP_AND, OP_EQ, OP_NEQ, OP_LEEQ, OP_GREQ, OP_CONTAINS, OP_BITSHIFT_LEFT, OP_BITSHIFT_RIGHT,
	OP_BIT_OR, OP_BIT_XOR, OP_BIT_AND, OP_LE, OP_GR, OP_SUB, OP_ADD, OP_DIV, OP_MULT, OP_MOD, OP_NOT,
	OP_BITWISE_COMPLEMENT, OP_COMMA
};


ScriptCompilerArgument *ScriptParserParseEvaluationStatement(const char *string)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);

	ScriptCompilerArgument *sca = NULL;
	ScriptCompilerOperation *sco;
	
	char *p, *pp;
	
	p = strcpy(buf, string);
	while (isspace(*p))
		++p;
	
	char *	bestOperatorStart = NULL;
	int		bestOp = OP_NONE;
	OperationPrecedence	bestOperatorPrecedence = OP_PRECEDENCE_NONE;
	
	while (*p)
	{
		while (*p && !ispunct(*p))	++p;
		if (!*p) break;
			
		if (*p == '(')			{ p = (char *)ScriptParserMatchParenthesis(p);	if (*p) p = p + 1; else ScriptParserParseScriptReportError("Unmatched '('"); }
		else if (*p == '"')		{ p = (char *)ScriptParserMatchQuote(p);		if (*p) p = p + 1; else ScriptParserParseScriptReportError("Unmatched '\"'"); }
		else if (*p == '$' && p[1] == '"')		{ p = (char *)ScriptParserMatchQuote(p + 1);		if (*p) p = p + 1; else ScriptParserParseScriptReportError("Unmatched '\"'"); }
		else if (*p == '%')		{ p = (char *)ScriptParserMatchPercent(p);		if (*p) p = p + 1; else ScriptParserParseScriptReportError("Unmatched '%%'"); }
		else
		{
			int i;
			
			for (i = 0; i < OP_COUNT; ++i)
			{
				Operation op = opsSortedByLength[i];
				OperationPrecedence precedence = opPrecedences[op];
				
				int len = strlen(ops[op]);
				if (!strncmp(ops[op], p, len))
				{
					
					//	We've found a matching operation.  Since we go from longest to shortest,
					//	this is the guaranteed best match!
					
					//	Special case handling for unary + and -
					if (op == OP_SUB || op == OP_ADD)
					{
						//	Scan to the right... see if a number follows this...
						
						//	Scan to left... if we find an operator or nothing to the left,
						//	this is a negative number, so we don't treat it as an operator!
						char *pr = p - 1;
						bool	bIsUnary = false;
						
						while (pr >= buf && isspace(*pr))	--pr;	//	Go skip spaces backwards
						
						if (pr < buf)	bIsUnary = true;
						else if (pr >= buf)
						{
							//	We haven't reached the beginning ... so see if we found another operator
							//	ANY operator forces it to be unary...
							const char *	opChars = "+-|^&=<>*/~!@,";
							
							char c = *pr;
							for (const char *opC = opChars; !bIsUnary && *opC; ++opC)
							{
								if (c == *opC)	bIsUnary = true;
							}
						}
							
						if (bIsUnary)	//	Warning!  Unary operator!  Ignore it, maybe!
						{
							char *nextP = p + len;
							while (isspace(*nextP))	++nextP;
							if (is_number(nextP))
								continue;	//	Yep, its a number, ignore it
							else
								precedence = OP_PRECEDENCE_UNARY;
						}
					}
					
					if (precedence < bestOperatorPrecedence)	//	Here for the unarys... oi!
						continue;
					
					bestOp = op;
					bestOperatorPrecedence = precedence;
					bestOperatorStart = p;
					
					p += len;
					
					break;
				}
			}
			
			if (i == OP_COUNT)
				++p;
		}
	}
	
	p = buf;
	while (isspace(*p))
		++p;
	
	if (bestOperatorStart)
	{
		*bestOperatorStart = '\0';
		pp = bestOperatorStart + strlen(ops[bestOp]);
		
//		if ((bestOp == OP_SUB || bestOp == OP_ADD) && bestOpIsUnary)
//		{
//			const char *const_p = p;
//			sca = ScriptParserParseOneArgument(const_p);
//		}
//		else
		{
			sco = new ScriptCompilerOperation;
			
			sco->mOperation = (Operation)bestOp;
			
			if (bestOp != OP_NOT && bestOp != OP_BITWISE_COMPLEMENT)
			{
				if (*p)						sco->mLeft = ScriptParserParseEvaluationStatement(p);
				else if (bestOp == OP_SUB || bestOp == OP_ADD)	sco->mLeft = new ScriptCompilerArgument("0");
				else						ScriptParserParseScriptReportError("Missing left-hand side of operation");
			}
			if (*pp)						sco->mRight = ScriptParserParseEvaluationStatement(pp);
			else							ScriptParserParseScriptReportError("Missing right-hand side of operation");
			
			if (!sco->mLeft)	sco->mLeft = new ScriptCompilerArgument("");
			if (!sco->mRight)	sco->mRight = new ScriptCompilerArgument("");
			
			sca = sco;
		}
	}
	else
	{
		//	no operator...
		if (*p == '(')
		{
			pp = (char *)ScriptParserMatchParenthesis(p);
			if (!*pp)	ScriptParserParseScriptReportError("Unmatched '('");
			*pp = 0;	//	Terminate
			sca = ScriptParserParseEvaluationStatement(p + 1);
		}
		else if (*p == '$' && p[1] == '"')
		{
			pp = (char *)ScriptParserMatchQuote(p + 1);
			if (!*pp)	ScriptParserParseScriptReportError("Unmatched '\"'");
			*pp = 0;	//	Terminate
			sca = new ScriptCompilerArgument(p + 2);
		}
		else if (*p == '"')
		{
			pp = (char *)ScriptParserMatchQuote(p);
			if (!*pp)	ScriptParserParseScriptReportError("Unmatched '\"'");
			*pp = 0;	//	Terminate
			sca = ScriptParserParseString(p + 1);	
		}
		else
		{
			const char *const_p = p;
			sca = ScriptParserParseOneArgument(const_p);
		}
	}
	
	return sca;
}


//	Grab one "arg"
ScriptCompilerArgument *ScriptParserParseString(const char *string)
{
	ScriptCompilerArgument *sca, *sca2, *scv = NULL;
	ScriptCompilerString *scs;
	const char *p, *pp;
	
	p = string;
	while (isspace(*p))
		++p;
	
	if (!*p)
		p = "";	//	Just for sanity
	
	pp = p;
	while ((pp = strchr(pp, '%')))
	{
		++pp;
		if (*pp != '%')
			break;
	}
	
	
	if (pp)	//	We gots us a variable-containing string!
	{
		bool justAVariable = false;
		if (*p == '%' && *(p + 1) != '%')
		{
			pp = ScriptParserMatchPercent(p);
			if (!*pp)
				ScriptParserParseScriptReportError("Unmatched '%%'");
			else
				++pp;
			
			while (isspace(*pp))
				++pp;
			
			if (!*pp)
				justAVariable = true;
		}
		
		if (justAVariable)
		{
			BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
			pp = ScriptParserMatchPercent(p);
			if (!*pp)
				ScriptParserParseScriptReportError("Unmatched '%%'");
			p = ScriptParserExtractContents(p, pp, buf);
			sca = ScriptParserParseVariable(buf);
		}
		else
		{
			BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
			BUFFER(buf2, MAX_SCRIPT_STRING_LENGTH);

			char *bufp;
			
			scs = new ScriptCompilerString;
		
			bufp = buf;
			while (*p)
			{
				if (*p == '%')
				{
					if (*(p + 1) == '%')		//	Copy the %%
					{
						*(bufp++) = *(p++);
						*(bufp++) = '%';
						++p;
					}
					else
					{
						//	variable time
						pp = ScriptParserMatchPercent(p);
						if (!*pp)
							ScriptParserParseScriptReportError("Unmatched '%%'");
						p = ScriptParserExtractContents(p, pp, buf2);
						sca2 = ScriptParserParseVariable(buf2);
						if (scv)
							scv->mNext = sca2;
						else
							scs->mVariables = sca2;
						scv = sca2;
						
						*(bufp++) = '%';
						*(bufp++) = 's';
						
						if (!*p)	//	Might be NULL if unterminated
							break;
					}
				}
				else
					*(bufp++) = *(p++);
			}
			*bufp = 0;
			
			scs->mString = buf;
			sca = scs;
		}
	}
	else
	{
		sca = new ScriptCompilerArgument(p);
	}
	
	return sca;
}






enum ScriptCompilerArgumentType
{
	SCARG_NONE,
	SCARG_LITERAL,			//	1 word, verbatim
	SCARG_LITERALSTRING,	//	The remainder of the line, verbatim
	SCARG_ARG,				//	1 word, potentially a string ("") or variable
	SCARG_VARARG,			//	1 or more ARG until end
	SCARG_STRING,			//	The remainder of the line, processing variables
	SCARG_EVAL,				//	A mathematical expression
	SCARG_VARIABLENAME,		//	A variable name, possibly including context
	SCARG_FUNCTION,			//	Special case: func(args) without the %%
};

ScriptCompilerArgumentType SCArgTypes[LexiScriptCommands::NumCommands][4] =
{
/*	None		*/	{	SCARG_NONE	},
/*	Comment		*/	{	SCARG_NONE	},
/*	If			*/	{	SCARG_EVAL, SCARG_NONE },
/*	ElseIf		*/	{	SCARG_EVAL, SCARG_NONE },
/*	Else		*/	{	SCARG_NONE	},
/*	While		*/	{	SCARG_EVAL, SCARG_NONE },
/*	Foreach		*/	{	SCARG_VARIABLENAME, SCARG_ARG, SCARG_NONE	},
/*	Switch		*/	{	SCARG_EVAL, SCARG_NONE },
/*	End			*/	{	SCARG_NONE	},
/*	Done		*/	{	SCARG_NONE	},
/*	Continue	*/	{	SCARG_NONE	},
/*	Break		*/	{	SCARG_ARG, SCARG_NONE	},
/*	Case		*/	{	SCARG_ARG, SCARG_NONE	},
/*	Default		*/	{	SCARG_NONE	},
/*	Delay		*/	{	SCARG_STRING	},
/*	Eval		*/	{	SCARG_VARIABLENAME, SCARG_EVAL, SCARG_NONE	},
/*	EvalGlobal	*/	{	SCARG_VARIABLENAME, SCARG_EVAL, SCARG_NONE	},
/*	EvalShared	*/	{	SCARG_VARIABLENAME, SCARG_EVAL, SCARG_NONE	},
/*	Global		*/	{	SCARG_VARIABLENAME, SCARG_NONE	},
/*	Shared		*/	{	SCARG_VARIABLENAME, SCARG_NONE	},
/*	Halt		*/	{	SCARG_NONE	},
/*	Return		*/	{	SCARG_ARG, SCARG_NONE	},
/*	Thread		*/	{	SCARG_ARG, SCARG_FUNCTION, SCARG_NONE	},
/*	Set			*/	{	SCARG_VARIABLENAME, SCARG_STRING, SCARG_NONE	},
/*	SetGlobal	*/	{	SCARG_VARIABLENAME, SCARG_STRING, SCARG_NONE	},
/*	SetShared	*/	{	SCARG_VARIABLENAME, SCARG_STRING, SCARG_NONE	},
/*	Unset		*/	{	/*SCARG_VARARG*/ /*SCARG_ARG*/ SCARG_STRING, SCARG_NONE	},
/*	Wait		*/	{	SCARG_STRING, SCARG_NONE	},
/*	Pushfront	*/	{	SCARG_VARIABLENAME, SCARG_STRING, SCARG_NONE	},
/*	Popfront	*/	{	SCARG_VARIABLENAME, SCARG_VARIABLENAME, SCARG_NONE	},
/*	Pushback	*/	{	SCARG_VARIABLENAME, SCARG_STRING, SCARG_NONE	},
/*	Popback		*/	{	SCARG_VARIABLENAME, SCARG_VARIABLENAME, SCARG_NONE	},
/*	Print		*/	{	SCARG_VARIABLENAME, SCARG_ARG, SCARG_VARARG, SCARG_NONE	},
/*	Function	*/	{	SCARG_LITERALSTRING, SCARG_NONE	},
/*	Import		*/	{	SCARG_LITERALSTRING, SCARG_NONE	},
/*	Label		*/	{	SCARG_NONE	},
/*	Jump		*/	{	SCARG_NONE	},
/*	Increment	*/	{	SCARG_VARIABLENAME, SCARG_NONE },
/*	Decrement	*/	{	SCARG_VARIABLENAME, SCARG_NONE },
/*	Command		*/	{	SCARG_STRING, SCARG_NONE	},
/*	PCCommand	*/	{	SCARG_STRING, SCARG_NONE	},
/*	Constant	*/	{	SCARG_LITERALSTRING, SCARG_NONE	},
};



ScriptCompilerCommand *	ScriptParserParseScript(const char *scriptText, char *errorBuffer, bool bDebugMode)
{
	ScriptCompilerCommand *sccRoot, *scc;
	ScriptCompilerArgument *sca, *sca2;
	//char *	s;
	int		line;
	int		totalErrors = 0;
	
	char *	script = str_dup(scriptText);
	
	sccRoot = scc = new ScriptCompilerCommand;
	
	//	First split it into commands
	char *scriptSrc = script;
	line = 0;
	//	using \n as a token rather than \n to catch empty lines for line counting
	while (char *s = strsep(&scriptSrc, "\n"))
	{
		int thisLine = ++line;
		
		skip_spaces(s);
		if (!*s)					continue;
		
		//	Handle <<<...>>> blocks
		char *	block = strrchr(s, '<');
		if (*s != '#' && *s != '*' && block && block[1] == '\0' && (block - 2) > s && !str_cmp(block - 2, "<<<"))
		{
			char *startOfLine = block - 2;
			
			do
			{
				char *s2 = strsep(&scriptSrc, "\n");
				++line;
				
				if (!s2)
					break;
				
				skip_spaces(s2);
				
				if (!strn_cmp(s2, ">>>", 3))
					break;
				
				//	Move the next line onto this line
				int length = strlen(s2);
				memmove(startOfLine, s2, length);	//	And copy the string terminator!
				
				startOfLine += length;
			} while (1);
			
			*startOfLine = '\0';
		}
		else if (*s != '#' && *s != '*')
		{
			do
			{
				//	Scan for last non-space char
				char *	continuation = strrchr(s, '\\');
				if (!continuation)
					break;
				
				char *	startOfLine = continuation;
				++continuation;
				skip_spaces(continuation);
				 
				if (*continuation)	//	Must be only spaces folowing it
					break;
					
				char *s2 = strsep(&scriptSrc, "\n");
				++line;
				
				if (!s2)
					break;
				
				skip_spaces(s2);
				
				if (!s2)
					break;
				
				//	Problem: we are modifying the string, yes.  Now we need to move 's2' to 'continuation'
				memmove(startOfLine, s2, strlen(s2) + 1);	//	And copy the string terminator!
			} while (1);
		}
		
//		bool bIgnoreDebugLine = false;
		if (*s == '\'')
		{
			if (!bDebugMode)	continue;
//			bIgnoreDebugLine = !bDebugMode;

			++s;
			skip_spaces(s);
			
			if (!*s)	continue;
		}
		
		LexiScriptCommands::Commands type = lookup_script_line(s);
		int cmd = 0;
		
		if (type == LexiScriptCommands::Comment)		continue;
//		else if (bIgnoreDebugLine
//			&& (type > LexiScriptCommands::Default
//			|| type == LexiScriptCommands::None))	continue;
		else if (type == LexiScriptCommands::None)
		{
			BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
			
			ScriptEngineGetArgument(s, buf);
			
			for (cmd = 0; *scriptCommands[cmd].command != '\n'; cmd++)
				if (!str_cmp(buf, scriptCommands[cmd].command))
					break;
			
			if (*scriptCommands[cmd].command != '\n') {
				type = LexiScriptCommands::Command;
			} else /* we should really only do this on Mob triggers */
			{
				type = LexiScriptCommands::PCCommand;
			
				for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
					if (is_abbrev(buf, complete_cmd_info[cmd].command))
						break;
				
				if (*complete_cmd_info[cmd].command == '\n')
					cmd = 0;
			}
			
			if (type == LexiScriptCommands::None)
				continue;
		}
		else if (type == LexiScriptCommands::ElseIf)
		{
			if (!strn_cmp(s, "else if ", 8))
				s += 5;	//	skip the 'else' for proper parsing below.
		}
		
		scc->mNext = new ScriptCompilerCommand;
		scc->mNext->mPrev = scc;
		scc = scc->mNext;
		
		scc->mCommand = type;
		scc->mCommandNum = cmd;
		scc->mLine = s;
		scc->mLineNum = thisLine;
		
		//	Lets parse out the lines here... first skip over the 'command'
		while (isspace(*s))			++s;
		if (type != LexiScriptCommands::PCCommand)
		{
			while (*s && !isspace(*s))	++s;
			while (isspace(*s))			++s;
		}

		sca = NULL;
		sca2 = NULL;
		int i;
		for (i = 0; *s && (SCArgTypes[scc->mCommand][i] != SCARG_NONE); ++i)
		{
			switch (SCArgTypes[scc->mCommand][i])
			{
				case SCARG_LITERAL:
					{
						const char *const_s = s;
						sca2 = ScriptParserParseLiteral(const_s);
						s = const_cast<char *>(const_s);
					}
					break;
					
				case SCARG_VARARG:
				case SCARG_ARG:
					{
						const char *const_s = s;
						sca2 = ScriptParserParseOneArgument(const_s);
						s = const_cast<char *>(const_s);
					}
					break;
					
				case SCARG_VARIABLENAME:
					{
						const char *const_s = s;
						sca2 = ScriptParserParseVariableName(const_s);
						s = const_cast<char *>(const_s);
					}
					break;
					
				case SCARG_EVAL:
					sca2 = ScriptParserParseEvaluationStatement(s);
					break;
					
				case SCARG_STRING:
					sca2 = ScriptParserParseString(s);
					break;
				
				case SCARG_FUNCTION:
					sca2 = ScriptParserParseFunction(s);
					break;
					
				case SCARG_LITERALSTRING:
					skip_spaces(s);
					sca2 = new ScriptCompilerArgument(s);
					break;
					
				case SCARG_NONE:	//	Shut GCC up
					break;
			}
			if (sca)	sca->mNext = sca2;
			else		scc->mArguments = sca2;
			sca = sca2;
			
			while (isspace(*s))	++s;
			
			if (SCArgTypes[scc->mCommand][i] == SCARG_VARARG)
			{
				++scc->mVarArgCount;
				if (*s)
					--i;
			}
		}
		
		//	This next line forces the minimum number of args
		if (!*s && SCArgTypes[scc->mCommand][i] != SCARG_NONE)
		{
			while (SCArgTypes[scc->mCommand][i] != SCARG_NONE)
			{
				sca2 = new ScriptCompilerArgument("");
				if (sca)	sca->mNext = sca2;
				else		scc->mArguments = sca2;
				sca = sca2;
				
				++i;
			}
		}
		
		
		if (type == LexiScriptCommands::Popfront)
		{
			sca2 = new ScriptCompilerArgument();
		}
		
		if (!ScriptParserError.empty())
		{
			if (errorBuffer && totalErrors <= MAX_SCRIPT_ERRORS)
			{
				if (totalErrors == MAX_SCRIPT_ERRORS)
					strcat(errorBuffer, "    **** Max Errors Reached\n");
				else
					sprintf_cat(errorBuffer,
							"Line %d: %s\n"
							"    %s\n", line, ScriptParserError.c_str(), scc->mLine.c_str());
			}
			
			ScriptParserParseScriptClearErrors();
		}
	}
	
	free(script);

	return sccRoot;
}




#define BYTECODE_INC 64

inline bytecode_t MAKE_OPCODE(bytecode_t op, int data) { return ((op) | ((data) << SBC_SHIFT)); }
inline int OPCODE_CODE(bytecode_t opcode) { return ((opcode) & SBC_MASK); }
inline int OPCODE_DATA(bytecode_t opcode) { return ((int)(opcode) >> SBC_SHIFT); }


struct ScriptCompilerBytecodeTable
{
					ScriptCompilerBytecodeTable();
					~ScriptCompilerBytecodeTable();					
					
	unsigned int	AddOpcode(bytecode_t op, int data = 0);
	unsigned int	AddOpcode(bytecode_t op, const char *entry) { return AddOpcode(op, AddTableEntry(entry)); }
	void			SetOpcode(int pos, bytecode_t op, int data = 0) { m_Bytecode[pos] = MAKE_OPCODE(op, data); }
	void			SetOpcode(int pos, bytecode_t op, const char *entry) { return SetOpcode(pos, op, AddTableEntry(entry)); }
	void			SetOpcodeData(int pos, int data) { m_Bytecode[pos] = MAKE_OPCODE( OPCODE_CODE(m_Bytecode[pos]), data ); }
	void			SetOpcodeData(int pos, const char *entry) { return SetOpcodeData(pos, AddTableEntry(entry)); }
	unsigned int	AddTableEntry(const char *entry);
	void			AddErrors(ScriptCompilerCommand *scc);

	int				GetOpcode(int pos) { return OPCODE_CODE(m_Bytecode[pos]); }
	int				GetOpcodeData(int pos) { return OPCODE_DATA(m_Bytecode[pos]); }
	
	unsigned int	Tell() { return m_BytecodeCount; }
	
	bytecode_t *	m_Bytecode;
	unsigned int	m_BytecodeCount;
	unsigned int	m_BytecodeAllocated;
	
	char *			m_Table;		//	Table buffer
	unsigned int	m_TableCount;	//	Number of entries
	unsigned int	m_TableSizeUsed;
	unsigned int	m_TableSizeAllocated;

	Lexi::StringList	m_Errors;
	
private:
	static inline unsigned	Align(unsigned size, unsigned alignment) { return (size + (alignment - 1)) & ~(alignment - 1); }
};


ScriptCompilerBytecodeTable::ScriptCompilerBytecodeTable()
:	m_Bytecode(NULL)
,	m_BytecodeCount(0)
,	m_BytecodeAllocated(0)
,	m_Table(NULL)
,	m_TableCount(0)
,	m_TableSizeUsed(0)
,	m_TableSizeAllocated(0)
{
}

ScriptCompilerBytecodeTable::~ScriptCompilerBytecodeTable()
{
	if (m_Bytecode)	FREE(m_Bytecode);
	if (m_Table)	FREE(m_Table);
}


unsigned int ScriptCompilerBytecodeTable::AddOpcode(bytecode_t op, int data)
{
	if (m_BytecodeCount == m_BytecodeAllocated)
	{
		m_BytecodeAllocated += BYTECODE_INC;
		RECREATE(m_Bytecode, bytecode_t, m_BytecodeAllocated);
	}
	m_Bytecode[m_BytecodeCount] = MAKE_OPCODE(op, data);
	return m_BytecodeCount++;
}


unsigned int ScriptCompilerBytecodeTable::AddTableEntry(const char *entry)
{
	if (entry == NULL)	//	Fail safe
		entry = "";
	
	unsigned int offset = 0;
	for (unsigned int i = 0; i < m_TableCount; ++i)
	{
		if (!strcmp(m_Table + offset, entry))
			return offset;
		offset += strlen(m_Table + offset) + 1;
	}
	
	unsigned int entryLength = strlen(entry) + 1;
	unsigned int newSize = m_TableSizeUsed + entryLength;
	
	if (newSize > m_TableSizeAllocated)
	{
		m_TableSizeAllocated = Align(newSize, 1024);
		RECREATE(m_Table, char, m_TableSizeAllocated);
	}
	
	strcpy(m_Table + offset, entry);
	
	m_TableSizeUsed = newSize;
	++m_TableCount;
	
	return offset;
}


void ScriptCompilerBytecodeTable::AddErrors(ScriptCompilerCommand *scc)
{
	if (ScriptParserError.empty())
		return;
	
	if (m_Errors.size() <= MAX_SCRIPT_ERRORS)
	{
		BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
		
		if (m_Errors.size() == MAX_SCRIPT_ERRORS)
			strcpy(buf, "    **** Too Many Errors\n");
		else
			sprintf(buf, "Line %d: %s\n"
						 "    %s\n", scc->mLineNum, ScriptParserError.c_str(), scc->mLine.c_str());
		
		m_Errors.push_back(buf);
	}
	
	ScriptParserError.clear();
}





void ScriptCompilerCompileArguments(ScriptCompilerArgument *sca, ScriptCompilerBytecodeTable &scbt)
{
	if (!sca)	return;
	
	if (sca->mNext)
		ScriptCompilerCompileArguments(sca->mNext, scbt);
	
	sca->Compile(scbt);
}


void ScriptCompilerArgument::Compile(ScriptCompilerBytecodeTable &scbt)
{
	scbt.AddOpcode(SBC_PUSH, mArg.c_str());
}


//	push current token
//		if subfield

void ScriptCompilerVariable::Compile(ScriptCompilerBytecodeTable &scbt)
{
//	unsigned int tableEntry = scbt.AddTableEntry(mArg.c_str());
	scbt.AddOpcode(SBC_PUSH, mArg.c_str());			//	PUSH this
	
	//	Valid combinations to come out of here:
	//		alone, subscript, subfield, context, field, subfield-subscript, context-subscript
	
	unsigned int	flags = 0;
	
	ScriptCompilerVariable *scv = mField;
	
	//	If this is a subvar, handle that first
	if (mSubfield)
	{
		mSubfield->Compile(scbt);	//	this()	
		flags |= DEREF_SUBFIELD;
	}
	else
	{
		if (mContext)
		{
			mContext->Compile(scbt);	//	this{#}
			flags |= DEREF_CONTEXT;
		}	
	
		//	Do subscripts
		if (mSubscript)
		{
			mSubscript->Compile(scbt);		//	this[#]
			flags |= DEREF_SUBSCRIPT;
		}
		else if (mField)
		{
			scbt.AddOpcode(SBC_PUSH, mField->mArg.c_str());	//	PUSH *.foo	
			flags |= DEREF_FIELD;
			
			if (mField->mSubfield)
			{
				mField->mSubfield->Compile(scbt);	//	this()	
				flags |= DEREF_SUBFIELD;
			}
			else
			{
				if (mField->mContext)
				{
					mField->mContext->Compile(scbt);	//	this{#}
					flags |= DEREF_FIELDCONTEXT;
				}
			
				if (mField->mSubscript)
				{
					mField->mSubscript->Compile(scbt);	//	this[#]
					flags |= DEREF_SUBSCRIPT;
				}
			}
			
			scv = scv->mField;
		}
	}

	scbt.AddOpcode(SBC_DEREFERENCE, flags);			//	DEREF this
	
	//	We have to break subvar/subfield-subscript combinations off to properly handle
	//	script-functions
	//	Subvar-Subscript combination
	if (mSubfield && mSubscript)
	{
		mSubscript->Compile(scbt);		//	this[#]
		scbt.AddOpcode(SBC_DEREFERENCE, DEREF_SUBSCRIPT | DEREF_SUBSCRIPT_ONLY);			//	DEREF this
	}
	//	Field-subfield-subscript combination
	else if (mField && mField->mSubfield && mField->mSubscript)
	{
		mField->mSubscript->Compile(scbt);	//	this[#]
		scbt.AddOpcode(SBC_DEREFERENCE, DEREF_SUBSCRIPT | DEREF_SUBSCRIPT_ONLY);			//	DEREF this
	}
	
	for (; scv; scv = scv->mField)
	{
		scbt.AddOpcode(SBC_PUSH, scv->mArg.c_str());	//	PUSH .this
		
		//	These are all fields
		unsigned int flags = DEREF_FIELD | DEREF_CONTINUED;
		
		//	Valid combinations to come out of here:
		//		alone, subscript, subfield, context, subfield-subscript, context-subscript
	
		if (scv->mSubfield)
		{
			scv->mSubfield->Compile(scbt);
			flags |= DEREF_SUBFIELD;

			//	Subfield-subscript results in an extra 'deref'
			if (scv->mSubscript)
			{
				scv->mSubscript->Compile(scbt);
				flags |= DEREF_SUBSCRIPT;
			}
		}
		else
		{
			if (scv->mContext)
			{
				scv->mContext->Compile(scbt);
				flags |= DEREF_CONTEXT;
			}
			
			if (scv->mSubscript)
			{
				scv->mSubscript->Compile(scbt);		//	this[#]
				flags |= DEREF_SUBSCRIPT;
			}
		}

		scbt.AddOpcode(SBC_DEREFERENCE, flags);			//	DEREF this

		//	Subfield-subscript results in an extra 'deref'
//		if (scv->mSubfield && scv->mSubscript)
//		{
//			scv->mSubscript->Compile(scbt);		//	this[#]
//			scbt.AddOpcode(SBC_DEREFERENCE, DEREF_SUBSCRIPT | DEREF_SUBSCRIPT_ONLY);			//	DEREF this
//		}
	}
}


void ScriptCompilerOperation::Compile(ScriptCompilerBytecodeTable &scbt)
{
	if (mOperation == OP_NONE)
		return;
	
	//	we COULD leave it in... op not will ignore left hand anyways... just less efficient... or not, leaves on stack...
	if (mOperation != OP_NOT && mOperation != OP_BITWISE_COMPLEMENT)
		mLeft->Compile(scbt);
	
	//	Lazy evaluation time!
	unsigned int lazyBranchFrom = 0;
	
	if (mOperation == OP_AND)		lazyBranchFrom = scbt.AddOpcode(SBC_BRANCH_LAZY_FALSE, mOperation);
	else if (mOperation == OP_OR)	lazyBranchFrom = scbt.AddOpcode(SBC_BRANCH_LAZY_TRUE, mOperation);
		
	mRight->Compile(scbt);
	
	unsigned int lazyBranchDestination = scbt.AddOpcode(SBC_OPERATION, mOperation);
	
	//	If this is a lazy branch, branch to immediately after the operation
	if (lazyBranchFrom)				scbt.SetOpcodeData(lazyBranchFrom, lazyBranchDestination + 1);
}


void ScriptCompilerString::Compile(ScriptCompilerBytecodeTable &scbt)
{
	ScriptCompilerArgument *sca;
	unsigned int count;
	
	if (mString.empty())
		return;
	
	count = 0;
	for (sca = mVariables; sca; sca = sca->mNext)
		++count;
	
	ScriptCompilerCompileArguments(mVariables, scbt);
	
	scbt.AddOpcode(SBC_PUSH, mString.c_str());
	scbt.AddOpcode(SBC_STRING, count);
}


void ScriptCompilerParseScript(const char *script, CompiledScript *compiledScript, char *errorBuffer, bool bDebugMode)
{
	ScriptCompilerCommand *	sccRoot;
	ScriptCompilerBytecodeTable scbt;
	
	sccRoot = ScriptParserParseScript(script, errorBuffer, bDebugMode);
	
	sccRoot->GenerateTreeUntil(-1);

	//	We now have it all parsed out and in a heirarchical tree; this parse tree could actually be executable!
	//	Instead, we'll generate bytecode from it:
	//	1. Generate byte code with some fillers, generate table while doing it
	
	ScriptCompilerCompileScript(sccRoot->mChildren, scbt);
	
	//	Separate out the Functions
	for (ScriptCompilerCommand *scc = sccRoot->mNext; scc; scc = scc->mNext)
	{
		if (scc->mCommand == LexiScriptCommands::Function)
		{
			//	Note start of function
			if (!scc->mArguments)
				continue;
			
			BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
			
			const char *src = scc->mArguments->mArg.c_str();
			char *dest = buf;
			
			char c = *src++;
			while (c && !isspace(c) && c != '(')
			{
				*dest++ = c;
				c = *src++;
			}
			
			*dest = '\0';
			
			compiledScript->m_Functions.push_back(CompiledScript::Function(buf, scbt.m_BytecodeCount));
			CompiledScript::Function &function = compiledScript->m_Functions.back();
			
			if (c == '(')
			{
				BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
			
				const char *pp = ScriptParserMatchParenthesis(src - 1);
				if (!*pp)	ScriptParserParseScriptReportError("Unmatched '('");
				ScriptParserExtractContents(src - 1, pp, buf);
				
				src = buf;
				
				c = *src++;
				while (c)
				{
					while (isspace(c))	c = *src++;
					
					dest = arg;
					while (c && !isspace(c) && c != ',')
					{
						*dest++ = c;
						c = *src++;
					}
					*dest = '\0';
					
					function.arguments.push_back(arg);
					
					//	skip to the ','
					while (c && c != ',')
					{
						*dest++ = c;
						c = *src++;
					}
					
					if (c)	//	skip the ','
						c = *src++;
				}

				function.hasArguments = true;
			}
			
			
			ScriptCompilerCompileScript(scc->mChildren, scbt /*, function.offset, errorBuffer*/);
		}
		else if (scc->mCommand == LexiScriptCommands::Import)
		{
			if (scc->mArguments)
			{
				//	Parse out the arguments
				BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
				const char *args = scc->mArguments->mArg.c_str();
				
				skip_spaces(args);
				while (*args)
				{
					args = ScriptEngineGetArgument(args, arg);
					
					if (IsVirtualID(arg))
					{
						ScriptPrototype *import = trig_index.Find(VirtualID(arg, compiledScript->m_Prototype->GetVirtualID().zone));
						if (import)
						{
							compiledScript->m_Imports.push_back(import);
						}
					}
				}
			}
		}
		else if (scc->mCommand == LexiScriptCommands::Constant)
		{
			BUFFER(name, MAX_STRING_LENGTH);
			int context;
			const char *data = ScriptEngineParseVariable(scc->mArguments->mArg.c_str(), name, context);
			if (*name)
			{
				compiledScript->m_Constants.Add(name, data, context);
			}
		}
	}
	
	delete sccRoot;	//	We do this, for now...
	
	
	compiledScript->m_Bytecode		= scbt.m_BytecodeCount ? new bytecode_t[scbt.m_BytecodeCount] : NULL;
	if (scbt.m_Bytecode)			  memcpy(compiledScript->m_Bytecode, scbt.m_Bytecode, sizeof(bytecode_t) * scbt.m_BytecodeCount);
	compiledScript->m_BytecodeCount = scbt.m_BytecodeCount;
	
	compiledScript->m_Table			= scbt.m_TableSizeUsed ? new char[scbt.m_TableSizeUsed] : NULL;
	if (scbt.m_Table)				  memcpy(compiledScript->m_Table, scbt.m_Table, scbt.m_TableSizeUsed);
	compiledScript->m_TableSize		= scbt.m_TableSizeUsed;
	compiledScript->m_TableCount	= scbt.m_TableCount;
	
	compiledScript->SortFunctions();	//	Sort the functions that were added
}



void ScriptCompilerCompileScript(ScriptCompilerCommand *scc, ScriptCompilerBytecodeTable &scbt)
{
	int		totalErrors = 0;
	
	scbt.AddOpcode(SBC_START);
	
	while (scc)
	{
		scc->Compile(scbt);
		scc = scc->mNext;
	}
	
	scbt.AddOpcode(SBC_FINAL);
}


void ScriptCompilerCommand::MoveFromNextToChild(ScriptCompilerCommand *scc)
{
	mChildren = mNext;
	mNext = NULL;
	
	if (mChildren)
	{
		mChildren->mPrev = NULL;
		mChildren->mParent = this;
	}
}


void ScriptCompilerCommand::MoveFromChildToNext(ScriptCompilerCommand *scc)
{
	if (scc)
	{
		if (scc->mPrev)	scc->mPrev->mNext = NULL;
		else
		{
			assert(scc->mParent->mChildren == scc);
			scc->mParent->mChildren = NULL;
		}
	}
	
	mNext = scc;
	if (mNext)
	{
		mNext->mPrev = this;
		mNext->mParent = mParent;
	}
}


//	Starts on the If command; return matching End or NULL
ScriptCompilerCommand *ScriptCompilerCommand::GenerateTree()
{
	ScriptCompilerCommand *next = mNext;
	
	switch (mCommand)
	{
		case LexiScriptCommands::If:
//			if (mNext && mNext->mCommand == LexiScriptCommands::Then)
//			{
//				MoveFromNextToChild(mNext);
//				MoveFromChildToNext(mNext->mNext);
//			}
//			else
				next = GenerateTreeUntil(LexiScriptCommands::End);
			break;
			
		case LexiScriptCommands::While:
		case LexiScriptCommands::Foreach:
		case LexiScriptCommands::Switch:
			next = GenerateTreeUntil(LexiScriptCommands::Done);
			break;
		
		case LexiScriptCommands::Function:
			next = GenerateTreeUntil(LexiScriptCommands::End);
			break;
	}
	
	if (mCommand == LexiScriptCommands::Function
		|| mCommand == LexiScriptCommands::Import
		|| mCommand == LexiScriptCommands::Constant)
	{
		//	Pull this block out and move it to the top
		ScriptCompilerCommand *root = mParent;
		
		//	Traverse to the root SCC node
		while (root->mParent)	root = root->mParent;
		//	Traverse to the end of the root list
		while (root->mNext)		root = root->mNext;
		
		if (!mPrev)		mParent->mChildren = mNext;
		else			mPrev->mNext = mNext;
		if (mNext)		mNext->mPrev = mPrev;
		
		mParent = NULL;
		
		root->mNext = this;
		mPrev = root;
		mNext = NULL;
	}
	
	return next;
}


//	Starts on the If command; return matching End or NULL
ScriptCompilerCommand *ScriptCompilerCommand::GenerateTreeUntil(int terminator)
{
	//	Move the Next link to the Child link
	mChildren = mNext;
	mNext = NULL;
	
	if (mChildren)	mChildren->mPrev = NULL;
	
	//	Loop over the following commands, attempting to find the terminator
	for (ScriptCompilerCommand *scc = mChildren, ScriptCompiler; scc; /*scc = scc->mNext*/)
	{
		scc->mParent = this;
		
		if (scc->mCommand == terminator)
		{
			//	Finish moving the chunk into the Child block by establishing the Next link
			//	And delete the terminator
			
			MoveFromChildToNext(scc->mNext);
			
			if (scc->mPrev)	scc->mPrev->mNext = NULL;
			else
			{
				assert(scc->mParent->mChildren == scc);
				scc->mParent->mChildren = NULL;	
			}
			
			assert(scc->mNext == NULL);
			
			delete scc;
			
			return mNext;
		}
		else
			scc = scc->GenerateTree();
	}
	
	return NULL;
}



void ScriptCompilerCommand::CompileArguments(ScriptCompilerBytecodeTable &scbt)
{
	ScriptCompilerCompileArguments(mArguments, scbt);
	scbt.AddErrors(this);
}


void ScriptCompilerCommand::CompileBlock(ScriptCompilerBytecodeTable &scbt)
{
	for (ScriptCompilerCommand *scc = mChildren; scc; scc = scc->mNext)
	{
		scc->Compile(scbt);
	}
}


void ScriptCompilerCommand::CompileIf(ScriptCompilerBytecodeTable &scbt)
{
	CompileArguments(scbt);
	
	//	Add Branch-false
	unsigned int branchOpcode = scbt.AddOpcode(SBC_BRANCH_FALSE);
	
	//	If a chain, add a branch to end for the chain, and then link original branch-false
	std::list<unsigned int>	chainBranches;
	
	for (ScriptCompilerCommand *scc = mChildren; scc; scc = scc->mNext)
	{
		if ((scc->mCommand == LexiScriptCommands::ElseIf
			|| scc->mCommand == LexiScriptCommands::Else))
		{
			if (branchOpcode == 0)	//	ELSE was found...
				break;				//	No more allowed!  This is actually an ERROR CONDITION
			
			chainBranches.push_back( scbt.AddOpcode(SBC_BRANCH) );	//	Add chain branch
			scbt.SetOpcodeData(branchOpcode, scbt.Tell());			//	Link the previous Branch-False
			
			if (scc->mCommand == LexiScriptCommands::ElseIf)
			{
				scc->mBytecodeStart = scbt.AddOpcode(SBC_LINE, scc->mLineNum);
				scc->CompileArguments(scbt);
				
				branchOpcode = scbt.AddOpcode(SBC_BRANCH_FALSE);
			}
			else
			{
				branchOpcode = 0;
			}
		}
		else
			scc->Compile(scbt);
	}

	if (branchOpcode)
		scbt.SetOpcodeData(branchOpcode, scbt.Tell());	//	Link the Branch-false
	
	FOREACH(std::list<unsigned int>, chainBranches, i)
		scbt.SetOpcodeData(*i, scbt.Tell());
}


void ScriptCompilerCommand::CompileWhile(ScriptCompilerBytecodeTable &scbt)
{
	unsigned int branchOpcode = 0;
	
	scbt.AddOpcode(SBC_WHILE);	//	Specialized Delay
		
	if (mCommand == LexiScriptCommands::Foreach)
	{
		//	Create the 'while'
		mArguments->Compile(scbt);
		scbt.AddOpcode(SBC_DEREFERENCE);
		scbt.AddOpcode(SBC_OPERATION, OP_SPECIAL_FOREACH);

		branchOpcode = scbt.AddOpcode(SBC_BRANCH_FALSE);

		CompileArguments(scbt);
		scbt.AddOpcode(SBC_POPFRONT);
	}
	else	//	while
	{
		//	Compile arguments
		CompileArguments(scbt);
		
		branchOpcode = scbt.AddOpcode(SBC_BRANCH_FALSE);
	}
	
	CompileBlock(scbt);
	
	scbt.AddOpcode(SBC_BRANCH, mBytecodeStart);
	scbt.SetOpcodeData(branchOpcode, scbt.Tell());
	
	FOREACH(std::list<unsigned int>, mClosures, i)
	{
		switch (scbt.GetOpcodeData(*i))
		{
			case LexiScriptCommands::Break:		scbt.SetOpcodeData(*i, scbt.Tell());			break;
			case LexiScriptCommands::Continue:	scbt.SetOpcode(*i, SBC_BRANCH, mBytecodeStart);	break;
		}
	}
}


void ScriptCompilerCommand::CompileSwitch(ScriptCompilerBytecodeTable &scbt)
{
	CompileArguments(scbt);
	
	//	Build the jump table...
	std::list<unsigned int>	caseStatements;
	
	unsigned int	defaultBranch = 0;
	unsigned int	failedMatchBranch = 0;
	
	ScriptCompilerCommand *scc = mChildren;
	while (scc)
	{
		if (scc->mCommand == LexiScriptCommands::Case)
		{
			scbt.AddOpcode(SBC_CASE, scc->mArguments->mArg.c_str());	//	Successful case should pop
			caseStatements.push_back( scbt.AddOpcode(SBC_BRANCH) );
		}
		scc = scc->mNext;
	}
	
	scbt.AddOpcode(SBC_DEFAULT);						//	Default should pop
	defaultBranch = scbt.AddOpcode(SBC_BRANCH);
	
	for (scc = mChildren; scc; scc = scc->mNext)
	{
		if (scc->mCommand == LexiScriptCommands::Case)
		{
			if (!caseStatements.empty())
			{
				scbt.SetOpcodeData(caseStatements.front(), scbt.Tell());
				caseStatements.pop_front();
			}
			continue;			
		}
		
		if (scc->mCommand == LexiScriptCommands::Default)
		{
			if (defaultBranch)	scbt.SetOpcodeData(defaultBranch, scbt.Tell());
			defaultBranch = 0;
			continue;
		}
		
		scc->Compile(scbt);
	}
	
	if (defaultBranch)
	{
		scbt.SetOpcodeData(defaultBranch, scbt.Tell());
	}
	
	FOREACH(std::list<unsigned int>, mClosures, i)
	{
		scbt.SetOpcodeData(*i, scbt.Tell());
	}
}


void ScriptCompilerCommand::Compile(ScriptCompilerBytecodeTable &scbt)
{
	mBytecodeStart = scbt.AddOpcode(SBC_LINE, mLineNum);
	
	if (mCommand == LexiScriptCommands::If)
	{
		CompileIf(scbt);
		return;
	}
	else if (mCommand == LexiScriptCommands::ElseIf)
	{
		CompileIf(scbt);
		return;
	}
	else if (mCommand == LexiScriptCommands::Else)
		return;	//	Handled by the IF, these should not be encountered
	else if (mCommand == LexiScriptCommands::While
		|| mCommand == LexiScriptCommands::Foreach)
	{
		CompileWhile(scbt);
		return;
	}
	else if (mCommand == LexiScriptCommands::Switch)
	{
		CompileSwitch(scbt);
		return;
	}
	
	if (mCommand != LexiScriptCommands::Break)
		CompileArguments(scbt);
	
	switch (mCommand)
	{
		case LexiScriptCommands::Continue:
		{
			for (ScriptCompilerCommand *parent = mParent; parent; parent = parent->mParent)
			{
				if (parent->mCommand == LexiScriptCommands::While
					|| parent->mCommand == LexiScriptCommands::Foreach)
				{
					parent->mClosures.push_back( scbt.AddOpcode(SBC_BRANCH, LexiScriptCommands::Continue) );
					break;
				}
			}
		}
		break;

		case LexiScriptCommands::Break:
		{
			int numBreakLevels = MAX(1, atoi(mArguments->mArg.c_str()));
			
			for (ScriptCompilerCommand *parent = mParent; parent; parent = parent->mParent)
			{
				if ((parent->mCommand == LexiScriptCommands::While
					|| parent->mCommand == LexiScriptCommands::Foreach
					|| parent->mCommand == LexiScriptCommands::Switch)
					&& (--numBreakLevels == 0 || !parent->mParent->mParent))
				{
					parent->mClosures.push_back( scbt.AddOpcode(SBC_BRANCH, LexiScriptCommands::Break) );
					break;
				}
			}
		}
		break;
		
		case LexiScriptCommands::Delay:			scbt.AddOpcode(SBC_DELAY);					break;
		case LexiScriptCommands::Eval:			scbt.AddOpcode(SBC_SET, Scope::Local);		break;
		case LexiScriptCommands::EvalGlobal:	scbt.AddOpcode(SBC_SET, Scope::Global);		break;
		case LexiScriptCommands::EvalShared:	scbt.AddOpcode(SBC_SET, Scope::Shared);		break;
		case LexiScriptCommands::Global:		scbt.AddOpcode(SBC_SCOPE, Scope::Global);	break;
		case LexiScriptCommands::Shared:		scbt.AddOpcode(SBC_SCOPE, Scope::Shared);	break;
		case LexiScriptCommands::Halt:			scbt.AddOpcode(SBC_HALT);					break;
		case LexiScriptCommands::Return:		scbt.AddOpcode(SBC_RETURN);					break;
		case LexiScriptCommands::Thread:		scbt.AddOpcode(SBC_THREAD);					break;
		case LexiScriptCommands::Set:			scbt.AddOpcode(SBC_SET, Scope::Local);		break;
		case LexiScriptCommands::SetGlobal:		scbt.AddOpcode(SBC_SET, Scope::Global);		break;
		case LexiScriptCommands::SetShared:		scbt.AddOpcode(SBC_SET, Scope::Shared);		break;
		case LexiScriptCommands::Unset:			scbt.AddOpcode(SBC_UNSET);					break;
		case LexiScriptCommands::Wait:
			if (mNext)		scbt.AddOpcode(SBC_LINE, mNext->mLineNum);	//	Classic Sstat support
			scbt.AddOpcode(SBC_WAIT);
			break;
		case LexiScriptCommands::Pushfront:		scbt.AddOpcode(SBC_PUSHFRONT);				break;
		case LexiScriptCommands::Popfront:		scbt.AddOpcode(SBC_POPFRONT);				break;
		case LexiScriptCommands::Pushback:		scbt.AddOpcode(SBC_PUSHBACK);				break;
		case LexiScriptCommands::Popback:		scbt.AddOpcode(SBC_POPBACK);				break;
		case LexiScriptCommands::Print:			scbt.AddOpcode(SBC_PRINT, mVarArgCount);	break;
		case LexiScriptCommands::Increment:		scbt.AddOpcode(SBC_FASTMATH, OP_ADD);		break;
		case LexiScriptCommands::Decrement:		scbt.AddOpcode(SBC_FASTMATH, OP_SUB);		break;
		case LexiScriptCommands::Command:		scbt.AddOpcode(SBC_COMMAND, mCommandNum);	break;
		case LexiScriptCommands::PCCommand:		scbt.AddOpcode(SBC_PCCOMMAND, mCommandNum);	break;
		case LexiScriptCommands::Function:
			//	This shouldn't be found, error!
			ScriptParserParseScriptReportError("Found function '%s' inside a previous function.  Check that the preceding function has matching END and DONE statements.",
				mArguments ? mArguments->mArg.c_str() : "<INVALID>");
			break;
	}
}




static void ScriptCompilerDumpCompiledBytecodes(CompiledScript *pCompiledScript, int start, char *buf, size_t buflen)
{
	bytecode_t *bytecode = pCompiledScript->m_Bytecode + start;
	int			bytecodeCount = pCompiledScript->m_BytecodeCount;
	char *		table = pCompiledScript->m_Table;
	int			tableCount = pCompiledScript->m_TableCount;
	int			data;
	
	int i = start;
	while (OPCODE_CODE(*bytecode) != SBC_FINAL)
	{
		data = OPCODE_DATA(*bytecode);
		
		sprintf_cat(buf, "%d] ", i++);
		switch (OPCODE_CODE(*bytecode))
		{
			case SBC_FINAL:		sprintf_cat(buf, "[FINAL]\n\n");		break;
			case SBC_START:		sprintf_cat(buf, "[START]\n");			break;
			case SBC_WHILE:		sprintf_cat(buf, "WHILE\n");break;
			case SBC_CASE:		sprintf_cat(buf, "CASE \"%s\"\n", table + data);		break;
			case SBC_DEFAULT:	sprintf_cat(buf, "DEFAULT\n");			break;
			case SBC_SCOPE:		sprintf_cat(buf, "SCOPE: %s\n",
				data == Scope::Global ? "GLOBAL" :
				(data == Scope::Shared ? "SHARED" :
				"Unknown"));										break;
			case SBC_HALT:		sprintf_cat(buf, "HALT\n");				break;
			case SBC_RETURN:	sprintf_cat(buf, "RETURN\n");				break;
			case SBC_THREAD:	sprintf_cat(buf, "THREAD\n");				break;
			case SBC_SET:		sprintf_cat(buf, "SET %s\n",
				data == Scope::Global ? "GLOBAL" :
				(data == Scope::Shared ? "SHARED" :
				""));												break;
			case SBC_UNSET:		sprintf_cat(buf, "UNSET\n");				break;
			case SBC_WAIT:		sprintf_cat(buf, "WAIT\n");				break;
			case SBC_PUSHFRONT:	sprintf_cat(buf, "PUSH-FRONT\n");			break;
			case SBC_POPFRONT:	sprintf_cat(buf, "POP-FRONT\n");			break;
			case SBC_PUSHBACK:	sprintf_cat(buf, "PUSH-BACK\n");			break;
			case SBC_POPBACK:	sprintf_cat(buf, "POP-BACK\n");			break;
			case SBC_DELAY:		sprintf_cat(buf, "DELAY\n");				break;
			case SBC_PRINT:		sprintf_cat(buf, "PRINT: %d\n", data);	break;
			case SBC_FASTMATH:	sprintf_cat(buf, "INCREMENT \"%s\"\n", ops[data]);	break;
			case SBC_COMMAND:	sprintf_cat(buf, "COMMAND: %s\n", scriptCommands[data].command);	break;
			case SBC_PCCOMMAND:	sprintf_cat(buf, "PCCOMMAND/FUNCTION: %s\n", data ? complete_cmd_info[data].command : "<Unknown>");	break;

		//	Stack Manipulation
			case SBC_PUSH:		sprintf_cat(buf, "PUSH \"%s\"\n", table + data);	break;
			case SBC_OPERATION:	sprintf_cat(buf, "OPERATION \"%s\"\n", ops[data]);	break;
			case SBC_DEREFERENCE:
				sprintf_cat(buf, "DEREFERENCE");
				if (data & DEREF_CONTEXT)			sprintf_cat(buf, "-CONTEXT");
				if (data & DEREF_FIELD)
				{
					if (data & DEREF_SUBFIELD)		sprintf_cat(buf, "-SUBFIELD");
					else							sprintf_cat(buf, "-FIELD");
					if (data & DEREF_FIELDCONTEXT)	sprintf_cat(buf, "-CONTEXT");
				}
				else if (data & DEREF_SUBFIELD)		sprintf_cat(buf, "-SUBVAR");
				if (data & DEREF_SUBSCRIPT)			sprintf_cat(buf, "-SUBSCRIPT");
				if (data & DEREF_CONTINUED)			sprintf_cat(buf, " [Continued]");

				sprintf_cat(buf, "\n");
				break;
			case SBC_STRING:	sprintf_cat(buf, "STRING: %d\n", data);	break;

			case SBC_BRANCH:	sprintf_cat(buf, "BRANCH %d\n", data);	break;
			case SBC_BRANCH_FALSE:sprintf_cat(buf, "BRANCH-FALSE %d\n", data);	break;
			case SBC_BRANCH_LAZY_FALSE:sprintf_cat(buf, "BRANCH-LAZY-FALSE %d\n", data);	break;
			case SBC_BRANCH_LAZY_TRUE:sprintf_cat(buf, "BRANCH-LAZY-TRUE %d\n", data);	break;

			case SBC_LINE:		sprintf_cat(buf, "LINE %d\n", data);		break;

			default:			sprintf_cat(buf, "UNKNOWN %d\n", data);
		}
		++bytecode;
		
		if (strlen(buf) > buflen - 256)
		{
			strcat(buf, "***TOO MANY INSTRUCTIONS***");
			return;
		}
	}
	sprintf_cat(buf, "[END]\n");
}


void ScriptCompilerDumpCompiledScript(CompiledScript *pCompiledScript, char *buf, size_t buflen)
{
	char *		table = pCompiledScript->m_Table;
	int			tableCount = pCompiledScript->m_TableCount;
	
	//	Now lets output it...
	strcat(buf, "Table:\n");
	for (int i = 0; i < tableCount; ++i)
	{
		sprintf_cat(buf, "    %s\n", table);
		table += strlen(table) + 1;	
	}
	
	strcat(buf, "\n\nCode:\n");
	
	ScriptCompilerDumpCompiledBytecodes(pCompiledScript, 0, buf, buflen);
	
	FOREACH(CompiledScript::FunctionList, pCompiledScript->m_Functions, iter)
	{
		if (strlen(buf) > buflen - 256)
			break;
		
		const CompiledScript::Function &function = *iter;
		if (function.name.empty() || function.offset == 0)
			continue;
		
		sprintf_cat(buf, "\n\n\nFunction: %s\n", function.name.c_str());
	
		ScriptCompilerDumpCompiledBytecodes(pCompiledScript, function.offset, buf, buflen);
	}
}


/*
	Potential optimizations:
	
	PUSH X; EVAL; PUSH X
	to
	COPY; PUSH X; EVAL
	
	Reason: saves searching for and reloading the variable
*/



//	WARNING: uses static internal buffer; volatile!
const char *ScriptEngineGetUIDString(Entity *e)
{
	if (!e || e->IsPurged())
		return "0";
	
	return e->GetIDString();
}


bool ScriptEngineOperationEvaluateAsBoolean(const char *value, STR_TYPE type)
{
	if (type == STR_TYPE_INTEGER)		return ScriptEngineParseInteger(value) != 0;
	else if (type == STR_TYPE_FLOAT)	return strtod(value, NULL) != 0.0;
	else								return *value != 0;
}


void ScriptEngineOperation(TrigThread *thread, const char *lhs, const char *rhs, int operation, char *result)
{
	long long	n;
	double		f;
	bool		booleanResult;
	
	STR_TYPE lhs_type = str_type(lhs);
	STR_TYPE rhs_type = str_type(rhs);
	
//	if (lhs_type == STR_TYPE_INTEGER && rhs_type == STR_TYPE_STRING && rhs && !*rhs)	rhs_type = STR_TYPE_INTEGER;
//	if (rhs_type == STR_TYPE_INTEGER && lhs_type == STR_TYPE_STRING && lhs && !*lhs)	lhs_type = STR_TYPE_INTEGER;
	
	STR_TYPE dominant_type = (STR_TYPE)MAX(lhs_type, rhs_type);	//	Simple: the higher the value, the more dominant
	
	switch (operation)
	{
		case OP_OR:
			strcpy(result,
					ScriptEngineOperationEvaluateAsBoolean(lhs, lhs_type) ||
					ScriptEngineOperationEvaluateAsBoolean(rhs, rhs_type) ? "1" : "0");
			break;
			
		case OP_XOR:
			strcpy(result,
					ScriptEngineOperationEvaluateAsBoolean(lhs, lhs_type) ^
					ScriptEngineOperationEvaluateAsBoolean(rhs, rhs_type) ? "1" : "0");
			break;
			
		case OP_AND:
			strcpy(result,
					ScriptEngineOperationEvaluateAsBoolean(lhs, lhs_type) &&
					ScriptEngineOperationEvaluateAsBoolean(rhs, rhs_type) ? "1" : "0");
			break;
	
		case OP_BIT_OR:
			if (rhs_type == STR_TYPE_INTEGER && lhs_type == STR_TYPE_INTEGER)
				ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) | ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_BIT_XOR:
			if (rhs_type == STR_TYPE_INTEGER && lhs_type == STR_TYPE_INTEGER)
				ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) ^ ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_BIT_AND:
			if (rhs_type == STR_TYPE_INTEGER && lhs_type == STR_TYPE_INTEGER)
				ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) & ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_EQ:
			if (lhs_type == STR_TYPE_STRING || rhs_type == STR_TYPE_STRING)
			{
				if (IsVirtualID(lhs) && IsVirtualID(rhs))
					booleanResult = VirtualID(lhs, thread->GetCurrentVirtualID().zone) ==
									VirtualID(rhs, thread->GetCurrentVirtualID().zone);
				else
					booleanResult = cstr_cmp(lhs, rhs) == 0;
			}
			else if (dominant_type == STR_TYPE_FLOAT)	booleanResult = atof(lhs) == atof(rhs);
			else 										booleanResult = ScriptEngineParseInteger(lhs) == ScriptEngineParseInteger(rhs);
			strcpy(result, booleanResult ? "1" : "0");
			break;
	
		case OP_NEQ:
			if (lhs_type == STR_TYPE_STRING || rhs_type == STR_TYPE_STRING)
			{
				if (IsVirtualID(lhs) && IsVirtualID(rhs))
					booleanResult = VirtualID(lhs, thread->GetCurrentVirtualID().zone) !=
									VirtualID(rhs, thread->GetCurrentVirtualID().zone);
				else
					booleanResult = cstr_cmp(lhs, rhs) != 0;
			}
			else if (dominant_type == STR_TYPE_FLOAT)	booleanResult = atof(lhs) != atof(rhs);
			else										booleanResult = ScriptEngineParseInteger(lhs) != ScriptEngineParseInteger(rhs);
			strcpy(result, booleanResult ? "1" : "0");
			break;
			
		case OP_LEEQ:
			if (dominant_type == STR_TYPE_FLOAT)		booleanResult = atof(lhs) <= atof(rhs);
			else if (dominant_type == STR_TYPE_INTEGER)	booleanResult = ScriptEngineParseInteger(lhs) <= ScriptEngineParseInteger(rhs);
			else if (IsVirtualID(lhs) && IsVirtualID(rhs))
				booleanResult = VirtualID(lhs, thread->GetCurrentVirtualID().zone) <=
								VirtualID(rhs, thread->GetCurrentVirtualID().zone);
			else										booleanResult = cstr_cmp(lhs, rhs) <= 0;
			strcpy(result, booleanResult ? "1" : "0");
			break;
		
		case OP_GREQ:
			if (dominant_type == STR_TYPE_FLOAT)		booleanResult = atof(lhs) >= atof(rhs);
			else if (dominant_type == STR_TYPE_INTEGER)	booleanResult = ScriptEngineParseInteger(lhs) >= ScriptEngineParseInteger(rhs);
			else if (IsVirtualID(lhs) && IsVirtualID(rhs))
				booleanResult = VirtualID(lhs, thread->GetCurrentVirtualID().zone) >=
								VirtualID(rhs, thread->GetCurrentVirtualID().zone);
			else										booleanResult = cstr_cmp(lhs, rhs) <= 0;
			strcpy(result, booleanResult ? "1" : "0");
			break;
		
		case OP_LE:
			if (dominant_type == STR_TYPE_FLOAT)		booleanResult = atof(lhs) < atof(rhs);
			else if (dominant_type == STR_TYPE_INTEGER)	booleanResult = ScriptEngineParseInteger(lhs) < ScriptEngineParseInteger(rhs);
			else if (IsVirtualID(lhs) && IsVirtualID(rhs))
				booleanResult = VirtualID(lhs, thread->GetCurrentVirtualID().zone) <
								VirtualID(rhs, thread->GetCurrentVirtualID().zone);
			else										booleanResult = cstr_cmp(lhs, rhs) < 0;
			strcpy(result, booleanResult ? "1" : "0");
			break;
	
		case OP_GR:
			if (dominant_type == STR_TYPE_FLOAT)		booleanResult = atof(lhs) > atof(rhs);
			else if (dominant_type == STR_TYPE_INTEGER)	booleanResult = ScriptEngineParseInteger(lhs) > ScriptEngineParseInteger(rhs);
			else if (IsVirtualID(lhs) && IsVirtualID(rhs))
				booleanResult = VirtualID(lhs, thread->GetCurrentVirtualID().zone) >
								VirtualID(rhs, thread->GetCurrentVirtualID().zone);
			else										booleanResult = cstr_cmp(lhs, rhs) > 0;
			strcpy(result, booleanResult ? "1" : "0");
			break;
	
		case OP_CONTAINS:
			strcpy(result, str_str(lhs, rhs) ? "1" : "0");	//	finds right hand in left hand... woops
			break;
			
		case OP_BITSHIFT_LEFT:
			if (rhs_type == STR_TYPE_INTEGER && lhs_type == STR_TYPE_INTEGER)
				ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) << ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_BITSHIFT_RIGHT:
			if (rhs_type == STR_TYPE_INTEGER && lhs_type == STR_TYPE_INTEGER)
				ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) >> ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_SUB:
			if (lhs_type == STR_TYPE_STRING && IsVirtualID(lhs) && rhs_type != STR_TYPE_STRING)
			{
				VirtualID vid(lhs, thread->GetCurrentVirtualID().zone);
				vid.number -= ScriptEngineParseInteger(rhs);
				strcpy(result, vid.Print().c_str());
			}
			else if (rhs_type == STR_TYPE_STRING && IsVirtualID(rhs) && lhs_type != STR_TYPE_STRING)
			{
				VirtualID vid(rhs, thread->GetCurrentVirtualID().zone);
				vid.number -= ScriptEngineParseInteger(lhs);
				strcpy(result, vid.Print().c_str());
			}
			else if (dominant_type == STR_TYPE_FLOAT)	ScriptEnginePrintDouble(result, atof(lhs) - atof(rhs));
			else if (dominant_type == STR_TYPE_INTEGER)	ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) - ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_ADD:
			if (lhs_type == STR_TYPE_STRING && IsVirtualID(lhs) && rhs_type != STR_TYPE_STRING)
			{
				VirtualID vid(lhs, thread->GetCurrentVirtualID().zone);
				vid.number += ScriptEngineParseInteger(rhs);
				strcpy(result, vid.Print().c_str());
			}
			else if (rhs_type == STR_TYPE_STRING && IsVirtualID(rhs) && lhs_type != STR_TYPE_STRING)
			{
				VirtualID vid(rhs, thread->GetCurrentVirtualID().zone);
				vid.number += ScriptEngineParseInteger(lhs);
				strcpy(result, vid.Print().c_str());
			}
			else if (dominant_type == STR_TYPE_FLOAT)	ScriptEnginePrintDouble(result, atof(lhs) + atof(rhs));
			else if (dominant_type == STR_TYPE_INTEGER)	ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) + ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_DIV:
			if (lhs_type == STR_TYPE_STRING && IsVirtualID(lhs) && rhs_type != STR_TYPE_STRING
				&& (n = ScriptEngineParseInteger(rhs)) != 0)
			{
				VirtualID vid(lhs, thread->GetCurrentVirtualID().zone);
				vid.number /= n;
				strcpy(result, vid.Print().c_str());
			}
			else if (dominant_type == STR_TYPE_FLOAT)	ScriptEnginePrintDouble(result, (f = atof(rhs)) != 0.0 ? (atof(lhs) / f) : 0);
			else if (dominant_type == STR_TYPE_INTEGER)	ScriptEnginePrintInteger(result, (n = ScriptEngineParseInteger(rhs)) != 0 ? (ScriptEngineParseInteger(lhs) / n) : 0);
			else										strcpy(result, "0");
			break;
			
		case OP_MULT:
			if (lhs_type == STR_TYPE_STRING && IsVirtualID(lhs) && rhs_type != STR_TYPE_STRING)
			{
				VirtualID vid(lhs, thread->GetCurrentVirtualID().zone);
				vid.number *= ScriptEngineParseInteger(rhs);
				strcpy(result, vid.Print().c_str());
			}
			else if (dominant_type == STR_TYPE_FLOAT)		ScriptEnginePrintDouble(result, atof(lhs) * atof(rhs));
			else if (dominant_type == STR_TYPE_INTEGER)	ScriptEnginePrintInteger(result, ScriptEngineParseInteger(lhs) * ScriptEngineParseInteger(rhs));
			else										strcpy(result, "0");
			break;
			
		case OP_MOD:
			if (lhs_type == STR_TYPE_STRING && IsVirtualID(lhs) && rhs_type != STR_TYPE_STRING
				 && (n = ScriptEngineParseInteger(rhs)) != 0)
			{
				VirtualID vid(lhs, thread->GetCurrentVirtualID().zone);
				vid.number %= n;
				strcpy(result, vid.Print().c_str());
			}
			else if (dominant_type == STR_TYPE_FLOAT)		ScriptEnginePrintDouble(result, (f = atof(rhs)) != 0.0 ? fmod(atof(lhs), f) : 0.0);
			else if (dominant_type == STR_TYPE_INTEGER)	ScriptEnginePrintInteger(result, (n = ScriptEngineParseInteger(rhs)) ? (ScriptEngineParseInteger(lhs) % n) : 0);
			else										strcpy(result, "0");
			break;
			
		case OP_NOT:
			if (dominant_type == STR_TYPE_FLOAT)		booleanResult = atof(rhs) == 0.0f;
			else if (dominant_type == STR_TYPE_INTEGER)	booleanResult = !ScriptEngineParseInteger(rhs);
			else										booleanResult = !*rhs;
			strcpy(result, booleanResult ? "1" : "0");
			break;
		
		case OP_BITWISE_COMPLEMENT:
			if (rhs_type == STR_TYPE_INTEGER)			ScriptEnginePrintInteger(result, ~strtoull(rhs, NULL, 0));
			else										strcpy(result, "0");
			break;
		
		case OP_COMMA:
			sprintf(result, "%s%c%s", lhs, PARAM_SEPARATOR, rhs);
			break;
		
		case OP_SPECIAL_FOREACH:
			skip_spaces(rhs);
			strcpy(result, *rhs != '\0' ? "1" : "0");
			break;
			
	}
}


void ScriptEngineString(const char *formatp, ThreadStack &stack, char *resultp)
{
	char *			bufp;
	char			c;
	
	while ((c = *(formatp++)))
	{
		if (c == '%')
		{
			c = *(formatp++);
			
			if (c == '%')	*(resultp++) = c;
			else if (c == 's')
			{
				//	Pop something off the stack and copy it...
				bufp = stack.Pop(); //ScriptEnginePopStack(stack, stackCount, stackSize);
				
				while ((c = *(bufp++)))
					*(resultp++) = c;
			}
			else { /* error, shouldn't happen */ }
		}
		else
			*(resultp++) = c;
		
	}
	*resultp = 0;
}


const char * ScriptEngineParseVariable(const char *var, char *out, int &context)
{
	char c;
	
	while (isspace(*var))
		++var;
	
	while ((c = *var) && !isspace(c) && c != '{')
	{
		*(out++) = c;
		++var;
	}
	*(out++) = '\0';
	
	if (c == '{')
	{
		++var;
		
		while (isspace(*var)) ++var;
		
//		if (isdigit(*var))
			context = strtol(var, NULL, 0);
/*		else
		{
			const char *p = var;
			while (isalnum(*p))	++p;
			context = GetStringFastHash(var, p - var);
			var = p;
		}
*/		
		while ((c = *var) && c != '}')
			++var;
		if (*var)
			++var;
	}
	else
		context = 0;
	
	while (isspace(*var))
		++var;
	
	return var;
	
//	return context;
}



static bool ParseVirtualIDFromVariableName(const char *str, VirtualID &vid, Hash relativeZone = 0)
{
	vid.Clear();	//	Initialize to invalid
	
	if (!*str)	return false;
	
	if (isdigit(*str) && is_number(str))
	{
		//	Get classic VNum
		vid.Parse(atoi(str));
		return true;
	}
	
	const char *p = str;
	
	bool	bUnknownVID = (*str == '#');
	if (bUnknownVID)		++str;
	
	while (isalnum(*str))	++str;
	
	if (*str != ':' || !isdigit(str[1]))	return false;	//	No ':#'?  Not a VirtualID
	
	//	The zonename starts at p and ends at str-1
	//	If the string is empty, it's a relative #
	if (str == p)			vid.zone = relativeZone;
	else if (bUnknownVID)	vid.zone = strtoul(p + 1, NULL, 16);
	else					vid.zone = GetStringFastHash(p, str - p);
	
	//	The number starts at str + 1
	vid.number = strtoul(str + 1, const_cast<char **>(&str), 0);
	
	return vid.IsValid();
};


const char * ScriptEngineParseVariableWithEntity(const char *var, char *out, int &context, Entity *&e)
{
	VirtualID vid;
	
	while (isspace(*var))
		++var;
		
	e = NULL;
	
	if (IsIDString(var))
	{
		IDNum		uid = ParseIDString(var);
		
		//	If vd is a reference, we find the entity, grab IT'S variable...
		if (uid == 0)	return NULL;
		e = IDManager::Instance()->Find(uid);
		if (e == NULL)	return NULL;
		
		while (*var && *var != '.')	++var;
		if (*var == '.')			++var;
		else						return NULL;
	}
	else if (ParseVirtualIDFromVariableName(var, vid))
	{
		e = world.Find(vid);
		if (e == NULL)	return NULL;

		while (*var && *var != '.')	++var;
		if (*var == '.')			++var;
		else						return NULL;
	}

	return ScriptEngineParseVariable(var, out, context);
}


ScriptVariable *ScriptEngineFindVariable(Scriptable *sc, TrigThread *thread, const char *name, Hash hash, int context, unsigned scope)
{
	ScriptVariable *var;
	bool extract = IS_SET(scope, Scope::Extract);
	
	if (extract)
	{
		//	Extractions must ALWAYS be context-specific!
		scope |= Scope::ContextSpecific;
	}
	
	if (hash == 0) hash = GetStringFastHash(name);
	
	if (context != 0)
	{
		if (IS_SET(scope, Scope::Local))
		{
			var = thread->GetCurrentVariables().Find(name, hash, context, extract);
			if (var)	return var;
		}
		
		if (IS_SET(scope, Scope::Global))
		{
			var = sc->m_Globals.Find(name, hash, context, extract);
			if (var)	return var;
		}
		
		if (IS_SET(scope, Scope::Shared))
		{
			FOREACH(TrigData::List, TRIGGERS(sc), trig)
			{
				if ((*trig)->GetPrototype() == NULL)
					continue;
				
				var = (*trig)->GetPrototype()->m_SharedVariables.Find(name, hash, context, extract);
				if (var)	return var;
			}
		}

		//	When setting the variable, if there is a context, we should not succeed in finding
		//	the non-context version!
		if (IS_SET(scope, Scope::ContextSpecific))
			return NULL;
		
		if (IS_SET(scope, Scope::Constant))
		{
			//	Do it here: check current thread trigger, then imports
			//	Just like functions, this uses LIVE changes... which means it scans the prototype!

			CompiledScript *pCompiledScript = thread->GetCurrentCompiledScript();
			var = pCompiledScript->m_Constants.Find(name, hash, context);
			if (var)	return var;
			
			/* Loop over attached trigs */
			if (!pCompiledScript->m_Imports.empty())
			{
				FOREACH(CompiledScript::ImportList, pCompiledScript->m_Imports, import)
				{
					var = (*import)->m_pProto->m_pCompiledScript->m_Constants.Find(name, hash, context);
					if (var)	return var;
				}
				
			}
		}
	}
	
	
	if (IS_SET(scope, Scope::Local))
	{
		var = thread->GetCurrentVariables().Find(name, hash, 0, extract);
		if (var)	return var;
	}
	
	if (IS_SET(scope, Scope::Global))
	{
		var = sc->m_Globals.Find(name, hash, 0, extract);
		if (var)	return var;
	}
	
	if (IS_SET(scope, Scope::Shared))
	{
		FOREACH(TrigData::List, TRIGGERS(sc), trig)
		{
			if ((*trig)->GetPrototype() == NULL)
				continue;
			
			var = (*trig)->GetPrototype()->m_SharedVariables.Find(name, hash, context, extract);
			if (var)	return var;
		}
	}
	
	
	if (IS_SET(scope, Scope::Constant))
	{
		//	Do it here: check current thread trigger, then imports
		//	Just like functions, this uses LIVE changes... which means it scans the prototype!

		CompiledScript *pCompiledScript = thread->GetCurrentCompiledScript();
		var = pCompiledScript->m_Constants.Find(name, hash, 0);
		if (var)	return var;
		
		/* Loop over attached trigs */
		if (!pCompiledScript->m_Imports.empty())
		{
			FOREACH(CompiledScript::ImportList, pCompiledScript->m_Imports, import)
			{
				var = (*import)->m_pProto->m_pCompiledScript->m_Constants.Find(name, hash, 0);
				if (var)	return var;
			}
		}
	}
	
	return NULL;
}


/* release memory allocated for a variable list */
bool runScripts = true;

int ScriptEngineRunScript(TrigThread *pThread)
{
	return ScriptEngineRunScript(GET_THREAD_ENTITY(pThread), GET_THREAD_TRIG(pThread), NULL, pThread);
}


extern RoomData *ScriptObjectAtRoom;
static int ScriptEngineRunScriptDepth = 0;

class ScriptEngineRunScriptCleanup : public PerfTimer
{
public:
	//	Initialization: Setup before running thread
	ScriptEngineRunScriptCleanup(TrigThread *thread)
	:	m_Thread(thread)
	,	m_SavedRoom(ScriptObjectAtRoom)
	{
		++ScriptEngineRunScriptDepth;
		ScriptObjectAtRoom = NULL;
		
		Start();
	}
	
	//	Terminator: Thread is finished or pausing
	~ScriptEngineRunScriptCleanup()
	{
		ScriptObjectAtRoom = m_SavedRoom;
		--ScriptEngineRunScriptDepth;
		
		double timeElapsed = GetTimeElapsed();
		
		m_Thread->m_TimeUsed += timeElapsed;
		
		if (m_Thread->m_pTrigger)
		{
			m_Thread->m_pTrigger->m_TotalTimeUsed += timeElapsed;
		}
	}
	
private:
	TrigThread *		m_Thread;
	RoomData *			m_SavedRoom;
};


int ScriptEngineRunScript(Entity *go, TrigData *trig, char *output, TrigThread *thread)
{
	BUFFER(result, MAX_SCRIPT_STRING_LENGTH * 2);
	result[MAX_SCRIPT_STRING_LENGTH / 2] = 0;
	
	int			ret_val = 1;
	const char *buf1;
	const char *buf2;
	const char *buf3;
	Scriptable *sc = SCRIPT(go);
	bool		halted = false;

	int			loops = 0;
	
	if (ScriptEngineRunScriptDepth > MAX_SCRIPT_DEPTH) {
		ScriptEngineReportError(NULL, true, "Triggers recursed beyond maximum allowed depth.");
		return 0;
	}
	
	if (!runScripts)
		return 1;
	
	switch (go->GetEntityType()) {
		case Entity::TypeCharacter:
		case Entity::TypeObject:
			if (PURGED(go))	return 1;
		case Entity::TypeRoom:
			break;
		default:
			return 1;
	}
	
	if (PURGED(trig))	
		return 1;
	
	if (thread == NULL)
	{
		thread = new TrigThread(trig, go);
		
		thread->GetCurrentVariables().swap(TrigData::ms_InitialVariables);
		
		if (*(thread->GetCurrentCompiledScript()->m_Bytecode) != SBC_START)
		{
			ScriptEngineReportError(NULL, true, "Script \"%s\" [%s] doesn't start with SBC_START opcode!",
					trig->GetName(), trig->GetVirtualID().Print().c_str());
			return 1;
		}
	}
	
	//	Enclose within a block for the CleanupObject, as it may in the future touch the thread, which can be deleted below
	{
		//	This object handles setup and cleanup of some things:
		//		ScriptEngineRunScriptDepth
		//		Saving/clearing and restoring ScriptObjectAtRoom
		ScriptEngineRunScriptCleanup CleanupObject(thread);
		
		ThreadStack &		stack = thread->m_Stack;
		Lexi::SharedPtr<CompiledScript>	pCompiledScript = thread->GetCurrentCompiledScript();
		bytecode_t *		bytecodePtr = thread->GetCurrentBytecodePosition();
		
		while (!halted && thread->m_ID != 0)
		{
			bytecode_t	opcode = *(bytecodePtr++);
			int			data = OPCODE_DATA(opcode);
			opcode = OPCODE_CODE(opcode);
			
			switch (opcode)
			{
				case SBC_HALT:
				case SBC_FINAL:
					halted = thread->PopCallstack();
					if (!halted)
					{
						pCompiledScript = thread->GetCurrentCompiledScript();
						bytecodePtr = thread->GetCurrentBytecodePosition();
					}
					break;
					
				case SBC_START:
					ScriptEngineReportError(NULL, true, "Script attempting to execute SBC_START opcode!");
					break;
				
				case SBC_WHILE:
					if (loops++ == 100)
					{
						ScriptEngineRunScriptProcessWait(sc, thread, "1p");
						
						thread->SetCurrentBytecodePosition(bytecodePtr);
						
						return ret_val;
					}
					break;
				
					break;
				
				case SBC_CASE:
					buf1 = stack.Peek();
					buf2 = (data >= 0 && data < pCompiledScript->m_TableSize ? pCompiledScript->m_Table + data : "");
				
					if (str_type(buf1) == STR_TYPE_INTEGER && str_type(buf2) == STR_TYPE_INTEGER
								? ScriptEngineParseInteger(buf1) == ScriptEngineParseInteger(buf2)
								: !cstr_cmp(buf1, buf2))
					{
						//	Match!  Pop the stack entry, we'll execute the next opcode.
						stack.Pop();
					}
					else
					{
						//	Skip past the next branch
						if (OPCODE_CODE(*bytecodePtr) == SBC_BRANCH)
							++bytecodePtr;
					}
					break;
				
				case SBC_DEFAULT:
					//	Pop the argument from the switch
					stack.Pop();
					break;

				case SBC_SCOPE:
					buf1 = stack.Pop();
					ScriptEngineRunScriptProcessScope(sc, thread, buf1, data);
					break;

				case SBC_RETURN:
					buf1 = stack.Pop();
					if (thread->m_Callstack.size() > 1)
					{
						thread->m_pResult = buf1;
					}
					else if (output)
					{
						//	Output must be at least MAX_STRING_LENGTH
						strncpy(output, buf1, MAX_STRING_LENGTH);
						output[MAX_STRING_LENGTH - 1] = '\0';
					}
					else
					{
						skip_spaces(buf1);
						if (!*buf1)						ret_val = 0;
						else
						{
							switch (str_type(buf1))
							{
								case STR_TYPE_STRING:	ret_val = 1;	break;
								case STR_TYPE_INTEGER:
								case STR_TYPE_FLOAT:	ret_val = strtol(buf1, NULL, 0);
							}
						}
					}
					break;
				
				case SBC_THREAD:
					buf1 = stack.Pop();
					buf2 = stack.Pop();
					buf3 = stack.Pop();
					ScriptEngineRunScriptThreadFunctionOntoEntity(sc, thread, buf1, buf2, buf3);
					break;
				
				case SBC_SET:
					buf1 = stack.Pop();
					buf2 = stack.Pop();
					ScriptEngineRunScriptProcessSet(sc, thread, buf1, buf2, data);
					break;
					
				case SBC_UNSET:
					buf1 = stack.Pop();
					ScriptEngineRunScriptProcessUnset(sc, thread, buf1);
					break;
					
				case SBC_WAIT:
					buf1 = stack.Pop();
					ScriptEngineRunScriptProcessWait(sc, thread, buf1);
					
					thread->SetCurrentBytecodePosition(bytecodePtr);
					
					return ret_val;
				
				case SBC_PUSHFRONT:
					buf1 = stack.Pop();
					buf2 = stack.Pop();
					ScriptEngineRunScriptProcessPushfront(sc, thread, buf1, buf2);
					break;
					
				case SBC_POPFRONT:
					buf1 = stack.Pop();
					buf2 = stack.Pop();
					ScriptEngineRunScriptProcessPopfront(sc, thread, buf1, buf2);
					break;

				case SBC_PUSHBACK:
					buf1 = stack.Pop();
					buf2 = stack.Pop();
					ScriptEngineRunScriptProcessPushback(sc, thread, buf1, buf2);
					break;
					
				case SBC_POPBACK:
					buf1 = stack.Pop();
					buf2 = stack.Pop();
					ScriptEngineRunScriptProcessPopback(sc, thread, buf1, buf2);
					break;
					
				case SBC_DELAY:
					buf1 = stack.Pop();
					ScriptEngineRunScriptProcessDelay(sc, thread, buf1);

					thread->SetCurrentBytecodePosition(bytecodePtr);
					
					return ret_val;

					break;
					
				case SBC_PRINT:
					buf1 = stack.Pop();
					buf2 = stack.Pop();
					ScriptEngineRunScriptProcessPrint(sc, thread, buf1, buf2, data, stack);
					break;
				
				case SBC_FASTMATH:
					buf1 = stack.Pop();
					ScriptEngineRunScriptProcessFastMath(sc, thread, buf1, (Operation)data);
					break;
					
				case SBC_COMMAND:
					buf1 = stack.Pop();
					script_command_interpreter(go, trig, thread, buf1, data);
					break;
					
				case SBC_PCCOMMAND:
					{
						buf1 = stack.Pop();
						
						if (!*buf1 || isdigit(*buf1))	break;
						
						/* Find the function first */
						if (!pCompiledScript->m_Functions.empty()
							|| !pCompiledScript->m_Imports.empty())
						{
							BUFFER(functionName, MAX_INPUT_LENGTH);
							const char *functionArgs;
							CompiledScript *pNewCompiledScript = NULL;
							
							functionArgs = ScriptEngineGetArgument(buf1, functionName); 
							
							CompiledScript::Function *	pFunction = thread->GetCurrentCompiledScript()->FindFunction(functionName, pNewCompiledScript);

							if (pFunction)
							{
								thread->SetCurrentBytecodePosition(bytecodePtr);
								ScriptEngineRunScriptEnterFunction(thread, pNewCompiledScript, pFunction, functionArgs);
								pCompiledScript = thread->GetCurrentCompiledScript();
								bytecodePtr = thread->GetCurrentBytecodePosition();
								break;
							}
							
						}
						if (go->GetEntityType() == Entity::TypeCharacter && !IS_STAFF((CharData *)go))
						{
							if (*buf1 && strlen(buf1) >= MAX_INPUT_LENGTH-1)
							{
								//	Security against the default "" case which should never happen...
								((char *)buf1)[MAX_INPUT_LENGTH-1] = 0;
								mudlogf(NRM, 101, FALSE, "SCRIPTERROR: Trigger: \"%s\" [%s]: (PC|UNKNOWN)COMMAND opcode had parameter > %d bytes; modified value before passing to command interpreter, may have written to static or table data!",
										trig->GetName(), trig->GetVirtualID().Print().c_str(), MAX_INPUT_LENGTH);
								mudlogf(NRM, 101, FALSE, "String: %s", buf1);
							}
							command_interpreter((CharData *)go, buf1);
						}
					}
					break;

				//	Stack Manipulation
				case SBC_PUSH:				//	table #
					stack.PushCharPtr(data >= 0 && data < pCompiledScript->m_TableSize ? pCompiledScript->m_Table + data : "");
					break;
				
				case SBC_OPERATION:			//	operation
					buf1 = stack.Pop();
					if (data != OP_NOT && data != OP_BITWISE_COMPLEMENT && data != OP_SPECIAL_FOREACH)
						buf2 = stack.Pop();
					else
						buf2 = NULL;
					ScriptEngineOperation(thread, buf2, buf1, data, result);
					stack.Push(result);
					break;
					
				case SBC_DEREFERENCE:
					{
						DereferenceData	deref;

						if (data & DEREF_SUBSCRIPT)	deref.subscript	= strtol(stack.Pop(), NULL, 0);
						
						if (data & DEREF_SUBFIELD)	deref.subfield	= stack.Pop();
						if (data & DEREF_FIELDCONTEXT)	deref.fieldcontext	= strtol(stack.Pop(), NULL, 0);
	/*					{
							const char *context = stack.Pop();
							skip_spaces(context);
							if (isdigit(*context))	deref.context	= strtol(context, NULL, 0);
							else
							{
								const char *p = context;
								while (isalnum(*p))	++p;
								deref.context = GetStringFastHash(context, p - context);
							}
						}					
	*/

						if (data & DEREF_FIELD)		deref.field		= stack.Pop();

						if (data & DEREF_CONTEXT)	deref.context	= strtol(stack.Pop(), NULL, 0);
	/*					{
							const char *context = stack.Pop();
							skip_spaces(context);
							if (isdigit(*context))	deref.context	= strtol(context, NULL, 0);
							else
							{
								const char *p = context;
								while (isalnum(*p))	++p;
								deref.context = GetStringFastHash(context, p - context);
							}
						}					
	*/

						deref.var		= stack.Pop();
						deref.result	= result;
						
						if (data & DEREF_SUBSCRIPT_ONLY)
						{
							extern const char *ScriptEngineListCopyWord(const char *src, char *&dst, int num);
							char *p = result;
							ScriptEngineListCopyWord(deref.var, p, deref.subscript);
							stack.Push(result);
							break;
						}
						
						if (data == DEREF_SUBFIELD	//	Could be a function
							&& (!pCompiledScript->m_Functions.empty()
								|| !pCompiledScript->m_Imports.empty()))
						{
							const char *	functionName = deref.var;
							const char *	functionArgs = deref.subfield;
							CompiledScript *pNewCompiledScript = NULL;
							
							CompiledScript::Function *	pFunction = thread->GetCurrentCompiledScript()->FindFunction(functionName, pNewCompiledScript);
							
	#if 0	//	Support calling Entity functions
			//	This doesnt work because it would change the 'self' entity and be effectively 'running' on the other entity,
			//	which can be disastrous if it is purged
			//	So, to really call a function on something else, it has to be threaded off
							if (!pFunction)
							{
								Hash hash = GetStringFastHash(functionName);
								
								FOREACH(TrigData::List, TRIGGERS(SCRIPT(go)), iter)
								{
									TrigData *t = *iter;
									if (TRIGGERS(t) == 0 && t->GetPrototype() != NULL)
									{
										pFunction = t->m_pCompiledScript->FindFunction(functionName, hash);
										
										if (pFunction)
										{
											pNewCompiledScript = t->m_pCompiledScript;
											break;
										}
									}
								}
							}
	#endif
							
							if (pFunction)
							{
								thread->SetCurrentBytecodePosition(bytecodePtr);
								ScriptEngineRunScriptEnterFunction(thread, pNewCompiledScript, pFunction, functionArgs, CallstackEntry::Call_Embedded);
								pCompiledScript = thread->GetCurrentCompiledScript();
								bytecodePtr = thread->GetCurrentBytecodePosition();
								break;
							}
						}
						
						ScriptEngineDereference(sc, thread, deref, (data & DEREF_CONTINUED) != 0);
						
						if (data & DEREF_SUBSCRIPT)
						{
							extern const char *ScriptEngineListCopyWord(const char *src, char *&dst, int num);
							char *p = result;
							if (deref.subscript > 0)	ScriptEngineListCopyWord(deref.GetResult(), p, deref.subscript);
							else						*p = '\0';
							
							//	Whatever the type was, it isn't anymore!
							deref.resultType = DereferenceData::ResultString;
						}
						
						stack.Push(deref.GetResult());
					}
					break;
					
				case SBC_STRING:			//	count
					buf1 = stack.Pop();
					ScriptEngineString(buf1, stack, /* stackCount, stackSize,*/ result);
					stack.Push(result);
					break;
					
				case SBC_BRANCH:
					bytecodePtr = pCompiledScript->m_Bytecode + MIN(data, (int)pCompiledScript->m_BytecodeCount);
					break;
					
				case SBC_BRANCH_FALSE:
					buf1 = stack.Pop();
					if (!*buf1 || (is_number(buf1) && atoi(buf1) == 0))
						bytecodePtr = pCompiledScript->m_Bytecode + MIN(data, (int)pCompiledScript->m_BytecodeCount);
					break;
					
				case SBC_BRANCH_LAZY_FALSE:
					buf1 = stack.Peek();

					if (!ScriptEngineOperationEvaluateAsBoolean(buf1, str_type(buf1)))
					{
						stack.Pop();
						stack.PushCharPtr("0");
						bytecodePtr = pCompiledScript->m_Bytecode + MIN(data, (int)pCompiledScript->m_BytecodeCount);
					}
					break;

				case SBC_BRANCH_LAZY_TRUE:
					buf1 = stack.Peek();
					
					if (ScriptEngineOperationEvaluateAsBoolean(buf1, str_type(buf1)))
					{
						stack.Pop();
						stack.PushCharPtr("1");
						bytecodePtr = pCompiledScript->m_Bytecode + MIN(data, (int)pCompiledScript->m_BytecodeCount);
					}
					break;
					
				case SBC_LINE:
					thread->SetCurrentLine(data);
					
					if (thread->m_bHasBreakpoints)
					{
						//	See if a breakpoint has been reached
						TrigThread::BreakpointSet::iterator bp = thread->m_Breakpoints.find(data);
						if (bp != thread->m_Breakpoints.end())
						{
							thread->m_bDebugPaused = true;
							
							thread->SetCurrentBytecodePosition(bytecodePtr);
							
							return ret_val;
						}
					}
					break;
			}
			
			if (PURGED(trig) || PURGED(go))	break;
			
			if (result[MAX_SCRIPT_STRING_LENGTH / 2])
			{
				mudlogf(NRM, 101, FALSE, "SCRIPTWARNING: Trigger: \"%s\" [%s]: stack entry of greater than of %d bytes (> 50%%)",
						trig->GetName(), trig->GetVirtualID().Print().c_str(), stack.Count());
				halted = true;
			}
		}
	}

	if (!PURGED(trig))
	{
		delete thread;
		
		if ((trig->GetPrototype() == NULL
			|| (trig->m_Type == MOB_TRIGGER && trig->m_Triggers == MTRIG_LOAD)
			|| (trig->m_Type == OBJ_TRIGGER && trig->m_Triggers == OTRIG_LOAD))
			&& trig->m_Threads.empty())
		{
			sc->RemoveTrigger(trig);
		}
	}
	
	switch (go->GetEntityType()) {
		case Entity::TypeCharacter:
		case Entity::TypeObject:
			if (PURGED(go))	return 1;
			break;
		default:
			//	Shut up compiler warning about unhandled enum
			break;
	}
	
	return ret_val;
}



void ScriptEngineRunScriptEnterFunction(TrigThread *pThread, CompiledScript *pCompiledScript, CompiledScript::Function *pFunction, const char *functionArgs, CallstackEntry::CallType calltype)
{
	if ( pThread->PushCallstack(pCompiledScript, pCompiledScript->m_Bytecode + pFunction->offset, pFunction->name.c_str(), calltype) )
	{
		if (pFunction->hasArguments)
		{
			BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
			
			FOREACH(Lexi::StringList, pFunction->arguments, arg)
			{
				functionArgs = ScriptEngineGetParameter(functionArgs, buf);
				if (!arg->empty())
					pThread->GetCurrentVariables().Add(arg->c_str(), buf);
			}
		}
		else
			pThread->GetCurrentVariables().Add("arg", functionArgs);
		
		bytecode_t *bytecodePtr = pThread->GetCurrentBytecodePosition();
		if (*bytecodePtr == SBC_START)
		{
			pThread->SetCurrentBytecodePosition(bytecodePtr + 1);
		}
		else
		{
			ScriptEngineReportError(pThread, true, "Script doesn't start with SBC_START opcode!");
		}
	}
}



void ScriptEngineRunScriptThreadFunctionOntoEntity(Scriptable *sc, TrigThread *thread, const char *entity, const char *functionName, const char *functionArgs)
{
	Entity *e = NULL;
	
	if (IsIDString(entity))
	{
		IDNum uid = ParseIDString(entity);
		
		//	If vd is a reference, we find the entity, grab IT'S variable...
		if (uid <= 0)		//	find the target script from the uid number
		{
			ScriptEngineReportError(thread, false, "thread: illegal uid '%s' when threading '%s'", entity, functionName);
			return;
		}

		e = IDManager::Instance()->Find(uid);
		if (e && e->GetEntityType() == Entity::TypeCharacter && IS_STAFF(static_cast<CharData *>(e)))
		{
			ScriptEngineReportError(thread, false, "thread: cannot thread onto staff; attempting to attach to '%s'", ((CharData *)e)->GetName());
			return;				
		}
	}
	else if (IsVirtualID(entity))
	{
		e = get_room(entity, thread->GetCurrentVirtualID().zone);	
	}
	
	if (!e)
	{
		ScriptEngineReportError(thread, true, "thread: \"%s\" [%s] unable to find entity \"%s\" for threading!",
				thread->GetCurrentScript()->GetName(),
				thread->GetCurrentVirtualID().Print().c_str(),
				entity);
		return;
	}
	
	CompiledScript *	functionScript = NULL;
	CompiledScript::Function *	pFunction = thread->GetCurrentCompiledScript()->FindFunction(functionName, functionScript);

	if (!pFunction)
	{
		ScriptEngineReportError(thread, true, "Script \"%s\" [%s] unable to find function \"%s\" for threading!",
				thread->GetCurrentScript()->GetName(),
				thread->GetCurrentVirtualID().Print().c_str(),
				functionName);
		return;
	}
	
	TrigData * newTrig = NULL;
	FOREACH(TrigData::List, e->GetScript()->m_Triggers, iter)
	{
		if ((*iter)->GetPrototype() == NULL && !str_cmp((*iter)->GetName(), "<Threaded>"))
		{
			newTrig = *iter;
			break;
		}
	}
	
	if (!newTrig)
	{
		newTrig = new TrigData;
		newTrig->SetName("<Threaded>");
		//	NEED TO COPY MORE IF WE DONT USE COPY CONSTRUCTOR!
		
		trig_list.push_front(newTrig);
		
		switch (e->GetEntityType()) {
			case Entity::TypeCharacter:	newTrig->m_Type = MOB_TRIGGER;	break;
			case Entity::TypeObject:	newTrig->m_Type = OBJ_TRIGGER;	break;
			case Entity::TypeRoom:		newTrig->m_Type = WLD_TRIGGER;	break;
			default:					newTrig->m_Type = -1;			break;
		}
		
		e->GetScript()->AddTrigger(newTrig);
	}
	
	TrigThread *newThread = new TrigThread(newTrig, e, TrigThread::DontCreateRoot);
	ScriptEngineRunScriptEnterFunction(newThread, functionScript, pFunction, functionArgs);
	ScriptEngineRunScript(newThread);
}


bool ScriptEngineRunScriptAsFunctionOnEntity(Scriptable *sc, TrigThread *thread, Entity *e, const char *functionName, const char *functionArgs, char *output)
{
	CompiledScript *	functionScript = NULL;
	CompiledScript::Function *	pFunction = NULL;

	if (PURGED(e))
		return false;
	
	{
		Hash hash = GetStringFastHash(functionName);
		
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(e)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGERS(t) == 0 && t->GetPrototype() != NULL)
			{
				pFunction = t->m_pCompiledScript->FindFunction(functionName, hash);
				
				if (pFunction)
				{
					functionScript = t->m_pCompiledScript;
					break;
				}
			}
		}
	}

#if 0
	if (!pFunction)
	{
		return false;
	}

	thread->SetCurrentBytecodePosition(bytecodePtr - pCompiledScript->bytecode);
	ScriptEngineRunScriptEnterFunction(thread, pTrigger, pFunction, functionArgs, CallstackEntry::Call_Embedded);
	pCompiledScript = thread->GetCurrentBytecode();
	bytecodePtr = pCompiledScript->bytecode + thread->GetCurrentBytecodePosition();
	thread->m_pEntity = e;

	return true;
#else
	if (!pFunction)
	{
//		ScriptEngineReportError(thread, true, "Script \"%s\" [%s] unable to find function \"%s\" for threading!",
//				thread->GetCurrentFunctionScript()->GetName(),
//				thread->GetCurrentFunctionScript()->GetVirtualID().Print().c_str(),
//				functionName);
		return false;
	}
	
	TrigData * newTrig = NULL;
	FOREACH(TrigData::List, e->GetScript()->m_Triggers, iter)
	{
		if ((*iter)->GetPrototype() == NULL && !str_cmp((*iter)->GetName(), "<Threaded>"))
		{
			newTrig = *iter;
			break;
		}
	}
	
	if (!newTrig)
	{
		newTrig = new TrigData;
		newTrig->SetName("<Threaded>");
		
		trig_list.push_front(newTrig);
		
		switch (e->GetEntityType()) {
			case Entity::TypeCharacter:	newTrig->m_Type = MOB_TRIGGER;	break;
			case Entity::TypeObject:	newTrig->m_Type = OBJ_TRIGGER;	break;
			case Entity::TypeRoom:		newTrig->m_Type = WLD_TRIGGER;	break;
			default:					newTrig->m_Type = -1;			break;
		}
		
		e->GetScript()->AddTrigger(newTrig);
	}
	
	TrigThread *newThread = new TrigThread(newTrig, e, TrigThread::DontCreateRoot);
	ScriptEngineRunScriptEnterFunction(newThread, functionScript, pFunction, functionArgs);
	ScriptEngineRunScript(e, newTrig, output, newThread);
	
	return true;
#endif
}



/*
	We need to handle extremely complex behaviors now!
	
	set var
	set var{}
	set list.word(4)
	set record.field.field.field
	
	This is going to get REALLY tricky... but it can be done!

	set var
		Look up VD 'var'
		Copy value into buffer
		Assign buffer to VD
		
	set var{}
		Look up VD 'var' with context {}
		Copy value into buffer
		Assign buffer to VD
		
	set list.word(4)
		Look up VD 'list'
		Examine field.
			Special case 'word', evaluate between ().  Special case: list index
			Copy until word 4 into buffer, append space if necessary
			Skip word of VD
			Copy remainder of VD into buffer
		

	OR:
	
	set list[4]
		Look up VD 'list'
		Evaluate between []
		Copy from VD until word 4 into buffer, append space if necessary
		Copy value into buffer
		Skip word of VD
		Copy remainder of VD into buffer
		
				
	set record.field.field.field
	
	set entity.var
	
	set list[3].var    (Where list.3 is an reference!)
		Look up VD 'list'
		Evaluate between []
		Copy until word 3 into buffer, append space if necessary
		Field, note that current item is a reference
		Look up reference, abort if not found
			Look up variable 'var' on reference, assign to VD if found
			Otherwise, run through script evaluator 
			
			
			
			
	Lets support:
	
	var
	record.field*
	var{context}
	record{context}.field
	thing.var
	thing.record.field*
	thing.var{context}
	thing.record{context}.field
	thing{context}.var
	thing{context}.record.field
	thing{context}.var{context}
	thing{context}.record{context}.field
*/

void ScriptEngineRunScriptProcessSetPerformSet(Scriptable *sc, TrigThread *thread, const char *name, const char *value, unsigned scope, bool bIsUnset);


struct VariableNameFragment
{
	VariableNameFragment() : name(NULL), type(None), error(NoError), offset(-1) { index = 0; }

	enum FragmentType
	{
		None,
		Reference,
		RoomVNum,
		Variable,
		Index,
		CharacterIndex,
		Field
	};
	
	enum Error
	{
		NoError,
		NoItem,
		NoField,
		NoRecord
	};
		
	char *				name;
	FragmentType		type;
	union
	{
		VNum				vnum;
		IDNum				id;
		int					context;
		int					index;
	};
	Error				error;
	int					offset;
};


static int SplitComplexVariableName(char *var, VariableNameFragment *fragments, int maxFragments)
{
	int numFragments = 0;
	
	fragments[numFragments].name = var;
	++numFragments;
	
	char c = *var;
	while (c && numFragments < maxFragments)
	{
		if (c == '.')
		{
			*var = '\0';
			
			c = *++var;
			if (!c || c == '.')
			{
				return -1;
			}

			fragments[numFragments].name = var;
			fragments[numFragments].type = VariableNameFragment::None;
			++numFragments;
			continue;
		}
		else if (c == '[')
		{
			*var = '\0';
			
			c = *++var;
			
			fragments[numFragments].name = var;
			fragments[numFragments].type = VariableNameFragment::Index;
			fragments[numFragments].index = strtol(var, NULL, 0);
			++numFragments;
			
			while (c && c != ']')
				c = *++var;

			if (c != ']')
			{
				return -1;
			}
			
			*var = '\0';
			c = *++var;
			continue;
		}
		
		c = *++var;
	}
	
	
	int firstVariableFrag = 0;
	if (IsIDString(fragments[0].name))
	{
		fragments[0].type = VariableNameFragment::Reference;
		fragments[0].id = ParseIDString(fragments[0].name);
		firstVariableFrag = 1;
	}
	else if (is_number(fragments[0].name))
	{
		fragments[0].type = VariableNameFragment::RoomVNum;
		fragments[0].vnum = atoi(fragments[0].name);
		firstVariableFrag = 1;
	}
	
	if (firstVariableFrag < numFragments)
	{
		fragments[firstVariableFrag].type = VariableNameFragment::Variable;
		ScriptEngineParseVariable(fragments[firstVariableFrag].name,
			fragments[firstVariableFrag].name, fragments[firstVariableFrag].context);
	}
	
	for (int i = firstVariableFrag + 1; i < numFragments; ++i)
	{
		VariableNameFragment &fragment = fragments[i];
		
		if (fragment.type != VariableNameFragment::None)
			continue;
		
		if (is_number(fragment.name))
		{
			fragment.type = VariableNameFragment::Index;
			fragment.index = strtol(fragment.name, NULL, 0);
		}
		else
		{
			fragment.type = VariableNameFragment::Field;
		}
	}
	
	return numFragments;
}


void ScriptEngineRunScriptProcessSetPerformSet(Scriptable *sc, TrigThread *thread, const char *name, const char *value, unsigned scope, bool bIsUnset)
{
	BUFFER(varname, MAX_SCRIPT_STRING_LENGTH);
	const int NUM_VAR_FRAGMENTS = 64;
	VariableNameFragment	varFragments[NUM_VAR_FRAGMENTS];
	int						targetScope;
	ScriptVariable *		vd;
	
	skip_spaces(name);
	
	if (!*name)
	{
		ScriptEngineReportError(thread, false, "set: bad parsing of '%s'", name);
		return;
	}
	
	//	Duplicate the name; SplitComplexVariableName is destructive
	strcpy(varname, name);
	
	int numFragments = SplitComplexVariableName(varname, varFragments, NUM_VAR_FRAGMENTS);
	int lastFragment = numFragments - 1;
		
	int varNameFragmentNum = 0;
	
	//	Step 1. RESOLVE FIRST COMPONENT:
	//		This component may be a reference or room vnum <deprecate!>
	//		We look up the entity, redirect our Scriptable to that, and advance to next component, 
	if (varFragments[0].type == VariableNameFragment::Reference)
	{
		Entity *		entity;
		int				id = varFragments[0].id;
		
		if (id <= 0)
		{
			ScriptEngineReportError(thread, false, "set: illegal uid '%d' when setting '%s'", id, name);
			return;
		}
		else if ((entity = IDManager::Instance()->Find(id)) == NULL)
		{
			ScriptEngineReportError(thread, false, "set: uid '%d' not found when setting '%s'", id, name);
			return;
		}
		
		sc = entity->GetScript();
		
		varNameFragmentNum = 1;
		targetScope = scope = Scope::Global;
	}
	else if (varFragments[0].type == VariableNameFragment::RoomVNum)
	{
		RoomData *		room = varFragments[0].vnum != 0 ? world.Find(varFragments[0].vnum) : NULL;
		
		if (room == NULL)	//	== 0 because we don't want room 0 to be valid for this, could be an error
		{
			ScriptEngineReportError(thread, false, "set: room # '%d' not found when setting '%s'", varFragments[0].vnum, name);
			return;
		}
		
		sc = room->GetScript();
		
		varNameFragmentNum = 1;
		targetScope = scope = Scope::Global;
	}
	else
	{
		targetScope = Scope::All;
	}
	
	//	Step 2. LOOK UP THE VARIABLE:
	//		Use the appropriate component as determined above to look up the variable on the Scriptable
	VariableNameFragment &varNameFragment = varFragments[varNameFragmentNum];
	if (varNameFragmentNum > lastFragment
		|| varNameFragment.type != VariableNameFragment::Variable)
	{
		ScriptEngineReportError(thread, false, "set: bad parsing of '%s'", name);
		return;
	}
	
	if (bIsUnset && varNameFragmentNum == lastFragment)
	{
		//	If we are unsetting, and we have reached the last fragment, then just delete it and be done
		vd = ScriptEngineFindVariable(sc, thread, varNameFragment.name, 0, varNameFragment.context, targetScope | Scope::Extract);
		delete vd;
		return;
	}
	else
	{
		vd = ScriptEngineFindVariable(sc, thread, varNameFragment.name, 0, varNameFragment.context, targetScope | Scope::ContextSpecific);
		
		if (bIsUnset && !vd)
			return;
	}
	
	//	Step 3. COMPLEX HANDLING:
	//		Handle the index and field components of the variable path
	BUFFER(dest, MAX_SCRIPT_STRING_LENGTH);
	if (varNameFragmentNum < lastFragment)
	{
		extern const char *ScriptEngineRecordSkipListWord(const char *src);
		extern const char *ScriptEngineRecordSkipToField(const char *src, const char *field, bool &found);
		extern const char *ScriptEngineRecordSkipField(const char *src);

		//	HANDLE INDICES & RECORDS!
		const char *source = vd ? vd->GetValue() : "";
		const char *sourceStart = source;
		
		const char *origDest = dest;
		
		//	We start with
		bool bTerminal = false;
		int numFrag = varNameFragmentNum + 1;
		for (; numFrag <= lastFragment; ++numFrag)
		{
			VariableNameFragment &fragment = varFragments[numFrag];
			
			if (fragment.type == VariableNameFragment::Index)
			{
				int n = fragment.index;
				while (--n > 0)	//	1-based index, skip to item, not over; 1 = skip 0.  -- = 0.
				{
					source = ScriptEngineRecordSkipListWord(source);	
				}
				
				fragment.offset = source - sourceStart;
				
				//	source is now at the point for the item we want to modify/erase
				
				//	See if we have hit the end of the possible run
				if (!*source || ScriptEngineRecordIsRecordItem(source) || ScriptEngineRecordIsRecordEnd(source))
				{
					fragment.error = VariableNameFragment::NoItem;
					if (numFrag == lastFragment)
						++numFrag;	//	We reached the end at least... don't treat it as early term!
					break;
				}
			}
			else if (fragment.type == VariableNameFragment::CharacterIndex)
			{
				//	Character in the string
				//	If we're in a record we want to make sure we don't go past the end of the field
			}
			else	//	Field
			{
				if (!ScriptEngineRecordIsRecordStart(source))
				{
					//	This was expected to be a record; it isn't.
					//	It becomes one, and we fill in the rest
					fragment.offset = source - sourceStart;
					fragment.error = VariableNameFragment::NoRecord;
					break;
				}
				
				bool found;
				source = ScriptEngineRecordSkipToField(source, fragment.name, found);
				fragment.offset = source - sourceStart;
				
				if (!found)
				{
					fragment.error = VariableNameFragment::NoField;
					break;
				}
			}
		}
		
		bool bEarlyTermination = numFrag <= lastFragment;
		
		if (!bEarlyTermination)
		{
			numFrag = lastFragment;	//	Drop back to the last fragment
		}
		else
		{
			for (int i = numFrag + 1; i <= lastFragment; ++i)
			{
				VariableNameFragment &fragment = varFragments[i];
				if (fragment.type == VariableNameFragment::Index)
					fragment.error = VariableNameFragment::NoItem;
				else if (fragment.type == VariableNameFragment::Field)
					fragment.error = VariableNameFragment::NoRecord;
			}
		}
		
		//	We copy everything up to numFrag...
		//	TODO: simple strcpy to varFragments[numFrag].offset
		int copyLength = varFragments[numFrag].offset;
		if (copyLength > 0)
		{
			strncpy(dest, sourceStart, copyLength);
			dest[copyLength] = '\0';
			dest += copyLength;
		}
		
		const char *sourcePreSkip = source;
		
		
		//	Skip the original value
		if (varFragments[numFrag].type == VariableNameFragment::Index)
		{
			source = ScriptEngineRecordSkipListWord(source);
		}
		else if (varFragments[numFrag].type == VariableNameFragment::Field)
		{
			//	If it was a no-record, then we just skip the word
			//	Otherwise blast the whole god damned thing!
			if (varFragments[numFrag].error == VariableNameFragment::NoRecord)
			{
				if (!bIsUnset)
				{
					source = ScriptEngineRecordSkipListWord(source);
				}
			}
			else	//	was either found or NoField
				source = ScriptEngineRecordSkipField(source);
		}
		
		
		if (!bIsUnset)
		{
			int numClosures = 0;
			
			if (varFragments[numFrag].type == VariableNameFragment::Index
				&& varFragments[numFrag].error == VariableNameFragment::NoItem
				&& varFragments[numFrag].index > 0)
			{
				//	Add a space
				strcat(dest, " ");
				dest += strlen(dest);
			}
			
			if (bEarlyTermination)
			{
				//	Handle record error fill-up if necessary
				bool bWasRecordError = false;
				for (int fillFrag = numFrag; fillFrag <= lastFragment; ++fillFrag)
				{
					VariableNameFragment &fragment = varFragments[fillFrag];
					if (fragment.type != VariableNameFragment::Field
						|| fragment.error == VariableNameFragment::NoError)
						continue;
					
					//	We've hit a fragment requiring a fill-in
					if (fragment.error == VariableNameFragment::NoRecord)
					{
						// We expected a record.  We didn't get one.
						strcat(dest, RECORD_START_DELIMITER);
						dest += strlen(dest);
						++numClosures;
					}
					else
					{
						//	Merely no-field
						strcat(dest, RECORD_ITEM_DELIMITER);
						dest += strlen(dest);
					}
					
					//	NoField is implied
					strcat(dest, fragment.name);
					strcat(dest, ":");
					dest += strlen(dest);
					
					for (++fillFrag; fillFrag <= lastFragment; ++fillFrag)
					{
						if (varFragments[fillFrag].type == VariableNameFragment::Field)
						{
							strcat(dest, RECORD_START_DELIMITER);
							strcat(dest, varFragments[fillFrag].name);
							strcat(dest, ":");
							dest += strlen(dest);
							++numClosures;
						}
					}
				}
			}
			
			//	Insert the value
			strcat(dest, value);
			dest += strlen(dest);
			
			if (source > sourceStart	//	We copied something
				&& varFragments[numFrag].type == VariableNameFragment::Index	//	and it was a list!
				&& *source && !isspace(*source)		//	And we have more to copy that isn't a space...
				&& !ScriptEngineRecordIsRecordItem(source)
				&& !ScriptEngineRecordIsRecordEnd(source))
			{
				//	Add a space
				strcat(dest, " ");
			}
			
			if (bEarlyTermination && numClosures > 0)
			{
				while (numClosures--)
				{
					strcat(dest, RECORD_END_DELIMITER);
				}
			}
			
			dest += strlen(dest);
			
			//	If it was a field and there was no record-related error
			//	and its not at the end of an item/record...
			if (varFragments[numFrag].type == VariableNameFragment::Field
				&& varFragments[numFrag].error == VariableNameFragment::NoError
				&& !ScriptEngineRecordIsRecordItem(source)
				&& !ScriptEngineRecordIsRecordEnd(source))
			{
				strcat(dest, RECORD_ITEM_DELIMITER);
				dest += strlen(dest);
			}
			else if (varFragments[numFrag].type == VariableNameFragment::Field
				&& varFragments[numFrag].error == VariableNameFragment::NoRecord
				&& *source && !isspace(*source))
			{
				*dest++ = ' ';
				*dest = '\0';
			}
		}
		else if (!bEarlyTermination)	//	This is an un-set
		{
			VariableNameFragment &fragment = varFragments[numFrag];
			
			if (fragment.type == VariableNameFragment::Field
				&& fragment.error == VariableNameFragment::NoError)
			{
				//	Zap the field name
				dest -= strlen(fragment.name) + 1;		//	Jump back by field name length + ':'
				*dest = '\0';							//	Terminate!
				
				if (ScriptEngineRecordIsRecordItem(dest - 2))
				{
					//	There was a record item prefix; this is after the first field
					if (ScriptEngineRecordIsRecordEnd(source))
					{
						//	Cut out the ## before it, since we're at the end
						dest -= 2;
						*dest = '\0';
					}
				}
				else if (!ScriptEngineRecordIsRecordStart(dest - 2))
				{
					//	Add a ## if there is another item next and no separator
					strcat(dest, RECORD_ITEM_DELIMITER);
				}
			}
		}
		
		//	Copy the rest of source
		strcat(dest, source);
		
		value = origDest;
	}
	
	if (vd)
	{
		vd->SetValue(value);
	}
	else if (!bIsUnset)
	{
		ScriptVariable::List *	varList = NULL;
		
		switch (scope)
		{
			case Scope::Global:		varList = &sc->m_Globals;	break;
			case Scope::Shared:
			{
				if (GET_THREAD_TRIG(thread)->GetPrototype())
					varList = &GET_THREAD_TRIG(thread)->GetPrototype()->m_SharedVariables;
				break;
			}
			default:				varList = &GET_THREAD_VARS(thread);	break;
		}
		
		if (varList)
		{
			varList->Add(varNameFragment.name, value, varNameFragment.context);
		}
	}
}



void ScriptEngineRunScriptProcessSet(Scriptable *sc, TrigThread *thread, const char *name, const char *value, unsigned scope)
{
	ScriptVariable *vd = NULL;
	
	skip_spaces(name);
	skip_spaces(value);
	
	if (!*name)
		ScriptEngineReportError(thread, false, "set w/o an arg: 'set %s %s'", name, value);
	else if (strchr(name, '.') || strchr(name, '['))
	{
		//	Complex handling.  Need to break into fragments, determine the vd, and do list/field alterations
		ScriptEngineRunScriptProcessSetPerformSet(sc, thread, name, value, scope, false);
	}
	else
	{
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
		
		int	numContext = 0;
		
		ScriptEngineParseVariable(name, var, numContext);
		vd = ScriptEngineFindVariable(sc, thread, var, 0, numContext, Scope::All | Scope::ContextSpecific);
		
		if (vd)
		{
			vd->SetValue(value);
		}
		else
		{
			ScriptVariable::List *	varlist = NULL;
		
			switch (scope)
			{
				case Scope::Global:		varlist = &sc->m_Globals;					break;
				case Scope::Shared:
				{
					if (GET_THREAD_TRIG(thread)->GetPrototype())
						varlist = &GET_THREAD_TRIG(thread)->GetPrototype()->m_SharedVariables;
					break;
				}
				default:				varlist = &GET_THREAD_VARS(thread);	break;
			}
			
			if (varlist)
				varlist->Add(var, value, numContext);
		}
	}
}


static void UnsetAllFindAndErase(ScriptVariable::List &list, const char *name, Hash hash)
{
	for (ScriptVariable::List::iterator iter = list.begin(); iter != list.end(); )
	{
		ScriptVariable *var = *iter;
		
		if (var->GetNameHash() == hash)
		{
			if (var->GetNameString() == name)
			{
				iter = list.erase(iter);
				delete var;
				continue;
			}
			
			ReportHashCollision(hash, name, var->GetName());
		}

		++iter;
	}
}


void ScriptEngineRunScriptProcessUnset(Scriptable *sc, TrigThread *thread, const char *buf)
{
	if (!*buf)
		ScriptEngineReportError(thread, false, "unset w/o an arg: unset");
	else {
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
		
		skip_spaces(buf);
		
		while (*buf)
		{
			buf = ScriptEngineGetArgument(buf, var);

			if (str_str(var, "{*}"))
			{
				//	All contexts
				Entity *e;
				int context = 0;
				
				ScriptEngineParseVariableWithEntity(var, var, context, e);
				Hash hash = GetStringFastHash(var);
			
				if (!e)	UnsetAllFindAndErase(thread->GetCurrentVariables(), var, hash);
				else	sc = SCRIPT(e);
				
				UnsetAllFindAndErase(sc->m_Globals, var, hash);
			}
			else if (strchr(var, '.') || strchr(var, '['))
			{
				//	Complex variables
				ScriptEngineRunScriptProcessSetPerformSet(sc, thread, var, NULL, Scope::All, true);
			}
			else
			{
				//	Simple case; most common but we have to do the checks above first anyways
				int context = 0;
				
				ScriptEngineParseVariable(var, var, context);
				
				ScriptVariable *vd = ScriptEngineFindVariable(sc, thread, var, 0, context, Scope::All | Scope::Extract);
				delete vd;
			}
		}
	}
}


//	makes a local variable into a global variable
void ScriptEngineRunScriptProcessScope(Scriptable *sc, TrigThread *thread, const char *name, unsigned scope)
{
	ScriptVariable *vd;

	skip_spaces(name);

	if (!*name)
		ScriptEngineReportError(thread, false, "global or shared w/o an arg");
	else {
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
		int	context = 0;
		
		ScriptEngineParseVariable(name, var, context);
		Hash hash = GetStringFastHash(var);
		vd = ScriptEngineFindVariable(sc, thread, var, hash, context, (Scope::All & ~scope) | Scope::Extract);
		
		ScriptVariable::List*	varlist = NULL;
		
		switch (scope)
		{
			case Scope::Global:		varlist = &sc->m_Globals;					break;
			case Scope::Shared:
			{
				if (GET_THREAD_TRIG(thread)->GetPrototype())
					varlist = &GET_THREAD_TRIG(thread)->GetPrototype()->m_SharedVariables;
				break;
			}
			default:				varlist = &GET_THREAD_VARS(thread);	break;
		}
		
		if (varlist)
		{
			if (vd)
			{
				//	We use 'Add' because it will overwrite a matching variable
				varlist->Add(vd);
			}
			else if (!ScriptEngineFindVariable(sc, thread, var, hash, context, scope | Scope::ContextSpecific))
				varlist->Add(var, "", context);
		}
	}
}



void ScriptEngineRunScriptProcessPushback(Scriptable *sc, TrigThread *thread, const char *name, const char *value)
{
	ScriptVariable *vd = NULL;
	
	if (!*name)
	{
		ScriptEngineReportError(thread, false, "pushback w/o an arg");
	}
	else
	{
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
		int	context = 0;
		
		Entity *e = NULL;
		if (!ScriptEngineParseVariableWithEntity(name, var, context, e))
		{
			ScriptEngineReportError(thread, false, "pushback with invalid variable '%s'", var);
			return;
		}
		
		vd = ScriptEngineFindVariable(e ? SCRIPT(e) : sc, thread, var, 0, context, (e ? Scope::Global : Scope::All) | Scope::ContextSpecific);

		if (vd)
		{
			skip_spaces(value);	//	because we're a list, we can freely destroy spacing
			
			if (!vd->GetValueString().empty())
			{
				//RECREATE(vd->value, char, strlen(vd->value) + strlen(value) + 2);
				//strcat(vd->value, " ");
				//strcat(vd->value, value);
				
				BUFFER(newval, MAX_SCRIPT_STRING_LENGTH);
				
				snprintf(newval, MAX_SCRIPT_STRING_LENGTH, "%s %s", vd->GetValue(), value);
				newval[MAX_SCRIPT_STRING_LENGTH - 1] = '\0';
				
				vd->SetValue(newval);
			}
			else
				vd->SetValue(value);
		}
		else if (e)
			SCRIPT(e)->m_Globals.Add(var, value, context);
		else
			GET_THREAD_VARS(thread).Add(var, value, context);
	}
}


void ScriptEngineRunScriptProcessPushfront(Scriptable *sc, TrigThread *thread, const char *name, const char *value)
{
	ScriptVariable *vd = NULL;
	
	if (!*name)
	{
		ScriptEngineReportError(thread, false, "pushfront w/o an arg");
	}
	else
	{
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
		int	context = 0;
		
		Entity *e = NULL;
		if (!ScriptEngineParseVariableWithEntity(name, var, context, e))
		{
			ScriptEngineReportError(thread, false, "pushfront with invalid variable '%s'", var);
			return;
		}
		
		vd = ScriptEngineFindVariable(e ? SCRIPT(e) : sc, thread, var, 0, context, (e ? Scope::Global : Scope::All) | Scope::ContextSpecific);

		if (vd)
		{
			skip_spaces(value);	//	because we're a list, we can freely destroy spacing
			
			if (!vd->GetValueString().empty())
			{
				BUFFER(newval, MAX_SCRIPT_STRING_LENGTH);
				
				snprintf(newval, MAX_SCRIPT_STRING_LENGTH, "%s %s", value, vd->GetValue());
				newval[MAX_SCRIPT_STRING_LENGTH - 1] = '\0';
				
				vd->SetValue(newval);
			}
			else
				vd->SetValue(value);
		}
		else if (e)
			SCRIPT(e)->m_Globals.Add(var, value, context);
		else
			GET_THREAD_VARS(thread).Add(var, value, context);
	}
}


void ScriptEngineRunScriptProcessPopfront(Scriptable *sc, TrigThread *thread, const char *name, const char *popvar, char *result)
{
	ScriptVariable *vd = NULL;
	
	if (!*name)
	{
		ScriptEngineReportError(thread, false, "popfront w/o an arg");
	}
	else
	{
		BUFFER(popped, MAX_SCRIPT_STRING_LENGTH);
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
		int	context = 0;
		
		Entity *e = NULL;
		if (!ScriptEngineParseVariableWithEntity(name, var, context, e))
		{
			ScriptEngineReportError(thread, false, "pushback with invalid variable '%s'", var);
			return;
		}
		
		vd = ScriptEngineFindVariable(e ? SCRIPT(e) : sc, thread, var, 0, context, (e ? Scope::Global : Scope::All) | Scope::ContextSpecific);
		
		if (vd)
		{
			BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
			
			strcpy(buf, vd->GetValue());
			
			char *temp = popped;
			const char *newval = ScriptEngineListCopyNextWord(buf, temp);
			skip_spaces(newval);
			
			vd->SetValue(newval);
		}
				
		if (popvar && *popvar)
		{
			context = 0;
			
			if (!ScriptEngineParseVariableWithEntity(popvar, var, context, e))
			{
				ScriptEngineReportError(thread, false, "pushback with invalid variable '%s'", var);
				return;
			}
			
			vd = ScriptEngineFindVariable(e ? SCRIPT(e) : sc, thread, var, 0, context, (e ? Scope::Global : Scope::All) | Scope::ContextSpecific);
			
			if (vd)			vd->SetValue(popped);
			else if (e)		SCRIPT(e)->m_Globals.Add(var, popped, context);
			else			GET_THREAD_VARS(thread).Add(var, popped, context);
		}
		
		if (result)
		{
			strcpy(result, popped);
		}
	}
}


void ScriptEngineRunScriptProcessPopback(Scriptable *sc, TrigThread *thread, const char *name, const char *popvar)
{
	if (!*name)
	{
		ScriptEngineReportError(thread, false, "popback w/o an arg");
	}
	else
	{
		BUFFER(newval, MAX_SCRIPT_STRING_LENGTH);
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);

		const char *popped = "";
		ScriptVariable *vd = NULL;
		
		int	context = 0;

		Entity *e = NULL;
		if (!ScriptEngineParseVariableWithEntity(name, var, context, e))
		{
			ScriptEngineReportError(thread, false, "pushback with invalid variable '%s'", var);
			return;
		}
		
		vd = ScriptEngineFindVariable(e ? SCRIPT(e) : sc, thread, var, 0, context, (e ? Scope::Global : Scope::All) | Scope::ContextSpecific);
		
		if (vd)
		{
			strcpy(newval, vd->GetValue());
			
			const char *nextWord = newval;
			const char *prevWord = nextWord;
			
			while(*nextWord)
			{
				prevWord = nextWord;
				nextWord = ScriptEngineListSkipWord(nextWord);
			}
			nextWord = prevWord;
			popped = nextWord;

			//	prevWord to nextWord is the string
			if (nextWord > newval)
			{
				while (nextWord > newval && isspace(*(nextWord - 1)))
					--nextWord;
				*const_cast<char *>(nextWord) = '\0';
				vd->SetValue(newval);
			}
			else
				vd->SetValue("");
		}
		
		if (*popvar)
		{
			context = 0;
		
			if (!ScriptEngineParseVariableWithEntity(popvar, var, context, e))
			{
				ScriptEngineReportError(thread, false, "pushback with invalid variable '%s'", var);
				return;
			}
			
			vd = ScriptEngineFindVariable(e ? SCRIPT(e) : sc, thread, var, 0, context, (e ? Scope::Global : Scope::All) | Scope::ContextSpecific);
			
			if (vd)		vd->SetValue(popped);
			else if (e)	SCRIPT(e)->m_Globals.Add(var, popped, context);
			else		GET_THREAD_VARS(thread).Add(var, popped, context);
		}
	}
}



void ScriptEngineRunScriptProcessFastMath(Scriptable *sc, TrigThread *thread, const char *name, Operation op, const char *constant)
{
	BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(result, MAX_SCRIPT_STRING_LENGTH);
	int	context = 0;
	
	if (!*name)
	{
		ScriptEngineReportError(thread, false, "fastmath (increment or decrement) w/o an arg");
		return;
	}
	
	ScriptEngineParseVariable(name, var, context);
	ScriptVariable *vd = ScriptEngineFindVariable(sc, thread, var, 0, context, Scope::All | Scope::ContextSpecific);
	
	ScriptEngineOperation(thread, vd ? vd->GetValue() : "0", constant, op, result);
	
	if (vd)
	{
		vd->SetValue(result);
	}
	else
	{
		GET_THREAD_VARS(thread).Add(var, result, context);
	}
}


void ScriptEngineRunScriptProcessPrint(Scriptable *sc, TrigThread *thread, const char *name, const char *format,
		int varArgCount, ThreadStack &stack)
{
	BUFFER(printedstring, MAX_SCRIPT_STRING_LENGTH);
	ScriptVariable *	vd;
	char buf[512];
	char *vararg;
	char *p;
	char *s;
	char c;

	/* JUSTIFICATION_OPTIONS: formats output to left justified, right justified and zero fill (no justification) */
	enum justification_options
	{
		left_justification,
		right_justification,
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

//	vararg = ScriptEnginePopStack(stack, stackCount, stackSize);
	
	if (*name)
	{
		p = printedstring;
		
		while (*format)
		{
			c = *(format++);
			
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
							
						case '+':
							sign = sign_always;
							break;
							
						case ' ':
							if (sign != sign_always)
								sign = space_holder;
							break;
							
						case '0':
							if (justification != left_justification)
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
				
				while (isdigit(c))
				{
					field_width = (field_width * 10) + (c - '0');
					c = *(format++);
				}
				if (field_width > conversion_max)
					field_width = conversion_max;
				
				if (c == '.')
				{
					precision_specified = true;
					
					c = *(format++);
					while (isdigit(c))
					{
						precision = (precision * 10) + (c - '0');
						c = *(format++);
					}
				}
				
				flagFound = true;
				
				s = "";
				num_chars = 0;
				buf[0] = 0;
				if (varArgCount > 0)
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
								
								//vararg = ScriptEnginePopStack(stack, stackCount, stackSize);
								if (varArgCount > 0)
								{
									vararg = stack.Pop();
									--varArgCount;
								}
								else
									vararg = "";
								
								
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
								
								//vararg = ScriptEnginePopStack(stack, stackCount, stackSize);
								if (varArgCount > 0)
								{
									vararg = stack.Pop();
									--varArgCount;
								}
								else
									vararg = "";
								
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
								//vararg = ScriptEnginePopStack(stack, stackCount, stackSize);
								if (varArgCount > 0)
								{
									vararg = stack.Pop();
									--varArgCount;
								}
								else
									vararg = "";

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
		
		BUFFER(var, MAX_SCRIPT_STRING_LENGTH);
		int	context = 0;
		
		ScriptEngineParseVariable(name, var, context);
		vd = ScriptEngineFindVariable(sc, thread, var, 0, context, Scope::All | Scope::ContextSpecific);
		if (vd)
		{
			vd->SetValue(printedstring);
		}
		else
			GET_THREAD_VARS(thread).Add(var, printedstring, context);
	}
	
	while (varArgCount > 0)
	{
		stack.Pop(); //ScriptEngineClearStack(stack, stackCount, stackSize);
		--varArgCount;
	}
}


class ScriptEngineRunScriptResumeEvent : public Event
{
public:
						ScriptEngineRunScriptResumeEvent(unsigned int when, TrigThread *thread) :
							Event(when), m_pThread(thread)
						{}
	
//	static const char *	Type;
	virtual const char *GetType() const { return "ScriptEngineRunScriptResumeEvent"; }

private:
	virtual unsigned int	Run();

	TrigThread *		m_pThread;
};

unsigned int ScriptEngineRunScriptResumeEvent::Run()
{
	m_pThread->m_pWaitEvent = NULL;
	m_pThread->m_bDebugPaused = false;
	
	ScriptEngineRunScript(m_pThread);
	return 0;
}


/* processes any 'wait' commands in a trigger */
void ScriptEngineRunScriptProcessWait(Scriptable *sc, TrigThread *thread, const char *arg) {
	int	time = 1;
	char c = '\0';

	extern struct TimeInfoData time_info;
	extern UInt32 pulse;

	skip_spaces(arg);

	if (!*arg)
		ScriptEngineReportError(thread, false, "wait w/o an arg: 'wait %s'", arg);
	else if (!strn_cmp(arg, "until ", 6)) {
		/* valid forms of time are 14:30 and 1430 */
		int hr = 0;
		int min = 0;
		
		if (sscanf(arg, "until %d:%d", &hr, &min) == 2)
			min += (hr * 60);
		else
			min = (hr % 100) + ((hr / 100) * 60);

		/* calculate the pulse of the day of "until" time */
		int ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

		/* calculate pulse of day of current time */
		time = (pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
				(time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

		/* adjust for next day */
		if (time >= ntime)	time = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - time + ntime;
		else				time = ntime - time;
	} else if (sscanf(arg, "%d %c", &time, &c) == 2) {
		if (c == 'd')		time *= 24 * SECS_PER_MUD_HOUR * PASSES_PER_SEC;
		else if (c == 'h')	time *= SECS_PER_REAL_HOUR * PASSES_PER_SEC;
		else if (c == 't')	time *= SECS_PER_MUD_HOUR * PASSES_PER_SEC;	//	't': ticks
		else if (c == 'm')	time *= PASSES_PER_SEC * SECS_PER_REAL_MIN;
//		else if (c == 's')	time *= PASSES_PER_SEC;
		else if (c == 'p')	;											//	'p': pulses
		else 				time *= PASSES_PER_SEC;						//	Default: seconds
	} else					time *= PASSES_PER_SEC;

	if (time < 1)	ScriptEngineReportError(thread, false, "wait with negative time: 'wait %s'", arg);
	
	thread->m_pWaitEvent = new ScriptEngineRunScriptResumeEvent(MAX(time, 1), thread);
}


void ScriptEngineReportError(TrigThread *pThread, bool toFile, const char *format, ...)
{
	BUFFER(message, MAX_STRING_LENGTH);
	va_list args;
	
	va_start(args, format);
	vsnprintf(message, MAX_STRING_LENGTH, format, args);
	va_end(args);
	
	if (pThread)
	{
//		BUFFER(callstack, MAX_STRING_LENGTH);
		BUFFER(what, MAX_STRING_LENGTH);
		
		if (pThread->m_NumReportedErrors > MAX_SCRIPT_ERRORS)
			return;	//	Enough of this shit!
		else if (pThread->m_NumReportedErrors == MAX_SCRIPT_ERRORS)
		{
			pThread->m_ID = 0;
			strcpy(message, "Error limit exceeded!");
		}
//		else if (pThread->m_Callstack.size() > 1)
#if 0
		{
			TrigThread::Callstack::iterator iter = pThread->m_Callstack.begin(),
				end = pThread->m_Callstack.end();
			
			if (*(*iter)->m_FunctionName)	sprintf_cat(callstack, " (%s)", (*iter)->m_FunctionName);
			sprintf_cat(callstack, "    %s line %d", (*iter)->GetVirtualID().Print().c_str(), (*iter)->m_Line);

			for (++iter; iter != end; ++iter)
			{
				sprintf_cat(callstack, "\n    %s [%s line %d]", (*iter)->GetVirtualID().Print().c_str(), (*iter)->m_Line, (*iter)->m_FunctionName);
			}
		}
#endif
//		else
//		{
//			sprintf_cat(callstack, "%s[%s]", pThread->m_pCurrentCallstack->m_pTrigger->GetVirtualID().Print().c_str());
//		}
	
		switch (pThread->m_pEntity->GetEntityType())
		{
			case Entity::TypeCharacter:
				sprintf(what, "Mob [%s] \"`(%s`)\"",
						((CharData *)pThread->m_pEntity)->GetVirtualID().Print().c_str(),
						((CharData *)pThread->m_pEntity)->GetName());
				break;
			case Entity::TypeObject:
				sprintf(what, "Obj [%s] \"`(%s`)\"",
						((ObjData *)pThread->m_pEntity)->GetVirtualID().Print().c_str(),
						((ObjData *)pThread->m_pEntity)->GetName());
				break;
			case Entity::TypeRoom:
				sprintf(what, "Wld [Room %s]",
						((RoomData *)pThread->m_pEntity)->GetVirtualID().Print().c_str());
				break;
			default:
				sprintf(what, "Unknown (Ptr %p)", pThread->m_pEntity);
				break;
		}
		
		mudlogf(NRM, 101, toFile, "SCRIPTERR: %s: `(%s`)", what, message);

		FOREACH(TrigThread::Callstack, pThread->m_Callstack, iter)
		{
			*what = '\0';
			if (*(*iter)->m_FunctionName)	sprintf(what, "\t%s", (*iter)->m_FunctionName);
			mudlogf(NRM, 101, toFile, "SCRIPTERR: ... %s, line %d%s", (*iter)->GetVirtualID().Print().c_str(), (*iter)->m_Line, what);
		}
		
		
		++pThread->m_NumReportedErrors;
	}
	else
	{
		mudlogf(NRM, 101, toFile, "SCRIPTERR: %s", message);
	}
}




class ScriptTimerCommand : public TimerCommand
{
public:
						ScriptTimerCommand(TrigThread *thread, int time);
	virtual				~ScriptTimerCommand();
						
	virtual int			Run();							//	Return 0 if ran, time if should run again
	virtual bool		Abort(CharData *ch, const char *str);//	Return false if failed to abort
	virtual void		Release(CharData *ch);
	virtual void		Dispose();
	
	void				AddCharacter(CharData *ch);
	
	TrigThread *		m_pThread;
	typedef Lexi::List<CharData *> CharacterList;
	CharacterList		m_characters;
	
	static const char * Type;
};

const char * ScriptTimerCommand::Type = "ScriptTimerCommand";


ScriptTimerCommand::ScriptTimerCommand(TrigThread *thread, int time) :
	TimerCommand(time, ScriptTimerCommand::Type),
	m_pThread(thread)
{
}


ScriptTimerCommand::~ScriptTimerCommand()
{
	//	If we have a thread (haven't run), it has a delay that is us, and it's trigger isn't purged
	//	(sanity check!), then the thread must die, for safety reasons.
	if (m_pThread && (this == m_pThread->m_pDelay) && !PURGED(m_pThread->m_pTrigger))
	{
		m_pThread->m_pDelay = NULL;
		delete m_pThread;
	}
}


void ScriptTimerCommand::AddCharacter(CharData *ch)
{
	if (ch)
	{
		++m_pRefCount;
		m_characters.push_front(ch);
			
		ch->m_pTimerCommand = this;
	}
}


void ScriptTimerCommand::Release(CharData *ch)
{
	if (ch)
	{
		ch->m_ScriptPrompt.clear();
		m_characters.remove(ch);
	}
	TimerCommand::Release(ch);
}



int ScriptTimerCommand::Run()
{	
	m_pThread->m_pDelay = NULL;
	
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	FOREACH(CharacterList, m_characters, iter)
	{
		(*iter)->ExtractTimer();

		sprintf_cat(buf, "%s ", ScriptEngineGetUIDString(*iter));
	}
	GET_THREAD_VARS(m_pThread).Add("aborted", "0")
							  .Add("delayed", buf);

	m_bRunning = true;
	ScriptEngineRunScript(m_pThread);
	m_bRunning = false;

	return 0;
}


bool ScriptTimerCommand::Abort(CharData *ch, const char *str)
{
	if (!m_bRunning)
	{
		BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
		m_pThread->m_pDelay = NULL;

		buf[0] = 0;
		FOREACH(CharacterList, m_characters, iter)
		{
			(*iter)->ExtractTimer();
			
			sprintf_cat(buf, "%s ", ScriptEngineGetUIDString(*iter));
		}
		
		GET_THREAD_VARS(m_pThread).Add("aborted", ch)
								  .Add("delayed", buf)
								  .Add("timeremaining", (int)m_pEvent->GetTimeRemaining())
								  .Add("arg", str ? str : "");
		
		m_bRunning = true;
		int result = ScriptEngineRunScript(m_pThread);
		m_bRunning = false;
		
		if (PURGED(ch) || ch->m_pTimerCommand || !result)
		{
			return false;	//	Failed to abort
		}
	}
	
	return true;		//	Successfully aborted (already running == success)
}


void ScriptTimerCommand::Dispose()
{
	if (!IsRunning())
	{
		m_pThread = NULL;
		++m_pRefCount;
		FOREACH(CharacterList, m_characters, iter)
		{
			(*iter)->ExtractTimer();
		}
		if (--m_pRefCount == 0)
			delete this;
	}
}


/* processes any 'wait' commands in a trigger */
void ScriptEngineRunScriptProcessDelay(Scriptable *sc, TrigThread *thread, const char *arg) {
	int time = 1;
	char c = '\0';
	CharData *ch;
	std::list<CharData *>	chars;

	extern struct TimeInfoData time_info;
	extern UInt32 pulse;

	skip_spaces(arg);

	if (!*arg)
		ScriptEngineReportError(thread, false, "delay w/o an arg");
	else
	{
		BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
		
		while (IsIDString(arg))
		{
			arg = ScriptEngineGetArgument(arg, buf);
			ch = CharData::Find(ParseIDString(buf));
			if (!ch)
				ScriptEngineReportError(thread, false, "delay: invalid character '%s'", buf);
			else if (!Lexi::IsInContainer(chars, ch))
				chars.push_back(ch);
		}
		
		if (chars.empty())
		{
			ScriptEngineReportError(thread, false, "delay: no valid characters");
		}
		
		
		arg = ScriptEngineGetArgument(arg, buf);

		if (sscanf(buf, "%d %c", &time, &c) == 2) {
			if (c == 't')		time *= SECS_PER_MUD_HOUR * PASSES_PER_SEC;	//	't': ticks
//			else if (c == 's')	time *= PASSES_PER_SEC;
			else if (c == 'p')	;											//	'p': pulses
			else 				time *= PASSES_PER_SEC;						//	Default: seconds
		} else					time *= PASSES_PER_SEC;
		
		if (time < 1)	ScriptEngineReportError(thread, false, "delay with negative time: 'delay %s'", arg);

		if (!chars.empty())
		{
			thread->m_pDelay = new ScriptTimerCommand(thread, MAX(time, 1));
		
			FOREACH(std::list<CharData *>, chars, iter)
			{
				ch = *iter;
				
				ch->AbortTimer();
				if (ch->m_pTimerCommand)
					ch->ExtractTimer();
				
				thread->m_pDelay->AddCharacter(ch);
			
				if (*arg)
					ch->m_ScriptPrompt = arg;
			}
		}
		else
		{
			thread->m_pWaitEvent = new ScriptEngineRunScriptResumeEvent(MAX(time, 1), thread);
		}
	}
}



CompiledScript::CompiledScript(ScriptPrototype *prototype)
:	m_Bytecode(NULL)
,	m_BytecodeCount(0)
,	m_Prototype(prototype)
,	m_Table(NULL)
,	m_TableSize(0)
,	m_TableCount(0)
{
}


CompiledScript::~CompiledScript()
{
	delete [] m_Bytecode;
	delete [] m_Table;
}


struct FindFunctionCompareFuncHelper
{
	FindFunctionCompareFuncHelper(const char *n, Hash h) : name(n), hash(h), found(NULL) {}
	
	const char *name;
	Hash hash;
	
	mutable CompiledScript::Function *found;
};


static bool FindFunctionCompareFunc(CompiledScript::Function &lhs, const FindFunctionCompareFuncHelper &rhs)
{
	//	False if (helper) comes before (i)
	Hash lhsHash = lhs.hash,
		 rhsHash = rhs.hash;
	
	if (lhsHash != rhsHash)
		return lhsHash < rhsHash;

	int nameCompare = lhs.name.compare(rhs.name);
	
	if (nameCompare == 0)
		rhs.found = &lhs;
	
	return nameCompare < 0;
}


static bool FindFunctionSortFunc(const CompiledScript::Function &lhs, const CompiledScript::Function &rhs)
{
	//	False if (helper) comes before (i)
	Hash lhsHash = lhs.hash,
		 rhsHash = rhs.hash;
	
	if (lhsHash != rhsHash)
		return lhsHash < rhsHash;

	int nameCompare = lhs.name.compare(rhs.name);
	
	if (nameCompare != 0)
		ReportHashCollision(lhsHash, lhs.name.c_str(), rhs.name.c_str());
	
	return nameCompare < 0;
	
}


CompiledScript::Function * CompiledScript::FindFunction(const char *functionName, Hash hash)
{
	if (m_Functions.empty())
		return NULL;
	
	if (hash == 0)
	{
		hash = GetStringFastHash(functionName);
	}

	FindFunctionCompareFuncHelper helper(functionName, hash);
	
	FunctionList::iterator iter = std::lower_bound(m_Functions.begin(), m_Functions.end(), helper, FindFunctionCompareFunc);

	if (helper.found)
	{
		return helper.found;
	}

//	FOREACH(FunctionList, m_Functions, functionIter)
//	{
//		Function *pFunction = &(*functionIter);
//		
//		if (pFunction->hash == hash && pFunction->name == functionName)
//		{
//			return pFunction;
//		}
//	}
	
	return NULL;
}


CompiledScript::Function *CompiledScript::FindFunction(const char *functionName, CompiledScript *&pMatchingScript)
{
	Hash hash = GetStringFastHash(functionName);
	
	Function *pFunction = FindFunction(functionName, hash);
	if (pFunction)
	{
		pMatchingScript = this;
		return pFunction;
	}

	FOREACH(CompiledScript::ImportList, m_Imports, import)
	{
		CompiledScript *pScript = (*import)->m_pProto->m_pCompiledScript;
		pFunction = pScript->FindFunction(functionName, hash);
		
		if (pFunction)
		{
			pMatchingScript = pScript;
			return pFunction;
		}
	}
	
	return NULL;
}


void CompiledScript::SortFunctions()
{
	std::sort(m_Functions.begin(), m_Functions.end(), FindFunctionSortFunc);
}


CallstackEntry::CallstackEntry(CompiledScript *pCompiledScript,
		bytecode_t *bytecodeStart,
		const char *pFunctionName,
		CallType type)
:	m_pCompiledScript(pCompiledScript)
,	m_BytecodePosition(bytecodeStart)
,	m_Type(type)
,	m_FunctionName(pFunctionName)
,	m_Line(0)
{
}


CallstackEntry::~CallstackEntry()
{
}


VirtualID CallstackEntry::GetVirtualID() const
{
	return m_pCompiledScript->m_Prototype->GetVirtualID();
}


int TrigThread::ms_TotalThreads = 0;


TrigThread::TrigThread(TrigData *pTrigger, Entity *pEntity, eCreateRoot createRoot)
:	m_pTrigger(pTrigger)
,	m_pEntity(pEntity)
,	m_ID(++pEntity->GetScript()->m_LastThreadID)
,	m_pWaitEvent(NULL)
,	m_pDelay(NULL)
,	m_pCurrentCallstack(NULL)
,	m_TimeUsed(0.0)
,	m_bDebugPaused(false)
,	m_bHasBreakpoints(false)
,	m_NumReportedErrors(0)
{
	m_pTrigger->m_Threads.push_front(this);
	if (createRoot == CreateRoot)
		PushCallstack(m_pTrigger->m_pCompiledScript, m_pTrigger->m_pCompiledScript->m_Bytecode + 1, /*"<root>"*/ "");
	++ms_TotalThreads;
}


TrigThread::~TrigThread()
{
	if (m_pTrigger)
		m_pTrigger->m_Threads.remove(this);
	
	FOREACH(Callstack, m_Callstack, entry)
		delete *entry;

	if (m_pWaitEvent)
		m_pWaitEvent->Cancel();
	m_pWaitEvent = NULL;
	
	if (m_pDelay)
	{
		ScriptTimerCommand *pDelay = m_pDelay;
		m_pDelay = NULL;
		pDelay->Dispose();
	}

	--ms_TotalThreads;
}


VirtualID TrigThread::GetCurrentVirtualID()
{
	return GetCurrentCompiledScript()->m_Prototype->GetVirtualID();
}


TrigData *TrigThread::GetCurrentScript()
{
	return GetCurrentCompiledScript()->m_Prototype->m_pProto;
}


bool TrigThread::PushCallstack(CompiledScript *pCompiledScript, bytecode_t *bytecodePosition, const char *pFunctionName, CallstackEntry::CallType type)
{
	if (m_Callstack.size() >= 32)
	{
		ScriptEngineReportError(this, true, "Maximum function recursion exceed (32 recursions max)!");
		return false;
	}
	
	m_pCurrentCallstack = new CallstackEntry(pCompiledScript, bytecodePosition, pFunctionName, type);
	m_Callstack.push_back( m_pCurrentCallstack );
	
	return true;
}


bool TrigThread::PopCallstack()
{
	CallstackEntry::CallType callType;
	
	//assert(!m_Callstack.empty() && m_Callstack.back() == m_pCurrentCallstack);
	m_Callstack.pop_back();
	callType = m_pCurrentCallstack->m_Type;
	delete m_pCurrentCallstack;

	m_pCurrentCallstack = NULL;
	if (!m_Callstack.empty())
	{
		//	Copy m_pResult to %result% in m_pCallstackEntry
		m_pCurrentCallstack = m_Callstack.back();
		if (callType == CallstackEntry::Call_Embedded)
			m_Stack.Push(m_pResult.c_str());
		else
			GetCurrentVariables().Add("result", m_pResult.c_str());
	}
	m_pResult.clear();
	
	return m_pCurrentCallstack == NULL;
}


void TrigThread::Terminate()
{
	m_ID = 0;
	
	if (m_pWaitEvent || m_pDelay)
	{
		delete this;
	}
}



ThreadStack::ThreadStack() :
	m_pStack(NULL),
	m_StackBufferSize(/*StackGrowSize*/ 0),
	m_StackBufferUsed(0),
	m_Count(0)
{
}


ThreadStack::~ThreadStack()
{
	delete [] m_pStack;
}


ThreadStack::StackEntry *ThreadStack::Push(const void *data, unsigned int dataSize, StackEntryType type)
{
	int stackEntrySize = sizeof(StackEntry) + dataSize - sizeof(StackEntry().data);	//	- 4 for StackEntry.data

	//	Minimum size of 8 bytes
	stackEntrySize = MAX(stackEntrySize, (int)sizeof(StackEntry));

	//	Align size to a multiple of 4; this will force alignment of the following entry!
	stackEntrySize = Align(stackEntrySize, 4);
	
	m_StackBufferUsed += stackEntrySize;
	
	if (m_StackBufferUsed > m_StackBufferSize)
	{
		int oldSize = m_StackBufferSize;
		m_StackBufferSize = Align(m_StackBufferUsed, StackGrowSize);
		
		//	We can't just realloc, because realloc copies from the beginning
		//	Since our stacks grow downwards, all our data is at the end of the buffer
		char *	pNewStack = new char[m_StackBufferSize];
		memcpy(pNewStack + (m_StackBufferSize - oldSize), m_pStack, oldSize);
		delete [] m_pStack;
		m_pStack = pNewStack;
	}
	
	StackEntry *pCurrentEntry = (StackEntry *)(m_pStack + m_StackBufferSize - m_StackBufferUsed);
	pCurrentEntry->size = stackEntrySize;
	pCurrentEntry->type = type;

	if (data)	memcpy(pCurrentEntry->data, data, dataSize);
	
	++m_Count;
	
	return pCurrentEntry;
}


void ThreadStack::Push(const char *str)
{
	int length = MIN(strlen(str) + 1, (unsigned)MAX_SCRIPT_STRING_LENGTH);
	StackEntry *entry = Push(str, length, TYPE_STRING);
	char *p = (char *)entry->data;
	p[length - 1] = '\0';
}


void ThreadStack::PushCharPtr(const char *str)
{
	StackEntry *entry = Push(&str, sizeof(const char *), TYPE_CHAR_PTR);
}


//	DONT ENABLE THIS - because of the way script variables work, returning the Value() then popping it off the stack
//	could return a pointer to a now invalid string, IF that was the last reference to the var.
//	To work around this popping off the stack needs something more reliable, like returning a container object
//void ThreadStack::PushScriptVariable(ScriptVariable *var)
//{
//	StackEntry *entry = Push(NULL, sizeof(ScriptVariable), TYPE_VARIABLE);
//	new ((ScriptVariable *)&entry->data) ScriptVariable(*var);
//}


char *ThreadStack::Pop()
{
	if (m_Count == 0)
		return "";	//	shouldn't happen, but in case it does...
	else
	{
		StackEntry *pCurrentEntry = (StackEntry *)(m_pStack + m_StackBufferSize - m_StackBufferUsed);
		char *result = "";
		
		switch (pCurrentEntry->type)
		{
			case TYPE_STRING:	result = (char *)pCurrentEntry->data;	break;
			case TYPE_CHAR_PTR:	result = *(char **)pCurrentEntry->data;	break;
			default:	assert(0);
		}
		
		m_StackBufferUsed -= pCurrentEntry->size;

		--m_Count;
		
		return result;
	}
	//	No shrinking the stack, we won't worry about THAT memory usage
}


char *ThreadStack::Peek()
{
	if (m_Count == 0)		return "";	//	shouldn't happen, but in case it does...
	else
	{
		StackEntry *pCurrentEntry = (StackEntry *)(m_pStack + m_StackBufferSize - m_StackBufferUsed);

		switch (pCurrentEntry->type)
		{
			case TYPE_STRING:	return (char *)pCurrentEntry->data;
			case TYPE_CHAR_PTR:	return *(char **)pCurrentEntry->data;
		}
		
		return "";
	}
}

