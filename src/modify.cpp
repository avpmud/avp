/* ************************************************************************
*   File: modify.c                                      Part of CircleMUD *
*  Usage: Run-time modification of game variables                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "interpreter.h"
#include "find.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "olc.h"
#include "lexifile.h"

/*
 * Action modes for parse_action().
 */
enum ParseCommand
{
	PARSE_FORMAT,
	PARSE_REPLACE,
	PARSE_HELP,
	PARSE_DELETE,
	PARSE_INSERT,
	PARSE_LIST_NORM,
	PARSE_LIST_UPLOAD,
	PARSE_LIST_NUM,
	PARSE_EDIT
};


enum
{
	FORMAT_INDENT		= 1<<0
};

enum
{
	EDIT_CONTINUE		= 0,
	EDIT_SAVE			= 1,
	EDIT_ABORT			= 2
};


#define UPLOAD_BEGIN "*** BEGIN\n"
#define UPLOAD_END "*** END\n"

void parse_action(ParseCommand command, char *string, DescriptorData *d);
ACMD(do_skillset);
extern char *MENU;
int		replace_str(Lexi::String &str, size_t max_size, const char *pattern, const char *replacement, bool bReplaceAll);
void	format_text(Lexi::String &str, size_t max_size, int mode);

char *string_fields[] =
{
  "name",
  "short",
  "long",
  "description",
  "title",
  "delete-description",
  "\n"
};


/*
 * Maximum length for text field x+1
 */
int length[] =
{
  15,
  60,
  256,
  240,
  60
};


/*************************************
 * Modification of malloc'ed strings.*
 *************************************/
void parse_action(ParseCommand command, char *string, DescriptorData *d)
{
	Flags flags = 0;
	bool bIndent = false;
	int total_len, replaced;
	int j = 0;
	int i, line_low, line_high;
	char *s, *t /*, *buf, *buf2 */;
	Lexi::String::size_type startPos, endPos;
	
	BUFFER(buf, 64 * 1024);
	
	DescriptorData::Pager::Flags	outputFlags;
			
	switch (command) { 
		case PARSE_HELP:
			d->Write("Editor command formats: /<letter>\n\n"
					"/a         -  aborts editor\n"
					"/c         -  clears buffer\n"
					"/d#        -  deletes a line #\n"
					"/e# <text> -  changes the line at # with <text>\n"
					"/f         -  formats text\n"
					"/fi        -  indented formatting of text\n"
					"/h         -  list text editor commands\n"
					"/i# <text> -  inserts <text> before line #\n"
					"/l         -  lists buffer\n"
					"/u         -  lists buffer for upload to client\n"
					"/n         -  lists buffer with line numbers\n"
					"/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\n"
					"/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\n"
					"              usage: /r[a] 'pattern' 'replacement'\n"
					"/s         -  saves text\n"
					"Additionally, /lc, /uc, and /nc will output 'raw' color codes.");
			break;
		case PARSE_FORMAT: 
			flags = 0;
			while (isalpha(*string))
			{
				switch (tolower(*string))
				{
					case 'i':
						bIndent = true;
						SET_BIT(flags, FORMAT_INDENT);
						break;
				}     
				++string;
			}
			
			format_text(*d->m_StringEditor, d->m_StringEditor->max(), flags);
			sprintf(buf, "Text formatted with%s indent.\n", (bIndent ? "" : "out")); 
			d->Write(buf);
			break;
		case PARSE_REPLACE: 
			{
				bool bReplaceAll = false;

				//	Process for /ra
				while (isalpha(*string))
				{
					switch (tolower(*string))
					{
						case 'a':
							bReplaceAll = true;
							break;
					}     
					++string;
				}
				
				//	Find separator
				while (*string && *string != '"' && *string != '\'')
					++string;
				
				const char *separator = NULL;
				switch (*string)
				{
					case '"':	separator = "\"";	break;
					case '\'':	separator = "'";	break;
				}
				++string;	//	Advance past separator
				
				if (!separator || (s = strsep(&string, separator)) == NULL)
				{
					d->Write("Invalid format.\n");
					return;
				}
				else if (!string)
				{
					d->Write("Target string must be enclosed in matching single or double quotes.\n");
					return;
				}
				
				//	Find 2nd separator
				while (*string && *string != '"' && *string != '\'')
					++string;
				
				separator = NULL;
				switch (*string)
				{
					case '"':	separator = "\"";	break;
					case '\'':	separator = "'";	break;
				}
				++string;
				
				if (!separator || (t = strsep(&string, separator)) == NULL)
				{
					d->Write("No replacement string.\n");
					return;
				}
				else if (!string)
				{
					d->Write("Replacement string must be enclosed in matching single or double quotes.\n");
					return;
				}
				
				total_len = ((strlen(t) - strlen(s)) + d->m_StringEditor->length());
				
				if (total_len > d->m_StringEditor->max())
				{
					d->Write("Not enough space left in buffer.\n");
					return;
				}

				replaced = replace_str(*d->m_StringEditor, d->m_StringEditor->max(), s, t, bReplaceAll);
				if (replaced > 0)
				{
					sprintf(buf, "Replaced %d occurance%sof '%s' with '%s'.\n", replaced, ((replaced != 1)?"s ":" "), s, t); 
					d->Write(buf);
				} 
				else if (replaced == 0)
				{
					sprintf(buf, "String '%s' not found.\n", s); 
					d->Write(buf);
				}
				else
				{
					d->Write("ERROR: Replacement string causes buffer overflow, aborted replace.\n");
				}
			}
			break;
		case PARSE_DELETE:
			switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
				case 0:
					d->Write("You must specify a line number or range to delete.\n");
					return;
				case 1:
					line_high = line_low;
					break;
				case 2:
					if (line_high < line_low) {
						d->Write("That range is invalid.\n");
						return;
					}
					break;
			}
      
			if (d->m_StringEditor->empty())
			{
				d->Write("Buffer is empty.\n");
				return;
			}
			else if (line_low > 0)
			{
				i = 1;
			
				startPos = 0;
				while (i < line_low
					&& startPos != Lexi::String::npos)
				{
					startPos = d->m_StringEditor->find('\n', startPos);
					if (startPos != Lexi::String::npos)
					{
						++i;		//	1 line found
						++startPos;	//	skip past the newline
					}
				}
				
				if ((i < line_low) || (startPos == Lexi::String::npos))
				{
					d->Write("Line(s) out of range; not deleting.\n");
					return;
				}
				
				endPos = startPos;
				
				total_len = 0;
				while (i <= line_high
					&& endPos != Lexi::String::npos)
				{
					endPos = d->m_StringEditor->find('\n', endPos);
					if (endPos != Lexi::String::npos)
					{
						++i;	//	1 line found
						++endPos;//	skip past the newline to the next line
						++total_len;//	total lines found count
					}
				}
				
				if (endPos != Lexi::String::npos)	d->m_StringEditor->erase(startPos, endPos - startPos);
				else								d->m_StringEditor->erase(startPos);
				
				sprintf(buf, "%d line%sdeleted.\n", total_len, ((total_len != 1)?"s ":" "));
				d->Write(buf);
			} else {
				d->Write("Invalid line numbers to delete must be higher than 0.\n");
				return;
			}
			break;
		case PARSE_LIST_NORM:
			while (isalpha(*string))
			{
				switch (tolower(*string))
				{
					case 'c':
						outputFlags.set(PAGER_RAW_OUTPUT);
						break;
				}     
				++string;
			}
			
			*buf = '\0';
			if (*string != '\0')
				switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
					case 0:
						line_low = 1;
						line_high = 999999;
						break;
					case 1:
						line_high = line_low;
						break;
				} else {
					line_low = 1;
					line_high = 999999;
				}
  
			if (line_low < 1) {
				d->Write("Line numbers must be greater than 0.\n");
				return;
			}
			if (line_high < line_low) {
				d->Write("That range is invalid.\n");
				return;
			}
			
			*buf = '\0';
			
			if ((line_high < 999999) || (line_low > 1))
				sprintf(buf, "Current buffer range [%d - %d]:\n", line_low, line_high);
			
			i = 1;
			total_len = 0;

			startPos = 0;
			while (i < line_low
				&& startPos != Lexi::String::npos)
			{
				startPos = d->m_StringEditor->find('\n', startPos);
				if (startPos != Lexi::String::npos)
				{
					++i;		//	1 line found
					++startPos;	//	skip past the newline
				}
			}
			
			if ((i < line_low) || (startPos == Lexi::String::npos))
			{
				d->Write("Line(s) out of range; no buffer listing.\n");
				return;
			}
			
			endPos = startPos;
			while (i <= line_high
				&& endPos != Lexi::String::npos)
			{
				endPos = d->m_StringEditor->find('\n', endPos);
				if (endPos != Lexi::String::npos)
				{
					++i;	//	1 line found
					++endPos;//	skip past the newline to the next line
					++total_len;//	total lines found count
				}
			}

			if (endPos != Lexi::String::npos)	strncat(buf, d->m_StringEditor->c_str() + startPos, endPos - startPos);
			else								strcat(buf, d->m_StringEditor->c_str() + startPos);
			
			/*
			 * This is kind of annoying...but some people like it.
			 */
#if 0
			sprintf_cat(buf, "\n%d line%sshown.\n", total_len, ((total_len != 1)?"s ":" ")); 
#endif
			d->PageString(buf, outputFlags);
			break;
		case PARSE_LIST_UPLOAD:
			while (isalpha(*string))
			{
				switch (tolower(*string))
				{
					case 'c':
						outputFlags.set(PAGER_RAW_OUTPUT);
						break;
				}     
				++string;
			}
 			//*buf = '\0';
			//strcat(buf, UPLOAD_BEGIN);
			//strcat(buf, *d->str);
			//strcat(buf, UPLOAD_END);
			//d->Write(buf, d);
			if (outputFlags.test(PAGER_RAW_OUTPUT))	d->Write("`[");	//	RAW
			d->Write(d->m_StringEditor->c_str());
			if (outputFlags.test(PAGER_RAW_OUTPUT))	d->Write("`]");	//	RAW
			break;
		case PARSE_LIST_NUM:
			while (isalpha(*string))
			{
				switch (tolower(*string))
				{
					case 'c':
						outputFlags.set(PAGER_RAW_OUTPUT);
						break;
				}     
				++string;
			}
			
			if (*string != '\0')
			{
				switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
					case 0:
						line_low = 1;
						line_high = 999999;
						break;
					case 1:
						line_high = line_low;
						break;
				}
			}
			else
			{
				line_low = 1;
				line_high = 999999;
			}
    
			if (line_low < 1) {
				d->Write("Line numbers must be greater than 0.\n");
				return;
			}
			if (line_high < line_low) {
				d->Write("That range is invalid.\n");
				return;
			}
			
			*buf = '\0';
			
			i = 1;
			total_len = 0;

			startPos = 0;
			while (i < line_low
				&& startPos != Lexi::String::npos)
			{
				startPos = d->m_StringEditor->find('\n', startPos);
				if (startPos != Lexi::String::npos)
				{
					++i;		//	1 line found
					++startPos;	//	skip past the newline
				}
			}
			
			if ((i < line_low) || (startPos == Lexi::String::npos))
			{
				d->Write("Line(s) out of range; no buffer listing.\n");
				return;
			}
			
			endPos = startPos;
			while (i <= line_high
				&& endPos != Lexi::String::npos
				&& startPos < d->m_StringEditor->size())
			{
				endPos = d->m_StringEditor->find('\n', startPos);
				
				++i;	//	1 line found
				++total_len;//	total lines found count
				
				sprintf_cat(buf, "%4d: ", (i-1));
				if (endPos != Lexi::String::npos)
				{
					++endPos;//	skip past the newline to the next line
					strncat(buf, d->m_StringEditor->c_str() + startPos, endPos - startPos);
				}
				else
				{
					strcat(buf, d->m_StringEditor->c_str() + startPos);
				}
				startPos = endPos;
			}
			
			/*
			 * This is kind of annoying .. seeing as the lines are numbered.
			 */
#if 0
			sprintf_cat(buf, "\n%d numbered line%slisted.\n", total_len, ((total_len != 1)?"s ":" ")); 
#endif
			d->PageString(buf, outputFlags);
			break;

		case PARSE_INSERT:
		{
			BUFFER(arg, MAX_INPUT_LENGTH);
			
			skip_spaces(string);
			
			if (*string == '\0') {
				d->Write("You must specify a line number before which to insert text.\n");
				return;
			}
			line_low = strtol(string, &string, 10);
			
			if (*string == '/')	++string;
			else				skip_spaces(string);

			sprintf(arg, "%s\n", string);
			
			if (d->m_StringEditor->empty())
				d->Write("Buffer is empty, nowhere to insert.\n");
			else if ((d->m_StringEditor->length() + strlen(arg) + 1 /*3*/) > d->m_StringEditor->max())
				d->Write("Insert text pushes buffer over maximum size, insert aborted.\n");
			else if (line_low > 0)
			{
				i = 1;
				startPos = 0;
				while (i < line_low
					&& startPos != Lexi::String::npos)
				{
					startPos = d->m_StringEditor->find('\n', startPos);
					if (startPos != Lexi::String::npos)
					{
						++i;		//	1 line found
						++startPos;	//	skip past the newline
					}
				}
				
				if ((i < line_low) || (startPos == Lexi::String::npos))
				{
					d->Write("Line number out of range; insert aborted.\n");
					return;
				}
				
				d->m_StringEditor->insert(startPos, arg);
				d->Write("Line inserted.\n");
			}
			else
				d->Write("Line number must be higher than 0.\n");
			break;
		}

		case PARSE_EDIT:
		{
			BUFFER(arg, MAX_INPUT_LENGTH);
			
			skip_spaces(string);
			
			if (!isdigit(*string) == '\0') {
				d->Write("You must specify a line number at which to change text.\n");
				return;
			}
			line_low = strtol(string, &string, 10);
			
			if (*string == '/')	++string;
			else				skip_spaces(string);

			sprintf(arg, "%s\n", string);

			if (d->m_StringEditor->empty())
				d->Write("Buffer is empty, nothing to change.\n");
			else if (line_low > 0)
			{
				i = 1;
				startPos = 0;
				while (i < line_low
					&& startPos != Lexi::String::npos)
				{
					startPos = d->m_StringEditor->find('\n', startPos);
					if (startPos != Lexi::String::npos)
					{
						++i;		//	1 line found
						++startPos;	//	skip past the newline
					}
				}
				
				/*
				 * Make sure that there was a THAT line in the text.
				 */
				if ((i < line_low) || (startPos == Lexi::String::npos))
				{
					d->Write("Line number out of range; change aborted.\n");
					return;
				}
				
				endPos = d->m_StringEditor->find('\n', startPos);
				
				if (endPos != Lexi::String::npos)	++endPos;
				else								endPos = d->m_StringEditor->size();

				Lexi::String::size_type replaceLength = endPos - startPos;
				
				if (((d->m_StringEditor->size() - replaceLength) + strlen(arg) + 1) > d->m_StringEditor->max())
				{
					d->Write("Change causes new length to exceed buffer maximum size, aborted.\n");
					return;
				}

				d->m_StringEditor->replace(startPos, replaceLength, arg);
			}
			else
				d->Write("Line number must be higher than 0.\n");
			break;
		}
		default:
			d->Write("Invalid option.\n");
			mudlog("SYSERR: invalid command passed to parse_action", BRF, LVL_CODER, TRUE);
			break;
	}
}

/* ************************************************************************
*  modification of malloc'ed strings                                      *
************************************************************************ */

/*
 * Add user input to the 'current' string (as defined by d->str).
 */
void DescriptorData::StringEditor::Parse(DescriptorData *d, char *str)
{
	delete_doubledollar(str);
	
	if (*str == '/')
	{
		int terminator = EDIT_CONTINUE;
		
		++str;
		switch (*str++)
		{
			case 'a':
				terminator = EDIT_ABORT; /* working on an abort message */
				break;
			case 'c':
				if (!/*d->m_StringEditor->*/empty())
				{
					/*d->m_StringEditor->*/clear();
					d->Write("Current buffer cleared.\n");
				} else
					d->Write("Current buffer empty.\n");
				break;
			case 'd':
				parse_action(PARSE_DELETE, str, d);
				break;
			case 'e':
				parse_action(PARSE_EDIT, str, d);
				break;
			case 'f': 
				if (!/*d->m_StringEditor->*/empty())	parse_action(PARSE_FORMAT, str, d);
				else								d->Write("Current buffer empty.\n");
				break;
			case 'i':
				if (!/*d->m_StringEditor->*/empty())	parse_action(PARSE_INSERT, str, d);
				else								d->Write("Current buffer empty.\n");
				break;
			case 'h': 
				parse_action(PARSE_HELP, str, d);
				break;
			case 'l':
				if (!/*d->m_StringEditor->*/empty())	parse_action(PARSE_LIST_NORM, str, d);
				else								d->Write("Current buffer empty.\n");
				break;
			case 'u':
				if (!/*d->m_StringEditor->*/empty())	parse_action(PARSE_LIST_UPLOAD, str, d);
				else								d->Write(UPLOAD_BEGIN UPLOAD_END);
				break;
			case 'n':
				if (!/*d->m_StringEditor->*/empty())	parse_action(PARSE_LIST_NUM, str, d);
				else								d->Write("Current buffer empty.\n");
				break;
			case 'r':   
				if (!/*d->m_StringEditor->*/empty())	parse_action(PARSE_REPLACE, str, d);
				else								d->Write("Current buffer empty.\n");
				break;
			case 's':
				terminator = EDIT_SAVE;
				break;
			default:
				d->Write("Invalid option.\n");
				break;
		}
		
		if (terminator != EDIT_CONTINUE) {
			//	OLC Edits
			if (terminator == EDIT_SAVE)
			{
				/*d->m_StringEditor->*/save();
				d->GetState()->OnEditorSave();
			}
			else
			{
				d->GetState()->OnEditorAbort();
			}
			
			d->GetState()->OnEditorFinished();

//			if (d->GetState()->IsPlaying() && d->m_Character && !IS_NPC(d->m_Character))
//			REMOVE_BIT(PLR_FLAGS(d->m_Character), PLR_WRITING | PLR_MAILING);
			
			d->m_StringEditor = NULL;	//	This destroys us!
			return;	//	We just destroyed ourselves...
		}
	}
	else
	{
		int maxAllowed = /*d->m_StringEditor->*/m_Max - 3;
		int curLength = /*d->m_StringEditor->*/length();
		
		if (curLength >= maxAllowed)
		{
			d->Write("String too long - cannot add.\n");
			return;
		}
		else if ((strlen(str) + curLength) > maxAllowed)
		{
			d->Write("String too long - truncated.\n");
			/*d->m_StringEditor->*/append(str, maxAllowed - curLength);
		}
		else
		{
			/*d->m_StringEditor->*/append(str);
		}
		
		/*d->m_StringEditor->*/append("\n");
	}
}



/* **********************************************************************
*  Modification of character skills                                     *
********************************************************************** */
ACMD(do_skillset) {
	CharData *vict;
	BUFFER(name, 128);
	int skill, value, i, qend;
	
	argument = one_argument(argument, name);

	if (!*name)			/* no arguments. print an informative text */
	{
		BUFFER(help, MAX_STRING_LENGTH);
		send_to_char("Syntax: skillset <name> '<skill>' <value>\n", ch);
		strcpy(help, "Skill being one of the following:\n");
		for (i = 1; i < NUM_SKILLS; ++i) {
			if (*skill_info[i].name == '!')
				continue;
			sprintf_cat(help, "%18s", skill_info[i].name);
			if (i % 4 == 3) {
				strcat(help, "\n");
				send_to_char(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			send_to_char(help, ch);
		send_to_char("\n", ch);
		return;
	}
	if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		send_to_char(NOPERSON, ch);
		return;
	}
	
	skip_spaces(argument);

	/* If there is no chars in argument */
	if (!*argument) {
		send_to_char("Skill name expected.\n", ch);
		return;
	}
	if (*argument != '\'') {
		send_to_char("Skill must be enclosed in: ''\n", ch);
		return;
	}
	/* Locate the last quote && lowercase the magic words (if any) */

	for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
		*(argument + qend) = tolower(*(argument + qend));

	if (*(argument + qend) != '\'') {
		send_to_char("Skill must be enclosed in: ''\n", ch);
		return;
	}
	BUFFER(help, MAX_STRING_LENGTH);
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';
	if ((skill = find_skill_abbrev(help)) == 0) {
		send_to_char("Unrecognized skill.\n", ch);
		return;
	}

	argument += qend + 1;		/* skip to next parameter */
	BUFFER(buf, MAX_INPUT_LENGTH);
	argument = one_argument(argument, buf);

	if (!*buf) {
		send_to_char("Learned value expected.\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0) {
		send_to_char("Minimum value for learned is 0.\n", ch);
		return;
	}
	if (value > MAX_SKILL_LEVEL) {
		ch->Send("Max value for learned is %d.\n", MAX_SKILL_LEVEL);
		return;
	}
	if (IS_NPC(vict)) {
		send_to_char("You can't set NPC skills.\n", ch);
		return;
	}
	mudlogf(BRF, LVL_STAFF, TRUE, "%s changed %s's %s to %d.", ch->GetName(), vict->GetName(), skill_info[skill].name, value);

	GET_REAL_SKILL(vict, skill) = value;
	vict->AffectTotal();
	
	sprintf(buf, "You change %s's %s to %d.\n", vict->GetName(), skill_info[skill].name, value);
	send_to_char(buf, ch);
}


/*********************************************************************
* New Pagination Code
* Michael Buselli submitted the following code for an enhanced pager
* for CircleMUD.  All functions below are his.  --JE 8 Mar 96
*********************************************************************/

#define PAGE_WIDTH      /*80*/ 79

/*
 * Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
const char *DescriptorData::Pager::FindNextPage(const char *str)
{
	int		col = 1,
			line = 1;
	bool	spec_code = false;
	bool	inLine = false;
	
	if (m_PageLength == 0)
		return NULL;
	
	for (;; str++) {
		//	If end of string, return NULL
		if (*str == '\0')				return NULL;
		//	If we're at the start of the next page, return this fact.
		else if (line > m_PageLength && !inLine)	return str;
		//	Check for the begining of an ANSI color code block.
/*		else if (*str == '\x1B' && !spec_code)
			spec_code = true;
		//	Check for the end of an ANSI color code block.
		else if (*str == 'm' && spec_code)
			spec_code = false;
*/		//	Check for everything else.
		else if (!spec_code)
		{
			//	Handle internal color coding :-)
			inLine = false;
/*			if (*str == '`') {
				++str;
				if (!*str)		--str;
//				else			++str;
			}
			//	Carriage return puts us in column one.
			else*/
			char c = *str;
			
			if (!m_Flags.test(PAGER_RAW_OUTPUT) && c == '`')
			{
				c = *(++str);
				if (c != '`')
				{
					if (c == '^')	c = *(++str);
					--col;	//	Reduce column by 1 to compensate for adding the width.
				}
			}
			
//			if ((c=='`' && (search_chars(str[1], "nkrgybmcwKRGYBMCWiIfF*`") >= 0)) /* ||
//				(c=='^' && (search_chars(str[1], "krgybmcw^") >= 0))*/) {
//				c = *(++str);
//				
//				if (/*c != '^' &&*/ c != '`')	--col;	//	let it see char but we decrement width
//			}
			
			//	Newline puts us on the next line.
			if (c == '\n') 
			{
				col = 1;
				++line;
			}
			//	We need to check here and see if we are over the page width,
			//	and if so, compensate by going to the begining of the next line.
			else if (col++ > PAGE_WIDTH)
			{
				if (line >= m_PageLength)
					inLine = true;
				else
				{
					++line;
					col = 1;
				}
			}
		}
	}
}


//	The call that gets the paging ball rolling...
void page_string(DescriptorData *d, const char *str)
{
	if (!d)	return;
	
	d->PageString(str);
}


//	The call that gets the paging ball rolling...
void DescriptorData::PageString(const char *str, Pager::Flags flags) {
	if (!str || !*str)
	{
/*		send_to_char("", d->m_Character); - ?? */
		return;
	}
	
	m_Pager = new DescriptorData::Pager(str, this->m_Character ? this->m_Character->GetPlayer()->m_PageLength : DEFAULT_PAGE_LENGTH, flags);
	m_Pager->Parse(this, "");
}


DescriptorData::Pager::Pager(const char *str, int pageLen, Pager::Flags flags)
:	Lexi::String(str)
,	m_PageLength(pageLen)
,	m_PageNumber(0)
,	m_Flags(flags)
{
/*	if (m_Flags.test(PAGER_SHOW_LINENUMS))
	{
		size_type pos = 0;
		int line = 1;
		
		for (;;)
		{
			insert(pos, Lexi::Format("%4d: ", line));
			pos = find('\n', pos);
			
			if (pos == Lexi::String::npos)
				break;
			
			++pos;	//	Skip the newline
			++line;
		}	
	}
*/
	str = c_str();	//	Switch to using the stored string, since we want char *s

	do
	{
		m_Pages.push_back(str);
		str = FindNextPage(str);
	} while (str);	
}


//	The call that displays the next page.
void DescriptorData::Pager::Parse(DescriptorData *d, char *input)
{
	BUFFER(buf, MAX_STRING_LENGTH);

	any_one_arg(input, buf);

	if (tolower(*buf) == 'q')		//	Q = quit
	{
		d->m_Pager = NULL;	//	This destroys us!
		return;	//	We just destroyed ourselves!
	}
	else if (tolower(*buf) == 'r')	m_PageNumber = MAX(0, m_PageNumber - 1);	//	R = refresh, back up one page
	else if (tolower(*buf) == 'b')	m_PageNumber = MAX(0, m_PageNumber - 2);	//	B = back, back up two pages
	else if (isdigit(*buf))			//	Feature to 'goto' a page
		m_PageNumber = RANGE(atoi(buf), 1, (int)m_Pages.size()) - 1;
	else if (*buf)
	{
		send_to_char("Valid commands while paging are RETURN, Q, R, B, or a numeric value.\n", d->m_Character);
		return;
	}
	
	//	If we're displaying the last page, just send it to the character, and
	//	then free up the space we used.
	if (m_PageNumber + 1 >= m_Pages.size())
	{
		OutputPage(d, m_Pages.back(), strlen(m_Pages.back()));
		d->m_Pager = NULL;	//	This destroys us!
		return;	//	We just destroyed ourselves!
	}
	else
	{ /* Or if we have more to show.... */
		size_t diff = m_Pages[m_PageNumber + 1] - m_Pages[m_PageNumber];
		OutputPage(d, m_Pages[m_PageNumber], diff);
		++m_PageNumber;
	}
}


void DescriptorData::Pager::OutputPage(DescriptorData *d, const char *page, size_t size)
{
	send_to_char("`(", d->m_Character);
	if (m_Flags.test(PAGER_RAW_OUTPUT))	send_to_char("`[", d->m_Character);
	d->Write(page, size);
	if (m_Flags.test(PAGER_RAW_OUTPUT))	send_to_char("`]", d->m_Character);
	send_to_char("`)", d->m_Character);
}



/*
//	TODO: This should probably fail gracefully if it can't replace all!
int replace_str(Lexi::String &str, size_t max_size, const char *pattern, const char *replacement, bool bReplaceAll)
{
	int patLength = strlen(pattern);
	int repLength = strlen(replacement);
	int diffLength = repLength - patLength;
	int totalLength = strlen(*string);

	if (totalLength + diffLength > max_size)	//	Cannot possibly fit a replacement
		return -1;

	int count = 0;
	
	Lexi::String::size_type matchPos = 0;
	do
	{
		matchPos = string.find(matchPos, pattern);
		
		if (matchPos == Lexi::String::npos)
			break;
		
		++count;
		
		//	Current Length + new length to copy + replacement diff + remainder to copy
		if (str.length() + diffLength >= max_size)
		{
			//	Cannot replace any more; we will error out assuming we failed
			//	entirely
			count = -1;
			break;
		}
		
		str.replace(matchPos, patLength, replacement);
		matchPos += patLength;
	}
	while ( bReplaceAll );
	
	return count;
}
*/


/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
int replace_str(Lexi::String &string, size_t max_size, const char *pattern, const char *replacement, bool bReplaceAll)
{
	int patLength = strlen(pattern);
	int repLength = strlen(replacement);
	int diffLength = repLength - patLength;
	int totalLength = string.length();

	if (totalLength + diffLength >= max_size)	//	Cannot possibly fit a replacement
		return -1;
	
	BUFFER(destBuf, max_size);
	char *dest = destBuf;

	int count = 0;
	
	const char *src = string.c_str();
	
	int curLength = 0;
	int remLength = totalLength;
	
	do
	{
		const char *nextMatch = str_str(src, pattern);
		
		if (!nextMatch)
			break;
		
		++count;
		
		//	Length of distance between previous location and new point
		int segLength = nextMatch - src;
		
		//	Current Length + new length to copy + replacement diff + remainder to copy
		if (curLength + remLength + diffLength >= max_size)
		{
			//	Cannot replace any more; we will error out assuming we failed
			//	entirely
			count = -1;
			break;
		}
		
		//	Copy segment from src to dest
		strncpy(dest, src, segLength);
		dest += segLength;
		src += segLength;

		//	Copy replacement to dest
		strcpy(dest, replacement);
		dest += repLength;
		
		//	Skip pattern in src
		src += patLength;
		
		//	Adjust lengths
		//	curLength + remLength == total estimate length with no future copies
		curLength += segLength + repLength;
		remLength -= segLength + patLength;
	}
	while ( bReplaceAll );
	
	if (remLength > 0)
	{
		if (curLength + remLength >= max_size)
		{
			count = -1;
		}
		else
		{
			strcpy(dest, src);
		}
	}
	
	if (count == 0) return 0;
	if (count > 0)
	{
		string = destBuf;
	}
	return count;
}


/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(char **ptr_string, int mode, DescriptorData *d, int maxlen) {
	int total_chars, cap_next = TRUE, cap_next_next = FALSE;
	/* warning: do not edit messages with max_str's of over this value */
   
	char *flow   = *ptr_string;
	if (!flow) return;

	BUFFER(formated, MAX_STRING_LENGTH);
   if (IS_SET(mode, FORMAT_INDENT))
   {
      strcpy(formated, "   ");
      total_chars = 3;
   }
   else {
      *formated = '\0';
      total_chars = 0;
   } 

	while (*flow)
	{
		while (isspace(*flow)) ++flow;

		if (*flow)
		{
			char *start = flow++;

			//	Find next word or sentence terminator
			while (*flow
				&& !isspace(*flow)
				&& *flow != '.'
				&& *flow != '!'
				&& *flow != '?')
				++flow;
			
			if (cap_next_next)
			{
				cap_next_next = FALSE;
				cap_next = TRUE;
			}
			
			//	Skip sentence terminators
			while (*flow == '.'
				|| *flow == '!'
				|| *flow == '?')
			{
				cap_next_next = TRUE;
				++flow;
			}
			
			int length = flow - start;
			int visibleLength = cstrnlen(start, length);
			
			if ((total_chars + visibleLength) > 79)
			{
				strcat(formated, "\n");
				total_chars = 0;
			}

			if (cap_next)
			{
				cap_next = FALSE;
				*start = toupper(*start);
			}
			else if (total_chars > 0)
			{
				strcat(formated, " ");
				total_chars++;
			}

			total_chars += visibleLength;
			strncat(formated, start, length);
		}

		if (cap_next_next)
		{
			if ((total_chars + 3) > 79)
			{
				strcat(formated, "\n");
				total_chars = 0;
			}
			else
			{
		    	strcat(formated, "  ");
		    	total_chars += 2;
			}
		}
	}
	strcat(formated, "\n");

	if (strlen(formated) > maxlen) formated[maxlen] = '\0';
	RECREATE(*ptr_string, char, MIN(maxlen, (int)strlen(formated)+3));
	strcpy(*ptr_string, formated);
}



/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(Lexi::String &string, size_t max_size, int mode)
{
	bool capNext = true;	//	Start off with capitalizing
	bool termPunctFound = false;
	
	BUFFER(destBuf, /*max_size*/ MAX_STRING_LENGTH);
	char *dest = destBuf;

	const char *src = string.c_str();
	
//	size_t curLength = 0;
	size_t lineLength = 0;
	if (IS_SET(mode, FORMAT_INDENT))
	{
		strcpy(dest, "   ");
		dest += strlen(dest);
//		curLength = 3;
		lineLength = 3;
	}
	
	
	while (*src)
	{
		while (isspace(*src))	++src;
		
		if (*src)
		{
			const char *mark = src;	//	This is where we'll copy from
			
			//	Skip to end of word.
			char c = *++src;
			while (c && !isspace(c) && c != '.' && c != '!' && c != '?')
				c = *++src;
			
			//	Skip any sentence terminators
			while (c == '.' || c == '!' || c == '?')
			{
				termPunctFound = true;
				c = *++src;
			}
			
			int length = src - mark;
			int visibleLength = cstrnlen(mark, length);
			
			if ((lineLength + visibleLength) >= 79)	//	>= 79 to account for the space to add
			{
				*dest++ = '\n';
//				curLength += 2;
				lineLength = 0;
			}
			
			if (!capNext && lineLength > 0)
			{
				*dest++ = ' ';
//				curLength += 1;
				lineLength += 1;
			}
			
			strncpy(dest, mark, length);
			
			if (capNext)
			{
				capNext = false;
				*dest = toupper(*dest);
			}
			
			lineLength += visibleLength;
			dest += length;
//			curLength += dest;
		}
		
		if (termPunctFound)
		{
			if ((lineLength + 3) >= 79)	//	2 spaces + smallest possible word (1 char)
			{
				*dest++ = '\n';
//				curLength += 2;
				lineLength = 0;
			}
			else
			{
				*dest++ = ' ';
				*dest++ = ' ';
//				curLength += 2;
				lineLength += 2;
			}
			
			capNext = true;
			termPunctFound = false;
		}
	}
	
	*dest++ = '\n';
	*dest = '\0';
	
	if (strlen(dest) > max_size)	dest[max_size] = '\0';
	
	string = destBuf;
}
