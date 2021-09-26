/************************************************************************
 * OasisOLC - oedit.c                                              v1.5 *
 * Copyright 1996 Harvey Gilpin.                                        *
 * Original author: Levork                                              *
 ************************************************************************/


#include "characters.h"
#include "descriptors.h"
#include "zones.h"
#include "objects.h"
#include "rooms.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "boards.h"
#include "shop.h"
#include "olc.h"
#include "interpreter.h"
#include "buffer.h"
#include "extradesc.h"
#include "affects.h"
#include "staffcmds.h"
#include "constants.h"

#include "lexifile.h"

/*------------------------------------------------------------------------*/

//	External variable declarations.
extern struct attack_hit_type attack_hit_text[];
extern int skill_sort_info[NUM_SKILLS+1];

/*------------------------------------------------------------------------*/

//	Handy macros.
#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*/

void oedit_disp_container_flags_menu(DescriptorData *d);
void oedit_disp_vehicle_flags_menu(DescriptorData * d);
void oedit_disp_extradesc(DescriptorData * d);
void oedit_disp_extradesc_menu(DescriptorData *d);
void oedit_disp_weapon_menu(DescriptorData *d);
void oedit_disp_val0_menu(DescriptorData *d);
void oedit_disp_val1_menu(DescriptorData *d);
void oedit_disp_val2_menu(DescriptorData *d);
void oedit_disp_val3_menu(DescriptorData *d);
void oedit_disp_val4_menu(DescriptorData *d);
void oedit_disp_val5_menu(DescriptorData *d);
void oedit_disp_val6_menu(DescriptorData *d);
void oedit_disp_val7_menu(DescriptorData *d);
void oedit_disp_fval0_menu(DescriptorData * d);
void oedit_disp_fval1_menu(DescriptorData * d);
void oedit_disp_vidval0_menu(DescriptorData * d);
void oedit_disp_value_menu(DescriptorData *d);
void oedit_disp_values(DescriptorData *d);
void oedit_disp_type_menu(DescriptorData *d);
void oedit_disp_extra_menu(DescriptorData *d);
void oedit_disp_restriction_menu(DescriptorData * d);
void oedit_disp_wear_menu(DescriptorData *d);
void oedit_disp_menu(DescriptorData *d);
void oedit_disp_gun_menu(DescriptorData *d);
void oedit_disp_prompt_apply_menu(DescriptorData *d);
void oedit_disp_apply_menu(DescriptorData *d);
void oedit_disp_apply_skill_menu(DescriptorData * d);
void oedit_disp_shoot_menu(DescriptorData *d);
void oedit_disp_ammo_menu(DescriptorData *d);
void oedit_disp_aff_menu(DescriptorData *d);
void oedit_disp_damage_menu(DescriptorData * d);
void oedit_disp_skill_menu(DescriptorData * d);
void oedit_disp_var_menu(DescriptorData * d);

void oedit_parse(DescriptorData *d, const char *arg);
void oedit_liquid_type(DescriptorData *d);
void oedit_setup(DescriptorData *d, ObjectPrototype *proto);
void oedit_save_to_disk(ZoneData *zone);
void oedit_save_internally(DescriptorData *d);

void oedit_disp_scripts_menu(DescriptorData *d);

char *skill_name(int skillnum);

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_setup(DescriptorData *d, ObjectPrototype *proto)
{
	ObjData *obj = NULL;
	
	if (proto)
	{
	/* allocate object */
		obj = new ObjData(*proto->m_pProto);

		//	Copy all strings over.
		//	er... is this necessary?  CAN the strings be blank?
		if (obj->m_Keywords.empty())		obj->m_Keywords = "undefined";
		if (obj->m_Name.empty())			obj->m_Name = "undefined";
		if (obj->m_RoomDescription.empty())	obj->m_RoomDescription = "undefined";

		OLC_BEHAVIORSETS(d) = proto->m_BehaviorSets;
		OLC_TRIGLIST(d) = proto->m_Triggers;
		OLC_GLOBALVARS(d) = proto->m_InitialGlobals;
	}
	else
	{
		obj = new ObjData;
	
		obj->m_Keywords			= "unfinished object";
		obj->m_Name				= "an unfinished object";
		obj->m_RoomDescription	= "An unfinished object is lying here.";
		GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
	}
	
	OLC_OBJ(d) = obj;
	OLC_VAL(d) = 0;
	
	oedit_disp_menu(d);
}


/*------------------------------------------------------------------------*/

void oedit_save_internally(DescriptorData *d)
{
	REMOVE_BIT(GET_OBJ_EXTRA(OLC_OBJ(d)), ITEM_DELETED);
	
	if ((GET_OBJ_TYPE(OLC_OBJ(d)) != ITEM_WEAPON) && IS_GUN(OLC_OBJ(d))) {
		delete GET_GUN_INFO(OLC_OBJ(d));
		GET_GUN_INFO(OLC_OBJ(d)) = NULL;
	}
	
	/* write to internal tables */
	ObjectPrototype *index = obj_index.Find(OLC_VID(d));
	if (index)
	{
		/* we need to run through each and every object currently in the
		 * game to see which ones are pointing to this prototype */

		/* if object is pointing to this prototype, then we need to replace
		 * with the new one */
		FOREACH(ObjectList, object_list, iter)
		{
			ObjData *obj = *iter;
			if (obj->GetPrototype() == index) {
				CharData *ch_wear = obj->WornBy();
				CharData *ch_carry = obj->CarriedBy();
				int worn_on = obj->WornOn();
				ObjData *from = obj->Inside();
				
				if (ch_wear)		unequip_char(ch_wear, worn_on);
				else if (ch_carry)	obj->FromChar();
				else if (from)		obj->FromObject();
				
/* Copy all the important data, if the item is the same */
#define SAME(field)	(obj->field == old->field)
#define COPY_ALWAYS(field) obj->field = OLC_OBJ(d)->field
#define COPY(field) if (SAME(field))	COPY_ALWAYS(field)

				ObjData *old = index->m_pProto;
				
				COPY(m_Keywords);
				COPY(m_Name);
				COPY(m_RoomDescription);
				COPY(m_Description);
				
				//COPY_ALWAYS(m_ExtraDescriptions);
				
				for (int i = 0; i < NUM_OBJ_VALUES; ++i)
				{
					COPY(m_Value[i]);
				}
				for (int i = 0; i < NUM_OBJ_FVALUES; ++i)
				{
					COPY(m_FValue[i]);
				}
				
				COPY_ALWAYS(cost);
				obj->totalWeight -= obj->weight;
				COPY_ALWAYS(weight);
				obj->totalWeight += obj->weight;
				COPY(m_Timer);
				COPY_ALWAYS(m_Type);
				COPY_ALWAYS(wear);
				COPY_ALWAYS(extra);
				COPY_ALWAYS(m_AffectFlags);
				COPY_ALWAYS(m_MinimumLevelRestriction);
				COPY_ALWAYS(m_RaceRestriction);
				if (IS_GUN(OLC_OBJ(d))) {
					if (!IS_GUN(obj))
						GET_GUN_INFO(obj) = new GunData();
					
					if (IS_GUN(old)) {
						COPY_ALWAYS(m_pGun->skill);
						COPY_ALWAYS(m_pGun->attack);
						COPY_ALWAYS(m_pGun->damagetype);
						COPY_ALWAYS(m_pGun->damage);
						COPY_ALWAYS(m_pGun->strength);
						COPY_ALWAYS(m_pGun->rate);
						COPY_ALWAYS(m_pGun->range);
						COPY_ALWAYS(m_pGun->optimalrange);
						COPY_ALWAYS(m_pGun->modifier);
						COPY_ALWAYS(m_pGun->ammo.m_Type);
					} else {
						obj->m_pGun->skill = OLC_OBJ(d)->m_pGun->skill;
						obj->m_pGun->attack = OLC_OBJ(d)->m_pGun->attack;
						obj->m_pGun->damagetype = OLC_OBJ(d)->m_pGun->damagetype;
						obj->m_pGun->damage = OLC_OBJ(d)->m_pGun->damage;
						obj->m_pGun->strength = OLC_OBJ(d)->m_pGun->strength;
						obj->m_pGun->rate = OLC_OBJ(d)->m_pGun->rate;
						obj->m_pGun->range = OLC_OBJ(d)->m_pGun->range;
						obj->m_pGun->optimalrange = OLC_OBJ(d)->m_pGun->optimalrange;
						obj->m_pGun->modifier = OLC_OBJ(d)->m_pGun->modifier;
						obj->m_pGun->ammo.m_Type = OLC_OBJ(d)->m_pGun->ammo.m_Type;
					}
				}
				else if (IS_GUN(obj))
				{
					delete GET_GUN_INFO(obj);
					GET_GUN_INFO(obj) = NULL;
				}
				
				FOREACH(ScriptVariable::List, OLC_GLOBALVARS(d), var)
				{
					ScriptVariable *objVar = obj->GetScript()->m_Globals.Find(*var);
					ScriptVariable *oldVar = index->m_InitialGlobals.Find(*var);
					
					if (!objVar || !oldVar)	//	Not on object
					{
						if  (!objVar && !oldVar)	//	Was added to prototype, doesn't already exist
						{
							obj->GetScript()->m_Globals.Add(new ScriptVariable(**var));
						}
						//	else Was Manually Set or Unset
					}
					else if (oldVar->GetValueString() != (*var)->GetValueString()
						&& objVar->GetValueString() == oldVar->GetValueString())
					{
						//	Was changed in prototype, and existing object variable matches
						objVar->SetValue((*var)->GetValue());
					}
				}
				
				if (ch_wear)		equip_char(ch_wear, obj, worn_on);
				else if (ch_carry)	obj->ToChar(ch_carry);
				else if (from)		obj->ToObject(from);
			}
		}
		
		delete index->m_pProto;
		index->m_pProto = new ObjData(*OLC_OBJ(d));
		index->m_pProto->SetPrototype(index);
		
		index->m_Triggers.swap(OLC_TRIGLIST(d));
		index->m_BehaviorSets.swap(OLC_BEHAVIORSETS(d));
		index->m_InitialGlobals.swap(OLC_GLOBALVARS(d));
	}
	else
	{
		index = obj_index.insert(OLC_VID(d), new ObjData(*OLC_OBJ(d)));
		index->m_pProto->SetPrototype(index);
		
		index->m_Triggers.swap(OLC_TRIGLIST(d));
		index->m_BehaviorSets.swap(OLC_BEHAVIORSETS(d));
		index->m_InitialGlobals.swap(OLC_GLOBALVARS(d));
		
		OLC_OBJ(d)->SetPrototype(index);
	}
	
	// Now save the board...
	if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_BOARD)
		Board::SaveBoard(OLC_VID(d));
  
	oedit_save_to_disk(OLC_SOURCE_ZONE(d));
}
/*------------------------------------------------------------------------*/

void oedit_save_to_disk(ZoneData *zone) {
	BUFFER(fname, MAX_STRING_LENGTH);

	sprintf(fname, OBJ_PREFIX "%s.new", zone->GetTag());
	Lexi::OutputParserFile	file(fname);

	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open objects file \"%s\"!", fname);
		return;
	}

	file.BeginParser();
		
	/* start running through all objects in this zone */
	FOREACH_BOUNDED(ObjectPrototypeMap, obj_index, zone->GetHash(), i)
	{
		/* write object to disk */
		ObjectPrototype *	proto = *i;
		ObjData *			obj = proto->m_pProto;

		if (OBJ_FLAGGED(obj, ITEM_DELETED))
			continue;
		
		file.BeginSection(Lexi::Format("Object %s", proto->GetVirtualID().Print().c_str()));
		obj->Write(file);
		file.EndSection();	//	Object
	}

	file.EndParser();
	file.close();

	/*
	 * We're fubar'd if we crash between the two lines below.
	 */
	BUFFER(buf2, MAX_STRING_LENGTH);
	sprintf(buf2, OBJ_PREFIX "%s" OBJ_SUFFIX, zone->GetTag());
	remove(buf2);
	rename(fname, buf2);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/* For container flags */
void oedit_disp_container_flags_menu(DescriptorData * d) {
	BUFFER(buf, MAX_STRING_LENGTH);

	sprintbit(OLC_OBJ(d)->GetValue(OBJVAL_CONTAINER_FLAGS), container_bits, buf);
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"`g1`n) CLOSEABLE\n"
			"`g2`n) PICKPROOF\n"
			"`g3`n) CLOSED\n"
			"`g4`n) LOCKED\n"
			"Container flags: `c%s`n\n"
			"Enter flag, 0 to quit : ",
			buf);
}

char *vehicle_bits[] = {
  "GROUND",
  "AIR",
  "SPACE",
  "DEEPSPACE",
  "ABOVEWATER",
  "UNDERWATER",
  "\n",
};

void oedit_disp_vehicle_flags_menu(DescriptorData * d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	
	sprintbit(OLC_OBJ(d)->GetValue(OBJVAL_VEHICLE_FLAGS), vehicle_bits, buf);

	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"`g1`n) Ground (rough terrain)\n"
			"`g2`n) Air (flight)\n"
			"`g3`n) Space (upper atmosphere)\n"
			"`g4`n) Deep space (interstellar space)\n"
			"`g5`n) Water Surface\n"
			"`g6`n) Underwater\n"
			"Vehicle flags: `c%s`n\n"
			"Enter flag, 0 to quit : ",
			buf);
}

/* For extra descriptions */
void oedit_disp_extradesc_menu(DescriptorData * d) {
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Extra desc menu\n");
	
	int count = 0;
	FOREACH(ExtraDesc::List, OLC_OBJ(d)->m_ExtraDescriptions, extradesc)
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
	OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}


void oedit_disp_extradesc(DescriptorData * d) {
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

	OLC_MODE(d) = OEDIT_EXTRADESC;
}

/* For guns */
void oedit_disp_gun_menu(DescriptorData * d) {
	ObjData *obj = OLC_OBJ(d)/*.Get()*/;

	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Gun menu\n"
			"`g1`n) Damage       : `c%d\n"
			"`g2`n) Damage Type  : `c%s\n"
			"`g3`n) Attack Type  : `y%s\n"
			"`g4`n) Rate of Fire : `c%d\n"
			"`g5`n) Range        : `c%d\n"
			"`g6`n) Optimal Range: `c%d\n"
			"`g7`n) Skill        : `y%s\n"
			"`g8`n) Skill2       : `y%s\n"
			"`g9`n) Ammo Type    : `y%s\n"
			"`gD`n) Delete Gun\n"
			"   BALANCE: DPS: %-4d    DPA: %d\n"
			"\n"
			"`gQ`n) Quit Gun Setup\n"
			"Enter choice : ",
			GET_GUN_DAMAGE(obj),
			findtype(GET_GUN_DAMAGE_TYPE(obj), damage_types),
			findtype(GET_GUN_ATTACK_TYPE(obj), attack_types),
			GET_GUN_RATE(obj),
			GET_GUN_RANGE(obj),
			GET_GUN_OPTIMALRANGE(obj),
			skill_name(GET_GUN_SKILL(obj)),
			GET_GUN_SKILL2(obj) ? skill_name(GET_GUN_SKILL2(obj)) : "None Set",
			GET_GUN_AMMO_TYPE(obj) == -1 ? "<NONE>" : findtype(GET_GUN_AMMO_TYPE(obj), ammo_types),
			(int)((GET_GUN_DAMAGE(obj) * MAX(1, GET_GUN_RATE(obj)) * DAMAGE_DICE_SIZE / 10) / obj->GetFloatValue(OBJFVAL_WEAPON_SPEED)),
								GET_GUN_DAMAGE(obj) * MAX(1, GET_GUN_RATE(obj)) * DAMAGE_DICE_SIZE / 2);

	OLC_MODE(d) = OEDIT_GUN_MENU;
}

/* Ask for *which* apply to edit */
void oedit_disp_prompt_apply_menu(DescriptorData * d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	int counter;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
		if (OLC_OBJ(d)->affect[counter].modifier) {
			if (OLC_OBJ(d)->affect[counter].location < 0)
				sprintf(buf, "Skill: %s", skill_name(-OLC_OBJ(d)->affect[counter].location));
			else
				strcpy(buf, GetEnumName((AffectLoc)OLC_OBJ(d)->affect[counter].location));
			d->m_Character->Send(" `g%d`n) `y%+d `nto `y%s\n",
					counter + 1, OLC_OBJ(d)->affect[counter].modifier, buf);
		} else {
			d->m_Character->Send(" `g%d`n) `cNone.\n", counter + 1);
		}
	}
	
	send_to_char("\n`nEnter affection to modify (0 to quit): ", d->m_Character);
	
	OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/*. Ask for liquid type .*/
void oedit_liquid_type(DescriptorData * d) {
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_LIQ_TYPES; counter++) {
		d->m_Character->Send(" `g%2d`n) `y%-20.20s %s", 
				counter, drinks[counter], !(++columns % 2) ? "\n" : "");
	}
	send_to_char("\n`nEnter drink type: ", d->m_Character);
	
	OLC_MODE(d) = OEDIT_VALUE_3;
}

/* The actual apply to set */
void oedit_disp_apply_menu(DescriptorData * d) {
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_APPLIES; counter++)
	{
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter, GetEnumName((AffectLoc)counter),
					!(++columns % 2) ? "\n" : "");
	}
	d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter, "[SKILL]",
				!(++columns % 2) ? "\n" : "");
	send_to_char("\n`nEnter apply type (0 is no apply): ", d->m_Character);
	
	OLC_MODE(d) = OEDIT_APPLY;
}


/* The actual apply to set */
void oedit_disp_apply_skill_menu(DescriptorData * d) {
	int counter, columns = 0;
	int prevstat = -1;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 1; counter < NUM_SKILLS; counter++) {
		int i = skill_sort_info[counter];
		if (*skill_info[i].name == '!')
			continue;
		
		if (skill_info[i].stat != prevstat)
		{
			d->m_Character->Send("%s--- `c%s Skills`n\n",
					counter > 1 && (columns % 2) ? "\n" : "",
					stat_types[skill_info[i].stat]);
			columns = 0;
			prevstat = skill_info[i].stat;
		}
		
		d->m_Character->Send("`g%2d`n) `c%-26.26s %s", counter, skill_name(i),
					!(++columns % 2) ? "\n" : "");
	}
	send_to_char("\n`nEnter skill (0 cancels): ", d->m_Character);
	
	OLC_MODE(d) = OEDIT_APPLY_SKILL;
}



/* weapon type */
void oedit_disp_weapon_menu(DescriptorData * d) {
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_ATTACK_TYPES; counter++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter, attack_types[counter],
				!(++columns % 2) ? "\n" : "");
	}
	send_to_char("\n`nEnter weapon type: ", d->m_Character);
}

void oedit_disp_shoot_menu(DescriptorData * d) {
	oedit_disp_weapon_menu(d);
	OLC_MODE(d) = OEDIT_GUN_ATTACK_TYPE;
}


void oedit_disp_damage_menu(DescriptorData * d) {
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_DAMAGE_TYPES; counter++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter, damage_types[counter],
				!(++columns % 2) ? "\n" : "");
	}
	send_to_char("\n`nEnter damage type: ", d->m_Character);
}


void oedit_disp_ammo_menu(DescriptorData * d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif

	send_to_char("`g-1`n) `c<NONE>`n\n", d->m_Character);
	for (counter = 0; counter < NUM_AMMO_TYPES; counter++) {
		sprintf(buf, "`g%2d`n) `c%-20.20s`n%s", counter, ammo_types[counter],
				!(++columns % 2) ? "\n" : " ");
		send_to_char(buf, d->m_Character);
	}
	send_to_char("\n`nEnter ammo type: ", d->m_Character);
}


void oedit_disp_skill_menu(DescriptorData * d) {
	int counter;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 1; counter < NUM_SKILLS; counter++) {
		d->m_Character->Send("`g%2d`n) `c%s`n\n", counter, skill_name(counter));
	}
	send_to_char("\n`nEnter skill: ", d->m_Character);
}


void oedit_disp_values(DescriptorData *d) {
	BUFFER(buf, MAX_STRING_LENGTH);

	ObjData *obj = OLC_OBJ(d)/*.Get()*/;
	
	switch (GET_OBJ_TYPE(obj)) {
		case ITEM_LIGHT:
			d->Write("   `g1`n) Hours left: ");
			if (obj->GetValue(OBJVAL_LIGHT_HOURS) == -1)	d->Write("Infinite\n");
			else							d->m_Character->Send("%d\n", obj->GetValue(OBJVAL_LIGHT_HOURS));
			break;
		case ITEM_THROW:
		case ITEM_BOOMERANG:
			d->m_Character->Send(
					"   `g1`n) Damage   : %-7d           `g5`n) Speed    : %.2f\n"
					"   `g2`n) Type     : `y%-18.18s`g6`n) Str Bonus: x%.2f`n\n"
					"   `g3`n) Skill    : `y%s`n\n"
					"   `g4`n) Range    : `y%d`n\n"
					"      DPS      : `c%-3d`n                  DPA      : `c%d`n\n",
					obj->GetValue(OBJVAL_THROWABLE_DAMAGE), obj->GetFloatValue(OBJFVAL_THROWABLE_SPEED),
					findtype(obj->GetValue(OBJVAL_THROWABLE_DAMAGETYPE), damage_types), obj->GetFloatValue(OBJFVAL_THROWABLE_STRENGTH), 
					skill_name(obj->GetValue(OBJVAL_THROWABLE_SKILL)),
					obj->GetValue(OBJVAL_THROWABLE_RANGE),
					(int)((obj->GetValue(OBJVAL_THROWABLE_DAMAGE) * DAMAGE_DICE_SIZE / 10) / obj->GetFloatValue(OBJFVAL_THROWABLE_SPEED)),
							obj->GetValue(OBJVAL_THROWABLE_DAMAGE) * DAMAGE_DICE_SIZE / 2);
			break;
		case ITEM_GRENADE:
			d->m_Character->Send(
					"   `g1`n) Damage   : %d\n"
					"   `g2`n) Type     : `y%s`n\n"
					"   `g3`n) Timer    : %d\n",
					obj->GetValue(OBJVAL_GRENADE_DAMAGE),
					findtype(obj->GetValue(OBJVAL_GRENADE_DAMAGETYPE), damage_types),
					obj->GetValue(OBJVAL_GRENADE_TIMER));
			break;
		case ITEM_WEAPON:
			{
//				WeaponProperties *pWeaponProps = obj->GetItemProperties();

				d->m_Character->Send(
						"   `g1`n) Damage   : %-7d           `g5`n) Speed    : %.2f\n"
						"   `g2`n) Type     : `y%-18.18s`n`g6`n) Str Bonus: x%.2f\n"
						"   `g3`n) Skill    : `y%-18.18s`n`g7`n) Skill2   : `y%-18.18s`n\n"
						"   `g4`n) Attack   : `y%-18.18s`n`g8`n) Rate/Fire: %d\n"
						"      DPS      : `c%-3d`n                  DPA      : `c%d`n\n",
						obj->GetValue(OBJVAL_WEAPON_DAMAGE),		obj->GetFloatValue(OBJFVAL_WEAPON_SPEED),
						findtype(obj->GetValue(OBJVAL_WEAPON_DAMAGETYPE), damage_types),	obj->GetFloatValue(OBJFVAL_WEAPON_STRENGTH),
						skill_name(obj->GetValue(OBJVAL_WEAPON_SKILL)),	obj->GetValue(OBJVAL_WEAPON_SKILL2) ? skill_name(obj->GetValue(OBJVAL_WEAPON_SKILL2)) : "None Set",
						findtype(obj->GetValue(OBJVAL_WEAPON_ATTACKTYPE), attack_types),	obj->GetValue(OBJVAL_WEAPON_RATE),
						(int)((obj->GetValue(OBJVAL_WEAPON_DAMAGE) * MAX(1, obj->GetValue(OBJVAL_WEAPON_RATE)) * DAMAGE_DICE_SIZE / 10) / obj->GetFloatValue(OBJFVAL_WEAPON_SPEED)),
								obj->GetValue(OBJVAL_WEAPON_DAMAGE) * MAX(1, obj->GetValue(OBJVAL_WEAPON_RATE)) * DAMAGE_DICE_SIZE / 2);
			}
			break;
		case ITEM_AMMO:
			d->m_Character->Send(
					"   `g1`n) Ammo Type: `y%s\n"
					"   `g2`n) Amount   : `y%d\n",
					findtype(obj->GetValue(OBJVAL_AMMO_TYPE), ammo_types),
					obj->GetValue(OBJVAL_AMMO_AMOUNT));
			break;
		case ITEM_ARMOR:
			d->m_Character->Send("   `gArmor`n:\n");
			for(int i = 0; i < NUM_DAMAGE_TYPES; ++i)
				d->m_Character->Send("   `g%1d`n) %-9.9s: %-7d%% %s",
						i + 1, damage_types[i], obj->GetValue((ObjectTypeValue)i),
						(i % 2) ? "\n" : "        ");
			break;
		case ITEM_CONTAINER:
			{
				VirtualID	vid = obj->GetVIDValue(OBJVIDVAL_CONTAINER_KEY);
				
				sprintbit(obj->GetValue(OBJVAL_CONTAINER_FLAGS), container_bits, buf);
				d->m_Character->Send(
						"   `g1`n) Capacity : %-4d\n"
						"   `g2`n) Lock     : `y%s\n"
						"   `g3`n) Key      : %s\n",
						obj->GetValue(OBJVAL_CONTAINER_CAPACITY),
						buf,
						vid.Print().c_str());				
			}
			break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
			d->m_Character->Send(
					"   `g1`n) Capacity : %5d\n"
					"   `g2`n) Contents : %d\n"
					"   `g3`n) Poisoned : `y%s\n"
					"   `g4`n) Liquid   : %s\n",
					obj->GetValue(OBJVAL_FOODDRINK_FILL),
					obj->GetValue(OBJVAL_FOODDRINK_CONTENT),
					YESNO(obj->GetValue(OBJVAL_FOODDRINK_POISONED)),
					findtype(obj->GetValue(OBJVAL_FOODDRINK_TYPE), drinks));
			break;
		case ITEM_NOTE:
		case ITEM_PEN:
		case ITEM_KEY:
			d->Write("[NONE]");
			break;
		case ITEM_FOOD:
			d->m_Character->Send(
					"   `g1`n) Fill     : %-5d\n"
					"   `g2`n) Poisoned : `y%s\n",
					obj->GetValue(OBJVAL_FOODDRINK_FILL),
					YESNO(obj->GetValue(OBJVAL_FOODDRINK_POISONED)));
			break;
		case ITEM_BOARD:
			d->m_Character->Send(
					"      Levels\n"
					"   `g1`n) Read     : %-5d\n"
					"   `g2`n) Write    : %-5d\n"
					"   `g3`n) Remove   : %-5d\n",
					obj->GetValue(OBJVAL_BOARD_READLEVEL),
					obj->GetValue(OBJVAL_BOARD_WRITELEVEL),
					obj->GetValue(OBJVAL_BOARD_REMOVELEVEL));
			break;
		case ITEM_VEHICLE:
			{
				VirtualID	vid = obj->GetVIDValue(OBJVIDVAL_VEHICLE_ENTRY);
				
				sprintbit(obj->GetValue(OBJVAL_VEHICLE_FLAGS), vehicle_bits, buf);
				d->m_Character->Send(
						"   `g1`n) EntryRoom: %s\n"
						"   `g2`n) Flags    : %s\n"
				        "   `g3`n) Size     : %-5d\n",
				        vid.Print().c_str(),
				        buf,
				        obj->GetValue(OBJVAL_VEHICLE_SIZE));

			}
			break;
		case ITEM_VEHICLE_CONTROLS:
		case ITEM_VEHICLE_WINDOW:
		case ITEM_VEHICLE_WEAPON:
		case ITEM_VEHICLE_HATCH:
			{
				VirtualID	vid = obj->GetVIDValue(OBJVIDVAL_VEHICLEITEM_VEHICLE);
				
				d->m_Character->Send(
						"   `g1`n) Vehicle  : %s\n",
						vid.Print().c_str());
			}
			break;
		case ITEM_BED:
		case ITEM_CHAIR:
			d->m_Character->Send(
					"   `g1`n) Capacity : %d\n",
					obj->GetValue(OBJVAL_FURNITURE_CAPACITY));
			break;
		case ITEM_INSTALLABLE:
			{
				VirtualID	vid = obj->GetVIDValue(OBJVIDVAL_INSTALLABLE_INSTALLED);
				
				ObjectPrototype *proto = obj_index.Find(vid);
				d->m_Character->Send(
						"   `g1`n) Object   : %s [%s]\n"
						"   `g2`n) Time     : %d\n"
						"   `g3`n) Skill    : %s\n",
				        vid.Print().c_str(),
				        		proto ? proto->m_pProto->GetName() : "NONE",
				        obj->GetValue(OBJVAL_INSTALLABLE_TIME),
				        obj->GetValue(OBJVAL_INSTALLABLE_SKILL) ? skill_name(obj->GetValue(OBJVAL_INSTALLABLE_SKILL)) : "None" );
			}
			break;
		case ITEM_INSTALLED:
			d->m_Character->Send(
					"   `g1`n) Time     : %d\n"
					"   `g2`n) Permanent: %s\n",
			        obj->GetValue(OBJVAL_INSTALLED_TIME),
					YESNO(obj->GetValue(OBJVAL_INSTALLED_PERMANENT)));
			break;
		case ITEM_TOKEN:
			d->m_Character->Send(
					"   `g1`n) Value    : %d\n"
					"   `g2`n) Set      : %s\n"
					"   `g3`n) Signed by: %s\n",
					obj->GetValue(OBJVAL_TOKEN_VALUE),
					obj->GetValue(OBJVAL_TOKEN_SET) >= 0 && obj->GetValue(OBJVAL_TOKEN_SET) < NUM_TOKEN_SETS + 1 ?
							token_sets[obj->GetValue(OBJVAL_TOKEN_SET)] : "<INVALID SET>",
					get_name_by_id(obj->GetValue(OBJVAL_TOKEN_SIGNER), "<NOT SIGNED>"));
			break;
		default:
			d->m_Character->Send(
					"   `g1`n) Value 1  : %-7d           `g5`n) Value 5  : %d\n"
					"   `g2`n) Value 2  : %-7d           `g6`n) Value 6  : %d\n"
					"   `g3`n) Value 3  : %-7d           `g7`n) Value 7  : %d\n"
					"   `g4`n) Value 4  : %-7d           `g8`n) Value 8  : %d\n",
					obj->GetValue((ObjectTypeValue)0), obj->GetValue((ObjectTypeValue)4),
					obj->GetValue((ObjectTypeValue)1), obj->GetValue((ObjectTypeValue)5),
					obj->GetValue((ObjectTypeValue)2), obj->GetValue((ObjectTypeValue)6),
					obj->GetValue((ObjectTypeValue)3), obj->GetValue((ObjectTypeValue)7));
			break;
	}
}


void oedit_disp_value_menu(DescriptorData *d) {
	OLC_MODE(d) = OEDIT_VALUE_MENU;
	oedit_disp_values(d);
	d->Write(
			"  Q) Exit Value Editor\n"
			"Choose a value: ");
}


/* object value 1 */
void oedit_disp_val0_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_0;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_VEHICLE:	d->Write(
				"Note: Vehicles can hold vehicles of 1/2 their size\n"
				"      However, there is currently no maximum.\n"
				"Guide: (size is roughly cubic meters or storage space)\n"
				"      APCs: 10        Dropships: 20\n"
				"      Small spaceships: 60\n"
				"      \"Capital\" ships: 100\n"
				"\n"
				"Vehicle size: ");	break;
		case ITEM_BED:
		case ITEM_CHAIR:		d->Write("Capacity of sittable object: ");		break;
		case ITEM_AMMO:			oedit_disp_ammo_menu(d);						break;
		case ITEM_ARMOR:		d->Write("Armor: ");							break;
		case ITEM_CONTAINER:	d->Write("Max weight to contain: ");			break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:		d->Write("Max drink units: ");					break;
		case ITEM_FOOD:			d->Write("Hours to fill stomach: ");			break;
		case ITEM_BOARD:		d->Write("Enter the minimum level to read this board: ");break;
		case ITEM_THROW:
		case ITEM_BOOMERANG:
		case ITEM_GRENADE:
		case ITEM_WEAPON:		d->Write("Damage: ");							break;
		case ITEM_INSTALLABLE:	d->Write("Installed object: ");					break;
		case ITEM_INSTALLED:	d->Write("Seconds to destroy: ");				break;
		case ITEM_TOKEN:		d->Write("Value: ");							break;
	}
}


/* object value 2 */
void oedit_disp_val1_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_1;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_VEHICLE:		oedit_disp_vehicle_flags_menu(d);				break;
		case ITEM_THROW:
		case ITEM_BOOMERANG:
		case ITEM_GRENADE:
		case ITEM_WEAPON:		oedit_disp_damage_menu(d);						break;
		case ITEM_ARMOR:		d->Write("Armor: ");							break;
		case ITEM_CONTAINER:	oedit_disp_container_flags_menu(d);				break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:		d->Write("Initial drink units: ");				break;
		case ITEM_AMMO:			d->Write("Ammo provided: ");					break;
		case ITEM_BOARD:		d->Write("Minimum level to write: ");			break;
		case ITEM_INSTALLED:
		case ITEM_TOKEN:
			for (int i = 0; i < NUM_TOKEN_SETS + 1; ++i)
				d->m_Character->Send(" %d) %s\n", i, token_sets[i]);
			d->Write("Set: ");
			break;
	}
}


/* object value 3 */
void oedit_disp_val2_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_2;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_LIGHT:	d->Write("Number of hours (0 = burnt, -1 is infinite): ");
							break;
		case ITEM_THROW:
		case ITEM_BOOMERANG:oedit_disp_skill_menu(d);							break;
		case ITEM_WEAPON:	oedit_disp_weapon_menu(d);							break;
		case ITEM_INSTALLABLE:	d->Write("Seconds to install: ");				break;
		case ITEM_GRENADE:	d->Write("Seconds to countdown to explosion : ");	break;
		case ITEM_ARMOR:	d->Write("Armor: ");								break;
		case ITEM_CONTAINER:d->Write("Key to open container (-1 for no key): ");
							break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:	oedit_liquid_type(d);							break;
		case ITEM_BOARD:	d->Write("Minimum level to remove messages: ");	break;
	}
}


void oedit_disp_val3_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_3;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_WEAPON:	oedit_disp_skill_menu(d);						break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
		case ITEM_FOOD:		d->Write("Poisoned (0 = not poison): ");		break;
		case ITEM_ARMOR:		d->Write("Armor: ");						break;
//		case ITEM_BOARD:	Board_save_board(GET_OBJ_VNUM(OLC_OBJ(d)));		break;
		case ITEM_THROW:
		case ITEM_BOOMERANG:	d->Write("Range: ");						break;
		case ITEM_INSTALLABLE:	oedit_disp_skill_menu(d);						break;
	}
}


void oedit_disp_val4_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_4;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_WEAPON:	oedit_disp_skill_menu(d);							break;
		case ITEM_ARMOR:		d->Write("Armor: ");							break;
	}
}


void oedit_disp_val5_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_5;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_WEAPON:		d->Write("Rate of Fire: ");						break;
		case ITEM_ARMOR:		d->Write("Armor: ");							break;
	}
}


void oedit_disp_val6_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_6;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_ARMOR:		d->Write("Armor: ");							break;
		default:				d->Write("Value: ");
	}
}

/* object value 8 */
void oedit_disp_val7_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VALUE_7;
	d->Write("Value: ");
}


/* object fvalue 0 */
void oedit_disp_fval0_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_FVALUE_0;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_WEAPON:
		case ITEM_BOOMERANG:
		case ITEM_THROW:		d->Write("Speed: ");							break;
	}
}

/* object fvalue 1 */
void oedit_disp_fval1_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_FVALUE_1;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_WEAPON:
		case ITEM_BOOMERANG:
		case ITEM_THROW:		d->Write("Strength bonus multiplier: ");		break;
	}
}

/* object fvalue 2 */
void oedit_disp_vidval0_menu(DescriptorData * d) {
	OLC_MODE(d) = OEDIT_VIDVALUE_0;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_VEHICLE:		d->Write("Vehicle entryway room: ");			break;
		case ITEM_VEHICLE_CONTROLS:
		case ITEM_VEHICLE_WINDOW:
		case ITEM_VEHICLE_WEAPON:
		case ITEM_VEHICLE_HATCH:d->Write("Vehicle: ");							break;
	}
}

/* object type */
void oedit_disp_type_menu(DescriptorData * d) {
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_ITEM_TYPES; counter++)
	{
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter, GetEnumName((ObjectType)counter),
				!(++columns % 2) ? "\n" : "");
	}
	send_to_char("\n`nEnter object type: ", d->m_Character);
}

/* object extra flags */
void oedit_disp_extra_menu(DescriptorData * d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_ITEM_FLAGS; counter++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter + 1, extra_bits[counter],
				!(++columns % 2) ? "\n" : "");
	}
	sprintbit(GET_OBJ_EXTRA(OLC_OBJ(d)), extra_bits, buf);
	d->m_Character->Send("\n`nObject flags: `c%s\n"
				"`nEnter object extra flag (0 to quit): ", buf);
}

/* object extra flags */
void oedit_disp_restriction_menu(DescriptorData * d)
{
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_RACES - 1; counter++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter + 1, restriction_bits[counter],
				!(++columns % 2) ? "\n" : "");
	}
	d->m_Character->Send("\n`nObject flags: `c%s\n"
				"`nEnter object restriction flag (0 to quit): ",
				GetDisplayRaceRestrictionFlags(OLC_OBJ(d)->GetRaceRestriction()).c_str());
}

/* object wear flags */
void oedit_disp_wear_menu(DescriptorData * d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	int counter, columns = 0;
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_ITEM_WEARS; counter++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter + 1, wear_bits[counter],
				!(++columns % 2) ? "\n" : "");
	}
	sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf);
	d->m_Character->Send("\n`nWear flags: `c%s\n"
				"`nEnter wear flag, 0 to quit: ", buf);
}


/* display main menu */
void oedit_disp_menu(DescriptorData * d) {
	BUFFER(buf1, MAX_INPUT_LENGTH);
	int num_applies, counter;
	
	ObjData *obj = OLC_OBJ(d)/*.Get()*/;

	/*. Build buffers for first part of menu .*/
	sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf1);

	/*. Build first hallf of menu .*/
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Item number : [`c%s`n]\n"
				"`G1`n) Namelist : `y%s\n"
				"`G2`n) S-Desc   : `y%s\n"
				"`G3`n) L-Desc   : `y%s\n"
				"`G4`n) A-Desc   :-\n`y%s\n"
				"`G5`n) Type        : `c%s\n"
				"`G6`n) Extra flags : `c%s\n",
		OLC_VID(d).Print().c_str(),
		obj->GetAlias(), obj->GetName(), obj->GetRoomDescription(),
		obj->m_Description.empty() ? "<not set>" : obj->GetDescription(),
		GetEnumName(GET_OBJ_TYPE(obj)),
		buf1);
		
	/*. Build second half of menu .*/
	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1);

	d->m_Character->Send(
				"`G7`n) Restrictions: `c%s\n"
				"`G8`n) Wear flags  : `c%s\n"
				"`G9`n) Weight      : `c%d\n",
			GetDisplayRaceRestrictionFlags(obj->GetRaceRestriction()).c_str(), buf1, GET_OBJ_WEIGHT(obj));
 	
	d->m_Character->Send(
				"`GA`n) Affects     : `c%s\n"
				"`GB`n) Timer       : `c%-6d         `GV`n) Cost     : `c%d\n"
                "`GD`n) Values:\n",
			AFF_FLAGS(obj).print().c_str(), GET_OBJ_TIMER(obj), GET_OBJ_COST(obj));

	oedit_disp_values(d);

	if (IS_GUN(obj)) {
		d->m_Character->Send("`GG`n) Gun setup   : %d %s `gx`n %d `g@`n %d `g(`n%d`g) `gType`n %s\n"
					"   `nSkill : `y%-20.20s `nSkill2: `y%-20.20s`nAmmo  : `y%s\n"
					"   `nDPS   : `c%-4d                 `nDPA   : `c%d`n\n",
				GET_GUN_DAMAGE(obj),
					findtype(GET_GUN_DAMAGE_TYPE(obj), damage_types),
					GET_GUN_RATE(obj),
					GET_GUN_RANGE(obj), GET_GUN_OPTIMALRANGE(obj),
				findtype(GET_GUN_ATTACK_TYPE(obj), attack_types),
				skill_name(GET_GUN_SKILL(obj)),
				GET_GUN_SKILL2(obj) ? skill_name(GET_GUN_SKILL2(obj)) : "None Set",
				GET_GUN_AMMO_TYPE(obj) == -1 ? "<NONE>" : findtype(GET_GUN_AMMO_TYPE(obj), ammo_types),
				(int)((GET_GUN_DAMAGE(obj) * MAX(1, GET_GUN_RATE(obj)) * DAMAGE_DICE_SIZE / 10) / obj->GetFloatValue(OBJFVAL_WEAPON_SPEED)),
								GET_GUN_DAMAGE(obj) * MAX(1, GET_GUN_RATE(obj)) * DAMAGE_DICE_SIZE / 2);
	} else if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
		d->m_Character->Send("`GG`n) Gun setup   : [None]\n");
  	
  	num_applies = 0;
  	for (counter = 0; counter < MAX_OBJ_AFFECT; ++counter)
  		if (OLC_OBJ(d)->affect[counter].location != APPLY_NONE)
  			num_applies++;
  	
	buf1[0] = '\0';
	
	FOREACH(ScriptVector, OLC_TRIGLIST(d), i)
	{
		sprintf_cat(buf1, "%s ", i->Print().c_str());
	}
  	
	d->m_Character->Send(
				 "`GE`n`g) Applies menu (%d)             `GF`n`g) Extra descriptions menu`n\n"
				 "`GC`n) Clan        : `c%-5d          `GL`n) Level       : `c%d\n"
				 "`GS`n) Scripts     : `c%s %s\n"
				 "`GQ`n) Quit\n"
				 "Enter choice : ",
			num_applies,
			obj->GetClanRestriction(),
			obj->GetMinimumLevelRestriction(),
			*buf1 ? buf1 : "None",
				!OLC_GLOBALVARS(d).empty() ? "(Has Variables)" : "");
  	
	OLC_MODE(d) = OEDIT_MAIN_MENU;
}


/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void oedit_disp_aff_menu(DescriptorData *d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	int counter, columns = 0;
			
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (counter = 0; counter < NUM_AFF_FLAGS; counter++) {
		d->m_Character->Send("`g%2d`n) %-20.20s  %s", counter + 1, GetEnumName<AffectFlag>(counter), !(++columns % 2) ? "\n" : "");
	}
	d->m_Character->Send("\nAffect flags : `c%s`n\n"
				"Enter object affect flag (0 to quit) : ",
			AFF_FLAGS(OLC_OBJ(d)).print().c_str());
}

/*-------------------------------------------------------------------*/
/*. Display scripts attached .*/

void oedit_disp_scripts_menu(DescriptorData *d) {
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
		send_to_char(	"`y-- Scripts:`n\n", d->m_Character);

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
			if (var->GetContext())		sprintf_cat(buf, " {%d}", var->GetContext());
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
	OLC_MODE(d) = OEDIT_TRIGGERS;
}


void oedit_disp_var_menu(DescriptorData * d) {
	ScriptVariable *vd = OLC_TRIGVAR(d);

	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Variable menu\n"
			"`gN`n) Name         : `y%s\n"
			"`gV`n) Value        : `y%s\n"
			"`gC`n) Context      : `c%d\n"
			"`gQ`n) Quit Variable Setup\n"
			"Enter choice : ",
			vd->GetName(),
			vd->GetValue(),
			vd->GetContext());

	OLC_MODE(d) = OEDIT_TRIGGER_VARIABLE;
}


/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/


void oedit_parse(DescriptorData * d, const char *arg) {
	int			number, max_val, min_val;
	float		fnumber, max_fval, min_fval;
	
	switch (OLC_MODE(d)) {
		case OEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
//					send_to_char("Saving object to memory.\n", d->m_Character);
					send_to_char("Saving changes to object - no need to 'oedit save'!\n", d->m_Character);
					oedit_save_internally(d);
					mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s edits obj %s", d->m_Character->GetName(), OLC_VID(d).Print().c_str());

					//	Fall through
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
				default:
					send_to_char("Invalid choice!\n"
								 "Do you wish to save this object? ", d->m_Character);
					break;
			}
			return;
		case OEDIT_MAIN_MENU:
			/* throw us out to whichever edit mode based on user input */
			switch (*arg) {
				case 'q':
				case 'Q':
					if (OLC_VAL(d)) { /*. Something has been modified .*/
						send_to_char("Do you wish to save this object? ", d->m_Character);
						OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
					} else 
						cleanup_olc(d);
					return;
				case '1':
					send_to_char("Enter namelist : ", d->m_Character);
					OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
					break;
				case '2':
					send_to_char("Enter short desc : ", d->m_Character);
					OLC_MODE(d) = OEDIT_SHORTDESC;
					break;
				case '3':
					send_to_char("Enter long desc :-\n| ", d->m_Character);
					OLC_MODE(d) = OEDIT_LONGDESC;
					break;
				case '4':
					OLC_MODE(d) = OEDIT_ACTDESC;
					d->Write("Enter action description: (/s saves /h for help)\n\n");
					d->Write(OLC_OBJ(d)->GetDescription());
					d->StartStringEditor(OLC_OBJ(d)->m_Description, MAX_MESSAGE_LENGTH);
					OLC_VAL(d) = 1;
					break;
				case '5':
					oedit_disp_type_menu(d);
					OLC_MODE(d) = OEDIT_TYPE;
					break;
				case '6':
					oedit_disp_extra_menu(d);
					OLC_MODE(d) = OEDIT_EXTRAS;
					break;
				case '7':
					oedit_disp_restriction_menu(d);
					OLC_MODE(d) = OEDIT_RESTRICTIONS;
					break;
				case '8':
					oedit_disp_wear_menu(d);
					OLC_MODE(d) = OEDIT_WEAR;
					break;
				case '9':
					send_to_char("Enter weight : ", d->m_Character);
					OLC_MODE(d) = OEDIT_WEIGHT;
					break;
				case 'v':
				case 'V':
					send_to_char("Enter cost : ", d->m_Character);
					OLC_MODE(d) = OEDIT_COST;
					break;
				case 'b':
				case 'B':
					send_to_char("Enter timer : ", d->m_Character);
					OLC_MODE(d) = OEDIT_TIMER;
					break;
				case 'a':
				case 'A':
					oedit_disp_aff_menu(d);	
					OLC_MODE(d) = OEDIT_AFFECTS;
			/*. Object level flags in my mud...
			send_to_char("Enter level : ", d->m_Character);
			OLC_MODE(d) = OEDIT_LEVEL;
			.*/
					break;
				case 'd':
				case 'D':
					/*. Clear any old values .*/
					oedit_disp_value_menu(d);
					break;
				case 'e':
				case 'E':
					oedit_disp_prompt_apply_menu(d);
					break;
				case 'f':
				case 'F':
					oedit_disp_extradesc_menu(d);
					break;
				case 'g':
				case 'G':
					if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WEAPON) {
						if (!OLC_OBJ(d)->m_pGun)
							OLC_OBJ(d)->m_pGun = new GunData();
						oedit_disp_gun_menu(d);
					}
					break;
				case 'c':
				case 'C':
					send_to_char("Enter clan number : ", d->m_Character);
					OLC_MODE(d) = OEDIT_CLAN;
					break;
				case 'l':
				case 'L':
					send_to_char("Enter level : ", d->m_Character);
					OLC_MODE(d) = OEDIT_LEVEL;
					break;
				case 's':
				case 'S':
					oedit_disp_scripts_menu(d);
					break;
				default:
					oedit_disp_menu(d);
					break;
			}
			return;			/* end of OEDIT_MAIN_MENU */

		case OEDIT_EDIT_NAMELIST:
			OLC_OBJ(d)->m_Keywords = arg;
			break;

		case OEDIT_SHORTDESC:
			OLC_OBJ(d)->m_Name = arg;
			break;

		case OEDIT_LONGDESC:
			OLC_OBJ(d)->m_RoomDescription = arg;
			break;

		case OEDIT_TYPE:
			number = atoi(arg);
			if ((number < 1) || (number >= NUM_ITEM_TYPES)) {
				send_to_char("Invalid choice, try again : ", d->m_Character);
				return;
			} else {
				if (number != OLC_OBJ(d)->GetType())
				{
					for (int v = 0; v < 8; v++)
						OLC_OBJ(d)->SetValue((ObjectTypeValue)v, 0);					
				}

				OLC_OBJ(d)->m_Type = (ObjectType)number;
			}
			break;

		case OEDIT_EXTRAS:
			number = atoi(arg);
			if ((number < 0) || (number > NUM_ITEM_FLAGS)) {
				oedit_disp_extra_menu(d);
				return;
			} else {
				/* if 0, quit */
				if (number == 0)
					break;
				else { /* if already set.. remove */
					number -= 1;
					if (((1 << number) == ITEM_LOG) && !STF_FLAGGED(d->m_Character, STAFF_ADMIN))
						send_to_char("Only admins can toggle that flag.\n", d->m_Character);
					else
					{
						TOGGLE_BIT(GET_OBJ_EXTRA(OLC_OBJ(d)), 1 << number);
					}
					oedit_disp_extra_menu(d);
					return;
				}
			}
			break;

		case OEDIT_RESTRICTIONS:
			number = atoi(arg);
			if ((number < 0) || (number >= NUM_RACES)) {
				oedit_disp_restriction_menu(d);
				return;
			} else {
				/* if 0, quit */
				if (number == 0)
					break;
				else { /* if already set.. remove */
					OLC_OBJ(d)->m_RaceRestriction.flip(number - 1);
					oedit_disp_restriction_menu(d);
					return;
				}
			}
			break;
			
		case OEDIT_AFFECTS:
			number = atoi(arg);
			if (number == 0)
				break;
			else if ((unsigned)number > NUM_AFF_FLAGS)
			{
				oedit_disp_aff_menu(d);
				return;
			}
			else
			{
				number -= 1;	//	1- to 0- based index
				//	Flip the bit
				AFF_FLAGS(OLC_OBJ(d)).flip((AffectFlag)number);
				oedit_disp_aff_menu(d);
				return;
			}
			break;

		case OEDIT_WEAR:
			number = atoi(arg);
			if (number == 0)
				break;
			else if ((unsigned)number > NUM_ITEM_WEARS)
			{
				send_to_char("That's not a valid choice!\n", d->m_Character);
				oedit_disp_wear_menu(d);
				return;
			}
			else
			{
				number -= 1;
				
				TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << number);
				oedit_disp_wear_menu(d);
				return;
			}
			break;

		case OEDIT_WEIGHT:
			number = atoi(arg);
			GET_OBJ_TOTAL_WEIGHT(OLC_OBJ(d)) = GET_OBJ_WEIGHT(OLC_OBJ(d)) = number;
			break;

		case OEDIT_COST:
			number = atoi(arg);
			GET_OBJ_COST(OLC_OBJ(d)) = number;
			break;

		case OEDIT_TIMER:
			number = atoi(arg);
			GET_OBJ_TIMER(OLC_OBJ(d)) = number;
			break;

		case OEDIT_LEVEL:
			//	Object level flags on my mud...
			number = atoi(arg);
			OLC_OBJ(d)->m_MinimumLevelRestriction = number;
			break;

		case OEDIT_CLAN:
			//	Object level flags on my mud...
			number = atoi(arg);
			OLC_OBJ(d)->m_ClanRestriction = 0;
			if (Clan::GetClan(number))		OLC_OBJ(d)->m_ClanRestriction = number;
			else							send_to_char("That clan doesn't exist... resetting\n", d->m_Character);
			break;

		case OEDIT_GUN_MENU:
			switch (tolower(*arg)) {
				case 'q':
				case 'Q':
					break;
				case '1':
					send_to_char("Gun Damage: ", d->m_Character);
					OLC_MODE(d) = OEDIT_GUN_DAMAGE;
					return;
				case '2':
					send_to_char("Gun Damage Type:\n", d->m_Character);
					oedit_disp_damage_menu(d);
					OLC_MODE(d) = OEDIT_GUN_DAMAGE_TYPE;
					return;
				case '3':
					oedit_disp_shoot_menu(d);
					return;
				case '4':
					send_to_char("Rate of Fire: ", d->m_Character);
					OLC_MODE(d) = OEDIT_GUN_RATE;
					return;
				case '5':
					send_to_char("Range of weapon: ", d->m_Character);
					OLC_MODE(d) = OEDIT_GUN_RANGE;
					return;
				case '6':
					send_to_char("Optimal range of weapon: ", d->m_Character);
					OLC_MODE(d) = OEDIT_GUN_OPTIMALRANGE;
					return;
				case '7':
					oedit_disp_skill_menu(d);
					OLC_MODE(d) = OEDIT_GUN_SKILL;
					return;
				case '8':
					oedit_disp_skill_menu(d);
					OLC_MODE(d) = OEDIT_GUN_SKILL2;
					return;
				case '9':
					oedit_disp_ammo_menu(d);
					OLC_MODE(d) = OEDIT_GUN_AMMO_TYPE;
					return;
				case 'd':
				case 'D':
					send_to_char("Gun status of object deleted.", d->m_Character);
					delete GET_GUN_INFO(OLC_OBJ(d));
					GET_GUN_INFO(OLC_OBJ(d)) = NULL;
					break;
				default:
					oedit_disp_gun_menu(d);
					return;
			}
			break;
		
		case OEDIT_GUN_SKILL:
			number = atoi(arg);
			if (number != 0)
			{
				GET_GUN_SKILL(OLC_OBJ(d)) = RANGE(number, 1, NUM_SKILLS - 1);
			}
			oedit_disp_gun_menu(d);	
			return;

		case OEDIT_GUN_SKILL2:
			number = atoi(arg);
			GET_GUN_SKILL2(OLC_OBJ(d)) = RANGE(number, 0, NUM_SKILLS - 1);
			oedit_disp_gun_menu(d);	
			return;

		case OEDIT_GUN_DAMAGE:
			number = atoi(arg);
			GET_GUN_DAMAGE(OLC_OBJ(d)) = number;
			oedit_disp_gun_menu(d);
			return;

		case OEDIT_GUN_AMMO_TYPE:
			number = atoi(arg);
			GET_GUN_AMMO_TYPE(OLC_OBJ(d)) = MAX(-1, MIN(number, NUM_AMMO_TYPES - 1));
			oedit_disp_gun_menu(d);
			return;

		case OEDIT_GUN_ATTACK_TYPE:
			number = atoi(arg);
			GET_GUN_ATTACK_TYPE(OLC_OBJ(d)) = MAX(0, MIN(number, NUM_ATTACK_TYPES - 1));
			oedit_disp_gun_menu(d);
			return;

		case OEDIT_GUN_DAMAGE_TYPE:
			number = atoi(arg);
			GET_GUN_DAMAGE_TYPE(OLC_OBJ(d)) = MAX(0, MIN(number, NUM_DAMAGE_TYPES - 1));
			oedit_disp_gun_menu(d);
			return;

		case OEDIT_GUN_RANGE:
			number = atoi(arg);
			GET_GUN_RANGE(OLC_OBJ(d)) = MAX(0, MIN(number, 8));
			oedit_disp_gun_menu(d);
			return;

		case OEDIT_GUN_OPTIMALRANGE:
			number = atoi(arg);
			GET_GUN_OPTIMALRANGE(OLC_OBJ(d)) = MAX(0, MIN(number, 8));
			oedit_disp_gun_menu(d);
			return;

		case OEDIT_GUN_RATE:
			number = atoi(arg);
			GET_GUN_RATE(OLC_OBJ(d)) = MAX(1, MIN(number, 12));
			oedit_disp_gun_menu(d);
			return;

		case OEDIT_VALUE_MENU:
			switch (*arg) {
				case 'q':
				case 'Q':						oedit_disp_menu(d);			return;
				case '1':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_VEHICLE:
						case ITEM_VEHICLE_CONTROLS:
						case ITEM_VEHICLE_WINDOW:
						case ITEM_VEHICLE_HATCH:oedit_disp_vidval0_menu(d);	return;
						case ITEM_AMMO:
						case ITEM_ARMOR:
						case ITEM_CONTAINER:
						case ITEM_DRINKCON:
						case ITEM_FOUNTAIN:
						case ITEM_FOOD:
						case ITEM_BOARD:
						case ITEM_BED:
						case ITEM_CHAIR:
						case ITEM_THROW:
						case ITEM_BOOMERANG:
						case ITEM_WEAPON:
						case ITEM_GRENADE:
						case ITEM_INSTALLABLE:
						case ITEM_INSTALLED:
						case ITEM_TOKEN:		oedit_disp_val0_menu(d);	return;
						case ITEM_LIGHT:		oedit_disp_val2_menu(d);	return;
					}
					break;
				case '2':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_VEHICLE:		oedit_disp_val0_menu(d);	return;
						case ITEM_CONTAINER:
						case ITEM_DRINKCON:
						case ITEM_FOUNTAIN:
						case ITEM_BOARD:
						case ITEM_AMMO:
						case ITEM_ARMOR:
						case ITEM_INSTALLED:
						case ITEM_GRENADE:
						case ITEM_BOOMERANG:
						case ITEM_THROW:
						case ITEM_WEAPON:
						case ITEM_TOKEN:		oedit_disp_val1_menu(d);	return;
						case ITEM_INSTALLABLE:	oedit_disp_val2_menu(d);	return;
						case ITEM_FOOD:			oedit_disp_val3_menu(d);	return;
					}
					break;
				case '3':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_VEHICLE:		oedit_disp_val1_menu(d);	return;
						case ITEM_CONTAINER:
						case ITEM_GRENADE:
						case ITEM_BOOMERANG:
						case ITEM_THROW:
						case ITEM_ARMOR:
						case ITEM_BOARD:		oedit_disp_val2_menu(d);	return;
						case ITEM_TOKEN:
							if (!STF_FLAGGED(d->m_Character, STAFF_ADMIN))
								d->Write("Only admins can sign tokens.\n");
							else
							{
								if (OLC_OBJ(d)->GetValue(OBJVAL_TOKEN_SIGNER) == 0)
									OLC_OBJ(d)->SetValue(OBJVAL_TOKEN_SIGNER, GET_ID(d->m_Character));
								else
									OLC_OBJ(d)->SetValue(OBJVAL_TOKEN_SIGNER, 0);
								d->Write(OLC_OBJ(d)->GetValue(OBJVAL_TOKEN_SIGNER) ? "Signed.\n" : "Signature Removed.\n");
								OLC_VAL(d) = 1;
							}
//							else
//								oedit_disp_val2_menu(d);
							break;
						case ITEM_WEAPON:
						case ITEM_DRINKCON:
						case ITEM_INSTALLABLE:
						case ITEM_FOUNTAIN:		oedit_disp_val3_menu(d);	return;
					}
					break;
				case '4':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_DRINKCON:
						case ITEM_FOUNTAIN:
						case ITEM_WEAPON:		oedit_disp_val2_menu(d);	return;
						case ITEM_ARMOR:
						case ITEM_THROW:
						case ITEM_BOOMERANG:	oedit_disp_val3_menu(d);	return;
					}
					break;
				case '5':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_ARMOR:		oedit_disp_val4_menu(d);	return;
						case ITEM_WEAPON:
						case ITEM_BOOMERANG:
						case ITEM_THROW:		oedit_disp_fval0_menu(d);	return;
					}
					break;
				case '6':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_ARMOR:		oedit_disp_val5_menu(d);	return;
						case ITEM_WEAPON:
						case ITEM_BOOMERANG:
						case ITEM_THROW:		oedit_disp_fval1_menu(d);	return;
					}
					break;
				case '7':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_ARMOR:		oedit_disp_val6_menu(d);	return;
						case ITEM_WEAPON:		oedit_disp_val4_menu(d);	return;
					}
					break;
					
				case '8':
					switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
						case ITEM_WEAPON:		oedit_disp_val5_menu(d);	return;
					}
					break;
			}
			oedit_disp_value_menu(d);
			return;


		case OEDIT_VALUE_0:
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_AMMO:		min_val = 0;		max_val = NUM_AMMO_TYPES - 1;	break;
				case ITEM_ARMOR:	min_val = 0;		max_val = 100;			break;
				case ITEM_CONTAINER:
				case ITEM_DRINKCON:
				case ITEM_FOUNTAIN:	min_val = 0;		max_val = 32000;		break;
				case ITEM_FOOD:		min_val = -100;		max_val = 100;			break;
				case ITEM_BOARD:	min_val = 0;		max_val = 105;			break;
				case ITEM_CHAIR:
				case ITEM_BED:		min_val = 1;		max_val = 50;			break;
				case ITEM_THROW:
				case ITEM_BOOMERANG:
				case ITEM_GRENADE:
				case ITEM_WEAPON:	min_val = 1;		max_val = 1000;			break;
				case ITEM_INSTALLABLE:
					OLC_OBJ(d)->m_VIDValue[OBJVIDVAL_INSTALLABLE_INSTALLED] = VirtualID(arg, OLC_VID(d).zone);;
					OLC_VAL(d) = 1;
					oedit_disp_value_menu(d);
					return;
				case ITEM_INSTALLED:	min_val = 0;		max_val = 300;			break;
				case ITEM_TOKEN:	min_val = 1;		max_val = 250;			break;
				case ITEM_VEHICLE:
					/* needs some special handling since we are dealing with flag values
					 * here */
					if (number == 0)
					{
						break;
					}
					else if ((unsigned int)number <= 6)
					{
						number = OLC_OBJ(d)->GetValue(OBJVAL_VEHICLE_FLAGS);
						TOGGLE_BIT(number, 1 << (number - 1));
						OLC_OBJ(d)->SetValue(OBJVAL_VEHICLE_FLAGS, number);
					}
					OLC_VAL(d) = 1;
					oedit_disp_val0_menu(d);
					return;
				default:			min_val = -32000;	max_val = 32000;
			}
			
			OLC_OBJ(d)->SetValue((ObjectTypeValue)0, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;
			
		case OEDIT_VALUE_1:
			/* here, I do need to check for outofrange values */
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_ARMOR:	min_val = 0;		max_val = 100;			break;
				case ITEM_AMMO:
				case ITEM_DRINKCON:
				case ITEM_FOUNTAIN:	min_val = 0;		max_val = 32000;		break;
				case ITEM_BOARD:	min_val = 0;		max_val = 105;			break;
				
				case ITEM_INSTALLED:
				case ITEM_GRENADE:
				case ITEM_BOOMERANG:
				case ITEM_THROW:
				case ITEM_WEAPON:	min_val = 0;		max_val = NUM_DAMAGE_TYPES - 1;	break;
				case ITEM_CONTAINER:
					/* needs some special handling since we are dealing with flag values
					 * here */
					if (number == 0) {
						oedit_disp_value_menu(d);
						return;
					}
					else if (number >= 1 && number <= 4)
					{
						Flags tmp = OLC_OBJ(d)->GetValue(OBJVAL_CONTAINER_FLAGS);
						TOGGLE_BIT(tmp, (1 << (number - 1)));
						OLC_OBJ(d)->SetValue(OBJVAL_CONTAINER_FLAGS, tmp);
						OLC_VAL(d) = 1;
					}
					oedit_disp_container_flags_menu(d);
					return;
				case ITEM_TOKEN:min_val = 0;			max_val = NUM_TOKEN_SETS;break;
				case ITEM_VEHICLE:	min_val = 0;		max_val = INT_MAX;		break;

				default:		min_val = -32000;		max_val = 32000;		break;
			}
			OLC_OBJ(d)->SetValue((ObjectTypeValue)1, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;

		case OEDIT_VALUE_2:
			number = atoi(arg);
			/*. Quick'n'easy error checking .*/
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_ARMOR:	min_val = 0;		max_val = 100;			break;
				case ITEM_LIGHT:	min_val = -1;		max_val = 32000;		break;
				case ITEM_THROW:
				case ITEM_BOOMERANG:min_val = 0;		max_val = NUM_SKILLS - 1;	break;
				case ITEM_WEAPON:	min_val = 0;		max_val = NUM_ATTACK_TYPES - 1;	break;
				case ITEM_GRENADE:	min_val = 0;		max_val = 3600;			break;
				case ITEM_BOARD:	min_val = 1;		max_val = 105;			break;
				case ITEM_DRINKCON:
				case ITEM_FOUNTAIN:	min_val = 0;		max_val = NUM_LIQ_TYPES -1;	break;
				
				case ITEM_CONTAINER:
					OLC_OBJ(d)->m_VIDValue[OBJVIDVAL_CONTAINER_KEY] = VirtualID(arg, OLC_VID(d).zone);
					OLC_VAL(d) = 1;
					oedit_disp_value_menu(d);
					return;
				case ITEM_INSTALLABLE: min_val = 0;		max_val = 60;			break;
				default:			min_val = -32000;	max_val = 32000;		break;
			}
			OLC_OBJ(d)->SetValue((ObjectTypeValue)2, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;

			oedit_disp_value_menu(d);
			return;

		case OEDIT_VALUE_3:
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_ARMOR:	min_val = 0;		max_val = 100;			break;
				case ITEM_THROW:
				case ITEM_BOOMERANG:min_val = 0;		max_val = 8;			break;
				case ITEM_WEAPON:	min_val = 0;		max_val = NUM_SKILLS - 1;	break;
				case ITEM_DRINKCON:
				case ITEM_FOUNTAIN:
				case ITEM_FOOD:		min_val = 0;		max_val = 1;			break;
				case ITEM_INSTALLABLE: min_val = 0;		max_val = NUM_SKILLS - 1;	break;
				default:			min_val = -32000;	max_val = 32000;		break;
			}
			OLC_OBJ(d)->SetValue((ObjectTypeValue)3, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;
			
			oedit_disp_value_menu(d);
			return;

		case OEDIT_VALUE_4:
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_ARMOR:	min_val = 0;		max_val = 100;			break;
				case ITEM_WEAPON:	min_val = 0;		max_val = NUM_SKILLS - 1;	break;
				default:			min_val = -32000;	max_val = 32000;		break;
			}
			OLC_OBJ(d)->SetValue((ObjectTypeValue)4, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;

		case OEDIT_VALUE_5:
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_ARMOR:	min_val = 0;		max_val = 100;			break;
				case ITEM_WEAPON:	min_val = 1;		max_val = 5;			break;
				default:			min_val = -32000;	max_val = 32000;		break;
			}
			OLC_OBJ(d)->SetValue((ObjectTypeValue)5, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;

		case OEDIT_VALUE_6:
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_ARMOR:	min_val = 0;		max_val = 100;			break;
				default:
					min_val = -32000;
					max_val = 32000;
					break;
			}
			OLC_OBJ(d)->SetValue((ObjectTypeValue)6, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;

		case OEDIT_VALUE_7:
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				default:
					min_val = -32000;
					max_val = 32000;
					break;
			}
			OLC_OBJ(d)->SetValue((ObjectTypeValue)7, RANGE(number, min_val, max_val));
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;


		case OEDIT_FVALUE_0:
			fnumber = atof(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_WEAPON:
				case ITEM_BOOMERANG:
				case ITEM_THROW:
					min_fval = 0.1f;
					max_fval = 10.0f;
					break;
				default:
					min_fval = 0.0f;
					max_fval = 1.0f;
					break;
			}
			OLC_OBJ(d)->m_FValue[0] = RANGE(fnumber, min_fval, max_fval);
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;

		case OEDIT_FVALUE_1:
			fnumber = atof(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_WEAPON:
				case ITEM_BOOMERANG:
				case ITEM_THROW:
					min_fval = 0.0f;
					max_fval = 5.0f;
					break;
				default:
					min_fval = 0.0f;
					max_fval = 1.0f;
					break;
			}
			OLC_OBJ(d)->m_FValue[1] = RANGE(fnumber, min_fval, max_fval);
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;
			
		case OEDIT_VIDVALUE_0:
			number = atoi(arg);
			switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
				case ITEM_VEHICLE:
					OLC_OBJ(d)->m_VIDValue[OBJVIDVAL_VEHICLE_ENTRY] = VirtualID(arg, OLC_VID(d).zone);
					break;
				case ITEM_VEHICLE_CONTROLS:
				case ITEM_VEHICLE_WINDOW:
				case ITEM_VEHICLE_HATCH:
				case ITEM_VEHICLE_WEAPON:
					OLC_OBJ(d)->m_VIDValue[OBJVIDVAL_VEHICLEITEM_VEHICLE] = VirtualID(arg, OLC_VID(d).zone);
					break;
			}
			
			OLC_VAL(d) = 1;
			oedit_disp_value_menu(d);
			return;
				
		case OEDIT_PROMPT_APPLY:
			number = atoi(arg);
			if (number == 0)
				break;
			else if (number < 0 || number > MAX_OBJ_AFFECT)
				oedit_disp_prompt_apply_menu(d);
			else {
				OLC_VAL(d) = number - 1;
				OLC_MODE(d) = OEDIT_APPLY;
				oedit_disp_apply_menu(d);
			}
			return;

		case OEDIT_APPLY:
			number = atoi(arg);
			if (number == 0) {
				OLC_OBJ(d)->affect[OLC_VAL(d)].location = 0;
				OLC_OBJ(d)->affect[OLC_VAL(d)].modifier = 0;
				oedit_disp_prompt_apply_menu(d);
			} else if (number >= 0 && number < NUM_APPLIES) {
				OLC_OBJ(d)->affect[OLC_VAL(d)].location = number;
				send_to_char("Enter value: ", d->m_Character);
				OLC_MODE(d) = OEDIT_APPLYMOD;
			} else if (number == NUM_APPLIES)
				oedit_disp_apply_skill_menu(d);
			else
				oedit_disp_apply_menu(d);
			return;
		
		case OEDIT_APPLY_SKILL:
			number = atoi(arg);
			if (number > 0 && number < NUM_SKILLS &&
					skill_info[skill_sort_info[number]].name[0] != '!')
			{
				OLC_OBJ(d)->affect[OLC_VAL(d)].location = -skill_sort_info[number];
				d->m_Character->Send("Enter value to modify %s: ", skill_info[skill_sort_info[number]].name);
				OLC_MODE(d) = OEDIT_APPLYMOD;
			}
			else
			{
				OLC_OBJ(d)->affect[OLC_VAL(d)].location = 0;
				OLC_OBJ(d)->affect[OLC_VAL(d)].modifier = 0;
				oedit_disp_prompt_apply_menu(d);
			}
			return;
			

		case OEDIT_APPLYMOD:
			number = atoi(arg);
			OLC_OBJ(d)->affect[OLC_VAL(d)].modifier = number;
			oedit_disp_prompt_apply_menu(d);
			return;
		
		case OEDIT_EXTRADESC_MENU:
			switch (*arg)
			{
				case 'a':
				case 'A':
					OLC_OBJ(d)->m_ExtraDescriptions.push_back(ExtraDesc());
					OLC_DESC(d) = &OLC_OBJ(d)->m_ExtraDescriptions.back();
					OLC_VAL(d) = 1;
					oedit_disp_extradesc(d);
					return;
				case 'd':
				case 'D':
					d->m_Character->Send("Delete which description: ");
					OLC_MODE(d) = OEDIT_EXTRADESC_DELETE;
					return;
				
				case 'q':
				case 'Q':
					break;
				
				default:
					if (is_number(arg))
					{
						number = atoi(arg);
						
						FOREACH(ExtraDesc::List, OLC_OBJ(d)->m_ExtraDescriptions, extradesc)
						{
							if (--number == 0)
							{
								OLC_DESC(d) = &*extradesc;
								oedit_disp_extradesc(d);
								return;
							}
						}
					}
					oedit_disp_extradesc_menu(d);
					return;
			}
			break;
		
		case OEDIT_EXTRADESC_DELETE:
			if (is_number(arg))
			{
				number = atoi(arg) - 1;
				
				if ((unsigned int)number >= OLC_OBJ(d)->m_ExtraDescriptions.size())
					send_to_char("Invalid extra description.", d->m_Character);
				else
				{
					ExtraDesc::List::iterator extradesc = OLC_OBJ(d)->m_ExtraDescriptions.begin();
					std::advance(extradesc, number);
					OLC_OBJ(d)->m_ExtraDescriptions.erase(extradesc);
					OLC_VAL(d) = 1;
				}
			}
			oedit_disp_extradesc_menu(d);
			return;
		
		case OEDIT_EXTRADESC_KEY:
			OLC_DESC(d)->keyword = *arg ? arg : NULL;
			oedit_disp_extradesc(d);
			return;

		case OEDIT_EXTRADESC:
			switch (*arg) {
				case 'q':
				case 'Q':	/* if something got left out */
/*					if (!SSData(OLC_DESC(d)->keyword) || !SSData(OLC_DESC(d)->description)) {
						OLC_OBJ(d)->exDesc.Remove(OLC_DESC(d));
						delete OLC_DESC(d);
						OLC_DESC(d) = NULL;
					}
*/					break;

				case '1':
					OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
					OLC_VAL(d) = 1;
					send_to_char("Enter keywords, separated by spaces :-\n| ", d->m_Character);
					return;

				case '2':
					OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
					d->Write("Enter the extra description: (/s saves /h for help)\n\n");
					d->Write(OLC_DESC(d)->description.c_str());
					d->StartStringEditor(OLC_DESC(d)->description, MAX_MESSAGE_LENGTH);
					OLC_VAL(d) = 1;
					return;
			}
			oedit_disp_extradesc_menu(d);
			return;
			
		case OEDIT_TRIGGERS:
			switch (*arg) {
				case 'b':
				case 'B':
					send_to_char("Enter the name of set to attach: ", d->m_Character);
					OLC_MODE(d) = OEDIT_TRIGGER_ADD_BEHAVIORSET;
					return;
				case 'a':
				case 'A':
					send_to_char("Enter the VNUM of script to attach: ", d->m_Character);
					OLC_MODE(d) = OEDIT_TRIGGER_ADD_SCRIPT;
					return;
				case 'v':
				case 'V':
					OLC_TRIGVAR(d) = OLC_GLOBALVARS(d).AddBlank();
					
					oedit_disp_var_menu(d);
					
					return;
				case 'e':
				case 'E':
					send_to_char("Enter item in list to edit: ", d->m_Character);
					OLC_MODE(d) = OEDIT_TRIGGER_CHOOSE_VARIABLE;
					return;
				case 'r':
				case 'R':
					send_to_char("Enter item in list to remove: ", d->m_Character);
					OLC_MODE(d) = OEDIT_TRIGGER_REMOVE;
					return;
				case 'q':
				case 'Q':
					oedit_disp_menu(d);
					return;
				default:
					oedit_disp_scripts_menu(d);
					return;
			}
			break;
		
		case OEDIT_TRIGGER_ADD_BEHAVIORSET:
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
			
			oedit_disp_scripts_menu(d);
			return;
		
		case OEDIT_TRIGGER_ADD_SCRIPT:
			if (IsVirtualID(arg))
			{
				OLC_TRIGLIST(d).push_back(VirtualID(arg));
				send_to_char("Script attached.\n", d->m_Character);
				OLC_VAL(d) = 1;
			}
			else
				send_to_char("Virtual IDs only, please.\n", d->m_Character);
			oedit_disp_scripts_menu(d);
			return;
			
		case OEDIT_TRIGGER_REMOVE:
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
			oedit_disp_scripts_menu(d);
			return;
		
		case OEDIT_TRIGGER_CHOOSE_VARIABLE:
			if (is_number(arg))
			{
				number = atoi(arg) - 1;
				
				number -= OLC_BEHAVIORSETS(d).size();
				number -= OLC_TRIGLIST(d).size();
				
				if (number < 0)
					send_to_char("Variables only.\n", d->m_Character);
				else if (number < OLC_GLOBALVARS(d).size())
				{
					
					ScriptVariable::List::iterator var = OLC_GLOBALVARS(d).begin();
					std::advance(var, number);
					
					OLC_TRIGVAR(d) = *var;
					oedit_disp_var_menu(d);
					return;
				}
				else
					send_to_char("Invalid number.\n", d->m_Character);
			}
			else
				send_to_char("Numbers only, please.\n", d->m_Character);
			oedit_disp_scripts_menu(d);
			return;
		
		case OEDIT_TRIGGER_VARIABLE:
			switch (*arg)
			{
				case 'n':
				case 'N':
					send_to_char("Name: ", d->m_Character);
					OLC_MODE(d) = OEDIT_TRIGGER_VARIABLE_NAME;
					break;
				case 'v':
				case 'V':
					send_to_char("Value: ", d->m_Character);
					OLC_MODE(d) = OEDIT_TRIGGER_VARIABLE_VALUE;
					break;
				case 'c':
				case 'C':
					send_to_char("Context (0 = default): ", d->m_Character);
					OLC_MODE(d) = OEDIT_TRIGGER_VARIABLE_CONTEXT;
					break;
				case 'q':
				case 'Q':
					if (OLC_TRIGVAR(d)->GetNameString().empty())
					{
//						OLC_GLOBALVARS(d).remove(OLC_TRIGVAR(d));
						ScriptVariable::List::iterator iter = Lexi::Find(OLC_GLOBALVARS(d), OLC_TRIGVAR(d));
						if (iter != OLC_GLOBALVARS(d).end())
							OLC_GLOBALVARS(d).erase(iter);
						delete OLC_TRIGVAR(d);
						OLC_TRIGVAR(d) = NULL;
					}
					
					oedit_disp_scripts_menu(d);
					break;
				default:
					oedit_disp_var_menu(d);
					break;
			}
			return;
		
		case OEDIT_TRIGGER_VARIABLE_NAME:
			if (*arg)
			{
				ScriptVariable var(arg, OLC_TRIGVAR(d)->GetContext());
				var.SetValue(OLC_TRIGVAR(d)->GetValue());
				
				*OLC_TRIGVAR(d) = var;
				OLC_GLOBALVARS(d).Sort();

				OLC_VAL(d) = 1;
			}
			oedit_disp_var_menu(d);
			return;
			
		case OEDIT_TRIGGER_VARIABLE_VALUE:
			if (*arg)
			{
				OLC_TRIGVAR(d)->SetValue(arg);
				OLC_VAL(d) = 1;
			}
			oedit_disp_var_menu(d);
			return;

		case OEDIT_TRIGGER_VARIABLE_CONTEXT:
			if (is_number(arg))
			{
				ScriptVariable var(OLC_TRIGVAR(d)->GetName(), atoi(arg));
				var.SetValue(OLC_TRIGVAR(d)->GetValue());
				
				*OLC_TRIGVAR(d) = var;
				OLC_GLOBALVARS(d).Sort();
				
				OLC_VAL(d) = 1;
			}
			oedit_disp_var_menu(d);
			return;
			
		default:
			mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Reached default case in oedit_parse()!  Case: (%d)", OLC_MODE(d));
			send_to_char("Oops...\n", d->m_Character);
			break;
	}

	/*. If we get here, we have changed something .*/
	OLC_VAL(d) = 1; /*. Has changed flag .*/
	oedit_disp_menu(d);
}
