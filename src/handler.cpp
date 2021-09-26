/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */




#include "structs.h"
#include "utils.h"
#include "scripts.h"
#include "buffer.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "event.h"
#include "find.h"
//#include "space.h"


/* external functions */
ACMD(do_return);

void free_purged_lists(void);

char *fname(const char *namelist) {
	static char holder[MAX_INPUT_LENGTH];
	char *point;

	for (point = holder; isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	*point = '\0';

	return (holder);
}

#define WHITESPACE " \t"

bool isname(const char *str, const char *namelist)
{
	BUFFER(newlist, MAX_STRING_LENGTH);

	strcpy(newlist, namelist);

	while (char *curtok = strsep(&newlist, WHITESPACE))
		if(is_abbrev(str, curtok))
			return true;
	
	return false;
}


//#define SILLY_WHITESPACE "- \t\n,"
#define SILLY_WHITESPACE " \t\n,"

//	Like isname, but supports:
//		matching two sets of names
//		'.' for an exact match
//		dash, comma, and newlines as separators
bool silly_isname(const char *str, const char *namelist)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(names, MAX_STRING_LENGTH);
	
	char  *argv[MAX_INPUT_LENGTH], *xargv[MAX_INPUT_LENGTH];
	int		argc, xargc, i, j;
	bool	exact = false;

	//	Split str into a series of words
	strcpy(buf, str);
	argc = split_string(buf, SILLY_WHITESPACE, argv);

	//	Split the namelist into a series of words
	strcpy(names, namelist);
	xargc = split_string(names, SILLY_WHITESPACE, xargv);

	//	Get last search word, if it ends in a dot, we want an exact match (huh?)
	char *s = argv[argc-1];
	s += strlen(s);
	if (*(--s) == '.')
	{
		exact = true;
		*s = 0;
	}
	
	/* the string has now been split into separate words with the '-'
	replaced by string terminators.  pointers to the beginning of
	each word are in argv */

	if (exact && argc != xargc)
		return false;

	//	For each match word, find a name word
	//	Clear each match so it can't be re-used
	//	If no match was found for that word, return a failure
	for (i = 0; i < argc; i++)
	{
		for (j = 0; j < xargc; j++)
		{
			if (!xargv[j])	continue;
			
			if (exact ? !str_cmp(argv[i], xargv[j]) : is_abbrev(argv[i], xargv[j]))
			{
				xargv[j] = NULL;
				break;
			}
		}
		if (j >= xargc)
			return false;
	}

	return true;
}

//	Split 'str' into the 'argv' array by separator 'sep'
int split_string(char *str, char *sep, char **argv)
{
	int   argc = 0;

	char *s = strsep(&str, sep);
	if (s)
		argv[argc++] = s;
	else
	{
		*argv = str;
		return 1;
	}

	while ((s = strsep(&str, sep)))
		argv[argc++] = s;

	argv[argc] = '\0';

	return argc;
}



int is_same_group(CharData *ach, CharData *bch )
{
	if (ach == NULL || bch == NULL)
		return FALSE;

	if ( ach->m_Following ) ach = ach->m_Following;
	if ( bch->m_Following ) bch = bch->m_Following;
	return ach == bch;
}


/* cleans out the purge lists */
void free_purged_lists()
{
//	Scriptable *sc;
	
	Entity::DeletePurged();


//	FOREACH(ShipList, purged_ships, ship)
//	{
//		delete *ship;
//	}
//	purged_ships.clear();

	FOREACH(TrigData::List, purged_trigs, trig)
	{
		delete *trig;
	}
	purged_trigs.clear();
}
