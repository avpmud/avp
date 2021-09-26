/* *************************************************************************
*   File: clan.h				     Addition to CircleMUD *
*  Usage: Contains structure definitions for clan.c                        *
*                                                                          *
*  Written by Daniel Muller                                                *
*                                                                          *
************************************************************************* */

#include "index.h"

struct ClanData {
	char *	title;
	char *	description;
	SInt32	owner;
	VNum	room;
	VNum	vnum;
	UInt32	savings;
	struct int_list *members;
};

#define PLAYERCLAN(ch)			((ch)->player_specials.saved.clannum)
#define PLAYERCLANNUM(ch)		(cross_reference[(ch)->player_specials.saved.clannum])
#define	GET_CLAN_VNUM(clan)		((clan).vnum)
#define	CLANRANK(ch)			((ch)->player_specials.saved.clanrank)
#define CLANPLAYERS(clan) 		((clan).nummembers)	
#define CLANOWNER(clan)   		((clan).owner)
#define CLANPOINTS(clan)		((clan).savings)
#define CLANROOM(clan)			((clan).room)
