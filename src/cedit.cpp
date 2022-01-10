/*************************************************************************
*   File: cedit.c++                  Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for characters                                    *
*************************************************************************/

#include "clans.h"
#include "descriptors.h"
#include "characters.h"
#include "olc.h"
#include "utils.h"
#include "comm.h"
#include "buffer.h"
#include "rooms.h"
#include "db.h"
#include "constants.h"

void cedit_parse(DescriptorData *d, const char *arg);
void cedit_setup(DescriptorData *d, Clan *clan);
void cedit_save_internally(DescriptorData *d);
void cedit_disp_menu(DescriptorData *d);
void cedit_disp_races(DescriptorData *d);



void cedit_setup(DescriptorData *d, Clan *clan)
{
	if (clan)
	{
			OLC_CLAN(d) = new Clan(*clan);
	}
	else
	{
		OLC_CLAN(d) = new Clan(OLC_NUM(d));
		OLC_CLAN(d)->m_Name = "New Clan";
		OLC_CLAN(d)->m_Tag = "New Clan";
	}
	
	OLC_VAL(d) = 0;
	
	cedit_disp_menu(d);
}


void cedit_save_internally(DescriptorData *d) {
	Clan *clan = Clan::GetClan(OLC_NUM(d));
	
	if (clan)
	{
		clan->m_Name = OLC_CLAN(d)->m_Name;
		clan->m_Tag = OLC_CLAN(d)->m_Tag;
		
		clan->m_Leader = OLC_CLAN(d)->m_Leader;
		clan->m_Room = OLC_CLAN(d)->m_Room;
		clan->m_Savings = OLC_CLAN(d)->m_Savings;
		clan->m_Races = OLC_CLAN(d)->m_Races;
		clan->m_MaxMembers = OLC_CLAN(d)->m_MaxMembers;
	}
	else
	{
		OLC_CLAN(d)->CreateDefaultRanks();
		
		Clan **	new_index = new Clan *[Clan::IndexSize + 1];
		
		unsigned	rnum;
		bool		found = false;
		for (rnum = 0; rnum < Clan::IndexSize; ++rnum)
		{
			if (!found)
			{
				if (Clan::Index[rnum]->GetID() > OLC_NUM(d))
				{
					found = true;
					new_index[rnum] = OLC_CLAN(d).Release();
					new_index[rnum]->m_RealNumber = rnum;					
					
					new_index[rnum + 1] = Clan::Index[rnum];
					++new_index[rnum + 1]->m_RealNumber;	
				}
				else
					new_index[rnum] = Clan::Index[rnum];
			}
			else
			{
				new_index[rnum + 1] = Clan::Index[rnum];
				++new_index[rnum + 1]->m_RealNumber;				
			}
		}
		
		if (!found)
		{
			new_index[rnum] = OLC_CLAN(d).Release();
			new_index[rnum]->m_RealNumber = rnum;
		}

		delete [] Clan::Index;
		Clan::Index = new_index;

		++Clan::IndexSize;
	}
	
	Clan::RebuildRelations();
	Clan::Save();
}


void cedit_disp_menu(DescriptorData *d)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	RoomData *	room;
	
	room = world.Find(OLC_CLAN(d)->m_Room);
	
	sprintbit(OLC_CLAN(d)->m_Races, race_types, buf);
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Clan Number:  [`c%d`n]\n"
				"`G1`n) Name        : `y%s\n"
				"`G2`n) Tag         : `y%s\n"
				"`G3`n) Owner       : `y%s\n"
				"`G4`n) Room        : `y%s `n[`c%s`n]\n"
				"`G5`n) Savings     : `y%d\n"
				"`G6`n) Races       : `y%s\n"
				"`G7`n) Member Limit: `y%d%s\n"
				"`GQ`n) Quit\n"
				"Enter choice : ",
				OLC_NUM(d),
				OLC_CLAN(d)->GetName(),
				OLC_CLAN(d)->GetTag(),
				get_name_by_id(OLC_CLAN(d)->m_Leader, "<NONE>"),
				room ? room->GetName() : "<Not Created>" , OLC_CLAN(d)->GetRoom().Print().c_str(),
				OLC_CLAN(d)->m_Savings,
				OLC_CLAN(d)->m_Races ? buf : "<ANY>",
				OLC_CLAN(d)->m_MaxMembers, OLC_CLAN(d)->m_MaxMembers == 0 ? " (Unlimited)" : "");

	OLC_MODE(d) = CEDIT_MAIN_MENU;
}


//	Display Opinion races
void cedit_disp_races(DescriptorData *d)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	for (int i = 0; i < NUM_RACES; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i + 1, race_types[i]);
	}

	sprintbit(OLC_CLAN(d)->m_Races, race_types, buf);
	d->m_Character->Send("`nCurrent races   : `c%s\n"
				 "`nEnter races (0 to quit): ", buf);
	
	OLC_MODE(d) = CEDIT_RACES;
}


void cedit_parse(DescriptorData *d, const char *arg)
{
	switch (OLC_MODE(d)) {
		case CEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
					send_to_char("Saving changes to clan - no need to 'cedit save'!\n", d->m_Character);
					cedit_save_internally(d);
					mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s edits clan %d", d->m_Character->GetName(), OLC_NUM(d));
					// Fall through
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
				default:
				send_to_char("Invalid choice!\n"
							 "Do you wish to save the clan? ", d->m_Character);
			}
			return;
		case CEDIT_MAIN_MENU:
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d)) {
						send_to_char("Do you wish to save this clan? ", d->m_Character);
						OLC_MODE(d) = CEDIT_CONFIRM_SAVESTRING;
					} else
						cleanup_olc(d);
					return;
				case '1':
					OLC_MODE(d) = CEDIT_NAME;
					send_to_char("Enter new name:\n] ", d->m_Character);
					return;
				case '2':
					OLC_MODE(d) = CEDIT_TAG;
					send_to_char("Enter new tag:\n] ", d->m_Character);
					return;
				case '3':
					OLC_MODE(d) = CEDIT_OWNER;
					send_to_char("Enter owners name: ", d->m_Character);
					return;
				case '4':
					OLC_MODE(d) = CEDIT_ROOM;
					send_to_char("Enter clan room: ", d->m_Character);
					return;
				case '5':
					OLC_MODE(d) = CEDIT_SAVINGS;
					send_to_char("Enter clan's total savings: ", d->m_Character);
					return;
				case '6':
					cedit_disp_races(d);
					return;
				case '7':
					OLC_MODE(d) = CEDIT_MEMBERLIMIT;
					send_to_char("Enter clan's member limit (0 for no limit): ", d->m_Character);
					return;
				default:
					cedit_disp_menu(d);
					return;
			}
			break;
		case CEDIT_NAME:
			if (*arg)	OLC_CLAN(d)->m_Name = arg;
			break;
		case CEDIT_TAG:
			if (*arg)	OLC_CLAN(d)->m_Tag = arg;
			break;
		case CEDIT_OWNER:
			OLC_CLAN(d)->m_Leader = get_id_by_name(arg);
			break;
		case CEDIT_ROOM:
			{
				RoomData *room = world.Find(arg);
				OLC_CLAN(d)->m_Room = room ? room->GetVirtualID() : VirtualID();
			}
			break;
		case CEDIT_SAVINGS:
			OLC_CLAN(d)->m_Savings = atoi(arg);
			break;
		case CEDIT_RACES:
			{
				int i = atoi(arg);
				if (i)
				{
					if (i > 0 && i <= NUM_RACES)
						TOGGLE_BIT(OLC_CLAN(d)->m_Races, 1 << (i - 1));
					cedit_disp_races(d);
					return;
				}
			}
			break;
		case CEDIT_MEMBERLIMIT:
			OLC_CLAN(d)->m_MaxMembers = RANGE(atoi(arg), 0, INT_MAX);
			break;		
		default:
			mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: cedit_parse(): Reached default case!  Case: %d", OLC_MODE(d));
			send_to_char("Oops...\n", d->m_Character);
			break;
	}
	OLC_VAL(d) = 1;
	cedit_disp_menu(d);
}
