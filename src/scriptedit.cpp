/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : scriptedit.cp                                                  [*]
[*] Usage: ScriptEdit - OLC Editor for Scripts                            [*]
\***************************************************************************/


#include "scripts.h"
#include "scriptcompiler.h"
#include "zones.h"
#include "characters.h"
#include "descriptors.h"
#include "buffer.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "event.h"
#include "constants.h"
#include "lexifile.h"

/* function protos */
void scriptedit_disp_menu(DescriptorData *d);
void scriptedit_parse(DescriptorData *d, const char *arg);
void scriptedit_setup(DescriptorData *d, ScriptPrototype *proto);
void scriptedit_save_to_disk(ZoneData *zone);
void scriptedit_save_internally(DescriptorData *d);
char *scriptedit_get_class(TrigData *trig);
char *scriptedit_get_type(DescriptorData *d);
void scriptedit_display_types(DescriptorData *d);
void scriptedit_display_classes(DescriptorData *d);

/*
 * Utils and exported functions.
 */

void scriptedit_setup(DescriptorData *d, ScriptPrototype *proto)
{
	TrigData *trig;
	
	if (proto)
	{
		trig = new TrigData(*proto->m_pProto);
		
		//	Copy the pure text version to the storage
		OLC_STORAGE(d) = trig->GetScript();
	}
	else
	{
		trig = new TrigData;
		
		trig->SetName("new script");
		GET_TRIG_DATA_TYPE(trig) = -1;
		
		OLC_STORAGE(d) = "* Empty script\n";
	}
	
	OLC_TRIG(d) = trig;
	OLC_VAL(d) = 0;
	
	scriptedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void scriptedit_save_internally(DescriptorData *d)
{
	int found = FALSE;
	TrigData *trig = NULL;
	
	REMOVE_BIT(GET_TRIG_TYPE(OLC_TRIG(d)), TRIG_DELETED);
	
	/* Recompile the command list from the new script */
	if (OLC_STORAGE(d).empty())
		OLC_STORAGE(d) = "* Empty script\n";
	
	ScriptPrototype *index = trig_index.Find(OLC_VID(d));
	if (index)
	{
		trig = index->m_pProto;
		*trig = *OLC_TRIG(d);
	}
	else
	{
		trig = new TrigData(*OLC_TRIG(d));
		index = trig_index.insert(OLC_VID(d), trig);
	}
	
	trig->SetPrototype(index);
	
	trig->SetCommands(OLC_STORAGE(d).c_str());
	trig->m_pCompiledScript = NULL;
	trig->Compile();
	if (*trig->GetErrors())	d->m_Character->Send("\n`RErrors were reported; please type 'vstat trig %s' to see the errors.`n\n", OLC_VID(d).Print().c_str());
	
	//	Fix up all scripts
	FOREACH(TrigData::List, trig_list, iter)
	{
		if ((*iter)->GetPrototype() == index)
			*(*iter) = *trig;
	}
	
	scriptedit_save_to_disk(OLC_SOURCE_ZONE(d));
}


/*------------------------------------------------------------------------*/

void scriptedit_save_to_disk(ZoneData *zone) {
	BUFFER(fname, MAX_INPUT_LENGTH);
	
	sprintf(fname, TRG_PREFIX "%s.new", zone->GetTag());
	Lexi::OutputParserFile	file(fname);
	
	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open trig file \"%s\"!", fname);
		return;
	}
		
	file.BeginParser();
	
	FOREACH_BOUNDED(ScriptPrototypeMap, trig_index, zone->GetHash(), i)
	{
		ScriptPrototype *	proto = *i;
		TrigData *			trig = proto->m_pProto;
			
		if (IS_SET(GET_TRIG_TYPE(trig), TRIG_DELETED))
			continue;
		
		file.BeginSection(Lexi::Format("Script %s", proto->GetVirtualID().Print().c_str()));
		
		file.WriteVirtualID("Virtual", proto->GetVirtualID(), zone->GetHash());
		
		file.WriteString("Name", trig->GetName());
		file.WriteEnum("Type", trig->m_Type, trig_data_types);
		if (trig->m_Type != -1)
		{
			if (trig->m_Triggers)
			{
				char **table = NULL;
				
				switch (trig->m_Type)
				{
					default:
					case MOB_TRIGGER:	table = mtrig_types;	break;
					case OBJ_TRIGGER:	table = otrig_types;	break;
					case WLD_TRIGGER:	table = wtrig_types;	break;
					case GLOBAL_TRIGGER:table = gtrig_types;	break;
				}
				
				file.WriteFlags("Triggers", trig->m_Triggers, table);
			}
		}
		file.WriteString("Argument", trig->m_Arguments);
		file.WriteInteger("NumArg", GET_TRIG_NARG(trig), 0);
		file.WriteFlags("Flags", trig->m_Flags);
		file.WriteLongString("Script", trig->GetScript());
		
		file.EndSection();	//	Script
	}

	file.EndParser();
	file.close();
	
	BUFFER(buf2, MAX_STRING_LENGTH);
	sprintf(buf2, TRG_PREFIX "%s" TRG_SUFFIX, zone->GetTag());
	remove(buf2);
	rename(fname, buf2);
}


/*------------------------------------------------------------------------*/

char *scriptedit_get_class(TrigData *trig) {
	switch(GET_TRIG_DATA_TYPE(trig)) {
		case MOB_TRIGGER:
			return "Mob Script";
		case OBJ_TRIGGER:
			return "Object Script";
		case WLD_TRIGGER:
			return "Room Script";
		case GLOBAL_TRIGGER:
			return "Global Script";
		default:
			return "Unknown";
	
	}

}


void scriptedit_display_classes(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	send_to_char(	"1) Mob script\n"
					"2) Object script\n"
					"3) Room script\n"
					"4) Global script\n"
					"Enter script type [0 to exit]: ", d->m_Character);
	OLC_MODE(d) = SCRIPTEDIT_CLASS;
}




char *scriptedit_get_type(DescriptorData *d) {
	static char buf[256];
	if (!GET_TRIG_TYPE(OLC_TRIG(d))) {
		strcpy(buf, "Undefined");
	} else {
		switch(GET_TRIG_DATA_TYPE(OLC_TRIG(d))) {
			case MOB_TRIGGER:
				sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), mtrig_types, buf);
				break;
			case OBJ_TRIGGER:
				sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), otrig_types, buf);
				break;
			case WLD_TRIGGER:
				sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), wtrig_types, buf);
				break;
			case GLOBAL_TRIGGER:
				sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), gtrig_types, buf);
				break;
			default:
				strcpy(buf, "Undefined");
				break;
		}
	}
	return buf;
}


void scriptedit_display_types(DescriptorData *d) {
	BUFFER(bitBuf, MAX_INPUT_LENGTH);

	int num_trig_types, i, columns = 0;
	char **trig_types;
	
	switch (GET_TRIG_DATA_TYPE(OLC_TRIG(d))) {
		case WLD_TRIGGER:	trig_types = wtrig_types;	num_trig_types = NUM_WTRIG_TYPES;	break;
		case OBJ_TRIGGER:	trig_types = otrig_types;	num_trig_types = NUM_OTRIG_TYPES;	break;
		case MOB_TRIGGER:	trig_types = mtrig_types;	num_trig_types = NUM_MTRIG_TYPES;	break;
		case GLOBAL_TRIGGER:trig_types = gtrig_types;	num_trig_types = NUM_GTRIG_TYPES;	break;
		default:
			send_to_char(	"Invalid script class - set class first.\n"
							"Enter choice : ", d->m_Character);
			return;
	}
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (i = 0; i < num_trig_types; i++) {
		d->m_Character->Send("`g%2d`n) `c %-20.20s  %s", i + 1, trig_types[i],
			!(++columns % 2) ? "\n" : "");
	}
	sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), trig_types, bitBuf);
	d->m_Character->Send(
				"\n"
				"`nCurrent types   : `c%s\n"
				"`nEnter types (0 to quit): ",
				bitBuf);
	OLC_MODE(d) = SCRIPTEDIT_TYPE;
}


/* Menu functions */

/* the main menu */
void scriptedit_disp_menu(DescriptorData *d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	
	char *bufptr = buf;
	unsigned int x = 0;
	for (const char *str = OLC_STORAGE(d).c_str(); *str; str++) {
		if (*str == '\n')	++x;	// Newline puts us on the next line.
		*bufptr++ = *str;
		if (x == 10)
			break;
	}
	*bufptr = '\0';
	
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Trig Number:  [`c%s`n]\n"
			"`g1`n) Script Name : `c%s\n"
			"`g2`n) Script Class: `y%s\n"
			"`g3`n) Triggers    : `y%s\n"
			"`g4`n) Arg         : `c%s\n"
			"`g5`n) Num Arg     : %d\n"
			"`gM`n) MultiThread : `y%3.3s       `gD`n) Debug Mode  : `y%s\n"
			"`g6`n) Script (First 10 lines)\n"
			"   `b-----------------------`n\n"
			"%s\n"
			"`gQ`n) Quit\n"
			"Enter choice : ",
			
			OLC_VID(d).Print().c_str(),
			OLC_TRIG(d)->GetName(),
			scriptedit_get_class(OLC_TRIG(d)/*.Get()*/),
			scriptedit_get_type(d),
			OLC_TRIG(d)->m_Arguments.c_str(),
			GET_TRIG_NARG(OLC_TRIG(d)),
			YESNO(OLC_TRIG(d)->m_Flags.test(TrigData::Script_Threadable)),
			YESNO(OLC_TRIG(d)->m_Flags.test(TrigData::Script_DebugMode)),
			buf);

	OLC_MODE(d) = SCRIPTEDIT_MAIN_MENU;
}


//	The main loop
void scriptedit_parse(DescriptorData *d, const char *arg) {
	int i, num_trig_types;

	switch (OLC_MODE(d)) {
		case SCRIPTEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
					send_to_char("Saving changes to script - no need to 'scriptedit save'!\n", d->m_Character);
					scriptedit_save_internally(d);
					mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s edits script %s", d->m_Character->GetName(), OLC_VID(d).Print().c_str());
					//	Fall through
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
				default:
					send_to_char("Invalid choice!\n"
								"Do you wish to save this script? ", d->m_Character);
					break;
			}
			return;			/* end of SCRIPTEDIT_CONFIRM_SAVESTRING */

			case SCRIPTEDIT_MAIN_MENU:
				switch (tolower(*arg)) {
					case 'q':
						if (OLC_VAL(d)) {		/* Something was modified */
							send_to_char("Do you wish to save this script? ", d->m_Character);
							OLC_MODE(d) = SCRIPTEDIT_CONFIRM_SAVESTRING;
						} else
							cleanup_olc(d);
						break;
					case '1':
						send_to_char("Enter the name of this script: ", d->m_Character);
						OLC_MODE(d) = SCRIPTEDIT_NAME;
						return;
					case '2':
						scriptedit_display_classes(d);
						return;
					case '3':
						scriptedit_display_types(d);
						return;
					case '4':
						send_to_char("Enter the argument for this script: ", d->m_Character);
						OLC_MODE(d) = SCRIPTEDIT_ARG;
						return;
					case '5':
						send_to_char("Enter the number argument for this script: ", d->m_Character);
						OLC_MODE(d) = SCRIPTEDIT_NARG;
						return;
					case 'm':
						OLC_TRIG(d)->m_Flags.flip(TrigData::Script_Threadable);
						OLC_VAL(d) = 1;
						scriptedit_disp_menu(d);
						break;
					case 'd':
						OLC_TRIG(d)->m_Flags.flip(TrigData::Script_DebugMode);
						OLC_VAL(d) = 1;
						scriptedit_disp_menu(d);
						break;
					case '6':
						OLC_MODE(d) = SCRIPTEDIT_SCRIPT;
						d->Write("\x1B[H\x1B[J");
						d->Write("Edit the script: (/s saves /h for help)\n\n");

						page_string(d, OLC_STORAGE(d).c_str());

						d->StartStringEditor(OLC_STORAGE(d), MAX_STRING_LENGTH * 4);
						OLC_VAL(d) = 1;
						break;
					default:
						scriptedit_disp_menu(d);
						break;
				}
				return;
			case SCRIPTEDIT_NAME:
				OLC_TRIG(d)->SetName(*arg ? arg : "un-named script");
				break;
			case SCRIPTEDIT_CLASS:
				i = atoi(arg);
				if (i > 0) {
					i--;
					if ((i >= MOB_TRIGGER) && (i <= GLOBAL_TRIGGER))
						GET_TRIG_DATA_TYPE(OLC_TRIG(d)) = i;
					else
						send_to_char("Invalid class.\n"
									 "Enter script type [0 to exit]: ", d->m_Character);
				}
				break;
			case SCRIPTEDIT_TYPE:
				if ((i = atoi(arg)) == 0) {
					scriptedit_disp_menu(d);
					return;
				} else {
					switch (GET_TRIG_DATA_TYPE(OLC_TRIG(d))) {
						case MOB_TRIGGER:	num_trig_types = NUM_MTRIG_TYPES;	break;
						case OBJ_TRIGGER:	num_trig_types = NUM_OTRIG_TYPES;	break;
						case WLD_TRIGGER:	num_trig_types = NUM_WTRIG_TYPES;	break;
						case GLOBAL_TRIGGER:num_trig_types = NUM_GTRIG_TYPES;	break;
						default:			num_trig_types = NUM_WTRIG_TYPES;	break;
					}
					if (!((i < 0) || (i > num_trig_types)))
						TOGGLE_BIT(GET_TRIG_TYPE(OLC_TRIG(d)), 1 << (i - 1));
					scriptedit_display_types(d);
					OLC_VAL(d) = 1;
					return;
				}
			case SCRIPTEDIT_ARG:
				OLC_TRIG(d)->m_Arguments = arg;
				break;
			case SCRIPTEDIT_NARG:
				GET_TRIG_NARG(OLC_TRIG(d)) = atoi(arg);
				break;
			case SCRIPTEDIT_SCRIPT:
				mudlog("SYSERR: Reached SCRIPTEDIT_ENTRY in scriptedit_parse", BRF, LVL_BUILDER, TRUE);
				break;
			default:
				/* we should never get here */
				break;
	}
	OLC_VAL(d) = 1;
	scriptedit_disp_menu(d);
}
