/*
 * File: hedit.c
 * Comment: OLC for MUDs -- this one edits help files
 * By Steve Wolfe
 * for use with OasisOLC
 */



#include "help.h"
#include "descriptors.h"
#include "interpreter.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "buffer.h"
#include "handler.h"

#include "lexifile.h"

/* function protos */
void hedit_disp_menu(DescriptorData *d);
void hedit_parse(DescriptorData *d, const char *arg);
void hedit_setup_new(DescriptorData *d, char *arg);
void hedit_setup_existing(DescriptorData *d);
void hedit_save_to_disk();
void hedit_save_internally(DescriptorData *d);
void index_boot(int mode);


/*
 * Utils and exported functions.
 */
void hedit_setup_new(DescriptorData *d, char *arg)
{
	OLC_HELP(d) = new HelpManager::Entry;
	
	OLC_HELP(d)->m_Keywords = arg;
	OLC_HELP(d)->m_Entry = "This help entry is unfinished.\n";
	OLC_HELP(d)->m_MinimumLevel = 0;
	hedit_disp_menu(d);
	OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void hedit_setup_existing(DescriptorData *d)
{
	OLC_HELP(d) = new HelpManager::Entry(*OLC_ORIGINAL_HELP(d));
	OLC_VAL(d) = 0;
	hedit_disp_menu(d);
}


void hedit_save_internally(DescriptorData *d) {
	/* add a new help element into the list */
	if (OLC_ORIGINAL_HELP(d))
	{
		*OLC_ORIGINAL_HELP(d) = *OLC_HELP(d);
	}
	else
	{
		HelpManager::Instance()->Add(*OLC_HELP(d));
	}
	hedit_save_to_disk();
}


/*------------------------------------------------------------------------*/

void hedit_save_to_disk()
{
	HelpManager::Instance()->Save();
}

/*------------------------------------------------------------------------*/

/* Menu functions */

/* the main menu */
void hedit_disp_menu(DescriptorData *d)
{
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"`g1`n) Keywords    : `y%s\n"
			"`g2`n) Entry       :\n`y%s"
			"`g3`n) Min Level   : `y%d\n"
			"`gQ`n) Quit\n"
			"Enter choice : ",
			
			OLC_HELP(d)->m_Keywords.c_str(),
			OLC_HELP(d)->m_Entry.c_str(),
			OLC_HELP(d)->m_MinimumLevel);

	OLC_MODE(d) = HEDIT_MAIN_MENU;
}

/*
 * The main loop
 */

void hedit_parse(DescriptorData *d, const char *arg) {
	int i;

	switch (OLC_MODE(d)) {
		case HEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
					hedit_save_internally(d);
					mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s edits help '%s'.", d->m_Character->GetName(), OLC_HELP(d)->m_Keywords.c_str());
					send_to_char("Saving changes to help - no need to 'hedit save'!\n", d->m_Character);
					
					//	Fall through
					
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
				default:
					send_to_char("Invalid choice!\nDo you wish to save this help entry? ", d->m_Character);
					break;
			}
			return;			/* end of HEDIT_CONFIRM_SAVESTRING */

		case HEDIT_MAIN_MENU:
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d)) {		/* Something was modified */
						send_to_char("Do you wish to save this help entry? ", d->m_Character);
						OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
					} else
						cleanup_olc(d);
					break;
				case '1':
					send_to_char("Enter keywords:-\n| ", d->m_Character);
					OLC_MODE(d) = HEDIT_KEYWORDS;
					break;
				case '2':
					OLC_MODE(d) = HEDIT_ENTRY;
#if defined(CLEAR_SCREEN)
					d->Write("\x1B[H\x1B[J");
#endif
					d->Write("Enter the help info: (/s saves /h for help)\n\n");
					d->Write(OLC_HELP(d)->m_Entry.c_str());
					
					d->StartStringEditor(OLC_HELP(d)->m_Entry, MAX_STRING_LENGTH);
					OLC_VAL(d) = 1;
					break;
				case '3':
					send_to_char("Enter the minimum level: ", d->m_Character);
					OLC_MODE(d) = HEDIT_MIN_LEVEL;
					return;
				default:
					hedit_disp_menu(d);
					break;
			}
			return;
		
		case HEDIT_KEYWORDS:
			//if (strlen(arg) > MAX_HELP_KEYWORDS)	arg[MAX_HELP_KEYWORDS - 1] = '\0';
			OLC_HELP(d)->m_Keywords = *arg ? arg : "UNDEFINED";
			break;
			
		case HEDIT_ENTRY:
			/* should never get here */
			mudlog("SYSERR: Reached HEDIT_ENTRY in hedit_parse", BRF, LVL_BUILDER, TRUE);
			break;

		case HEDIT_MIN_LEVEL:
			if (*arg) {
				i = atoi(arg);
				if ((i < 0) || (i > LVL_CODER)) {
					hedit_disp_menu(d);
					return;
				} else
					OLC_HELP(d)->m_MinimumLevel = i;
			} else {
				hedit_disp_menu(d);
				return;
			}
			break;
		default:
			/* we should never get here */
			break;
	}
	OLC_VAL(d) = 1;
	hedit_disp_menu(d);
}
