/************************************************************************
 * OasisOLC - medit.c                                              v1.5 *
 * Copyright 1996 Harvey Gilpin.                                        *
 ************************************************************************/

#include "characters.h"
#include "descriptors.h"
#include "zones.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "interpreter.h"
#include "clans.h"
#include "buffer.h"
#include "opinion.h"
#include "objects.h"
#include "constants.h"
#include "lexifile.h"

/*
 * External variable declarations.
 */
extern PlayerData dummy_player;
extern struct attack_hit_type attack_hit_text[];
extern int stat_base[NUM_RACES][7];


/*
 * Handy internal macros.
 */
#define GET_ALIAS(themob)		((themob)->m_Keywords)
#define GET_SDESC(themob)		((themob)->m_Name)
#define GET_LDESC(themob)		((themob)->m_RoomDescription)
#define GET_DDESC(themob)		((themob)->m_Description)
//#define GET_ATTACK(themob)		((themob)->mob->attack_type)
#define S_KEEPER(shop)			((shop)->keeper)

#define OLC_MOBEQ_CMD(d)		(OLC_MOB(d)->mob->m_StartingEquipment[OLC_VAL(d)])

/*
 * Function prototypes.
 */
void medit_parse(DescriptorData * d, const char *arg);
void medit_disp_menu(DescriptorData * d);
void medit_setup(DescriptorData *d, CharacterPrototype *proto);
void medit_save_internally(DescriptorData *d);
void medit_save_to_disk(ZoneData *zone);
void medit_disp_positions(DescriptorData *d);
void medit_disp_mob_flags(DescriptorData *d);
void medit_disp_aff_flags(DescriptorData *d);
void medit_disp_attack_types(DescriptorData *d);
void medit_disp_sex(DescriptorData *d);
void medit_disp_race(DescriptorData *d);
void medit_disp_scripts_menu(DescriptorData *d);
void medit_disp_attributes_menu(DescriptorData *d);
void medit_disp_opinion(const Opinion &op, int num, DescriptorData *d);
void medit_disp_opinions_menu(DescriptorData *d);
void medit_disp_opinions_genders(DescriptorData *d);
void medit_disp_opinions_races(DescriptorData *d);
void medit_disp_armor_menu(DescriptorData *d);
void medit_disp_var_menu(DescriptorData * d);
void medit_disp_equipment(DescriptorData *d);
void medit_disp_equipment_item(DescriptorData *d);
void medit_disp_equipment_positions(DescriptorData *d);
void medit_disp_damage_types(DescriptorData *d);
void medit_disp_attack_menu(DescriptorData *d);
void medit_disp_ai_menu(DescriptorData *d);
void medit_disp_teams(DescriptorData *d);

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/


void medit_setup(DescriptorData *d, CharacterPrototype *proto)
{
	CharData *mob = NULL;
	
	if (proto)
	{
		//	Allocate a scratch mobile. 
		mob = new CharData(*proto->m_pProto);
		
		OLC_BEHAVIORSETS(d) = proto->m_BehaviorSets;
		OLC_TRIGLIST(d) = proto->m_Triggers;
		OLC_GLOBALVARS(d) = proto->m_InitialGlobals;
	}
	else
	{
		mob = new CharData;
		
		mob->m_pPlayer = &dummy_player;
		mob->mob = new MobData;
		
		GET_MOB_BASE_HP(mob) = 1;
		GET_MOB_VARY_HP(mob) = 0;

		mob->real_abils.Strength =
		mob->real_abils.Health =
		mob->real_abils.Coordination =
		mob->real_abils.Agility =
		mob->real_abils.Perception =
		mob->real_abils.Knowledge = 50;
		mob->aff_abils = mob->real_abils;

		SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);

		/*
		 * Set up some default strings.
		 */
		GET_ALIAS(mob) = "mob unfinished";
		GET_SDESC(mob) = "the unfinished mob";
		GET_LDESC(mob) = "An unfinished mob stands here.\n";
		GET_DDESC(mob) = "";
	}
	
	OLC_MOB(d) = mob;
	OLC_VAL(d) = 0;
	medit_disp_menu(d);
}


/*-------------------------------------------------------------------*/

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(DescriptorData *d) {
	REMOVE_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_DELETED);
	
	OLC_MOB(d)->real_abils = OLC_MOB(d)->aff_abils;
	
	/*
	 * Mob exists? Just update it.
	 */
	CharacterPrototype *index = mob_index.Find(OLC_VID(d));
	if (index)
	{
		CharData *proto = new CharData(*OLC_MOB(d));
	
		delete index->m_pProto;
		index->m_pProto = proto;
		index->m_pProto->SetPrototype(index);
		
		index->m_Triggers.swap(OLC_TRIGLIST(d));
		index->m_BehaviorSets.swap(OLC_BEHAVIORSETS(d));
		index->m_InitialGlobals.swap(OLC_GLOBALVARS(d));
		
		/*
		 * Update live mobiles.
		 */

		FOREACH(CharacterList, character_list, iter)
		{
			CharData *live_mob = *iter;
			
			if(IS_MOB(live_mob) && live_mob->GetPrototype() == index)
			{
				/*
				 * Only really need to update the strings, since other stuff
				 * changing could interfere with the game.
				 */
				GET_ALIAS(live_mob) = GET_ALIAS(proto);
				GET_SDESC(live_mob) = GET_SDESC(proto);
				GET_LDESC(live_mob) = GET_LDESC(proto);
				GET_DDESC(live_mob) = GET_DDESC(proto);
			}
		}
	}
	else
	{
		index = mob_index.insert(OLC_VID(d), new CharData(*OLC_MOB(d)));
		index->m_pProto->SetPrototype(index);
		
		index->m_Triggers.swap(OLC_TRIGLIST(d));
		index->m_BehaviorSets.swap(OLC_BEHAVIORSETS(d));
		index->m_InitialGlobals.swap(OLC_GLOBALVARS(d));
		
		OLC_MOB(d)->SetPrototype(index);
	}

	medit_save_to_disk(OLC_SOURCE_ZONE(d));
}


/*
 * Save ALL mobiles for a zone to their .mob file, mobs are all 
 * saved in Extended format, regardless of whether they have any
 * extended fields.  Thanks to Samedi for ideas on this bit of code 
 */


void medit_save_to_disk(ZoneData *zone)
{ 
	BUFFER(fname, MAX_STRING_LENGTH);

	sprintf(fname, MOB_PREFIX "%s.new", zone->GetTag());
	Lexi::OutputParserFile	file(fname);
	
	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open mob file \"%s\"!", fname);
		return;
	}

		
	file.BeginParser();
		
	//	Seach the database for mobs in this zone and save them.
	FOREACH_BOUNDED(CharacterPrototypeMap, mob_index, zone->GetHash(), i)
	{
		CharacterPrototype *proto = *i;
		CharData *			mob = proto->m_pProto;
		
		if (MOB_FLAGGED(mob, MOB_DELETED))
			continue;
		
		file.BeginSection(Lexi::Format("Mob %s", proto->GetVirtualID().Print().c_str()));
		mob->Write(file);
		file.EndSection();	//	Mob
	}

	//	Finished writing mob, close it out
	file.EndParser();
	
	//	Finished writing file, close the file out
	file.close();

	//	We're fubar'd if we crash between the two lines below.
	BUFFER(buf2, MAX_STRING_LENGTH);
	sprintf(buf2, MOB_PREFIX "%s" MOB_SUFFIX, zone->GetTag());
	remove(buf2);
	rename(fname, buf2);
}


/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif
	for (int i = 0; i < NUM_POSITIONS; ++i)
	{
		d->m_Character->Send("`g%2d`n) `c%s\n", i, GetEnumName((Position)i));
	}
	d->m_Character->Send("`nEnter position number: ");
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(DescriptorData *d)
{
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif
	for (int i = 0; i < NUM_GENDERS; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i, GetEnumName<Sex>(i));
	}
	d->m_Character->Send("`nEnter gender number: ");
}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void medit_disp_attack_types(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif
	for (int i = 0; i < NUM_ATTACK_TYPES; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i, attack_types[i]);
	}
	d->m_Character->Send("`nEnter attack type: ");
	OLC_MODE(d) = MEDIT_ATTACK_TYPE;
}
 
 
void medit_disp_damage_types(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif
	for (int i = 0; i < NUM_DAMAGE_TYPES; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i, damage_types[i]);
	}
	d->m_Character->Send("`nEnter damage type: ");
	OLC_MODE(d) = MEDIT_ATTACK_DAMAGETYPE;
}
 
/*-------------------------------------------------------------------*/

/*
 * Display races menu.
 */
void medit_disp_race(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif
	for (int i = 0; i < NUM_RACES; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i, race_types[i]);
	}
	d->m_Character->Send("`nEnter race: ");
}
 
 
 /*
 * Display races menu.
 */
void medit_disp_teams(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif
	for (int i = 0; i < 5; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i, team_titles[i]);
	}
	d->m_Character->Send("`nEnter team: ");
}
 

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(DescriptorData *d) {
	int i, columns = 0;
	BUFFER(buf, MAX_INPUT_LENGTH);
			
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (i = 0; i < NUM_MOB_FLAGS; i++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s  %s",
				i+1, action_bits[i], !(++columns % 2) ? "\n" : "");
	}
	sprintbit(MOB_FLAGS(OLC_MOB(d)), action_bits, buf);
	d->m_Character->Send("\n"
				"`nCurrent flags : `c%s\n"
				"`nEnter mob flags (0 to quit): ",
				buf);
}

/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(DescriptorData *d) {
	int i, columns = 0;
	BUFFER(buf, MAX_INPUT_LENGTH);
  
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (i = 0; i < NUM_AFF_FLAGS; i++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s  %s", i+1, GetEnumName<AffectFlag>(i),
				!(++columns % 2) ? "\n" : "");
	}
	d->m_Character->Send("\n"
				"`nCurrent flags   : `c%s\n"
				"`nEnter aff flags (0 to quit): ",
				AFF_FLAGS(OLC_MOB(d)).print().c_str());
}
 

void medit_disp_equipment(DescriptorData *d)
{
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	d->m_Character->Send("[Equipment List]\n");

	int i = 0;
	FOREACH(MobEquipmentList, OLC_MOB(d)->mob->m_StartingEquipment, eq)
	{
		ObjectPrototype *proto = obj_index.Find(eq->m_Virtual);
		
		d->m_Character->Send("%d - %-25.25s %s [`c%s`n]\n",
				i++,
				eq->m_Position != -1 ? equipment_types[eq->m_Position] : "Inventory",
				proto ? proto->m_pProto->GetName() : "<NOTHING>",
				eq->m_Virtual.Print().c_str());
	}
		
	d->m_Character->Send(
		"%d - <END OF LIST>\n"
		"`gN`y) New item\n"
		"`gE`y) Edit item\n"
		"`gD`y) Delete item\n"
		"`gQ`y) Quit menu\n"
		"`cEnter your choice:`n ", i);
	
	OLC_MODE(d) = MEDIT_EQUIPMENT;
}


void medit_disp_equipment_item(DescriptorData *d)
{
	ObjectPrototype *proto = obj_index.Find(OLC_MOBEQ_CMD(d).m_Virtual);
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	d->m_Character->Send(
		"`g1`y) Object  :`n %s `n[`c%s`n]\n"
		"`g2`y) Location:`n %s\n"
		"`gQ`y) Quit\n"
		"`cEnter your choice:`n ",
		proto ? proto->m_pProto->GetName() : "<NOTHING>",
		OLC_MOBEQ_CMD(d).m_Virtual.Print().c_str(),
		OLC_MOBEQ_CMD(d).m_Position != -1 ? equipment_types[OLC_MOBEQ_CMD(d).m_Position] : "Inventory");
	
	OLC_MODE(d) = MEDIT_EQUIPMENT_EDIT;
}


void medit_disp_equipment_positions(DescriptorData *d)
{
	int columns = 0;
	d->m_Character->Send("`g%2d`n) `y%26.26s %s", 0, "Inventory", !(++columns % 2) ? "\n" : "");
	for (int counter = 0; counter < NUM_WEARS; ++counter) {
		d->m_Character->Send("`g%2d`n) `y%26.26s %s",
			counter + 1, equipment_types[counter], !(++columns % 2) ? "\n" : "");
	}
	
	send_to_char("\n`nEnter location to equip: ", d->m_Character);
	OLC_MODE(d) = MEDIT_EQUIPMENT_EDIT_POS;
}


/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(DescriptorData * d) {
	BUFFER(buf1, MAX_INPUT_LENGTH);
	BUFFER(buf2, MAX_INPUT_LENGTH);
	CharData *mob = OLC_MOB(d)/*.Get()*/;

	sprintbit(MOB_FLAGS(mob), action_bits, buf1);
	FOREACH(ScriptVector, OLC_TRIGLIST(d), i)
	{
		sprintf_cat(buf2, "%s ", i->Print().c_str());
	}
	
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif

	d->m_Character->Send(
				"-- Mob Number:  [`c%s`n]\n"
				"`G1`n) Sex: `y%-7.7s`n 	    `G2`n) Alias: `y%s\n"
				"`G3`n) S-Desc: `y%s\n"
				"`G4`n) L-Desc:-\n`y%s"
				"`G5`n) D-Desc:-\n`y%s"
				"`G6`n) Level     :  [`c%4d`n]      `G7`n) Race      :   [`c%s `n]\n"
				"`G8`n) HP     [`c%5d+%-4d`n]      `G9`n) MP        :  [`c%4d`n]\n"
				"`G0`n) Armor     :   BLN SLA IMP BAL FIR ENR ACD\n"
				"               [`c %3d %3d %3d %3d %3d %3d %3d `n]\n"
				"`GA`n) Attributes:  Str [`c%3d`n]  Hea [`c%3d`n]  Coo [`c%3d`n]\n"
				    "                Agi [`c%3d`n]  Per [`c%3d`n]  Kno [`c%3d`n]\n"
				"`GC`n) Clan      : [`c%4d`n] (`y%s`n)\n"
				"`GD`n) Position  : `y%-12.12s `GE`n) Default   : `y%-20.20s\n"
				"`GF`n) Attack    : `y%d %s `gx`n %d `g@`n %.2f `g(%s)`n [DPS: %-4d DPA: %-4d]\n"
				"`GG`n) NPC Flags : `c%s\n"
				"`GH`n) AFF Flags : `c%s\n"
				"`GI`n) Equipment (%d)\n"
				"`GN`n) A.I. Menu             `GO`n) Opinions Menu\n"
				"`GS`n) Scripts   : `c%s %s\n"
				"`GT`n) Team      : `c%s\n"
				"`GQ`n) Quit\n"
				"Enter choice : ",
			OLC_VID(d).Print().c_str(),
			GetEnumName(mob->GetSex()),		GET_ALIAS(mob).c_str(),
			GET_SDESC(mob).c_str(),
			GET_LDESC(mob).c_str(),
			GET_DDESC(mob).c_str(),
			mob->GetLevel(),					findtype(mob->GetRace(), race_abbrevs),
			GET_MOB_BASE_HP(mob), GET_MOB_VARY_HP(mob), mob->points.lifemp,
			GET_ARMOR(mob, ARMOR_LOC_NATURAL, 0), GET_ARMOR(mob, ARMOR_LOC_NATURAL, 1), GET_ARMOR(mob, ARMOR_LOC_NATURAL, 2),
			GET_ARMOR(mob, ARMOR_LOC_NATURAL, 3), GET_ARMOR(mob, ARMOR_LOC_NATURAL, 4), GET_ARMOR(mob, ARMOR_LOC_NATURAL, 5),
			GET_ARMOR(mob, ARMOR_LOC_NATURAL, 6),
			GET_STR(mob),	GET_HEA(mob),	GET_COO(mob),
			GET_AGI(mob),	GET_PER(mob),	GET_KNO(mob),
			GET_CLAN(mob) ? GET_CLAN(mob)->GetID() : 0,
				GET_CLAN(mob) ? GET_CLAN(mob)->GetName() : "None",
			GetEnumName(GET_POS(mob)), GetEnumName(GET_DEFAULT_POS(mob)),
			mob->mob->m_AttackDamage,
					findtype(mob->mob->m_AttackDamageType, damage_types),
					mob->mob->m_AttackRate,
					mob->mob->m_AttackSpeed,
					findtype(mob->mob->m_AttackType, attack_types),
					(int)((mob->mob->m_AttackDamage * MAX(1, mob->mob->m_AttackRate) * DAMAGE_DICE_SIZE / 10) / mob->mob->m_AttackSpeed),
								mob->mob->m_AttackDamage * MAX(1, mob->mob->m_AttackRate) * DAMAGE_DICE_SIZE / 2,
			buf1,
			AFF_FLAGS(mob).print().c_str(),
			OLC_MOB(d)->mob->m_StartingEquipment.size(),
			*buf2 ? buf2 : "None",
					!OLC_GLOBALVARS(d).empty() ? "(Has Variables)" : "",
			GET_TEAM(OLC_MOB(d)) != 0 ? team_titles[GET_TEAM(OLC_MOB(d))] : "<None>");

	OLC_MODE(d) = MEDIT_MAIN_MENU;
}


void medit_disp_attributes_menu(DescriptorData *d) {
	CharData *mob = OLC_MOB(d)/*.Get()*/;
	
	d->m_Character->Send(
			"-- Attributes:\n"
			"`g1`n) Strength     [`c%3d`n]\n"
			"`g2`n) Health       [`c%3d`n]\n"
			"`g3`n) Coordination [`c%3d`n]\n"
			"`g4`n) Agility      [`c%3d`n]\n"
			"`g5`n) Perception   [`c%3d`n]\n"
			"`g6`n) Knowledge    [`c%3d`n]\n"
			"`GQ`n) Exit Menu\n"
			"Enter choice: ",
			GET_STR(mob),
			GET_HEA(mob),
			GET_COO(mob),
			GET_AGI(mob),
			GET_PER(mob),
			GET_KNO(mob));
	OLC_MODE(d) = MEDIT_ATTRIBUTES;
}


void medit_disp_attack_menu(DescriptorData *d) {
	CharData *mob = OLC_MOB(d)/*.Get()*/;
	
	d->m_Character->Send(
			"-- Attributes:\n"
			"`g1`n) Damage      : %d\n"
			"`g2`n) Damage Type : %s\n"
			"`g3`n) Speed       : %.2f\n"
			"`g4`n) Rate of Fire: %d\n"
			"`g5`n) Attack Type : `c%s`n\n"
			"       DPS         : `c%-3d`n         DPA         : `c%d`n\n"
			"`GQ`n) Exit Menu\n"
			"Enter choice: ",
			mob->mob->m_AttackDamage,
			findtype(mob->mob->m_AttackDamageType, damage_types),
			mob->mob->m_AttackSpeed,
			mob->mob->m_AttackRate,
			findtype(mob->mob->m_AttackType, attack_types),
			(int)((mob->mob->m_AttackDamage * MAX(1, mob->mob->m_AttackRate) * DAMAGE_DICE_SIZE / 10) / mob->mob->m_AttackSpeed),
						mob->mob->m_AttackDamage * MAX(1, mob->mob->m_AttackRate) * DAMAGE_DICE_SIZE / 2);
	OLC_MODE(d) = MEDIT_ATTACK_MENU;
}


void medit_disp_armor_menu(DescriptorData *d) {
	CharData *mob = OLC_MOB(d)/*.Get()*/;
	
	d->m_Character->Send("-- Armor:\n");
	for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
		d->m_Character->Send("`g%d`n) %-12.12s [`c%3d`n]\n", i + 1, damage_types[i], GET_ARMOR(OLC_MOB(d), ARMOR_LOC_NATURAL, i));
	d->m_Character->Send(
			"`GQ`n) Exit Menu\n"
			"Enter choice: ");
	OLC_MODE(d) = MEDIT_ARMOR;
}


//	Display scripts attached
void medit_disp_scripts_menu(DescriptorData *d) {
	BUFFER(buf, MAX_STRING_LENGTH);

	int counter = 0;
	
	if (!OLC_BEHAVIORSETS(d).empty())
	{
		send_to_char(	"`y-- Behavior Sets:`n\n", d->m_Character);

		for (BehaviorSets::iterator i = OLC_BEHAVIORSETS(d).begin(); i != OLC_BEHAVIORSETS(d).end(); ++i)
		{
			BehaviorSet *set = *i;
			if (set)	d->m_Character->Send("`g%2d`n) %s\n", ++counter, set->GetName());
		}
	}
	
	if (!OLC_TRIGLIST(d).empty())
	{
		send_to_char(	"-- Scripts :\n", d->m_Character);
		
		FOREACH(ScriptVector, OLC_TRIGLIST(d), i)
		{
			ScriptPrototype *proto = trig_index.Find(*i);
			d->m_Character->Send("`g%2d`n) [`c%10s`n] %s\n", ++counter, i->Print().c_str(), proto ? proto->m_pProto->GetName() : "(Uncreated)");
		}
	}
	else
		send_to_char("`y-- No scripts`n\n", d->m_Character);
	
	if (!OLC_GLOBALVARS(d).empty())
	{
		d->m_Character->Send("`y-- Global Variables (%d):`n\n", OLC_GLOBALVARS(d).size());
		for (ScriptVariable::List::iterator iter = OLC_GLOBALVARS(d).begin(); iter != OLC_GLOBALVARS(d).end(); ++iter)
		{
			ScriptVariable *var = *iter;
			strcpy(buf, var->GetName());
			if (var->GetContext())	sprintf_cat(buf, " {%d}", var->GetContext());
			d->m_Character->Send("`g%2d`n) %-30.30s: %s\n", ++counter, buf, var->GetValue());
		}
	}
	else
		send_to_char("`y-- No Variables`n\n", d->m_Character);

	send_to_char(	"`GB`n) Add Behavior Set\n"
					"`GA`n) Add Script\n"
					"`GV`n) Add Variable\n"
					"`GE`n) Edit Variable\n"
					"`GR`n) Remove Script/Variable\n"
					"`GQ`n) Exit Menu\n"
					"Enter choice:  ", d->m_Character);
	OLC_MODE(d) = MEDIT_TRIGGERS;
}


void medit_disp_var_menu(DescriptorData * d) {
	ScriptVariable *vd = OLC_TRIGVAR(d);

#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif

	d->m_Character->Send(
			"-- Variable menu\n"
			"`gN`n) Name         : `y%s\n"
			"`gV`n) Value        : `y%s\n"
			"`gC`n) Context      : `c%d\n"
			"`gQ`n) Quit Variable Setup\n"
			"Enter choice : ",
			vd->GetName(),
			vd->GetValue(),
			vd->GetContext());

	OLC_MODE(d) = MEDIT_TRIGGER_VARIABLE;
}


extern char *race_types[];

void medit_disp_opinion(const Opinion &op, int num, DescriptorData *d)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	if (op.m_Sexes) {
		sprintbit(op.m_Sexes, (char **)GetEnumNameResolver<Sex>().GetNames(), buf);
		d->m_Character->Send("   `G%1d`n) Sexes  : %s\n", num, buf);
	} else
		d->m_Character->Send("   `G%1d`n) Sexes  : <NONE>\n", num);
	
	if (op.m_Races) {
		sprintbit(op.m_Races, race_types, buf);
		d->m_Character->Send("   `G%1d`n) Races  : %s\n", num+1, buf);
	} else
		d->m_Character->Send("   `G%1d`n) Races  : <NONE>\n", num+1);
	
	if (op.m_VNum.IsValid())
	{
		CharacterPrototype *proto = mob_index.Find(op.m_VNum);
		
		d->m_Character->Send("   `G%1d`n) Virtual: %s (%s)\n", num+2, op.m_VNum.Print().c_str(),
				(proto ?
					(proto->m_pProto->m_Name.empty() ? "Unnamed"
					 : proto->m_pProto->GetName())
				: "Invalid VNUM"));
	}
	else
		d->m_Character->Send("   `G%1d`n) Virtual: <NONE>\n", num+2);
}


//	Opinions menus
void medit_disp_opinions_menu(DescriptorData *d) {
	send_to_char("-- Opinions :\n", d->m_Character);
	send_to_char("Hates\n"
				 "-----\n", d->m_Character);	
	medit_disp_opinion(OLC_MOB(d)->mob->m_Hates, 1, d);
	send_to_char("\n"
				 "Fears\n"
				 "-----\n", d->m_Character);	
	medit_disp_opinion(OLC_MOB(d)->mob->m_Fears, 4, d);
	send_to_char("\n"
				 "`G0`n) Exit\n"
				 "`nEnter choice:  ", d->m_Character);
	OLC_MODE(d) = MEDIT_OPINIONS;
}


//	Display Opinion gender flags.
void medit_disp_opinions_genders(DescriptorData *d) {
	BUFFER(buf, MAX_INPUT_LENGTH);
	int i;
	
	for (i = 0; i < NUM_GENDERS; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i + 1, GetEnumName<Sex>(i));
	}
	sprintbit(OLC_OPINION(d)->m_Sexes, (char **)GetEnumNameResolver<Sex>().GetNames(), buf);
	d->m_Character->Send("`nCurrent genders : `c%s\n"
				 "`nEnter genders (0 to quit): ", buf);
}


//	Display Opinion races
void medit_disp_opinions_races(DescriptorData *d) {
	BUFFER(buf, MAX_INPUT_LENGTH);
	int i;
	
	for (i = 0; i < NUM_RACES; i++) {
		d->m_Character->Send("`g%2d`n) `c%s\n", i + 1, race_types[i]);
	}
	sprintbit(OLC_OPINION(d)->m_Races, race_types, buf);
	d->m_Character->Send("`nCurrent races   : `c%s\n"
				 "`nEnter races (0 to quit): ", buf);
}


void medit_disp_ai_menu(DescriptorData *d)
{
	CharData *mob = OLC_MOB(d)/*.Get()*/;	
	
	d->m_Character->Send(
			"-- AI Settings:\n"
			"`g1`n) Aggressive: Room    [`c%3d`n] (Percentage)\n"
			"`g2`n) Aggressive: Ranged  [`c%3d`n] (Percentage)\n"
			"`g3`n) Aggressive: Loyalty [`c%3d`n] (Percentage)\n"
			"`g4`n) Awareness:  Range   [`c%3d`n] (Rooms)\n"
			"`g5`n) Movement:   Rate    [`c%3d`n] (Percentage)\n"
			"`GQ`n) Exit Menu\n"
			"Enter choice: ",
			mob->mob->m_AIAggrRoom,
			mob->mob->m_AIAggrRanged,
			mob->mob->m_AIAggrLoyalty,
			mob->mob->m_AIAwareRange,
			mob->mob->m_AIMoveRate);
	OLC_MODE(d) = MEDIT_AI_MENU;
}


/**************************************************************************
 *                      The GARGANTAUN event handler                      *
 **************************************************************************/

void medit_parse(DescriptorData * d, const char *arg) {
	int i, number;
	
	if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
		if(*arg && !is_number(arg)) {
			send_to_char("Field must be numerical, try again : ", d->m_Character);
			return;
		}
	}

	switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
		case MEDIT_CONFIRM_SAVESTRING:
			/*
			 * Ensure mob has MOB_ISNPC set or things will go pair shaped.
			 */
			SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
			switch (*arg) {
				case 'y':
				case 'Y':
					/*
					 * Save the mob in memory and to disk.
					 */
					send_to_char("Saving changes to mobile - no need to 'medit save'!\n", d->m_Character);
					medit_save_internally(d);
					mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s edits mob %s", d->m_Character->GetName(), OLC_VID(d).Print().c_str());

					//	Fall through
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
					
				default:
					send_to_char("Invalid choice!\n"
								 "Do you wish to save this mobile? ", d->m_Character);
			}
			return;
/*-------------------------------------------------------------------*/
		case MEDIT_MAIN_MENU:
			i = 0;
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d))	//	Anything been changed?
					{
						send_to_char("Do you wish to save this mobile? ", d->m_Character);
						OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
					}
					else
						cleanup_olc(d);
					return;
				case '1':
					OLC_MODE(d) = MEDIT_SEX;
					medit_disp_sex(d);
					return;
				case '2':
					OLC_MODE(d) = MEDIT_ALIAS;
					i--;
					break;
				case '3':
					OLC_MODE(d) = MEDIT_S_DESC;
					i--;
					break;
				case '4':
					OLC_MODE(d) = MEDIT_L_DESC;
					i--;
					break;
				case '5':
					OLC_MODE(d) = MEDIT_D_DESC;
					OLC_VAL(d) = 1;
					d->Write("Enter mob description: (/s saves /c clears /h for help)\n\n");
					d->Write(OLC_MOB(d)->m_Description.c_str());
					d->StartStringEditor(OLC_MOB(d)->m_Description, MAX_MOB_DESC);
					return;
				case '6':
					OLC_MODE(d) = MEDIT_LEVEL;
					i++;
					break;
				case '7':
					OLC_MODE(d) = MEDIT_RACE;
					medit_disp_race(d);
					i++;
					break;
				case '8':
					d->m_Character->Send("Base HP: ");
					OLC_MODE(d) = MEDIT_BASE_HP;
					i++;
					return;
				case '9':
					OLC_MODE(d) = MEDIT_MP;
					i++;
					break;
				case '0':
					medit_disp_armor_menu(d);
					return;
				case 'a':
				case 'A':
					medit_disp_attributes_menu(d);
					return;
				case 'c':
				case 'C':
					OLC_MODE(d) = MEDIT_CLAN;
					i++;
					break;
				case 'd':
				case 'D':
					OLC_MODE(d) = MEDIT_POS;
					medit_disp_positions(d);
					return;
				case 'e':
				case 'E':
					OLC_MODE(d) = MEDIT_DEFAULT_POS;
					medit_disp_positions(d);
					return;
				case 'f':
				case 'F':
					OLC_MODE(d) = MEDIT_ATTACK_MENU;
					medit_disp_attack_menu(d);
					return;
				case 'g':
				case 'G':
					OLC_MODE(d) = MEDIT_NPC_FLAGS;
					medit_disp_mob_flags(d);
					return;
				case 'h':
				case 'H':
					OLC_MODE(d) = MEDIT_AFF_FLAGS;
					medit_disp_aff_flags(d);
					return;
				case 'i':
				case 'I':
					medit_disp_equipment(d);
					return;
				case 'n':
				case 'N':
					medit_disp_ai_menu(d);
					return;
				case 'o':
				case 'O':
					medit_disp_opinions_menu(d);
					return;
				case 's':
				case 'S':
					medit_disp_scripts_menu(d);
					return;
				case 't':
				case 'T':
					OLC_MODE(d) = MEDIT_TEAM;
					medit_disp_teams(d);
					return;
				default:
					medit_disp_menu(d);
					return;
			}
			if (i != 0) {
				send_to_char((i == 1) ? "\nEnter new value : " :
						((i == -1) ? "\nEnter new text :\n] " :
						"\nOops...:\n"), d->m_Character);
				return;
			}
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_ALIAS:
			GET_ALIAS(OLC_MOB(d)) = *arg ? arg : "undefined mob"; 
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_S_DESC:
			GET_SDESC(OLC_MOB(d)) = *arg ? arg : "undefined mob"; 
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_L_DESC:
			if (*arg) {
				BUFFER(buf, MAX_STRING_LENGTH);
				strcpy(buf, arg);
				strcat(buf, "\n");
				GET_LDESC(OLC_MOB(d)) = buf;
			} else
				GET_LDESC(OLC_MOB(d)) = "undefined mob\n";
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_D_DESC:
			/*
			 * We should never get here.
			 */
//			cleanup_olc(d);
			mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!",BRF,LVL_BUILDER,TRUE);
			send_to_char("Oops...\n", d->m_Character);
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_NPC_FLAGS:
			if ((i = atoi(arg)) == 0)
				break;
			else if (!((i < 0) || (i > NUM_MOB_FLAGS)))
				TOGGLE_BIT(MOB_FLAGS(OLC_MOB(d)), 1 << (i - 1));
			medit_disp_mob_flags(d);
			return;
/*-------------------------------------------------------------------*/
		case MEDIT_AFF_FLAGS:
			if ((i = atoi(arg) - 1) == -1)
				break;
			else if ((unsigned)i < NUM_AFF_FLAGS)
				AFF_FLAGS(OLC_MOB(d)).flip((AffectFlag)i);
			medit_disp_aff_flags(d);
			return;
/*-------------------------------------------------------------------*/
/*
 * Numerical responses
 */
		case MEDIT_SEX:
			OLC_MOB(d)->m_Sex = (Sex)RANGE(atoi(arg), 0, NUM_GENDERS - 1);
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_BASE_HP:
			GET_MOB_BASE_HP(OLC_MOB(d)) = RANGE(atoi(arg), 0, 32000);
			d->m_Character->Send("Maximum bonus: ");
			OLC_MODE(d) = MEDIT_VARY_HP;
			return;
/*-------------------------------------------------------------------*/
		case MEDIT_VARY_HP:
			GET_MOB_VARY_HP(OLC_MOB(d)) = RANGE(atoi(arg), 0, 32000);
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_MP:
			OLC_MOB(d)->points.lifemp = RANGE(atoi(arg), 0, 300);
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_ARMOR:
			switch (*arg)
			{
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					d->m_Character->Send("Armor vs %s: ", damage_types[*arg - '1']);
					OLC_MODE(d) = MEDIT_ARMOR_1 + (*arg - '1');
					break;
					
				case 'q':
				case 'Q':
					medit_disp_menu(d);
					return;
				default:
					medit_disp_armor_menu(d);
					return;
			}
			return;
		
		case MEDIT_ARMOR_1:
		case MEDIT_ARMOR_2:
		case MEDIT_ARMOR_3:
		case MEDIT_ARMOR_4:
		case MEDIT_ARMOR_5:
		case MEDIT_ARMOR_6:
		case MEDIT_ARMOR_7:
			GET_ARMOR(OLC_MOB(d), ARMOR_LOC_NATURAL, OLC_MODE(d) - MEDIT_ARMOR_1) = RANGE(atoi(arg), 0, 100);
			OLC_VAL(d) = 1;
			medit_disp_armor_menu(d);
			return;

/*-------------------------------------------------------------------*/
		case MEDIT_POS:
			GET_POS(OLC_MOB(d)) = (Position)MAX(0, MIN(NUM_POSITIONS - 1, atoi(arg)));
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_DEFAULT_POS:
			GET_DEFAULT_POS(OLC_MOB(d)) = (Position)MAX(0, MIN(NUM_POSITIONS - 1, atoi(arg)));
			break;

/*-------------------------------------------------------------------*/
		
		case MEDIT_ATTACK_MENU:
			switch (*arg)
			{
				case '1':
					d->m_Character->Send("Damage: ");
					OLC_MODE(d) = MEDIT_ATTACK_DAMAGE;
					break;
					
				case '2':
					medit_disp_damage_types(d);
					break;
					
				case '3':
					d->m_Character->Send("Hit delay multiplier (decimals allowed): ");
					OLC_MODE(d) = MEDIT_ATTACK_SPEED;
					break;
					
				case '4':
					d->m_Character->Send("Rate of attack: ");
					OLC_MODE(d) = MEDIT_ATTACK_RATE;
					break;
					
				case '5':
					medit_disp_attack_types(d);
					break;
					
				case 'q':
				case 'Q':
					medit_disp_menu(d);
					return;
					
				default:
					medit_disp_attack_menu(d);
					return;
			}
			return;

		case MEDIT_ATTACK_SPEED:
			if (*arg)
			{
				OLC_MOB(d)->mob->m_AttackSpeed = RANGE(atof(arg), 0.1f, 10.0f);
				OLC_VAL(d) = 1;
			}
			
			medit_disp_attack_menu(d);
			return;

		case MEDIT_ATTACK_DAMAGE:
			OLC_MOB(d)->mob->m_AttackDamage = RANGE(atoi(arg), 0, 2000);
			OLC_VAL(d) = 1;
			medit_disp_attack_menu(d);
			return;

		case MEDIT_ATTACK_DAMAGETYPE:
			OLC_MOB(d)->mob->m_AttackDamageType = RANGE(atoi(arg), 0, NUM_DAMAGE_TYPES - 1);
			OLC_VAL(d) = 1;
			medit_disp_attack_menu(d);
			return;

		case MEDIT_ATTACK_RATE:
			OLC_MOB(d)->mob->m_AttackRate = RANGE(atoi(arg), 0, 5);
			OLC_VAL(d) = 1;
			medit_disp_attack_menu(d);
			return;
			
		case MEDIT_ATTACK_TYPE:
			OLC_MOB(d)->mob->m_AttackType = RANGE(atoi(arg), 0, NUM_ATTACK_TYPES - 1);
			OLC_VAL(d) = 1;
			medit_disp_attack_menu(d);
			return;

/*-------------------------------------------------------------------*/
		case MEDIT_LEVEL:
			OLC_MOB(d)->m_Level = MAX(1, MIN(100, atoi(arg)));
			break;
/*-------------------------------------------------------------------*/
		case MEDIT_RACE:
			OLC_MOB(d)->m_Race = (Race)MAX(0, MIN(NUM_RACES, atoi(arg)));
			break;
		case MEDIT_TRIGGERS:
			switch (*arg) {
				case 'b':
				case 'B':
					send_to_char("Enter the name of set to attach: ", d->m_Character);
					OLC_MODE(d) = MEDIT_TRIGGER_ADD_BEHAVIORSET;
					return;
				case 'a':
				case 'A':
					send_to_char("Enter the VNUM of script to attach: ", d->m_Character);
					OLC_MODE(d) = MEDIT_TRIGGER_ADD_SCRIPT;
					return;
				case 'v':
				case 'V':
					OLC_TRIGVAR(d) = OLC_GLOBALVARS(d).AddBlank();
					
					medit_disp_var_menu(d);
					
					return;
				case 'e':
				case 'E':
					send_to_char("Enter item in list to edit: ", d->m_Character);
					OLC_MODE(d) = MEDIT_TRIGGER_CHOOSE_VARIABLE;
					return;
				case 'r':
				case 'R':
					send_to_char("Enter item in list to remove: ", d->m_Character);
					OLC_MODE(d) = MEDIT_TRIGGER_REMOVE;
					return;
				case 'q':
				case 'Q':
					medit_disp_menu(d);
					return;
				default:
					medit_disp_scripts_menu(d);
					return;
			}
			break;
			
		case MEDIT_TRIGGER_ADD_BEHAVIORSET:
			if (*arg)
			{
				BehaviorSet *set = BehaviorSet::Find(arg);
				if (!set)
					send_to_char("No such set found.\n", d->m_Character);
				else
				{
					OLC_BEHAVIORSETS(d).push_back(set);
					send_to_char("Set added.\n", d->m_Character);
					OLC_VAL(d) = 1;
				}
			}
			
			medit_disp_scripts_menu(d);
			return;
		
		case MEDIT_TRIGGER_ADD_SCRIPT:
			if (IsVirtualID(arg))
			{
				OLC_TRIGLIST(d).push_back(VirtualID(arg));
				send_to_char("Script attached.\n", d->m_Character);
				OLC_VAL(d) = 1;
			}
			else
				send_to_char("Virtual IDs only, please.\n", d->m_Character);
			medit_disp_scripts_menu(d);
			return;
		case MEDIT_TRIGGER_REMOVE:
			if (is_number(arg))
			{
				number = atoi(arg) - 1;
				
				if (number < 0)
					send_to_char("Invalid number.\n", d->m_Character);
				else
				{
					if (number < OLC_BEHAVIORSETS(d).size())
					{
						OLC_BEHAVIORSETS(d).erase(OLC_BEHAVIORSETS(d).begin() + number);
						OLC_VAL(d) = 1;
						send_to_char("Behavior set removed.\n", d->m_Character);
					}
					else
					{
						number -= OLC_BEHAVIORSETS(d).size();
						
						if (number < OLC_TRIGLIST(d).size())
						{
							OLC_TRIGLIST(d).erase(OLC_TRIGLIST(d).begin() + number);
							OLC_VAL(d) = 1;
							send_to_char("Script detached.\n", d->m_Character);
						}
						else
						{
							number -= OLC_TRIGLIST(d).size();
				
							if (number < OLC_GLOBALVARS(d).size())
							{
								ScriptVariable::List::iterator var = OLC_GLOBALVARS(d).begin();
								std::advance(var, number);
								
								d->m_Character->Send("Variable \"%s\" removed.\n", (*var)->GetName());
								delete *var;
								OLC_GLOBALVARS(d).erase(var);
								OLC_VAL(d) = 1;
							}
							else
								send_to_char("Invalid number.\n", d->m_Character);
						}
					}
				}
			}
			else
				send_to_char("Numbers only, please.\n", d->m_Character);
			medit_disp_scripts_menu(d);
			return;

		case MEDIT_TRIGGER_CHOOSE_VARIABLE:
			if (is_number(arg))
			{
				number = atoi(arg) - 1;
				
				number -= OLC_TRIGLIST(d).size();
				
				if (number < 0)
					send_to_char("Variables only.\n", d->m_Character);
				else if (number < OLC_GLOBALVARS(d).size())
				{
					ScriptVariable::List::iterator var = OLC_GLOBALVARS(d).begin();
					std::advance(var, number);
							
					OLC_TRIGVAR(d) = *var;
					medit_disp_var_menu(d);
					return;
				}
				else
					send_to_char("Invalid number.\n", d->m_Character);
			}
			else
				send_to_char("Numbers only, please.\n", d->m_Character);
			medit_disp_scripts_menu(d);
			return;
		
		case MEDIT_TRIGGER_VARIABLE:
			switch (*arg)
			{
				case 'n':
				case 'N':
					send_to_char("Name: ", d->m_Character);
					OLC_MODE(d) = MEDIT_TRIGGER_VARIABLE_NAME;
					break;
				case 'v':
				case 'V':
					send_to_char("Value: ", d->m_Character);
					OLC_MODE(d) = MEDIT_TRIGGER_VARIABLE_VALUE;
					break;
				case 'c':
				case 'C':
					send_to_char("Context (0 = default): ", d->m_Character);
					OLC_MODE(d) = MEDIT_TRIGGER_VARIABLE_CONTEXT;
					break;
				case 'q':
				case 'Q':
					if (OLC_TRIGVAR(d)->GetNameString().empty())
					{
						//OLC_GLOBALVARS(d).remove(OLC_TRIGVAR(d));
						ScriptVariable::List::iterator iter = Lexi::Find(OLC_GLOBALVARS(d), OLC_TRIGVAR(d));
						if (iter != OLC_GLOBALVARS(d).end())
							OLC_GLOBALVARS(d).erase(iter);
						delete OLC_TRIGVAR(d);
						OLC_TRIGVAR(d) = NULL;
					}
					
					medit_disp_scripts_menu(d);
					break;
				default:
					medit_disp_var_menu(d);
					break;
			}
			return;
		
		case MEDIT_TRIGGER_VARIABLE_NAME:
			if (*arg)
			{
				ScriptVariable var(arg, OLC_TRIGVAR(d)->GetContext());
				var.SetValue(OLC_TRIGVAR(d)->GetValue());
				
				*OLC_TRIGVAR(d) = var;
				OLC_GLOBALVARS(d).Sort();

				OLC_VAL(d) = 1;
			}
			medit_disp_var_menu(d);
			return;
			
		case MEDIT_TRIGGER_VARIABLE_VALUE:
			if (*arg)
			{
				OLC_TRIGVAR(d)->SetValue(arg);
				OLC_VAL(d) = 1;
			}
			medit_disp_var_menu(d);
			return;

		case MEDIT_TRIGGER_VARIABLE_CONTEXT:
			if (is_number(arg))
			{
				ScriptVariable var(OLC_TRIGVAR(d)->GetName(), atoi(arg));
				var.SetValue(OLC_TRIGVAR(d)->GetValue());
				
				*OLC_TRIGVAR(d) = var;
				OLC_GLOBALVARS(d).Sort();

				OLC_VAL(d) = 1;
			}
			medit_disp_var_menu(d);
			return;
			


		case MEDIT_CLAN:
			GET_CLAN(OLC_MOB(d)) = is_number(arg) ? Clan::GetClan(atoi(arg)) : NULL;
			break;
		
		
		case MEDIT_ATTRIBUTES:
			switch (*arg) {
				case '1':
					d->m_Character->Send("Strength (%d): ", stat_base[OLC_MOB(d)->GetRace()][0]);
					OLC_MODE(d) = MEDIT_ATTR_STR;
					break;
				case '2':
					d->m_Character->Send("Health (%d): ", stat_base[OLC_MOB(d)->GetRace()][1]);
					OLC_MODE(d) = MEDIT_ATTR_HEA;
					break;
				case '3':
					d->m_Character->Send("Coordination (%d): ", stat_base[OLC_MOB(d)->GetRace()][2]);
					OLC_MODE(d) = MEDIT_ATTR_COO;
					break;
				case '4':
					d->m_Character->Send("Agility (%d): ", stat_base[OLC_MOB(d)->GetRace()][3]);
					OLC_MODE(d) = MEDIT_ATTR_AGI;
					break;
				case '5':
					d->m_Character->Send("Perception (%d): ", stat_base[OLC_MOB(d)->GetRace()][4]);
					OLC_MODE(d) = MEDIT_ATTR_PER;
					break;
				case '6':
					d->m_Character->Send("Knowledge (%d): ", stat_base[OLC_MOB(d)->GetRace()][5]);
					OLC_MODE(d) = MEDIT_ATTR_KNO;
					break;
				case 'q':
				case 'Q':
					medit_disp_menu(d);
					return;
				default:
					medit_disp_attributes_menu(d);
					return;
			}
			OLC_VAL(d) = 1;
			return;
		case MEDIT_ATTR_STR:
			GET_STR(OLC_MOB(d)) = MAX(0, MIN(MAX_STAT, atoi(arg)));
			medit_disp_attributes_menu(d);
			return;
		case MEDIT_ATTR_HEA:
			GET_HEA(OLC_MOB(d)) = MAX(0, MIN(MAX_STAT, atoi(arg)));
			medit_disp_attributes_menu(d);
			return;
		case MEDIT_ATTR_COO:
			GET_COO(OLC_MOB(d)) = MAX(0, MIN(MAX_STAT, atoi(arg)));
			medit_disp_attributes_menu(d);
			return;
		case MEDIT_ATTR_AGI:
			GET_AGI(OLC_MOB(d)) = MAX(0, MIN(MAX_STAT, atoi(arg)));
			medit_disp_attributes_menu(d);
			return;
		case MEDIT_ATTR_PER:
			GET_PER(OLC_MOB(d)) = MAX(0, MIN(MAX_STAT, atoi(arg)));
			medit_disp_attributes_menu(d);
			return;
		case MEDIT_ATTR_KNO:
			GET_KNO(OLC_MOB(d)) = MAX(0, MIN(MAX_STAT, atoi(arg)));
			medit_disp_attributes_menu(d);
			return;
//		Opinions Menu
		case MEDIT_OPINIONS:
			OLC_OPINION(d) = NULL;
			switch (*arg) {
				//	Hates
				case '1':	// Sex
					OLC_OPINION(d) = &OLC_MOB(d)->mob->m_Hates;
					medit_disp_opinions_genders(d);
					OLC_MODE(d) = MEDIT_OPINIONS_HATES_SEX;
					break;
				case '2':	// Race
					OLC_OPINION(d) = &OLC_MOB(d)->mob->m_Hates;
					medit_disp_opinions_races(d);
					OLC_MODE(d) = MEDIT_OPINIONS_HATES_RACE;
					break;
				case '3':	// VirtualID
					OLC_OPINION(d) = &OLC_MOB(d)->mob->m_Hates;
					send_to_char("VirtualID (-1 for none): ", d->m_Character);
					OLC_MODE(d) = MEDIT_OPINIONS_HATES_VNUM;
					break;
				
				//	Fears
				case '4':	// Sex
					OLC_OPINION(d) = &OLC_MOB(d)->mob->m_Fears;
					medit_disp_opinions_genders(d);
					OLC_MODE(d) = MEDIT_OPINIONS_FEARS_SEX;
					break;
				case '5':	// Race
					OLC_OPINION(d) = &OLC_MOB(d)->mob->m_Fears;
					medit_disp_opinions_races(d);
					OLC_MODE(d) = MEDIT_OPINIONS_FEARS_RACE;
					break;
				case '6':	// VirtualID
					OLC_OPINION(d) = &OLC_MOB(d)->mob->m_Fears;
					send_to_char("VirtualID (-1 for none): ", d->m_Character);
					OLC_MODE(d) = MEDIT_OPINIONS_FEARS_VNUM;
					break;
					
				case '0':
					medit_disp_menu(d);
					return;
			}
			return;
		
		case MEDIT_OPINIONS_HATES_SEX:
		case MEDIT_OPINIONS_FEARS_SEX:
			if ((i = atoi(arg)))
			{
				if (i > 0 && i <= NUM_GENDERS)
					OLC_OPINION(d)->Toggle(Opinion::TypeSex, 1 << (i - 1));
				medit_disp_opinions_genders(d);
				OLC_VAL(d) = 1;
			} else
				medit_disp_opinions_menu(d);
			return;
		
		case MEDIT_OPINIONS_HATES_RACE:
		case MEDIT_OPINIONS_FEARS_RACE:
			if ((i = atoi(arg)))
			{
				if (i > 0 && i <= NUM_RACES)
					OLC_OPINION(d)->Toggle(Opinion::TypeRace, 1 << (i - 1));
				medit_disp_opinions_races(d);
				OLC_VAL(d) = 1;
			} else
				medit_disp_opinions_menu(d);
			return;
		
		case MEDIT_OPINIONS_HATES_VNUM:
		case MEDIT_OPINIONS_FEARS_VNUM:
			{
				CharacterPrototype *proto = mob_index.Find(arg);
				if (!proto)		OLC_OPINION(d)->Remove(Opinion::TypeVNum);
				else			OLC_OPINION(d)->Add(Opinion::TypeVNum, proto->GetVirtualID());
				OLC_VAL(d) = 1;
				medit_disp_opinions_menu(d);
			}
			return;
		
		case MEDIT_EQUIPMENT:
			switch (*arg)
			{
				case 'n':
				case 'N':
					send_to_char("What number in the list should the new item be: ", d->m_Character);
					OLC_MODE(d) = MEDIT_EQUIPMENT_NEW;
					break;
					
				case 'e':
				case 'E':
					send_to_char("Which item do you wish to edit: ", d->m_Character);
					OLC_MODE(d) = MEDIT_EQUIPMENT_EDIT_CHOOSE;
					break;
					
				case 'd':
				case 'D':
					send_to_char("Which item do you wish to delete: ", d->m_Character);
					OLC_MODE(d) = MEDIT_EQUIPMENT_DELETE;
					break;
					
				case 'q':
				case 'Q':
					OLC_VAL(d) = 1;
					medit_disp_menu(d);
					break;
			}
			return;
		
		case MEDIT_EQUIPMENT_NEW:
			if (!*arg)
				medit_disp_equipment(d);
			else
			{
				OLC_VAL(d) = atoi(arg);
				if (OLC_VAL(d) < 0 || OLC_VAL(d) > OLC_MOB(d)->mob->m_StartingEquipment.size())
					medit_disp_equipment(d);
				else
				{
					MobEquipmentList::iterator iter = OLC_MOB(d)->mob->m_StartingEquipment.begin();
					std::advance(iter, OLC_VAL(d));
					OLC_MOB(d)->mob->m_StartingEquipment.insert(iter, MobEquipment());
					
					medit_disp_equipment_item(d);
				}
			}
			return;
		
		case MEDIT_EQUIPMENT_EDIT_CHOOSE:
			if (!*arg)
				medit_disp_equipment(d);
			else
			{
				OLC_VAL(d) = atoi(arg);
				
				if (OLC_VAL(d) < 0 || OLC_VAL(d) >= OLC_MOB(d)->mob->m_StartingEquipment.size())
					medit_disp_equipment(d);
				else
					medit_disp_equipment_item(d);
			}
			return;
			
		case MEDIT_EQUIPMENT_DELETE:
			if (*arg)
			{
				OLC_VAL(d) = atoi(arg);
				
				if (OLC_VAL(d) >= 0 && OLC_VAL(d) < OLC_MOB(d)->mob->m_StartingEquipment.size())
				{
					OLC_MOB(d)->mob->m_StartingEquipment.erase(OLC_MOB(d)->mob->m_StartingEquipment.begin() + OLC_VAL(d));
				}
			}
			medit_disp_equipment(d);
			return;
			
		case MEDIT_EQUIPMENT_EDIT:
			switch (*arg)
			{
				case '1':
					send_to_char("Enter vnum of object: ", d->m_Character);
					OLC_MODE(d) = MEDIT_EQUIPMENT_EDIT_VNUM;
					break;
					
				case '2':
					medit_disp_equipment_positions(d);
					break;
					
				case 'q':
				case 'Q':
					medit_disp_equipment(d);
					break;
					
				default:
					medit_disp_equipment_item(d);
			}
			return;
		
		case MEDIT_EQUIPMENT_EDIT_VNUM:
			if (*arg)
			{
				ObjectPrototype *proto = obj_index.Find(arg);
				if (!proto)		send_to_char("`RInvalid object.`n\n", d->m_Character);
				else			OLC_MOBEQ_CMD(d).m_Virtual = proto->GetVirtualID();
			}
			medit_disp_equipment_item(d);
			return;
		
		case MEDIT_EQUIPMENT_EDIT_POS:
			if (*arg)
			{
				int pos = atoi(arg);
				
				if (pos < 0 || pos > NUM_WEARS)
					send_to_char("`RInvalid position.`n\n", d->m_Character);
				else
					OLC_MOBEQ_CMD(d).m_Position = pos - 1;
			}
			medit_disp_equipment_item(d);
			return;
		
		case MEDIT_AI_MENU:
			switch (*arg) {
				case '1':
					d->m_Character->Send("Aggressiveness to enemies in room: ");
					OLC_MODE(d) = MEDIT_AI_AGGRROOM;
					break;
				case '2':
					d->m_Character->Send("Aggressiveness to nearby enemies: ");
					OLC_MODE(d) = MEDIT_AI_AGGRRANGED;
					break;
				case '3':
					d->m_Character->Send("Aggressiveness to enemies attacking friends: ");
					OLC_MODE(d) = MEDIT_AI_AGGRLOYALTY;
					break;
				case '4':
					d->m_Character->Send("Maximum visual distance: ");
					OLC_MODE(d) = MEDIT_AI_AWARERANGE;
					break;
				case '5':
					d->m_Character->Send("Movement rate: ");
					OLC_MODE(d) = MEDIT_AI_MOVERATE;
					break;
				case '0':
				case 'q':
				case 'Q':
					medit_disp_menu(d);
					return;
				default:
					medit_disp_ai_menu(d);
					return;
			}
			OLC_VAL(d) = 1;
			return;
		
		
		case MEDIT_AI_AGGRROOM:
			OLC_MOB(d)->mob->m_AIAggrRoom = MAX(0, MIN(100, atoi(arg)));
			SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_AI);
			medit_disp_ai_menu(d);
			return;
		
		case MEDIT_AI_AGGRRANGED:
			OLC_MOB(d)->mob->m_AIAggrRanged = MAX(0, MIN(100, atoi(arg)));
			SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_AI);
			medit_disp_ai_menu(d);
			return;
		
		case MEDIT_AI_AGGRLOYALTY:
			OLC_MOB(d)->mob->m_AIAggrLoyalty = MAX(0, MIN(100, atoi(arg)));
			SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_AI);
			medit_disp_ai_menu(d);
			return;
		
/*		case MEDIT_AI_AWAREROOM:
			OLC_MOB(d)->mob->m_AIAwareRoom = MAX(0, MIN(100, atoi(arg)));
			SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_AI);
			medit_disp_ai_menu(d);
			return;
*/		
		case MEDIT_AI_AWARERANGE:
			OLC_MOB(d)->mob->m_AIAwareRange = MAX(0, MIN(8, atoi(arg)));
			SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_AI);
			medit_disp_ai_menu(d);
			return;
		
		case MEDIT_AI_MOVERATE:
			OLC_MOB(d)->mob->m_AIMoveRate = MAX(0, MIN(100, atoi(arg)));
			SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_AI);
			medit_disp_ai_menu(d);
			return;
		
		case MEDIT_TEAM:
			GET_TEAM(OLC_MOB(d)) = RANGE(atoi(arg), 0, 5);
			break;
/*-------------------------------------------------------------------*/
		default:
			/*
			 * We should never get here.
			 */
//			cleanup_olc(d);
			mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached default case!  Case: %d", OLC_MODE(d));
			send_to_char("Oops...\n", d->m_Character);
//			return;
			break;
	}
	/*
	 * END OF CASE 
	 * If we get here, we have probably changed something, and now want to
	 * return to main menu.  Use OLC_VAL as a 'has changed' flag  
	 */
	OLC_VAL(d) = 1;
	medit_disp_menu(d);
}

