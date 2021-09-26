/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - redit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*. Original author: Levork .*/




#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "scripts.h"
#include "db.h"
#include "olc.h"
#include "buffer.h"
#include "extradesc.h"
#include "staffcmds.h"
//#include "space.h"
#include "constants.h"


#include "lexifile.h"

/*------------------------------------------------------------------------*/

/*
 * Function Prototypes
 */


/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void REdit::setup_new(DescriptorData *d) {
	OLC_ROOM(d) = new RoomData(OLC_VID(d));
	OLC_ROOM(d)->SetName("An unfinished room");
	OLC_ROOM(d)->SetDescription("You are in an unfinished room.\n");
	REdit::disp_menu(d);
	OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void REdit::setup_existing(DescriptorData *d, RoomData *room) {
	//	Attatch room copy to players descriptor
	OLC_ROOM(d) = new RoomData(*room);
	OLC_VAL(d) = 0;
	REdit::disp_menu(d);
}


/*------------------------------------------------------------------------*/
      
void REdit::save_internally(DescriptorData *d)
 {
	OLC_ROOM(d)->m_Flags.clear(ROOM_DELETED);
	
	if (OLC_ROOM(d)->m_Flags.test(ROOM_STARTHIVED))
		OLC_ROOM(d)->m_Flags.set(ROOM_HIVED);
			
	RoomData *room = world.Find(OLC_VID(d));
	if (room)
	{
		*room = *OLC_ROOM(d);
	}
	else
	{
		room = OLC_ROOM(d).Release();
//		room->m_Zone = OLC_SOURCE_ZONE(d);
		
		world.insert(room);
	}

	REdit::save_to_disk(OLC_SOURCE_ZONE(d));
}


/*------------------------------------------------------------------------*/

void REdit::save_to_disk(ZoneData *zone) {
	BUFFER(fname, MAX_STRING_LENGTH);
	
	sprintf(fname, WLD_PREFIX "%s.new", zone->GetTag());
	Lexi::OutputParserFile	file(fname);

	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open room file \"%s\"!", fname);
		return;
	}
	
	file.BeginParser();
	
	FOREACH_BOUNDED(RoomMap, world, zone->GetHash(), i)
	{
		RoomData *room = *i;
		
		if (ROOM_FLAGGED(room, ROOM_DELETED))
			continue;
		
		file.BeginSection(Lexi::Format("Room %s", room->GetVirtualID().Print().c_str()));
		room->Write(file);
		file.EndSection();	//	Room
	}

	file.EndParser();
	file.close();
	
	BUFFER(buf2, MAX_STRING_LENGTH);
	sprintf(buf2, WLD_PREFIX "%s" WLD_SUFFIX, zone->GetTag());
	remove(buf2);
	rename(fname, buf2);
}


/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void REdit::disp_extradesc_menu(DescriptorData * d) {
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Extra desc menu\n");
	
	int count = 0;
	FOREACH(ExtraDesc::List, OLC_ROOM(d)->m_ExtraDescriptions, extradesc)
	{
		d->m_Character->Send(
			"`g%2d`n) %s%s\n",
			++count,
			extradesc->keyword.empty() ? "<NONE>" : extradesc->keyword.c_str(),
			extradesc->description.empty() ? " (NO DESC)" : "");
	}
	if (count == 0)
		d->m_Character->Send("<No Extra Descriptions Defined>\n");
	
	d->m_Character->Send(
			"`gA`n) Add\n"
			"`gD`n) Delete\n"
			"`gQ`n) Quit\n"
			"Enter choice: ");

	OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}


void REdit::disp_extradesc(DescriptorData * d) {
	ExtraDesc *extra_desc = OLC_DESC(d);

	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Extra desc menu\n"
			"`g1`n) Keyword: `y%s\n"
			"`g2`n) Description:\n`y%s\n"
			"`gQ`n) Quit\n"
			"Enter choice : ",
			extra_desc->keyword.empty() ? "<NONE>" : extra_desc->keyword.c_str(),
			extra_desc->description.empty() ? "<NONE>" : extra_desc->description.c_str());

	OLC_MODE(d) = REDIT_EXTRADESC;
}


/*
 * For exits.
 */
void REdit::disp_exit_menu(DescriptorData * d) {
	/*
	 * if exit doesn't exist, alloc/create it 
	 */
	if(!OLC_EXIT(d))	OLC_EXIT(d) = new RoomExit;

	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"`g1`n) Exit to     : `c%s\n"
				"`g2`n) Description :-\n`y%s\n"
				"`g3`n) Door name   : `y%s\n"
				"`g4`n) Key         : `c%s\n"
				"`g5`n) Door flags  : `c%s\n"
				"`g6`n) Purge exit.\n"
				"Enter choice, 0 to quit : ",
			OLC_EXIT(d)->ToRoom() ? OLC_EXIT(d)->ToRoom()->GetVirtualID().Print().c_str() : "<NOWHERE>",
			*OLC_EXIT(d)->GetDescription() ? OLC_EXIT(d)->GetDescription() : "<NONE>",
			OLC_EXIT(d)->GetKeyword(),
			OLC_EXIT(d)->key.Print().c_str(),
			OLC_EXIT(d)->m_Flags.any() ? OLC_EXIT(d)->m_Flags.print().c_str() : "None");
	
	OLC_MODE(d) = REDIT_EXIT_MENU;
}


/*
 * For exit flags.
 */
void REdit::disp_exit_flag_menu(DescriptorData * d) {
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (int counter = 0; counter < NUM_EXIT_FLAGS; ++counter) {
		d->m_Character->Send("`g%2d`n) %-20.20s %s", counter + 1, GetEnumName<ExitFlag>(counter),
				(counter % 2) ? "\n" : "");
	}
	d->m_Character->Send(
			"\nExit flags: `c%s`n\n"
			"Enter room flags, 0 to quit : ",
			OLC_EXIT(d)->m_Flags.print().c_str());

	OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
}


/*
 * For room flags.
 */
void REdit::disp_flag_menu(DescriptorData * d) {
	int counter, columns = 0;

#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_OLC_ROOM_FLAGS; counter++) {
		d->m_Character->Send("`g%2d`n) %-20.20s %s", counter + 1, GetEnumName((RoomFlag)counter),
				!(++columns % 2) ? "\n" : "");
	}
	d->m_Character->Send(
			"\nRoom flags: `c%s`n\n"
			"Enter room flags, 0 to quit : ",
			OLC_ROOM(d)->m_Flags.print().c_str());

	OLC_MODE(d) = REDIT_FLAGS;
}


//	For sector type.
void REdit::disp_sector_menu(DescriptorData * d) {
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (int counter = 0; counter < NUM_SECTORS; counter++) {
		d->m_Character->Send("`g%2d`n) %-20.20s %s", counter, GetEnumName((RoomSector)counter),
				(counter % 2) ? "\n" : "");
	}
	send_to_char("\nEnter sector type : ", d->m_Character);
	OLC_MODE(d) = REDIT_SECTOR;
}


//	The main menu.
void REdit::disp_menu(DescriptorData * d)
{
	RoomData *room = OLC_ROOM(d)/*.Get()*/;

	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Room number : [`c%s`n]\n"
				"`G1`n) Name        : `y%s\n"
				"`G2`n) Description :\n`y%s",
			OLC_VID(d).Print().c_str(),
			room->GetName(),
			room->GetDescription());

	if (STF_FLAGGED(d->m_Character, STAFF_OLCADMIN )
		|| (STF_FLAGGED(d->m_Character, /*STAFF_MINIOLC |*/ STAFF_OLC)
			&& OLC_SOURCE_ZONE(d)->IsBuilder(d->m_Character->GetID(), OLC_VID(d).ns)))
	{
		d->m_Character->Send(
					"`G3`n) Room flags  : `c%s\n"
					"`G4`n) Sector type : `c%s\n"
					"`G5`n) Exit north  : `c%s\n"
					"`G6`n) Exit east   : `c%s\n"
					"`G7`n) Exit south  : `c%s\n"
					"`G8`n) Exit west   : `c%s\n"
					"`G9`n) Exit up     : `c%s\n"
					"`GA`n) Exit down   : `c%s\n",
				room->m_Flags.print().c_str(),
				GetEnumName(room->GetSector()),
				room->m_Exits[NORTH]&& room->m_Exits[NORTH]->ToRoom()	?	room->m_Exits[NORTH]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
				room->m_Exits[EAST]	&& room->m_Exits[EAST]->ToRoom()	?	room->m_Exits[EAST]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
				room->m_Exits[SOUTH]&& room->m_Exits[SOUTH]->ToRoom()	?	room->m_Exits[SOUTH]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
				room->m_Exits[WEST]	&& room->m_Exits[WEST]->ToRoom()	?	room->m_Exits[WEST]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
				room->m_Exits[UP]	&& room->m_Exits[UP]->ToRoom()		?	room->m_Exits[UP]->ToRoom()->GetVirtualID().Print().c_str()		: "<NOWHERE>",
				room->m_Exits[DOWN]	&& room->m_Exits[DOWN]->ToRoom()	?	room->m_Exits[DOWN]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>");
	}
	
	d->m_Character->Send(
				"`GB`n) Extra descriptions menu\n"
				"`GQ`n) Quit\n"
				"Enter choice : ");

	OLC_MODE(d) = REDIT_MAIN_MENU;
}



/**************************************************************************
 * The main loop
 **************************************************************************/

void REdit::parse(DescriptorData * d, const char *arg) {
	int number;

	switch (OLC_MODE(d)) {
		case REDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
					REdit::save_internally(d);
					mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s edits room %s.", d->m_Character->GetName(), OLC_VID(d).Print().c_str());
					send_to_char("Saving changes to room - no need to 'redit save'!\n", d->m_Character);
					
					//	Fall through
					
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
				default:
					send_to_char("Invalid choice!\n"
								 "Do you wish to save this room? ", d->m_Character);
					break;
			}
			return;
		case REDIT_MAIN_MENU:
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d)) { /* Something has been modified. */
						send_to_char("Do you wish to save this room? ", d->m_Character);
						OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
					} else
						cleanup_olc(d);
					break;
				case '1':
					send_to_char("Enter room name:-\n| ", d->m_Character);
					OLC_MODE(d) = REDIT_NAME;
					break;
				case '2':
					OLC_MODE(d) = REDIT_DESC;
#if defined(CLEAR_SCREEN)
					d->Write("\x1B[H\x1B[J");
#endif
					d->Write("Enter room description: (/s saves /h for help)\n\n");
					d->Write(OLC_ROOM(d)->GetDescription());
					
					d->StartStringEditor(OLC_ROOM(d)->m_pDescription, MAX_ROOM_DESC);
					OLC_VAL(d) = 1;
					break;
				case '3':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					else
						REdit::disp_flag_menu(d);
					break;
				case '4':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC /*| STAFF_MINIOLC*/))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					
					REdit::disp_sector_menu(d);
					break;
				case '5':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC /*| STAFF_MINIOLC*/))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					
					OLC_VAL(d) = NORTH;
					REdit::disp_exit_menu(d);
					break;
				case '6':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC /*| STAFF_MINIOLC*/))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					
					OLC_VAL(d) = EAST;
					REdit::disp_exit_menu(d);
					break;
				case '7':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC /*| STAFF_MINIOLC*/))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					
					OLC_VAL(d) = SOUTH;
					REdit::disp_exit_menu(d);
					break;
				case '8':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC /*| STAFF_MINIOLC*/))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					
					OLC_VAL(d) = WEST;
					REdit::disp_exit_menu(d);
					break;
				case '9':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC /*| STAFF_MINIOLC*/))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					
					OLC_VAL(d) = UP;
					REdit::disp_exit_menu(d);
					break;
				case 'a':
				case 'A':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN | STAFF_OLC /*| STAFF_MINIOLC*/))
					{
						send_to_char("Invalid choice!", d->m_Character);
						REdit::disp_menu(d);
						break;
					}
					
					OLC_VAL(d) = DOWN;
					REdit::disp_exit_menu(d);
					break;
				case 'b':
				case 'B':
					REdit::disp_extradesc_menu(d);
					break;
				default:
					send_to_char("Invalid choice!", d->m_Character);
					REdit::disp_menu(d);
					break;
			}
			return;

		case REDIT_NAME:
			{
				BUFFER(buf, MAX_INPUT_LENGTH);
				strcpy(buf, arg);
				delete_doubledollar(buf);
				if (strlen(buf) > MAX_ROOM_NAME)	buf[MAX_ROOM_NAME - 1] = 0;
				OLC_ROOM(d)->SetName(*arg ? arg : "A blank room.");
			}
			break;
		case REDIT_DESC:
			/*
			 * We will NEVER get here, we hope.
			 */
			mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: %s reached REDIT_DESC case in parse_redit", d->m_Character->GetName());
			break;

		case REDIT_FLAGS:
			number = atoi(arg);
			
			if (number == 0)
				break;
			else if ((unsigned)number > NUM_OLC_ROOM_FLAGS)
				send_to_char("That is not a valid choice!\n", d->m_Character);
			else
			{
				--number;
				OLC_ROOM(d)->m_Flags.flip((RoomFlag)number);
			}
			
			REdit::disp_flag_menu(d);
			return;

		case REDIT_SECTOR:
			number = atoi(arg);
			if ((unsigned)number >= NUM_SECTORS) {
				send_to_char("Invalid choice!", d->m_Character);
				REdit::disp_sector_menu(d);
				return;
			} else
				OLC_ROOM(d)->SetSector((RoomSector)number);
			break;

		case REDIT_EXIT_MENU:
			switch (*arg) {
				case '0':
					break;
				case '1':
					OLC_MODE(d) = REDIT_EXIT_NUMBER;
					send_to_char("Exit to room number : ", d->m_Character);
					return;
				case '2':
					OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
					d->Write("Enter exit description: (/s saves /h for help)\n\n");
					d->Write(OLC_EXIT(d)->m_pDescription.c_str());
					d->StartStringEditor(OLC_EXIT(d)->m_pDescription, MAX_EXIT_DESC);
					return;
				case '3':
					OLC_MODE(d) = REDIT_EXIT_KEYWORD;
					send_to_char("Enter keywords : ", d->m_Character);
					return;
				case '4':
					OLC_MODE(d) = REDIT_EXIT_KEY;
					send_to_char("Enter key number : ", d->m_Character);
					return;
				case '5':
					REdit::disp_exit_flag_menu(d);
					OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
					return;
				case '6':
					/*
					 * Delete an exit.
					 */
					OLC_EXIT(d) = NULL;
					break;
				default:
					send_to_char("Try again : ", d->m_Character);
					return;
			}
			break;

		case REDIT_EXIT_NUMBER:
			{
				RoomData *room = NULL;
				if (atoi(arg) != NOWHERE && IsVirtualID(arg))
				{
					room = world.Find(arg);
					if (room == NULL)
					{
						send_to_char("That room does not exist, try again : ", d->m_Character);
						return;
					}
				}
				OLC_EXIT(d)->room = room;
				REdit::disp_exit_menu(d);
			}
			return;

		case REDIT_EXIT_DESCRIPTION:
			/*
			 * We should NEVER get here, hopefully.
			 */
			mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: %s reached REDIT_EXIT_DESC case in parse_redit", d->m_Character->GetName());
			break;

		case REDIT_EXIT_KEYWORD:
			OLC_EXIT(d)->SetKeyword(*arg ? arg : "door");
			REdit::disp_exit_menu(d);
			return;

		case REDIT_EXIT_KEY:
			OLC_EXIT(d)->key = VirtualID(arg);
			REdit::disp_exit_menu(d);
			return;

		case REDIT_EXIT_DOORFLAGS:
			number = atoi(arg);

			if (number == 0)
			{
				REdit::disp_exit_menu(d);
				return;
			}
			else if ((unsigned)number > NUM_EXIT_FLAGS)
			{
				send_to_char("That's not a valid choice!\n"
							 "Enter room flags (0 to quit) : ", d->m_Character);
				return;
			}
			else
			{
				--number;
				
				OLC_EXIT(d)->m_Flags.flip((ExitFlag)number);
				
				if (!EXIT_FLAGGED(OLC_EXIT(d), EX_ISDOOR))
					OLC_EXIT(d)->m_Flags.clear(EX_PICKPROOF);
			}
			
			REdit::disp_exit_flag_menu(d);
			return;

		case REDIT_EXTRADESC_MENU:
			switch (*arg)
			{
				case 'a':
				case 'A':
					OLC_ROOM(d)->m_ExtraDescriptions.push_back(ExtraDesc());
					OLC_DESC(d) = &OLC_ROOM(d)->m_ExtraDescriptions.back();
					OLC_VAL(d) = 1;
					REdit::disp_extradesc(d);
					return;
				case 'd':
				case 'D':
					d->m_Character->Send("Delete which description: ");
					OLC_MODE(d) = REDIT_EXTRADESC_DELETE;
					return;
				
				case 'q':
				case 'Q':
					break;
				
				default:
					if (is_number(arg))
					{
						number = atoi(arg);
						
						FOREACH(ExtraDesc::List, OLC_ROOM(d)->m_ExtraDescriptions, extradesc)
						{
							if (--number == 0)
							{
								OLC_DESC(d) = &*extradesc;
								REdit::disp_extradesc(d);
								return;
							}
						}
					}
					REdit::disp_extradesc_menu(d);
					return;
			}
			break;


		case REDIT_EXTRADESC_KEY:
			OLC_DESC(d)->keyword = *arg ? arg : NULL;
			REdit::disp_extradesc(d);
			return;
			
		case REDIT_EXTRADESC_DELETE:
			number = atoi(arg) - 1;
			
			if ((unsigned int)number >= OLC_ROOM(d)->m_ExtraDescriptions.size())
				send_to_char("Invalid extra description.", d->m_Character);
			else
			{
				ExtraDesc::List::iterator i = OLC_ROOM(d)->m_ExtraDescriptions.begin();
				std::advance(i, number);
				OLC_ROOM(d)->m_ExtraDescriptions.erase(i);
				OLC_VAL(d) = 1;
			}
			
			REdit::disp_extradesc_menu(d);
			return;

		case REDIT_EXTRADESC:
			switch (*arg) {
				case 'q':
				case 'Q':	/* if something got left out */
/*					if (!SSData(OLC_DESC(d)->keyword) || !SSData(OLC_DESC(d)->description)) {
						OLC_ROOM(d)->ex_description.Remove(OLC_DESC(d));
						delete OLC_DESC(d);
						OLC_DESC(d) = NULL;
					}
*/					break;

				case '1':
					OLC_MODE(d) = REDIT_EXTRADESC_KEY;
					OLC_VAL(d) = 1;
					send_to_char("Enter keywords, separated by spaces :-\n| ", d->m_Character);
					return;

				case '2':
					OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
					d->Write("Enter extra description: (/s saves /h for help)\n\n");
					d->Write(OLC_DESC(d)->description.c_str());
					d->StartStringEditor(OLC_DESC(d)->description, MAX_MESSAGE_LENGTH);
					OLC_VAL(d) = 1;
					return;
			}
			REdit::disp_extradesc_menu(d);
			return;

		default:
			/* we should never get here */
			mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: %s reached default case in parse_redit", d->m_Character->GetName());
			break;
	}
	/*
	 * If we get this far, something has be changed
	 */
	OLC_VAL(d) = 1;
	REdit::disp_menu(d);
}
