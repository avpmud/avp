/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __DB_H__
#define __DB_H__

#include "types.h"

#include "lexistring.h"

#include <list>

#define WORLD_BASE		"world/"			/* room definitions		*/
#define WLD_PREFIX		"world/wld/"	/* room definitions		*/
#define MOB_PREFIX		"world/mob/"	/* monster prototypes		*/
#define OBJ_PREFIX		"world/obj/"	/* object prototypes		*/
#define ZON_PREFIX		"world/zon/"	/* zon defs & command tables	*/
#define SHP_PREFIX		"world/shp/"	/* shop definitions		*/
#define TRG_PREFIX		"world/trg/"

#define WLD_SUFFIX		".wld"	/* room definitions		*/
#define MOB_SUFFIX		".mob"	/* monster prototypes		*/
#define OBJ_SUFFIX		".obj"	/* object prototypes		*/
#define ZON_SUFFIX		".zon"	/* zon defs & command tables	*/
#define SHP_SUFFIX		".shp"	/* shop definitions		*/
#define TRG_SUFFIX		".trg"

#define ZONE_LIST		WORLD_BASE "zones.lst"

#define HLP_PREFIX		"text/help/"	/* for HELP <keyword>		*/
#define BRD_PREFIX		"etc/board/"
#define PLR_PREFIX		"pfiles/"	/* player ascii file directory	*/
#define TXT_PREFIX		"text/"
#define MSC_PREFIX		"misc/"
#define ETC_PREFIX		"etc/"
#define HOUSE_DIR		"house/"
//#define IMC_DIR			"imc/"

#define BOARD_DIRECTORY ETC_PREFIX "boards/"
	
#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */


#define CREDITS_FILE	TXT_PREFIX "credits"	/* for the 'credits' command	*/
#define NEWS_FILE		TXT_PREFIX "news"	/* for the 'news' command	*/
#define MOTD_FILE		TXT_PREFIX "motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE		TXT_PREFIX "imotd"	/* messages of the day / immort	*/
#define HELP_PAGE_FILE	HLP_PREFIX "screen" /* for HELP <CR>		*/
#define INFO_FILE		TXT_PREFIX "info"	/* for INFO			*/
#define WIZLIST_FILE	TXT_PREFIX "wizlist"	/* for WIZLIST			*/
#define IMMLIST_FILE	TXT_PREFIX "immlist"	/* for IMMLIST			*/
#define BACKGROUND_FILE	TXT_PREFIX "background" /* for the background story	*/
#define POLICIES_FILE	TXT_PREFIX "policies"	/* player policies/rules	*/
#define HANDBOOK_FILE	TXT_PREFIX "handbook"	/* handbook for new immorts	*/
#define LOGIN_FILE		TXT_PREFIX "login"		/* first screen on connection */
#define WELCOME_FILE	TXT_PREFIX "welcome"	/* returning player message */
#define START_FILE		TXT_PREFIX "start"		/* new player message	*/
#define PLR_INDEX_FILE	PLR_PREFIX "plr_index" /* player index file		*/
#define HELP_FILE		HLP_PREFIX "help.hlp"

#define IDEA_FILE		MSC_PREFIX "ideas"	/* for the 'idea'-command	*/
#define TYPO_FILE		MSC_PREFIX "typos"	/*         'typo'		*/
#define BUG_FILE		MSC_PREFIX "bugs"	/*         'bug'		*/
#define FEEDBACK_FILE	MSC_PREFIX "feedback"	/*     'feedback'		*/
#define MESS_FILE		MSC_PREFIX "messages"	/* damage messages		*/
#define SOCMESS_FILE	MSC_PREFIX "socials"	/* messgs for social acts	*/
#define XNAME_FILE		MSC_PREFIX "xnames"	/* invalid name substrings	*/

#define MAIL_FILE		ETC_PREFIX "plrmail"	/* for the mudmail system	*/
#define BAN_FILE		ETC_PREFIX "badsites"	/* for the siteban system	*/
#define CLAN_FILE		ETC_PREFIX "clans"		/* For clan info		*/
#define HOUSE_FILE		HOUSE_DIR "houses"  /* for the house system		*/
#define GLOBALSCRIPTS_FILE		ETC_PREFIX "globalscripts"		/* For clan info		*/

/* public procedures in db.c */
void	boot_db(void);
void	zone_update(void);
IDNum	get_id_by_name(const char *name);
const char	*get_name_by_id(IDNum id, const char *def = NULL);

void	save_player_index(void);

void sprintbits(unsigned int vektor,char *outstring);

int	vnum_mobile(char *searchname, CharData *ch, ZoneData *zone = NULL);
int	vnum_object(char *searchname, CharData *ch, ZoneData *zone = NULL);
int vnum_trigger(char *searchname, CharData * ch, ZoneData *zone = NULL);

void vwear_object(int wearpos, CharData * ch);

extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;

#endif	//	__DB_H__
