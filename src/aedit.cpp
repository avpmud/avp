/*
 * File: aedit.c
 * Comment: OLC for MUDs -- this one edits socials
 * by Michael Scott <scottm@workcomm.net> -- 06/10/96
 * for use with OasisOLC
 * ftpable from ftp.circlemud.org:/pub/CircleMUD/contrib/code
 */



#include "characters.h"
#include "descriptors.h"
#include "interpreter.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "socials.h"
#include "constants.h"
#include "buffer.h"

/* WARNING: if you have added diagonal directions and have them at the
 * beginning of the command list.. change this value to 11 or 15 (depending) */
/* reserve these commands to come straight from the cmd list then start
 * sorting */
#define RESERVE_CMDS		7

/* external functs */
void sort_commands(void); /* aedit patch -- M. Scott */
void create_command_list(void);


/* function protos */
void aedit_disp_menu(DescriptorData * d);
void aedit_disp_positions_menu(DescriptorData * d);
void aedit_parse(DescriptorData * d, const char *arg);
void aedit_setup_new(DescriptorData *d);
void aedit_setup_existing(DescriptorData *d);
void aedit_save_to_disk();
void aedit_save_internally(DescriptorData *d);



/*
 * Utils and exported functions.
 */

void aedit_setup_new(DescriptorData *d)
{
	OLC_ACTION(d) = new Social;
	
	OLC_ACTION(d)->command = OLC_STORAGE(d);
	OLC_ACTION(d)->sort_as = OLC_STORAGE(d);
	OLC_ACTION(d)->hide    = 0;
	OLC_ACTION(d)->min_victim_position = POS_STANDING;
	OLC_ACTION(d)->min_char_position   = POS_STANDING;
	OLC_ACTION(d)->min_level_char      = 0;
	OLC_ACTION(d)->char_no_arg = "This action is unfinished.";
	OLC_ACTION(d)->others_no_arg = "This action is unfinished.";
	aedit_disp_menu(d);
	OLC_VAL(d) = 0;
	OLC_NUM(d) = -1;
}

/*------------------------------------------------------------------------*/

void aedit_setup_existing(DescriptorData *d)
{
	OLC_ACTION(d) = new Social(*socials[OLC_NUM(d)]);
	OLC_VAL(d) = 0;
	aedit_disp_menu(d);
}


      
void aedit_save_internally(DescriptorData *d)
{
	/* add a new social into the list */
	if (OLC_NUM(d) < 0)
	{
		socials.push_back(OLC_ACTION(d).Release());
		
		create_command_list();
		sort_commands();
	}
	/* pass the editted action back to the list - no need to add */
	else
	{
		int i = find_command(OLC_ACTION(d)->command.c_str());
		OLC_ACTION(d)->act_nr = socials[OLC_NUM(d)]->act_nr;
		*socials[OLC_NUM(d)] = *OLC_ACTION(d);
		if (i > -1) {
			complete_cmd_info[i].command = (char *)socials[OLC_NUM(d)]->command.c_str();
			complete_cmd_info[i].sort_as = (char *)socials[OLC_NUM(d)]->sort_as.c_str();
			complete_cmd_info[i].minimum_position = socials[OLC_NUM(d)]->min_char_position;
			complete_cmd_info[i].minimum_level	   = socials[OLC_NUM(d)]->min_level_char;
		}
	}
	aedit_save_to_disk();
}


/*------------------------------------------------------------------------*/

void aedit_save_to_disk() {
	FILE *fp;
	int i;
	
	if (!(fp = fopen(SOCMESS_FILE, "w+")))  {
		log("SYSERR: Can't open socials file '%s'", SOCMESS_FILE);
		exit(1);
	}

	for (i = 0; i < socials.size(); i++)  {
		fprintf(fp, "~%s %s %d %d %d %d\n",
				socials[i]->command.c_str(),
				socials[i]->sort_as.c_str(),
				socials[i]->hide,
				socials[i]->min_char_position,
				socials[i]->min_victim_position,
				socials[i]->min_level_char);
		fprintf(fp, "%s\n%s\n%s\n%s\n",
				((!socials[i]->char_no_arg.empty())?socials[i]->char_no_arg.c_str():"#"),
				((!socials[i]->others_no_arg.empty())?socials[i]->others_no_arg.c_str():"#"),
				((!socials[i]->char_found.empty())?socials[i]->char_found.c_str():"#"),
				((!socials[i]->others_found.empty())?socials[i]->others_found.c_str():"#"));
		fprintf(fp, "%s\n%s\n%s\n%s\n",
				((!socials[i]->vict_found.empty())?socials[i]->vict_found.c_str():"#"),
				((!socials[i]->not_found.empty())?socials[i]->not_found.c_str():"#"),
				((!socials[i]->char_auto.empty())?socials[i]->char_auto.c_str():"#"),
				((!socials[i]->others_auto.empty())?socials[i]->others_auto.c_str():"#"));
		fprintf(fp, "%s\n%s\n%s\n",
				((!socials[i]->char_body_found.empty())?socials[i]->char_body_found.c_str():"#"),
				((!socials[i]->others_body_found.empty())?socials[i]->others_body_found.c_str():"#"),
				((!socials[i]->vict_body_found.empty())?socials[i]->vict_body_found.c_str():"#"));
		fprintf(fp, "%s\n%s\n\n",
				((!socials[i]->char_obj_found.empty())?socials[i]->char_obj_found.c_str():"#"),
				((!socials[i]->others_obj_found.empty())?socials[i]->others_obj_found.c_str():"#"));
	}
   
	fprintf(fp, "$\n");
	fclose(fp);
}

/*------------------------------------------------------------------------*/

/* Menu functions */

/* the main menu */
void aedit_disp_menu(DescriptorData * d) {
	Social *action = OLC_ACTION(d)/*.Get()*/;
	CharData *ch        = d->m_Character;
	
	d->m_Character->Send("\x1B[H\x1B[J"
			"`n-- Action editor\n\n"
			"`gn`n) Command         : `y%-15.15s `g1`n) Sort as Command  : `y%-15.15s\n"
			"`g2`n) Min Position[CH]: `c%-8.8s        `g3`n) Min Position [VT]: `c%-8.8s\n"
			"`g4`n) Min Level   [CH]: `c%-3d             `g5`n) Show if Invisible: `c%s\n"
			"`ga`n) Char    [NO ARG]: `c%s\n"
			"`gb`n) Others  [NO ARG]: `c%s\n"
			"`gc`n) Char [NOT FOUND]: `c%s\n"
			"`gd`n) Char  [ARG SELF]: `c%s\n"
			"`ge`n) Others[ARG SELF]: `c%s\n"
			"`gf`n) Char      [VICT]: `c%s\n"
			"`gg`n) Others    [VICT]: `c%s\n"
			"`gh`n) Victim    [VICT]: `c%s\n"
			"`gi`n) Char  [BODY PRT]: `c%s\n"
			"`gj`n) Others[BODY PRT]: `c%s\n"
			"`gk`n) Victim[BODY PRT]: `c%s\n"
			"`gl`n) Char       [OBJ]: `c%s\n"
			"`gm`n) Others     [OBJ]: `c%s\n"
			"`gq`n) Quit\n",
			action->command.c_str(), action->sort_as.c_str(),
			GetEnumName(action->min_char_position), GetEnumName(action->min_victim_position),
			action->min_level_char, (action->hide?"HIDDEN" : "NOT HIDDEN"),
			!action->char_no_arg.empty() ? action->char_no_arg.c_str() : "<Null>",
			!action->others_no_arg.empty() ? action->others_no_arg.c_str() : "<Null>",
			!action->not_found.empty() ? action->not_found.c_str() : "<Null>",
			!action->char_auto.empty() ? action->char_auto.c_str() : "<Null>",
			!action->others_auto.empty() ? action->others_auto.c_str() : "<Null>",
			!action->char_found.empty() ? action->char_found.c_str() : "<Null>",
			!action->others_found.empty() ? action->others_found.c_str() : "<Null>",
			!action->vict_found.empty() ? action->vict_found.c_str() : "<Null>",
			!action->char_body_found.empty() ? action->char_body_found.c_str() : "<Null>",
			!action->others_body_found.empty() ? action->others_body_found.c_str() : "<Null>",
			!action->vict_body_found.empty() ? action->vict_body_found.c_str() : "<Null>",
			!action->char_obj_found.empty() ? action->char_obj_found.c_str() : "<Null>",
			!action->others_obj_found.empty() ? action->others_obj_found.c_str() : "<Null>");

	d->m_Character->Send("Enter choice: ");

	OLC_MODE(d) = AEDIT_MAIN_MENU;
}


void aedit_disp_positions_menu(DescriptorData * d) {
	int counter;
	
	for (counter = 0; counter < NUM_POSITIONS; counter++)
		d->m_Character->Send("`g%2d`n) %-20.20s \n", counter, GetEnumName((Position) counter));
}

/*
 * The main loop
 */

void aedit_parse(DescriptorData * d, const char *arg)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	int i;

	switch (OLC_MODE(d)) {
		case AEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y': case 'Y':
					send_to_char("Saving changes to action - no need to 'aedit save'!\n", d->m_Character);
					mudlogf(NRM, LVL_SRSTAFF, TRUE, "OLC: %s edits action %s", d->m_Character->GetName(),
							OLC_ACTION(d)->command.c_str());
					aedit_save_internally(d);
					
					//	Fall through!

				case 'n': case 'N':
					/* free everything up, including strings etc */
					cleanup_olc(d);
					break;
					
				default:
					send_to_char("Invalid choice!\nDo you wish to save this action? ", d->m_Character);
					break;
			}
			return; /* end of AEDIT_CONFIRM_SAVESTRING */

		case AEDIT_CONFIRM_EDIT:
			switch (*arg)  {
				case 'y': case 'Y':
					aedit_setup_existing(d);
					break;
				case 'q': case 'Q':
					cleanup_olc(d);
					break;
				case 'n': case 'N':
					for (++OLC_NUM(d); (OLC_NUM(d) < socials.size()); ++OLC_NUM(d))
						if (is_abbrev(OLC_STORAGE(d).c_str(), socials[OLC_NUM(d)]->command.c_str()))
							break;
						
					if (OLC_NUM(d) >= socials.size())
					{
						if (find_command(OLC_STORAGE(d).c_str()) > -1)
						{
							cleanup_olc(d);
							break;
						}
						d->m_Character->Send("Do you wish to add the '%s' action? ", OLC_STORAGE(d).c_str());
						OLC_MODE(d) = AEDIT_CONFIRM_ADD;
					}
					else
					{
						d->m_Character->Send("Do you wish to edit the '%s' action? ", socials[OLC_NUM(d)]->command.c_str());
						OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
					}
					break;
				default:
					d->m_Character->Send("Invalid choice!\nDo you wish to edit the '%s' action? ", socials[OLC_NUM(d)]->command.c_str());
					break;
			}
			return;

		case AEDIT_CONFIRM_ADD:
			switch (*arg)  {
				case 'y': case 'Y':
					aedit_setup_new(d);
					break;
				case 'n': case 'N': case 'q': case 'Q':
					cleanup_olc(d);
					break;
				default:
					d->m_Character->Send("Invalid choice!\nDo you wish to add the '%s' action? ", OLC_STORAGE(d).c_str());
					break;
			}
			return;

		case AEDIT_MAIN_MENU:
			switch (*arg) {
				case 'q': case 'Q':
					if (OLC_VAL(d))  { /* Something was modified */
						send_to_char("Do you wish to save this action? ", d->m_Character);
						OLC_MODE(d) = AEDIT_CONFIRM_SAVESTRING;
					} else
						cleanup_olc(d);
					break;
				case 'n':
					send_to_char("Enter action name: ", d->m_Character);
					OLC_MODE(d) = AEDIT_ACTION_NAME;
					break;
				case '1':
					send_to_char("Enter sort info for this action (for the command listing): ", d->m_Character);
					OLC_MODE(d) = AEDIT_SORT_AS;
					break;
				case '2':
					aedit_disp_positions_menu(d);
					send_to_char("Enter the minimum position the Character has to be in to activate social [0 - 8]: ", d->m_Character);
					OLC_MODE(d) = AEDIT_MIN_CHAR_POS;
					break;
				case '3':
					aedit_disp_positions_menu(d);
					send_to_char("Enter the minimum position the Victim has to be in to activate social [0 - 8]: ", d->m_Character);
					OLC_MODE(d) = AEDIT_MIN_VICT_POS;
					break;
				case '4':
					send_to_char("Enter new minimum level for social: ", d->m_Character);
					OLC_MODE(d) = AEDIT_MIN_CHAR_LEVEL;
					break;
				case '5':
					OLC_ACTION(d)->hide = !OLC_ACTION(d)->hide;
					aedit_disp_menu(d);
					OLC_VAL(d) = 1;
					break;
				case 'a': case 'A':
					d->m_Character->Send("Enter social shown to the Character when there is no argument supplied.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->char_no_arg.empty())?OLC_ACTION(d)->char_no_arg.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_NOVICT_CHAR;
					break;
				case 'b': case 'B':
					d->m_Character->Send("Enter social shown to Others when there is no argument supplied.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->others_no_arg.empty())?OLC_ACTION(d)->others_no_arg.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_NOVICT_OTHERS;
					break;
				case 'c': case 'C':
					d->m_Character->Send("Enter text shown to the Character when his victim isnt found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->not_found.empty())?OLC_ACTION(d)->not_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_VICT_NOT_FOUND;
					break;
				case 'd': case 'D':
					d->m_Character->Send("Enter social shown to the Character when it is its own victim.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->char_auto.empty())?OLC_ACTION(d)->char_auto.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_SELF_CHAR;
					break;
				case 'e': case 'E':
					d->m_Character->Send("Enter social shown to Others when the Char is its own victim.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->others_auto.empty())?OLC_ACTION(d)->others_auto.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_SELF_OTHERS;
					break;
				case 'f': case 'F':
					d->m_Character->Send("Enter normal social shown to the Character when the victim is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->char_found.empty())?OLC_ACTION(d)->char_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_VICT_CHAR_FOUND;
					break;
				case 'g': case 'G':
					d->m_Character->Send("Enter normal social shown to Others when the victim is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->others_found.empty())?OLC_ACTION(d)->others_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_VICT_OTHERS_FOUND;
					break;
				case 'h': case 'H':
					d->m_Character->Send("Enter normal social shown to the Victim when the victim is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->vict_found.empty())?OLC_ACTION(d)->vict_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_VICT_VICT_FOUND;
					break;
				case 'i': case 'I':
					d->m_Character->Send("Enter 'body part' social shown to the Character when the victim is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->char_body_found.empty())?OLC_ACTION(d)->char_body_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_VICT_CHAR_BODY_FOUND;
					break;
				case 'j': case 'J':
					d->m_Character->Send("Enter 'body part' social shown to Others when the victim is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->others_body_found.empty())?OLC_ACTION(d)->others_body_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_VICT_OTHERS_BODY_FOUND;
					break;
				case 'k': case 'K':
					d->m_Character->Send("Enter 'body part' social shown to the Victim when the victim is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->vict_body_found.empty())?OLC_ACTION(d)->vict_body_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_VICT_VICT_BODY_FOUND;
					break;
				case 'l': case 'L':
					d->m_Character->Send("Enter 'object' social shown to the Character when the object is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->char_obj_found.empty())?OLC_ACTION(d)->char_obj_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_OBJ_CHAR_FOUND;
					break;
				case 'm': case 'M':
					d->m_Character->Send("Enter 'object' social shown to the Room when the object is found.\n[OLD]: %s\n[NEW]: ",
							((!OLC_ACTION(d)->others_obj_found.empty())?OLC_ACTION(d)->others_obj_found.c_str():"NULL"));
					OLC_MODE(d) = AEDIT_OBJ_OTHERS_FOUND;
					break;
				default:
					aedit_disp_menu(d);
					break;
			}
			return;

		case AEDIT_ACTION_NAME:
			if (*arg) {
				if (strchr(arg,' ')) {
					aedit_disp_menu(d);
					return;
				} else {
					OLC_ACTION(d)->command = arg;
				}
			} else {
				aedit_disp_menu(d);
				return;
			}
			break;

		case AEDIT_SORT_AS:
			if (*arg) {
				if (strchr(arg,' ')) {
					aedit_disp_menu(d);
					return;
				} else  {
					OLC_ACTION(d)->sort_as = arg;
				}
			} else {
				aedit_disp_menu(d);
				return;
			}
			break;

		case AEDIT_MIN_CHAR_POS:
		case AEDIT_MIN_VICT_POS:
			if (*arg)  {
				i = atoi(arg);
				if ((unsigned)i >= NUM_POSITIONS)
				{
					aedit_disp_menu(d);
					return;
				}
				else
				{
					if (OLC_MODE(d) == AEDIT_MIN_CHAR_POS)
						OLC_ACTION(d)->min_char_position = (Position)i;
					else
						OLC_ACTION(d)->min_victim_position = (Position)i;
				}
			} else  {
				aedit_disp_menu(d);
				return;
			}
			break;

		case AEDIT_MIN_CHAR_LEVEL:
			if (*arg)  {
				i = atoi(arg);
				if ((i < 0) || (i > LVL_CODER))  {
					aedit_disp_menu(d);
					return;
				} else
					OLC_ACTION(d)->min_level_char = i;
			} else  {
				aedit_disp_menu(d);
				return;
			}
			break;

		case AEDIT_NOVICT_CHAR:
			strcpy(buf, arg);
			OLC_ACTION(d)->char_no_arg = delete_doubledollar(buf);
			break;

		case AEDIT_NOVICT_OTHERS:
			strcpy(buf, arg);
			OLC_ACTION(d)->others_no_arg = delete_doubledollar(buf);
			break;

		case AEDIT_VICT_CHAR_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->char_found = delete_doubledollar(buf);
			break;

		case AEDIT_VICT_OTHERS_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->others_found = delete_doubledollar(buf);
			break;

		case AEDIT_VICT_VICT_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->vict_found = delete_doubledollar(buf);
			break;

		case AEDIT_VICT_NOT_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->not_found = delete_doubledollar(buf);
			break;

		case AEDIT_SELF_CHAR:
			strcpy(buf, arg);
			OLC_ACTION(d)->char_auto = delete_doubledollar(buf);
			break;

		case AEDIT_SELF_OTHERS:
			strcpy(buf, arg);
			OLC_ACTION(d)->others_auto = delete_doubledollar(buf);
			break;

		case AEDIT_VICT_CHAR_BODY_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->char_body_found = delete_doubledollar(buf);
			break;

		case AEDIT_VICT_OTHERS_BODY_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->others_body_found = delete_doubledollar(buf);
			break;

		case AEDIT_VICT_VICT_BODY_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->vict_body_found = delete_doubledollar(buf);
			break;

		case AEDIT_OBJ_CHAR_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->char_obj_found = delete_doubledollar(buf);
			break;

		case AEDIT_OBJ_OTHERS_FOUND:
			strcpy(buf, arg);
			OLC_ACTION(d)->others_obj_found = delete_doubledollar(buf);
			break;

		default:
			/* we should never get here */
			break;
	}
	OLC_VAL(d) = 1;
	aedit_disp_menu(d);
}
