/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include <math.h>


#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "bitflags.h"

void tag_argument(char *argument, char *tag);

/* local functions */
struct TimeInfoData real_time_passed(time_t t2, time_t t1);
struct TimeInfoData mud_time_passed(time_t t2, time_t t1);
void die_follower(CharData * ch);
void add_follower(CharData * ch, CharData * leader);
int wear_message_number (ObjData *obj, WearPosition where);


float GetInverseTriangularNumber(int v)
{
	float result = (sqrt((8.0f * v) + 1.0f) - 1.0f) / 2.0f;
	return result;
}

/* creates a random number in interval [from;to] */
int Number(int from, int to)
{
  /* error checking in case people call Number() incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
//    log("SYSERR: Number() should be called with lowest, then highest. Number(%d, %d), not Number(%d, %d).", from, to, to, from);
  }

  return ((circle_random() % (to - from + 1)) + from);
}


const int DiceFactor = 100000;

float BellCurve()
{
	int result = Number(0, 100);
	
	if (result < 66)			result = 0;
	else if (result < 100)		result = Number(-10 * DiceFactor, 10 * DiceFactor);
	else						result = Number(-20 * DiceFactor, 20 * DiceFactor);
	
//	result += Number(3, 30);
	result += Number(1 * DiceFactor, 10 * DiceFactor) + Number(1 * DiceFactor, 10 * DiceFactor) + Number(1 * DiceFactor, 10 * DiceFactor);
//	result += Number(2 * DiceFactor, 15 * DiceFactor) + Number(1 * DiceFactor, 15 * DiceFactor);
	
	return (float)result / (float)DiceFactor;

#if 0

	int	d1, d2, d3;
	
	int d1 = Number(1 * DiceFactor, 10 * DiceFactor);
	int d2 = Number(1 * DiceFactor, 10 * DiceFactor);
	int d3 = Number(1 * DiceFactor, 10 * DiceFactor);
	
	int nd1 = d1 / DiceFactor;
	int nd2 = d2 / DiceFactor;
	int nd3 = d3 / DiceFactor;
	
	
#endif
}


float LinearCurve()
{
	int result = Number(0, 100);
	
	if (result < 66)			result = 0;
	else if (result < 100)		result = Number(-10 * DiceFactor, 10 * DiceFactor);
	else						result = Number(-20 * DiceFactor, 20 * DiceFactor);
	
	result += Number(0, 30 * DiceFactor);
	
	return (float)result / (float)DiceFactor;
}


float SkillRoll(float stat)
{
	return (stat - BellCurve());
}


float LinearSkillRoll(float stat)
{
	return (stat - LinearCurve());
}


/* simulates dice roll */
int dice(int number, int size) {
	int sum = 0;

	if (size <= 0 || number <= 0)
		return 0;

	while (number-- > 0)
		sum += ((circle_random() % size) + 1);

	return sum;
}


int sprintf_cat(char *str, const char *format, ...)
{
	va_list args;
	
	va_start(args, format);
	int chars = vsprintf(str + strlen(str), format, args);
	va_end(args);
	return chars;	
}



int snprintf_cat(char *str, size_t size, const char *format, ...)
{
	va_list args;
	
	int len = strlen(str);
	int remainingLength = size - len;
	str += len;
	
	if (remainingLength <= 0)
		return 0;

	va_start(args, format);
	int chars = vsnprintf(str, remainingLength, format, args);
	va_end(args);
	return chars;	
}



/* Create a duplicate of a string */
char *str_dup(const char *source) {
	char *new_s;
	
	if (!source)
		return NULL;
	
	CREATE(new_s, char, strlen(source) + 1);
	return (strcpy(new_s, source));
}



/* str_cmp: a case-insensitive version of strcmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
#if 0
int str_cmp(const char *arg1, const char *arg2) {
	int chk, i;

	for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
		if ((chk = tolower(*(arg1 + i)) - tolower(*(arg2 + i)))) {
			if (chk < 0)	return -1;
			else			return 1;
		}
//	int	chk;
//	while (*arg1 || *arg2)
//		if ((chk = tolower(*(arg1++)) - tolower(*(arg2++))))
//			if (chk < 0)	return -1;
//			else			return 1;
	return 0;
}


/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(const char *arg1, const char *arg2, int n) {
	int chk, i;
	for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
		if ((chk = tolower(*(arg1 + i)) - tolower(*(arg2 + i)))) {
			if (chk < 0)	return -1;
			else			return 1;
		}
//	int	chk;
//	while (*arg1 || *arg2)
//		if ((chk = tolower(*(arg1++)) - tolower(*(arg2++))))
//			if (chk < 0)	return -1;
//			else			return 1;
//		else if (n-- <= 0)
//			break;
	return 0;
}
#endif



//	Numeric-sensitive string ordering comparison
//	"str3" < "str4" and "str21" > "str3"
//	"str004" == "str4"
int str_cmp_numeric(const char *arg1, const char *arg2)
{
	int		i = 0;
	
	char	a1 = *arg1;
	char	a2 = *arg2;

	while (a1 || a2)
	{
		int check = tolower(a1) - tolower(a2);
		
		if (check != 0)
		{
			if (isdigit(a1) && isdigit(a2))
			{
				long long int n1 = strtoll(arg1, const_cast<char **>(&arg1), 10);
				long long int n2 = strtoll(arg2, const_cast<char **>(&arg2), 10);
				
				check = n1 - n2;
			}
			
			if (check < 0)		return -1;
			else if (check > 0)	return 1;
		}
		
		a1 = *++arg1;
		a2 = *++arg2;
	}

	return 0;
}


/* Return pointer to first occurrence of string ct in */
/* cs, or NULL if not present.  Case insensitive */
const char *str_str(const char *cs, const char *ct)
{
	const char *s;
	const char *t;

	if (!cs || !ct)
		return NULL;

	while (*cs) {
		t = ct;

		while (*cs && (tolower(*cs) != tolower(*t)))
			cs++;

		s = cs;

		while (*t && *cs && (tolower(*cs) == tolower(*t))) {
			t++;
			cs++;
		}
    
		if (!*t)
			return s;
	}

	return NULL;
}



#if defined(_WIN32)
/*
char * strsep(char **sp, const char *sep)
{
    char *p, *s;
    if (sp == NULL || *sp == NULL || **sp == '\0') return(NULL);
    s = *sp;
    p = s + strcspn(s, sep);
    if (*p != '\0')	*p++ = '\0';
    *sp = p;
    if (**sp == '\0')
    	*sp = NULL;
    return(s);
}
*/

char *strsep(char **pSrc, const char *delim)
{
	char *src = *pSrc;
	char *orig = src;
	
	if (src == NULL)
		return NULL;
	
	//	Find the deliminator for the token
	char *tok = src + strcspn(src, delim);
	if (*tok)
	{
		//	If we find one, terminate the token, update the original source
		*tok = '\0';
		*pSrc = tok + 1;
	}
	else
	{
		//	If we don't find a delimiter
		*pSrc = NULL;
	}
	
	//	Return the start of the token (the original contents of *pSrc)
	return src;
}

#endif


int cstr_cmp(const char *arg1, const char *arg2)
{
	char a1 = *arg1++;
	char a2 = *arg2++;
	
	while (a1 || a2)
	{
		while (a1 == '`')
		{
			a1 = *arg1++;
			if (a1 && a1 != '`')
			{
				if (a1 == '^')	a1 = *arg1++;
				if (a1)			a1 = *arg1++;
			}
		}

		while (a2 == '`')
		{
			a2 = *arg2++;
			if (a2 && a2 != '`')
			{
				if (a2 == '^')	a2 = *arg2++;
				if (a2)			a2 = *arg2++;
			}
		}
		
		int check = tolower(a1) - tolower(a2);
		if (check)
		{
			if (check > 0)	return 1;
			else			return -1;
		}
		
		if (a1)	a1 = *arg1++;
		if (a2)	a2 = *arg2++;
	}
	
	return 0;
}


size_t cstrlen(const char *str)
{
	int			length = 0;
	char		c;
	bool		bIsColor = false;
	
	if (!str)	return 0;
	
	while ((c = *str++))
	{
		if (c == '`')
		{
			if (bIsColor)	++length;
			bIsColor = !bIsColor;
		}
		else if (bIsColor)
		{
			if (c != '^')	bIsColor = false;	//	Skip ^ code; this breaks on `^^
		}
		else				++length;
	}
	
	return length;
}


size_t cstrnlen(const char *str, size_t maxlength)
{
	int			length = 0;
	char		c;
	bool		bIsColor = false;
	
	if (!str || maxlength == 0)	return 0;
	
	while ((c = *str++) && maxlength > 0)
	{
		if (c == '`')
		{
			if (bIsColor)	++length;
			bIsColor = !bIsColor;
		}
		else if (bIsColor)
		{
			if (c != '^')	bIsColor = false;	//	Skip ^ code; this breaks on `^^
		}
		else				++length;
		--maxlength;
	}
	
	return length;
}


//	Return maxlength + number of color code characters
size_t cstrnlen_for_sprintf(const char *str, size_t n)
{
	int			length = n;
	char		c;
	bool		bIsColor = false;
	
	if (!str || n == 0)	return 0;
	
	while ((c = *str++))
	{
		if (c == '`')
		{
			if (!bIsColor)	++length;	//	the first ` is not printable, second is...
			else			--n;		//	it was a printable...
			bIsColor = !bIsColor;
		}
		else if (bIsColor)
		{	
			++length;	//	anything following ` except another ` as handled above is unrpinted
			if (c != '^')	bIsColor = false;	//	^ will conver to background so is kept
		}
		else if (n == 0)
			break;
		else
			--n;
	}
	
	return length;
}


//	Color Aware
char *CAP(char *in)
{
	char *p = in;
	
	char c = *p;
	while (c && !isalnum(c))
	{
		if (c == '`')
		{
			c = *++p;
			if (c == '^')	c = *++p;
			if (!c)			break;
		}
		c = *++p;
	}
	
	if (c)	*p = toupper(*p);
	
	return in;
}


const char *AN(const char *p)
{
	char c = *p;
	while (c && !isalnum(c))
	{
		if (c == '`')
		{
			c = *++p;
			if (c == '^')	c = *++p;
			if (!c)			break;
		}
		c = *++p;
	}
	
	return strchr("aeiouAEIOU", c) ? "an" : "a";
}


/*
int cstrlen(const char *str) {
	int		length = 0;
	char		c, nc;
	
	if (!str)	return 0;
	
	while ((c = *(str++)))
	{
		nc = *str;
		
		if ((nc != 0) &&
			((c=='`' && (search_chars(nc, "nkrgybmcwKRGYBMCWiIfF*`") >= 0)) ||
			(c=='^' && (search_chars(nc, "krgybmcw^") >= 0))))
		{
			++str;
	
			if (nc == '^' || nc == '`')
				++length;
		} else
			++length;
	}
	
	return length;
}


int cstrlendiff(const char *str) {
	int		length = 0;
	
	char c, nc;
	
	if (!str)	return 0;
	
	while ((c = *(str++)))
	{
		nc = *str; 
		
		if ((nc != 0) && 
			((c=='`' && (search_chars(nc, "nkrgybmcwKRGYBMCWiIfF*`") >= 0)) ||
			(c=='^' && (search_chars(nc, "krgybmcw^") >= 0))))
		{
			++str;
			++length;
			
			if (nc != '`' && nc != '^')
				++length;	//	safety catch to prevent ^\0 and `\0
		}
	}
	
	return length;
}


int cstrlen_for_sprintf(const char *str, size_t n) {
	int		length = n;
	
	char c, nc;
	
	if (!str)	return 0;
	if (n <= 0)	return 0;
	
	while ((c = *str) && (n > 0))
	{
		++str;
		nc = *str;
		
		if ((nc != 0) && 
			((c=='`' && strchr("nkrgybmcwKRGYBMCWiIfF*`", nc)) ||
			 (c=='^' && strchr("krgybmcw^", nc))))
		{
			++str;
			++length;
			
			if (nc != '`' && nc != '^')
				++length;	//	safety catch to prevent ^\0 and `\0
			else
				--n;
		}
		else
			--n;
	}
	
	//	Reached the max or c == '\0'
	//	We'll look for more color codes to skip if at the max...
	if (c)
	{
		while ((c = *(str++)))
		{
			nc = *str;
			
			if ((nc != 0) && 
				((c=='`' && strchr("nkrgybmcwKRGYBMCWiIfF*", nc)) ||
				(c=='^' && strchr("krgybmcw", nc))))
			{
				++str;
				++length;
				++length;
			}
			else
				break;
		}
	}
	
	return length;
}
*/


/* the "touch" command, essentially. */
int touch(char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    perror(path);
    return -1;
  } else {
    fclose(fl);
    return 0;
  }
}


void sprintbit(Flags bitvector, char *names[], char *result)
{
	long nr;
  
	*result = '\0';

	for (nr = 0; bitvector; bitvector >>= 1)
	{
		if (IS_SET(bitvector, 1))
		{
			if (*names[nr] != '\n')
			{
				strcat(result, names[nr]);
				strcat(result, " ");
			}
			else
				strcat(result, "UNDEFINED ");
		}
		if (*names[nr] != '\n')
			nr++;
	}

	if (!*result)
		strcpy(result, "NOBITS ");
}


void sprinttype(int type, char **names, char *result)
{
	strcpy(result, findtype(type, names));
}


const char *findtype(int type, char **names)
{
	int nr = 0;

	while (type && *names[nr] != '\n') {
		type--;
		nr++;
	}

	if (*names[nr] != '\n')	return names[nr];
	else					return "UNDEFINED";
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct TimeInfoData real_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct TimeInfoData now;

  secs = (long) (t2 - t1);

  now.minutes = (secs / SECS_PER_REAL_MIN) % 60;
  secs -= SECS_PER_REAL_MIN * now.minutes;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..29 days  */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = -1;
  now.year = -1;

  return now;
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct TimeInfoData mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct TimeInfoData now;

  secs = (long) (t2 - t1);

  now.total_hours = (secs / SECS_PER_MUD_HOUR);	//	total hours elapsed

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	//	0..23 hours
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.weekday = (secs / SECS_PER_MUD_DAY) % 7;	//	Day of the week
  
  now.day = (secs / SECS_PER_MUD_DAY) % 30;		//	0..29 days
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 12;	//	0..11 months
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);		//	0..XX? years

  return now;
}



struct TimeInfoData age(CharData * ch)
{
	struct TimeInfoData player_age;

	player_age = mud_time_passed(time(0), ch->GetPlayer()->m_Creation);
//	player_age = mud_time_passed(ch->player->time.played, 0);

	switch (ch->GetRace())
	{
		case RACE_HUMAN:
		case RACE_PREDATOR:
		  	player_age.year += 19;	/* All Humans and Predators start at 19 */
		  	break;
		case RACE_SYNTHETIC:
		  	player_age.year += 2;	/* All Synthetics start at 2 */
		  	break;
		default:					/* Aliens start at 0 */
			break;
	}
	
	return player_age;
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
int circle_follow(CharData * ch, CharData * victim)
{
  CharData *k;

  for (k = victim; k; k = k->m_Following) {
    if (k == ch)
      return TRUE;
  }

  return FALSE;
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(CharData * ch, CharData * leader) {
	if (ch->m_Following)
	{
		core_dump();
		return;
	}

	ch->m_Following = leader;
	leader->m_Followers.push_back(ch);
	
	act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
	act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
	act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf) {
	BUFFER(temp, MAX_INPUT_LENGTH);
	int lines = 0;

	do {
		lines++;
		fgets(temp, MAX_INPUT_LENGTH, fl);
		if (*temp)
			temp[strlen(temp) - 1] = '\0';
	} while (!feof(fl) && (!*temp));

	if (feof(fl)) {
		*buf = '\0';
		lines = 0;
	} else
		strcpy(buf, temp);
	
	return lines;
}


/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_any_line(FILE * fl, char *buf, int length) {
	*buf = '\0';
	
	fgets(buf, length, fl);
	if (*buf)
		buf[strlen(buf) - 1] = '\0';

	return 1;
}


void Free_Error(const char * file, int line) {
	mudlogf( BRF, LVL_SRSTAFF, TRUE,  "CODEERR: NULL pointer in file %s:%d", file, line);
}


/*thanks to Luis Carvalho for sending this my way..it's probably a
  lot shorter than the one I would have made :)  */
void sprintbits(unsigned int vektor,char *outstring) {
	int i;
	const char flags[53]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	*outstring = '\0';
	for (i=0;i<32;i++) {
		if (vektor & 1) {
			*outstring=flags[i];
			outstring++;
		}
		vektor>>=1;
	}
	*outstring=0;
}


void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for(i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';
  
  while(*tmp == ':' || *tmp == ' ')
    tmp++;

  while(*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}


/* returns the message number to display for where[] and equipment_types[] */
int wear_message_number (ObjData *obj, WearPosition where) {
	if (!obj) {
		log("SYSERR:  NULL passed for obj to wear_message_number");
		return where;
	}
	if (where < WEAR_HAND_R)						return where;
	else if (OBJ_FLAGGED(obj, ITEM_TWO_HAND)) {
		if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)		return POS_WIELD_TWO;
		else										return POS_HOLD_TWO;
	} else if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
		if (where == WEAR_HAND_R)					return POS_WIELD;
		else										return POS_WIELD_OFF;
	} else if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)		return POS_LIGHT;
	else											return POS_HOLD;
}


/* string prefix routine */
int str_prefix(const char *astr, const char *bstr) {
	if (!astr) {
		log("SYSERR: str_prefix: null astr.");
		return TRUE;
	}
	if (!bstr) {
		log("SYSERR: str_prefix: null bstr.");
		return TRUE;
	}
	for(; *astr; astr++, bstr++) {
		if(tolower(*astr) != tolower(*bstr)) return TRUE;
	}
	return FALSE;
}




/* ========================================================================
 * Table of CRC-32's of all single-byte values (made by make_crc_table)
 */
static const CRC32 CRC32Table[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

/* ========================================================================= */
#if 0
#define DO1(buf) crc = CRC32Table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
CRC32 GetCRC32(const unsigned char *buf, unsigned int len, CRC32 crc)
{
	if (buf == NULL) return 0L;

	crc = crc ^ 0xffffffffL;
	while (len >= 8)
	{
		DO8(buf);
		len -= 8;
	}
	if (len) do {
		DO1(buf);
	} while (--len);
	return crc ^ 0xffffffffL;
}
#endif

/* ========================================================================= */
CRC32 GetStringCRC32(const char *buf)
{
	if (buf == NULL || *buf == 0) return 0L;

	const CRC32 *crctable = CRC32Table;

	CRC32 crc =  0xffffffffL;
	
	unsigned char c = *buf++;
	if (c)
	{
		do
		{
			if (c >= 'A' && c <= 'Z')
				c = c | 0x20;	//	Convert to lowercase
			
			crc = crctable[(crc ^ c) & 0xFF] ^ (crc >> 8);
			
			c = *buf++;
		} while (c);
	}
	return crc ^ 0xffffffffL;
}


/* ========================================================================= */
CRC32 GetStringCRC32(const char *buf, unsigned int length)
{
	if (buf == NULL) return 0L;

	const CRC32 *crctable = CRC32Table;

	CRC32 crc =  0xffffffffL;
	
	if (length > 0)
	{
		do
		{
			unsigned char c = *buf++;
			
			--length;
			if (c >= 'A' && c <= 'Z')
				c |= 0x20;	//	Convert to lowercase
			
			crc = crctable[(crc ^ c) & 0xFF] ^ (crc >> 8);
		}
		while (length > 0);
	}
	return crc ^ 0xffffffffL;
}


//	Based on ELF Hash
//	Case insensitive hash
//		Converst to uppercase
//	OPTIONAL: Converts to uppercase, subtracts 32, masks to low 6 bits
//		(ONLY safe for alphanumeric strings.  Any symbol >= 96 that is { | } and ~) will be lost
Hash GetStringFastHash(const char *buf)
{
	if (buf == NULL) return 0;
	
	Hash hash = 0;

	unsigned char c = *buf++;
	if (c)
	{
		do
		{
//			hash = (hash << 4) + ((toupper(c) - 0x20) & 0x3F);	//	Only use top 6 bits

//			hash = (hash << 4) + toupper(c);
			if (c >= 'a' && c <= 'z')	c &= (unsigned char)~0x20;	//	Faster toupper?
			
			hash = (hash << 4) + c;
			
			Hash x = hash & 0xF0000000L;
			if(x)
			{
				hash ^= (x >> 24);
				hash &= ~x;
			}
			
			c = *buf++;
		}
		while (c);
	}

   return hash;
}


Hash GetStringFastHash(const char *buf, unsigned int length, Hash hash)
{
	if (buf == NULL) return hash;

	if (length > 0)
	{
		do
		{
			--length;
			
			unsigned char c = *buf++;
			
//			hash = (hash << 4) + ((toupper(c) - 0x20) & 0x3F);	//	Only use top 6 bits

//			hash = (hash << 4) + toupper(c);
			if (c >= 'a' && c <= 'z')	c &= (unsigned char)~0x20;	//	Faster toupper?
			
			hash = (hash << 4) + c;
			
			Hash x = hash & 0xF0000000L;
			if(x)
			{
				hash ^= (x >> 24);
				hash &= ~x;
			}
		}
		while (length > 0);
	}

   return hash;
}



Hash16 GetStringFastHash16(const char *buf)
{
	if (buf == NULL) return 0;
	
	Hash hash = 0;

	unsigned char c = *buf++;
	if (c)
	{
		do
		{
//			hash = (hash << 4) + ((toupper(c) - 0x20) & 0x3F);	//	Only use top 6 bits

//			hash = (hash << 4) + toupper(c);
			if (c >= 'a' && c <= 'z')	c &= (unsigned char)~0x20;	//	Faster toupper?
			
			hash = (hash << 4) + c;
			
			Hash x = hash & 0xF0000000L;
			if(x)
			{
				hash ^= (x >> 24);
				hash &= ~x;
			}
			
			c = *buf++;
		}
		while (c);
	}

   return hash ^ (hash >> 16);
}


Hash16 GetStringFastHash16(const char *buf, unsigned int length, Hash hash)
{
	if (buf == NULL) return hash;

	if (length > 0)
	{
		do
		{
			--length;
			
			unsigned char c = *buf++;
			
//			hash = (hash << 4) + ((toupper(c) - 0x20) & 0x3F);	//	Only use top 6 bits

//			hash = (hash << 4) + toupper(c);
			if (c >= 'a' && c <= 'z')	c &= (unsigned char)~0x20;	//	Faster toupper?
			
			hash = (hash << 4) + c;
			
			Hash x = hash & 0xF0000000L;
			if(x)
			{
				hash ^= (x >> 24);
				hash &= ~x;
			}
		}
		while (length > 0);
	}

   return hash ^ (hash >> 16);
}


typedef std::multimap<Hash, Lexi::String> HashCollisionMap;

HashCollisionMap gHashCollisions;

void ReportHashCollision(Hash hash, const char *lhsName, const char *rhsName)
{
	HashCollisionMap::iterator iter = gHashCollisions.lower_bound(hash);
	HashCollisionMap::iterator end = gHashCollisions.upper_bound(hash);
	
	bool bLeftFound = false;
	bool bRightFound = false;
	
	for (; iter != end; ++iter)
	{
		if (!bLeftFound && iter->second == lhsName)		bLeftFound = true;
		if (!bRightFound && iter->second == rhsName)	bLeftFound = true;
	}
		
	if (!bLeftFound)	gHashCollisions.insert(HashCollisionMap::value_type(hash, lhsName));
	if (!bRightFound)	gHashCollisions.insert(HashCollisionMap::value_type(hash, rhsName));
}


/* ========================================================================= */

#undef DO1
#undef DO2
#undef DO4
#undef DO8


#if defined(CIRCLE_UNIX)
/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_func(const char *who, int line) {
	log("SYSERR: *** Dumping core from %s:%d. ***", who, line);

	//	These would be duplicated otherwise...
	fflush(stdout);
	fflush(stderr);
	fflush(logfile);

	//	 Kill the child so the debugger or script doesn't think the MUD
	//	crashed.  The 'autorun' script would otherwise run it again.
	if (!fork())
		abort();
}
#else

//	Any other OS should just note the failure and (attempt to) cope.
void core_dump_func(const char *who, int line) {
	log("SYSERR: *** Assertion failed at %s:%d. ***", who, line);
	//	Some people may wish to die here.  Try 'assert(FALSE);' if so.
}

#endif



void Lexi::tolower(char *str)
{
	for (; *str; ++str)
		*str = ::tolower(*str);
}







#if 0
#include "bitflags.h"

enum TestEnum
{
	R_DARK,
	R_DEATH,
	R_NOMOB,
	R_INDOORS,
	R_PEACEFUL,
	R_SOUNDPROOF,
	R_NOTRACK,
	R_PARSE,
	R_TUNNEL,
	R_PRIVATE,
	R_GODROOM,
	R_HOUSE,
	R_HOUSECRASH,
	R_UNUSED13,
	R_NOPK,
	R_SPECIAL_MISSION,
	R_VEHICLE,
	R_GRGODROOM,
	R_NOLITTER,
	R_GRAVITY,
	R_VACUUM,
	R_NORECALL,
	R_NOHIVE,
	R_STARTHIVED,
	R_CAN_LAND,
	R_SHIPYARD,
	R_GARAGE,
	R_SURFACE,
	R_NUMFLAGS
};

typedef Lexi::BitFlags<R_NUMFLAGS, TestEnum> RBits;
static void TestFunc()
{
	RBits foo;
	
	foo.clear(R_VACUUM);
	foo.set(R_VACUUM);
	foo.test(R_NOLITTER);
	
	RBits::Mask	MAKE_BITSET(bitMask, R_VACUUM, R_NOLITTER);
	foo.testForAny(bitMask);
	foo.testForAll(bitMask);
	foo.testForNone(bitMask);
	foo.set(bitMask);
	foo.clear(bitMask);
	
	if (foo == bitMask)
		(void)0;
	if (foo != bitMask)
		(void)0;
	
	if (foo[R_VACUUM])
		(void)0;
	
//	if (foo)
//		(void)0;
	
	~foo;
	foo.flip();
}
#endif

Lexi::String Lexi::PrintBitFlags(BitFlagsWrapper::Ptr bf, char **names)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	bool bHitEndOfNames = false;
	*buf = '\0';

	for (unsigned int nr = 0; nr < bf->size(); ++nr)
	{
		if (bf->test(nr))
		{
			if (!bHitEndOfNames && *names[nr] != '\n')
			{
				strcat(buf, names[nr]);
				strcat(buf, " ");
			}
			else
			{
				strcat(buf, "UNDEFINED ");
				bHitEndOfNames = true;
			}
		}
	}

	if (!*buf)
		strcpy(buf, "NOBITS ");
	
	return buf;
}


Lexi::String Lexi::PrintBitFlags(BitFlagsWrapper::Ptr bf, const EnumNameResolverBase &nameResolver)
{
	BUFFER(buf, MAX_STRING_LENGTH);

	for (unsigned int nr = 0; nr < bf->size(); ++nr)
	{
		if (bf->test(nr))
		{
			strcat(buf, *buf ? " " : "");
			strcat(buf, nameResolver.GetName(nr));
		}
	}

	if (!*buf)
		strcpy(buf, "NOBITS");
	
	return buf;
}


Lexi::String Lexi::CreateDateString(time_t date, const char *format)
{
	if (!date)	return "Never";
	
	BUFFER(buf, MAX_STRING_LENGTH);
	strftime(buf, MAX_STRING_LENGTH, format, localtime( &date ));
	buf[MAX_STRING_LENGTH - 1] = 0;
	return buf;
}



#if defined(_WIN32)

PerfTimer::Frequency PerfTimer::ms_Frequency;

void PerfTimer::Start()
{
	QueryPerformanceCounter(&m_Start);
}


double PerfTimer::GetTimeElapsed()
{
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);
	
	return ((double)end.QuadPart - (double)m_Start.QuadPart) / (double)ms_Frequency.Get();
}

#else

void PerfTimer::Start()
{
	gettimeofday(&m_Start, NULL);
}


double PerfTimer::GetTimeElapsed()
{
	timeval end;
	
	gettimeofday(&end, NULL);
	
	double t1 = (double)m_Start.tv_sec + (double)m_Start.tv_usec/(1000*1000);
	double t2 = (double)end.tv_sec + (double)end.tv_usec/(1000*1000);
	
	return t2 - t1;
}

#endif






#if 0
static bool string_sort_numeric(const Lexi::String &arg1, const Lexi::String &arg2)
{
	return (str_cmp_numeric(arg1.c_str(), arg2.c_str()) < 0);
}

void TestNumericSort()
{
	int result = str_cmp_numeric("Test", "Test");
	result = str_cmp_numeric("Test 539", "Test 392");
	result = str_cmp_numeric("Test 539", "Test 540");
	result = str_cmp_numeric("Test 539", "Test 538");
	result = str_cmp_numeric("Test 539 foo 3", "Test 0539 foo 2");
	
	std::vector<Lexi::String>	vec;
/*	vec.push_back("Test 101");
	vec.push_back("Test200");
	vec.push_back("Test 105");
	vec.push_back("Test 1010");
	vec.push_back("Testing");
	vec.push_back("Test203");
*/	

	vec.push_back("foo 5 bar 6");
	vec.push_back("foo 6 bar 7");
	vec.push_back("foo 5 bar 7");	
	vec.push_back("foo 6 bar 6");
	vec.push_back("foo 52 bar 6");
	vec.push_back("Test 1010");
	vec.push_back("Testing");
	vec.push_back("Test203");

	sort(vec.begin(), vec.end(), string_sort_numeric);

	for (int i = 0; i < vec.size(); ++i)
		printf("%s\n", vec[i].c_str());
}
#endif

#if 0
class TestHashCollision
{
public:

	typedef std::map<Hash, Lexi::String>	LexiStringMap;

	LexiStringMap m;

	void AddHashedName(const Lexi::String &s)
	{
		Hash h = GetStringFastHash(s.c_str());
		
		LexiStringMap::iterator i = m.find(h);
		if (i != m.end())
		{
			//printf("Collision %x: %s / %s\n", h, s.c_str(), i->second.c_str());
			return;
		}
		
		m[h] = s;
	}


	TestHashCollision::TestHashCollision()
	{
		const char *names[8] =
		{
			"fighter",
			"shuttle",
			"wing",
			"ship",
			"escort",
			"cargo",
			"capital",
			"carrier"
		};
		
		circle_srandom(time(0));
		
		printf("Testing collisions\n");
		
//		for (int n = 0; n < 8; ++n)
//		{
//			printf("...adding %s\n", names[n]);
//			for (int i = 0; i < 1000000; ++i)
//			{
//				int type = Number(0,7);
//				AddHashedName(Lexi::Format("%s%d", names[type], i));
//			}
//		}

/*
		for (int i = 0; i < 1000000;)
		{
			if (AddHashedName(Lexi::Format("%c%c%c%3.3d",
				Number(0,25) + 'A',
				Number(0,25) + 'A',
				Number(0,25) + 'A',
				Number(0,999))))
				++i;
		}
*/
		for (char c1 = 'A'; c1 <= 'Z'; ++c1)
			for (char c2 = 'A'; c2 <= 'Z'; ++c2)
				for (char c3 = 'A'; c3 <= 'Z'; ++c3)
					for (int i = 0; i < 1000; ++i)
						AddHashedName(Lexi::Format("%c%c%c%3.3d", c1, c2, c3, i));

		printf("...done, total size is %d\n", m.size());
	}
};
TestHashCollision s_UnitTest;
#endif

#if 0
static bool StringByHashSort(const Lexi::String &lhs, const Lexi::String &rhs)
{
	return GetStringFastHash(lhs.c_str()) < GetStringFastHash(rhs.c_str());
}


static void SortNumericStringsByHash()
{
	std::vector<Lexi::String>	vec;
	for (int i = 0; i < 320; ++i)
	{
		vec.push_back(Lexi::Format("%d", i));
	}
	
	Lexi::Sort(vec, StringByHashSort);
	
	for (int i = 0; i < vec.size(); ++i)
		printf("%s\n", vec[i].c_str());
}


class AutomaticUnitTest
{
public:
	AutomaticUnitTest(void (*Func)(void))
	{
		Func();
	}
};

//AutomaticUnitTest unitTestHashedNumberSort(SortNumericStringsByHash);
//AutomaticUnitTest unitTestNumericSort(TestNumericSort);
#endif




#if 0
struct WeatherCell
{
	int				temperature;	//	Fahrenheit because I'm American, by god
	int				pressure;		//	0..100 for now, later change to barometric pressures
	int				humidity;		//	0+
	int				precipitation;	//	0..100
	
	//	Instead of a wind direction we use an X/Y speed
	//	It makes the math below much simpler this way.
	//	Its not hard to determine a basic cardinal direction from this
	//	If you want to, a good rule of thumb is that if one directional
	//	speed is more than double that of the other, ignore it; that is
	//	if you have speed X = 15 and speed Y = 3, the wind is obviously
	//	to the east.  If X = 15 and Y = 10, then its a north-east wind.
	int				windSpeedX;		//	< 0 = west, > 0 = east
	int				windSpeedY;		//	< 0 = north, > 0 = south
};


const int WEATHER_SIZE_X = 10;
const int WEATHER_SIZE_Y = 10;

//	This is the Weather Map.  It is a grid of cells representing X-mile square
//	areas of weather
WeatherCell	weatherMap[WEATHER_SIZE_X][WEATHER_SIZE_Y];

//	This is the Weather Delta Map.  It is used to accumulate changes to be
//	applied to the Weather Map.  Why accumulate changes then apply them, rather
//	than just change the Weather Map as we go?
//	Because doing that can mean a change just made to a neighbor can
//	immediately cause ANOTHER change to a neighbor, causing things
//	to get out of control or causing cascading weather, propagating much
//	faster and unpredictably (in a BAD unpredictable way)
//	Instead, we determine all the changes that should occur based on the current
//	'snapshot' of weather, than apply them all at once!
WeatherCell	weatherDelta[WEATHER_SIZE_X][WEATHER_SIZE_Y];


//	Set everything up to a nice cool summer day
void InitializeWeatherMap()
{
	for (int x = 0; x < WEATHER_SIZE_X; ++x)
	{
		for (int y = 0; y < WEATHER_SIZE_Y; ++y)
		{
			WeatherCell &cell = weatherMap[x][y];
			
			cell.temperature	= 70;	//	Balmy 70
			cell.pressure		= 50;	//
			cell.humidity		= 60;	//	60% humidity
			cell.precipitation	= 0;	//	Not raining
			cell.windSpeedX		= 5;	//	Calm east blowing wind
			cell.windSpeedY		= 0;
		}
	}
}


void ApplyDeltaChanges()
{
	for (int x = 0; x < WEATHER_SIZE_X; ++x)
	{
		for (int y = 0; y < WEATHER_SIZE_Y; ++y)
		{
			WeatherCell &cell = weatherMap[x][y];
			WeatherCell &delta = weatherDelta[x][y];
			
			//	TODO: Messages/state changes
			
			cell.temperature	+= delta.temperature;
			cell.pressure		+= delta.pressure;
			cell.humidity		+= delta.humidity;
			cell.precipitation	+= delta.precipitation;
			cell.windSpeedX		+= delta.windSpeedX;
			cell.windSpeedY		+= delta.windSpeedY;
		}
	}
}


void UpdateWeather()
{
	//	Clear delta map
	memset(weatherDelta, 0, sizeof(weatherDelta));


	//	Do global weather affects here
	//	Maybe randomly pick a cell to apply a random change to
	//	and see what happens!
	
	
	//	Iterate over every cell and set up the changes
	//	that will occur in that cell and it's neighbors
	//	based on the weather
	for (int x = 0; x < WEATHER_SIZE_X; ++x)
	{
		for (int y = 0; y < WEATHER_SIZE_Y; ++y)
		{
			WeatherCell &cell = weatherMap[x][y];	//	Weather cell
			WeatherCell &delta = weatherDelta[x][y];//	Where we accumulate the changes to apply
			
			//	Precipitation drops humidity by 10% of precip level
			if (cell.precipitation > 0)
				delta.humidity -= cell.precipitation / 10;
			
			//	Humidity and pressure can increase the precipitation level
			int humidityAndPressure = cell.humidity + cell.pressure;
			if ((humidityAndPressure / 2) > 60)
				delta.precipitation	+= cell.humidity / 10;

			//	Changes applied based on what is going on in adjacent cells
			for (int dx = -1; dx <= 1; ++dx)
			{
				for (int dy = -1; dy <= 1; ++dy)
				{
					int nx = x + dx;
					int ny = y + dy;
					
					//	Skip THIS cell
					if (dx == 0 && dy == 0)
						continue;
					
					//	We're cheap, we ignore corner neighbors
					if (dx != 0 && dy != 0)
						continue;
					
					//	Prevent array over/underruns
					if (nx < 0 || nx >= WEATHER_SIZE_X)
						continue;
					if (ny < 0 || ny >= WEATHER_SIZE_Y)
						continue;
					
					WeatherCell &neighborCell = weatherMap[nx][ny];
					WeatherCell &neighborDelta = weatherDelta[nx][ny];
					
					//	We'll apply wind changes here
					//	Wind speeds up in a given direction based on pressure
					
					//	1/2 of the pressure difference applied to wind speed
					
					//	Wind should move from higher pressure to lower pressure
					//	and some of our pressure difference should go with it!
					//	If we are pressure 60, and they are pressure 40
					//	then with a difference of 20, lets make that a 10mph
					//	wind increase towards them!
					//	So if they are west neighbor (dx < 0)
					
					int	pressureDelta = cell.pressure - neighborCell.pressure;
					int windSpeedDelta = pressureDelta / 2;
					
					if (dx != 0)		//	Neighbor to east or west
						delta.windSpeedX += pressureDelta * dx;	//	dx = -1 or 1
					else				//	Neighbor to north or south
						delta.windSpeedY += pressureDelta * dy;	//	dy = -1 or 1
					
					//	Subtract because if positive means we are higher
					delta.pressure -= pressureDelta;
					
					//	Now GIVE them a bit of temperature and humidity change
					//	IF our wind is blowing towards them
					int temperatureDelta = cell.temperature - neighborCell.temperature;
					temperatureDelta /= 4;
					
					int humidityDelta = cell.humidity - neighborCell.humidity;
					humidityDelta /= 4;
					
					if (   (cell.windSpeedX < 0 && dx < 0)
						|| (cell.windSpeedX > 0 && dx > 0)
						|| (cell.windSpeedY < 0 && dy < 0)
						|| (cell.windSpeedY > 0 && dy > 0))
					{
						neighborDelta.temperature += temperatureDelta;
						neighborDelta.humidity += humidityDelta;
					}
				}
			}
		}
	}
	
	//	Apply the changes
	ApplyDeltaChanges();
}

#endif
