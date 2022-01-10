/*************************************************************************
*   File: socials.h                  Part of Aliens vs Predator: The MUD *
*  Usage: Header file for socials                                        *
*************************************************************************/

#ifndef __SOCIALS_H__
#define __SOCIALS_H__

#include "types.h"
#include "characters.h"
#include "lexistring.h"

class Social
{
public:
						Social();

	int					act_nr;
	Lexi::String		command;               /* holds copy of activating command */
	Lexi::String		sort_as;		/* holds a copy of a similar command or
				 * abbreviation to sort by for the parser */
	bool				hide;			/* ? */
	Position			min_victim_position;	/* Position of victim */
	Position			min_char_position;	/* Position of char */
	int					min_level_char;          /* Minimum level of socialing char */

	/* No argument was supplied */
	Lexi::String		char_no_arg;
	Lexi::String		others_no_arg;

	/* An argument was there, and a victim was found */
	Lexi::String		char_found;
	Lexi::String		others_found;
	Lexi::String		vict_found;

	/* An argument was there, as well as a body part, and a victim was found */
	Lexi::String		char_body_found;
	Lexi::String		others_body_found;
	Lexi::String		vict_body_found;

	/* An argument was there, but no victim was found */
	Lexi::String		not_found;

	/* The victim turned out to be the character */
	Lexi::String		char_auto;
	Lexi::String		others_auto;

	/* If the char cant be found search the char's inven and do these: */
	Lexi::String		char_obj_found;
	Lexi::String		others_obj_found;

						
	static bool SortFunc(const Social *a, const Social *b);
};

//extern int				top_of_socialt;
//extern class Social **	socials;
typedef std::vector<Social *>	SocialVector;
extern SocialVector			socials;


#endif

