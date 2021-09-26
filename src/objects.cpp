/*************************************************************************
*   File: objects.cp                 Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for objects                                       *
*************************************************************************/



#include "types.h"
#include "objects.h"
#include "rooms.h"
#include "characters.h"
#include "utils.h"
#include "scripts.h"
#include "comm.h"
#include "buffer.h"
#include "files.h"
#include "event.h"
#include "affects.h"
#include "db.h"
#include "extradesc.h"
#include "handler.h"
#include "interpreter.h"
#include "find.h"
#include "constants.h"
#include "spells.h"

#include <map>
#include <vector>

#include "lexiparser.h"
#include "idmap.h"

// External function declarations
CharData *get_char_on_obj(ObjData *obj);
void tag_argument(char *argument, char *tag);


ObjectList				object_list;		//	global linked list of objs
ObjectList				persistentObjects;
ObjectPrototypeMap		obj_index;			//	index table for object file


GunData::GunData(void) :
	skill(0),
	skill2(0),
	attack(0),
	damagetype(0),
	damage(0),
	strength(0),
	rate(0),
	range(0),
	optimalrange(0),
	modifier(0)
{
	ammo.m_Type = 0;
	ammo.m_Amount = 0;
	ammo.m_Flags = 0;
}


ObjData::ObjData(void)
:	m_Prototype(NULL)
,	m_InRoom(NULL)
,	m_Inside(NULL)
,	m_CarriedBy(NULL)
,	m_WornBy(NULL)
,	m_WornOn(WEAR_NOT_WORN)
,	cost(0)
,	weight(0)
,	totalWeight(0)
,	m_Timer(0)
,	m_Type(ITEM_UNDEFINED)
,	wear(0)
,	extra(0)
,	m_MinimumLevelRestriction(0)
,	m_ClanRestriction(0)
,	bought(0)
,	buyer(0)
,	owner(0)
,	m_pGun(NULL)
{
	int i;
	
	for (i = 0; i < NUM_OBJ_VALUES; ++i)		m_Value[i] = 0;
	for (i = 0; i < NUM_OBJ_FVALUES; ++i)		m_FValue[i] = 0.0f;
	
	for (i = 0; i < MAX_OBJ_AFFECT; i++) {
		affect[i].location = APPLY_NONE;
		affect[i].modifier = 0;
	}
}


ObjData::ObjData(const ObjData &obj) :
	Entity(obj),
	m_Prototype(obj.m_Prototype),
	m_InRoom(NULL),
	m_Inside(NULL),
	m_CarriedBy(NULL),			//	These aren't inherited
	m_WornBy(NULL),
	m_WornOn(WEAR_NOT_WORN),
	m_Keywords(obj.m_Keywords),
	m_Name(obj.m_Name),
	m_RoomDescription(obj.m_RoomDescription),
	m_Description(obj.m_Description),
	m_ExtraDescriptions(obj.m_ExtraDescriptions),
	cost(obj.cost),
	weight(obj.weight),
	totalWeight(obj.weight),	//	We always start at obj.weight!
	m_Timer(obj.m_Timer),
	m_Type(obj.m_Type),
	wear(obj.wear),
	extra(obj.extra),
	m_AffectFlags(obj.m_AffectFlags),
	m_MinimumLevelRestriction(obj.m_MinimumLevelRestriction),
	m_RaceRestriction(obj.m_RaceRestriction),
	m_ClanRestriction(obj.m_ClanRestriction),
	bought(obj.bought),
	buyer(obj.buyer),
	owner(obj.owner),
	m_pGun(NULL)					//	Allocate ourselves
{	
	for (int i = 0; i < 8; ++i)						m_Value[i] = obj.m_Value[i];
	for (int i = 0; i < NUM_OBJ_FVALUES; ++i)		m_FValue[i] = obj.m_FValue[i];
	for (int i = 0; i < NUM_OBJ_VIDVALUES; ++i)		m_VIDValue[i] = obj.m_VIDValue[i];
	for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
		affect[i] = obj.affect[i];
		
	//	Share GUN data
	if (obj.m_pGun)
		m_pGun = new GunData(*obj.m_pGun);
	
	//	This doesn't happen on protos - only on copied real objects
	FOREACH_CONST(ObjectList, obj.contents, iter)
	{
		ObjData *newObj = ObjData::Clone(*iter);
		newObj->ToObject(this, true);				//	This messes up weight... but totalWeight should get around that!
	}
}

#if 0
void ObjData::operator=(const ObjData &obj)
{
	if (&obj == this)
		return;
	
	Entity::operator=(obj);
	
	SetID(max_id++);
	
	m_RealNumber = obj.m_RealNumber;
//	m_InRoom = NULL;
	m_Keywords = obj.m_Keywords;
	m_Name = obj.m_Name;
	m_RoomDescription = obj.m_RoomDescription;
	m_Description = obj.m_Description;
//	m_CarriedBy = NULL;
//	m_WornBy = NULL;
//	m_WornOn = -1;
//	in_obj = NULL;
	cost = obj.cost;
	weight = obj.weight;
	totalWeight = obj.weight;	//	Always start at obj.weight!
	timer = obj.timer;
	type = obj.type;
	wear = obj.wear;
	extra = obj.extra;
	affects = obj.affects;
	bought = obj.bought;
	buyer = obj.buyer;
	owner = obj.owner;
	m_MinimumLevelRestriction = obj.m_MinimumLevelRestriction;
	m_RaceRestriction = obj.m_RaceRestriction;
	m_ClanRestriction = obj.m_ClanRestriction;

	for (int i = 0; i < NUM_OBJ_VALUES; ++i)
		value[i] = obj.value[i];
	for (int i = 0; i < NUM_OBJ_FVALUES; ++i)
		fvalue[i] = obj.fvalue[i];
	for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
		affect[i] = obj.affect[i];
		
	//	Share GUN data
	delete m_pGun;
	m_pGun = obj.m_pGun ? new GunData(*obj.m_pGun) : NULL;
	
	m_ExtraDescriptions = obj.m_ExtraDescriptions;
	
	//	This doesn't happen on protos - only on copied real objects
	FOREACH_CONST(ObjectList, obj.contents, iter)
	{
		ObjData *newObj = Object::Clone(*iter);
		newObj->ToObject(this, true);				//	This messes up weight... but totalWeight fixes that
//		contains.Append(newObj);	//	Safer, and we can add to end of list
//		newObj->in_obj = this;			//	Plus no need for weight correction
	}
}
#endif

/* Delete the object */
ObjData::~ObjData(void)
{
	delete m_pGun;
}


/* Extract an object from the world */
void ObjData::Extract(void)
{
	if (IsPurged())
		return;
	
	if (WornBy())
		if (unequip_char(WornBy(), WornOn()) != this)
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");

	if (InRoom())			FromRoom();
	else if (CarriedBy())	FromChar();
	else if (Inside())		FromObject();
	
	/* Get rid of the contents of the object, as well. */
	FOREACH(ObjectList, this->contents, iter)
	{
		(*iter)->Extract();
	}

	while (!m_Events.empty())
	{
		Event *event = m_Events.front();
		m_Events.pop_front();
		event->Cancel();
	}
	
	FromWorld();
	
	Purge();
	
	Entity::Extract();
}


/* give an object to a char   */
void ObjData::ToChar(CharData * ch, bool append) {
	if (!ch) {
		log("SYSERR: NULL char passed to ObjData::ToChar");
		return;
	}
	else if (PURGED(this) || PURGED(ch))
		return;
		
	m_CarriedBy = ch;
	
	if (append)	CarriedBy()->carrying.push_back(this);
	else		CarriedBy()->carrying.push_front(this);
	
//	m_InRoom = NOWHERE;	//	This shouldn't be needed
	if (!OBJ_FLAGGED(this, ITEM_STAFFONLY))
	{
		IS_CARRYING_W(ch) += GET_OBJ_TOTAL_WEIGHT(this);
		IS_CARRYING_N(ch)++;
		ch->UpdateWeight();
	}

	/* set flag for crash-save system */
	if (!IS_NPC(CarriedBy()))
		SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


/* take an object from a char */
void ObjData::FromChar()
{
	if (PURGED(this))
		return;
	else if (!CarriedBy()) {
		log("SYSERR: Object not carried by anyone in call to obj_from_char.");
		return;
	}
	
	//	Perform these in the REVERSE ORDER as ToChar
	
	/* set flag for crash-save system */
	if (!IS_NPC(CarriedBy()))
		SET_BIT(PLR_FLAGS(CarriedBy()), PLR_CRASH);

	if (!OBJ_FLAGGED(this, ITEM_STAFFONLY))
	{
		IS_CARRYING_W(CarriedBy()) -= GET_OBJ_TOTAL_WEIGHT(this);
		IS_CARRYING_N(CarriedBy())--;
		CarriedBy()->UpdateWeight();
	}
	
	CarriedBy()->carrying.remove(this);
	
	m_CarriedBy = NULL;
}


/* put an object in a room */
void ObjData::ToRoom(RoomData *room, bool append)
{
	if (room == NULL)
		log("SYSERR: Illegal value(s) passed to obj_to_room. (Room NULL, obj %s)", GetName());
	else if (!PURGED(this))
	{
		if (append)		room->contents.push_back(this);
		else			room->contents.push_front(this);
		
		m_InRoom = room;
		
		m_CarriedBy = NULL;
		if (ROOM_FLAGGED(room, ROOM_HOUSE))
			room->GetFlags().set(ROOM_HOUSE_CRASH);
		
		if (GET_OBJ_TYPE(this) == ITEM_LIGHT && !IS_SET(GET_OBJ_WEAR(this), ITEM_WEAR_WIELD | ITEM_WEAR_HOLD))
			room->AddLight(GetValue(OBJVAL_LIGHT_RADIUS));
	}
}


/* Take an object from a room */
void ObjData::FromRoom(void)
{
	if (InRoom() == NULL)
	{
		log("SYSERR: Object (%s) not in a room passed to obj_from_room", GetName());
		return;
	}
	
	if (IsPurged())
		return;

	FOREACH(CharacterList, m_InRoom->people, tch)
	{
		if ((*tch)->sitting_on == this)
			(*tch)->sitting_on = NULL;
	}
	
	if (GetType() == ITEM_LIGHT && !IS_SET(GET_OBJ_WEAR(this), ITEM_WEAR_WIELD | ITEM_WEAR_HOLD))
		InRoom()->RemoveLight(GetValue(OBJVAL_LIGHT_RADIUS));
	
	if (InRoom()->GetFlags().test(ROOM_HOUSE))
		InRoom()->GetFlags().set(ROOM_HOUSE_CRASH);
	
	InRoom()->contents.remove(this);
	m_InRoom = NULL;
}


/* put an object in an object (quaint)  */
void ObjData::ToObject(ObjData* obj_to, bool append) {
	ObjData *temp;

	if (!obj_to || this == obj_to) {
		log("SYSERR: NULL object (%p) or same source and target obj passed to obj_to_obj", obj_to);
		return;
	}
	
	if (IsPurged())
		return;
	
	if (append)		obj_to->contents.push_back(this);
	else			obj_to->contents.push_front(this);
	m_Inside = obj_to;

//#if 0
	for (temp = Inside(); temp->Inside(); temp = temp->Inside())
		GET_OBJ_TOTAL_WEIGHT(temp) += GET_OBJ_TOTAL_WEIGHT(this);

	/* top level object.  Subtract weight from inventory if necessary. */
	GET_OBJ_TOTAL_WEIGHT(temp) += GET_OBJ_TOTAL_WEIGHT(this);
	if (temp->CarriedBy())
	{
		IS_CARRYING_W(temp->CarriedBy()) += GET_OBJ_TOTAL_WEIGHT(this);
		temp->CarriedBy()->UpdateWeight();
	}
	else if (temp->WornBy())
	{
		//	Worn EQ no effect!
		IS_CARRYING_W(temp->WornBy()) += GET_OBJ_TOTAL_WEIGHT(this);
		temp->WornBy()->UpdateWeight();
	}
	else if (temp->InRoom())
	{
		if (temp->InRoom()->GetFlags().test(ROOM_HOUSE))
			temp->InRoom()->GetFlags().set(ROOM_HOUSE_CRASH);
	}
//#endif
	
	if ((GetType() == ITEM_ATTACHMENT) && (GET_OBJ_TYPE(Inside()) != ITEM_CONTAINER)) {
		// DO THE ATTACHMENT
	}
}


/* remove an object from an object */
void ObjData::FromObject() {
	ObjData *temp, *obj_from;

	if (!Inside()) {
		log("SYSERR: trying to illegally extract obj from obj");
		return;
	}
	if (PURGED(this))
		return;
		
	obj_from = Inside();
	obj_from->contents.remove(this);

//#if 0
	/* Subtract weight from containers container */
	for (temp = Inside(); temp->Inside(); temp = temp->Inside())
		GET_OBJ_TOTAL_WEIGHT(temp) -= GET_OBJ_TOTAL_WEIGHT(this);

	/* Subtract weight from char that carries the object */
	GET_OBJ_TOTAL_WEIGHT(temp) -= GET_OBJ_TOTAL_WEIGHT(this);
	if (temp->CarriedBy())
	{
		IS_CARRYING_W(temp->CarriedBy()) -= GET_OBJ_TOTAL_WEIGHT(this);
		temp->CarriedBy()->UpdateWeight();
	}
	else if (temp->WornBy())
	{
		IS_CARRYING_W(temp->WornBy()) -= GET_OBJ_TOTAL_WEIGHT(this);
		temp->WornBy()->UpdateWeight();
	}
	else if (temp->InRoom())
	{
		if (temp->InRoom()->GetFlags().test(ROOM_HOUSE))
			temp->InRoom()->GetFlags().set(ROOM_HOUSE_CRASH);
	}
//#endif
	
	m_Inside = NULL;

	if ((GetType() == ITEM_ATTACHMENT) && (obj_from->GetType() != ITEM_CONTAINER)) {
		// DO THE DETACHMENT
	}
}


/*
void ObjData::unequip() {


}

void ObjData::equip(CharData *ch) {

}
*/


void ObjData::update(unsigned int use) {
	if (GET_OBJ_TIMER(this) > 0)		GET_OBJ_TIMER(this) -= use;
	FOREACH(ObjectList, this->contents, obj)
	{
		(*obj)->update(use);
	}
//	if (this->contains && this->contains != this)			this->contains->update(use);
//	if (this->next_content && this->next_content != this)	this->next_content->update(use);
}


int ObjData::TotalCost(void) {
	int value = OBJ_FLAGGED(this, ITEM_MISSION) ? GET_OBJ_COST(this) : 0;
//	value = GET_OBJ_COST(this);
	
	FOREACH(ObjectList, this->contents, obj)
	{
		value += (*obj)->TotalCost();
	}
	
	return value;
}


int ObjData::TotalCost(CharData *ch) {
	int			value;
	time_t		deltaTime = time(0) - this->bought;
	
	value = OBJ_FLAGGED(this, ITEM_MISSION) ? GET_OBJ_COST(this) : 0;
	
#if 0	//	Temporarily disabled
	if (this->buyer != 0 && this->bought != 0) {
		if ((deltaTime > 86400 * 7) || (this->buyer != GET_ID(ch)))	//	24 * 7 hours, or !buyer
			value /= 2;	//	50%
		else if (deltaTime > 86400)	//	24 hours
			value = value * 3 / 4;	//	75%
	}
#else
	if (this->buyer != 0 && this->bought != 0 && (this->buyer != GET_ID(ch))) //	24 * 7 hours, or !buyer
	{
		value /= 5;	//	20%
	}
#endif
		
	FOREACH(ObjectList, this->contents, obj)
	{
		value += (*obj)->TotalCost(ch);
	}
	
	return value;
}


void ObjData::SetBuyer(CharData *ch)
{
	if (!ch)
		return;
	
	this->buyer = GET_ID(ch);
	this->bought = time(0);
	
	FOREACH(ObjectList, this->contents, obj)
	{
		(*obj)->SetBuyer(ch);
	}
}


bool ObjData::load(FILE *fl, const char *filename) {
	char tag[6];
	int	num, t[3];
	bool	result = true;
//	ExtraDesc *new_descr;
	
	BUFFER(line, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_INPUT_LENGTH);

	sprintf(buf, "plr obj file %s", filename);

	while(get_line(fl, line)) {
		if (*line == '$')	break;	// Done reading object
		tag_argument(line, tag);
		num = atoi(line);
		if (!strcmp(tag, "Actn")) {
			m_Description = fread_lexistring(fl, buf, filename);
		}
		else if (!strcmp(tag, "Ammo")) {
//			if (!IS_GUN(this))	GET_GUN_INFO(this) = new GunData();
			if (IS_GUN(this)) {
				t[0] = t[1] = t[2] = 0;
				sscanf(line, "%d %d %d", t, t + 1, t + 2);
				GET_GUN_AMMO_VID(this) = t[0];
				GET_GUN_AMMO(this) = t[1];
				GET_GUN_AMMO_FLAGS(this) = t[2];
			}
		}/* else if (!strcmp(tag, "Cost"))	this->cost = num;*/
		else if (!strcmp(tag, "Desc"))
		{
			char *term = strchr(line, '\n');
			if (term)
				*term = 0;
			m_RoomDescription = line;
		}
/*		else if (!strcmp(tag, "GAtk")) {
			if (!IS_GUN(this))	GET_GUN_INFO(this) = new GunData();
			t[0] = t[1] = t[2] = 0;
			sscanf(line, "%d %d %d", t, t + 1);
			GET_GUN_ATTACK_TYPE(this) = t[0];
			GET_GUN_SKILL(this) = t[1];
			GET_GUN_STRENGTH(this) = t[2];
		} else if (!strcmp(tag, "GATp")) {
			if (!IS_GUN(this))	GET_GUN_INFO(this) = new GunData();
			GET_GUN_AMMO_TYPE(this) = num;
		} else if (!strcmp(tag, "GDam")) {
			if (!IS_GUN(this))	GET_GUN_INFO(this) = new GunData();
			if (!IS_GUN(this))	GET_GUN_INFO(this) = new GunData();
			t[0] = t[1] = 0;
			sscanf(line, "%d %d", t, t + 1);
			GET_GUN_DAMAGE(this) = t[0];
			GET_GUN_DAMAGE_TYPE(this) = t[1];
		} else if (!strcmp(tag, "GRat")) {
			if (!IS_GUN(this))	GET_GUN_INFO(this) = new GunData();
			GET_GUN_RATE(this) = num;
		} else if (!strcmp(tag, "GRng")) {
			if (!IS_GUN(this))	GET_GUN_INFO(this) = new GunData();
			t[0] = t[1] = t[2] = 0;
			sscanf(line, "%d %d %d", t, t + 1, t + 2);
			GET_GUN_RANGE(this) = t[0];
			GET_GUN_OPTIMALRANGE(this) = t[1];
			GET_GUN_MODIFIER(this) = t[2];
		} */
		else if (!strcmp(tag, "Name")) {
			m_Keywords = line;
		} else if (!strcmp(tag, "Shrt")) {
			m_Name = line;
		} //else if (!strcmp(tag, "Sped"))	this->speed = /*num*/ atof(line);
		else if (!strcmp(tag, "Time"))		this->m_Timer = num;
/*		else if (!strcmp(tag, "Trig")) {
			if ((trig = read_trigger(num, VIRTUAL))) {
				this->CreateScript();
				add_trigger(SCRIPT(this), trig); //, 0);
			}
		}*/
		else if (!strcmp(tag, "Type"))	;//this->type = num;	//	We really don't want this anymore
		else if (!strcmp(tag, "XDsc")) {
//			new_descr = new ExtraDesc();
//			new_descr->keyword = SSCreate(line);
//			new_descr->description = fread_lexistring(fl, buf, filename);
//			new_descr->next = this->exDesc;
//			this->exDesc = new_descr;
		}
		else if (!strcmp(tag, "Val0"))		this->m_Value[0] = num;
		else if (!strcmp(tag, "Val1"))		this->m_Value[1] = num;
		else if (!strcmp(tag, "Val2"))		this->m_Value[2] = num;
		else if (!strcmp(tag, "Val3"))		this->m_Value[3] = num;
		else if (!strcmp(tag, "Val4"))		this->m_Value[4] = num;
		else if (!strcmp(tag, "Val5"))		this->m_Value[5] = num;
		else if (!strcmp(tag, "Val6"))		this->m_Value[6] = num;
		else if (!strcmp(tag, "Val7"))		this->m_Value[7] = num;
		else if (!strcmp(tag, "FVa0"))		;//this->m_FValue[0] = atof(line);
		else if (!strcmp(tag, "FVa1"))		;//this->m_FValue[1] = atof(line);
		else if (!strcmp(tag, "Wate"))		{ this->weight = MAX(this->weight, num); this->totalWeight = this->weight; }
		else if (!strcmp(tag, "Wear"))		this->wear = asciiflag_conv(line);
		else if (!strcmp(tag, "Xtra"))		this->extra = asciiflag_conv(line);
/*		else if (!strcmp(tag, "Lvel"))		this->level = num;
*/		else if (!strcmp(tag, "Ownr"))		this->owner = num;
		else if (!strcmp(tag, "Buyr"))		this->buyer = num > 0 ? num : 0;
		else if (!strcmp(tag, "Bght"))		this->bought = num;
		//	CJ: Add 2 flags - one settable by OLC, for saving variables
		//		the other set here, that means there were variables saved,
		//		so don't loadscript/startscript
		else if (!strcmp(tag, "Vrbl")) {
			//	line is the variable
			BUFFER(name, MAX_INPUT_LENGTH);
			BUFFER(context, MAX_INPUT_LENGTH);
			BUFFER(value, MAX_STRING_LENGTH);
			
			two_arguments(line, name, context);
			
			get_any_line(fl, value, MAX_STRING_LENGTH);	//	TODO: last usage of this function!
			
			ScriptVariable *var = new ScriptVariable(name, *context ? atoi(context) : 0);
			var->SetValue(value);
			
			//	next line is the contents
			const ScriptVariable *preexistingVar = SCRIPT(this)->m_Globals.Find(var);
			if (preexistingVar
				&& !strcmp(preexistingVar->GetValue(), var->GetValue()))
				continue;	//	SKIP IT!

			SCRIPT(this)->m_Globals.Add(var);
		}
		else {
			log("SYSERR: PlrObj file %s has unknown tag %s (rest of line: %s)", filename, tag, line);
//			result = false;
		}
	}
	
	SET_BIT(GET_OBJ_EXTRA(this), ITEM_RELOADED);
	
	return result;
}


void ObjData::AddOrReplaceEvent(Event *newEvent)
{
	Event *	event = Event::FindEvent(m_Events, newEvent->GetType());
	
	if (event)
	{
		m_Events.remove(event);
		event->Cancel();
	}

	m_Events.push_front(newEvent);
}



void equip_char(CharData * ch, ObjData * obj, unsigned int pos)
{
	int j;

	if (pos >= NUM_WEARS) {
		core_dump();
		return;
	}

	if (PURGED(obj))
		return;

	if (GET_EQ(ch, pos)) {
//		log("SYSERR: Char is already equipped: %s, %s", ch->GetName(), obj->GetName());
		return;
	}
	if (obj->CarriedBy()) {
		log("SYSERR: EQUIP: Obj is carried_by when equip.");
		return;
	}
	if (obj->InRoom()) {
		log("SYSERR: EQUIP: Obj is in_room when equip.");
		return;
	}

	if (ch->CanUse(obj) != NotRestricted) {
		act("You can't figure out how to use $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n tries to use $p, but can't figure it out.", FALSE, ch, obj, 0, TO_ROOM);
		obj->ToChar(ch);	/* changed to drop in inventory instead of ground */
		return;
	}

	GET_EQ(ch, pos) = obj;
	obj->m_WornBy = ch;
	obj->m_WornOn = pos;

	if (GET_OBJ_TOTAL_WEIGHT(obj) /*&& GET_OBJ_TYPE(obj) != ITEM_CONTAINER*/)
	{
		IS_CARRYING_W(ch) += GET_OBJ_TOTAL_WEIGHT(obj);
		ch->UpdateWeight();
	}

	if (ch->InRoom()) {
		if (/*((pos == WEAR_HAND_R) || (pos == WEAR_HAND_L)) &&*/ GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			if (obj->GetValue(OBJVAL_LIGHT_HOURS))	/* if light is ON */
				ch->InRoom()->AddLight(obj->GetValue(OBJVAL_LIGHT_RADIUS));
		if (AFF_FLAGGED(obj, AFF_LIGHT) && !AFF_FLAGGED(ch, AFF_LIGHT))
			ch->InRoom()->AddLight();
	}
/*	 else
		log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", ch->GetName());
*/
	for (j = 0; j < MAX_OBJ_AFFECT; j++)
		Affect::Modify(ch, obj->affect[j].location, obj->affect[j].modifier, obj->m_AffectFlags, TRUE);

	ch->AffectTotal();
}



ObjData *unequip_char(CharData * ch, unsigned int pos) {
	int j;
	ObjData *obj;

	if ((pos >= NUM_WEARS) || !GET_EQ(ch, pos)) {
		core_dump();
		return NULL;
	}

//	if(pos > NUM_WEARS) {
//		log("SYSERR: pos passed to unequip_char is invalid.  Pos = %d", pos);
//		return read_object(0, REAL);
//	}
//	if (!GET_EQ(ch, pos)) {
//		log("SYSERR: no EQ at position %d on character %s", pos, ch->GetName());
//		return read_object(0, REAL);
//	}

	obj = GET_EQ(ch, pos);
	obj->m_WornBy = NULL;
	obj->m_WornOn = -1;
	if (GET_OBJ_TOTAL_WEIGHT(obj) /*&& GET_OBJ_TYPE(obj) != ITEM_CONTAINER*/ )
	{
		IS_CARRYING_W(ch) -= GET_OBJ_TOTAL_WEIGHT(obj);
		ch->UpdateWeight();
	}
	
	if (ch->InRoom())
	{
		if (/*((pos == WEAR_HAND_R) || (pos == WEAR_HAND_L)) &&*/ GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			if (obj->GetValue(OBJVAL_LIGHT_HOURS))	/* if light is ON */
				ch->InRoom()->RemoveLight(obj->GetValue(OBJVAL_LIGHT_RADIUS));
		if (AFF_FLAGGED(obj, AFF_LIGHT))
			ch->InRoom()->RemoveLight();
	}
	else
		log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", ch->GetName());

	GET_EQ(ch, pos) = NULL;

	for (j = 0; j < MAX_OBJ_AFFECT; j++)
		Affect::Modify(ch, obj->affect[j].location, obj->affect[j].modifier,
				obj->m_AffectFlags, FALSE);

	ch->AffectTotal();
	
	if (ch->InRoom() && AFF_FLAGGED(obj, AFF_LIGHT) && AFF_FLAGGED(ch, AFF_LIGHT))
		ch->InRoom()->AddLight();
	
	return (obj);
}


/* create a new object from a prototype */
ObjData *ObjData::Create(VirtualID vnum)
{
	ObjectPrototype *proto = obj_index.Find(vnum);
	
	if (!proto)
	{
		log("Object (V) %s does not exist in database.", vnum.Print().c_str());
		return NULL;
	}

	return ObjData::Create(proto);
}


ObjData *ObjData::Create(ObjectPrototype *proto)
{
	return proto ? Create(proto->m_pProto) : Create();
}


ObjData *ObjData::Create(ObjData *orig)
{
	ObjData *obj = orig ? new ObjData(*orig) : new ObjData;
	
	ObjectPrototype *prototype = orig ? orig->GetPrototype() : NULL;
	if (prototype)
	{
		BehaviorSet::ApplySets(prototype->m_BehaviorSets, obj->GetScript());
		
		/* add triggers */
		FOREACH(ScriptVector, prototype->m_Triggers, trn)
		{
			ScriptPrototype *proto = trig_index.Find(*trn);

			if (!proto)
				mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Invalid trigger %s assigned to object %s",
						trn->Print().c_str(),
						obj->GetVirtualID().Print().c_str());
			else
			{
				obj->GetScript()->AddTrigger(TrigData::Create(proto));
			}
		}
		
		SCRIPT(obj)->m_Globals.Add(prototype->m_InitialGlobals);
	}
	
	IDManager::Instance()->GetAvailableIDNum(obj);
	obj->ToWorld();
	
	return obj;
}


void ObjData::Parse(Lexi::Parser &parser, VirtualID vid)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	ObjData *obj = new ObjData();
	
	ObjectPrototype *proto = obj_index.insert(vid, obj);
	obj->SetPrototype(proto);
	
	obj->m_Keywords				= parser.GetString("Keywords", "invalid");
	obj->m_Name					= parser.GetString("Name");
	obj->m_RoomDescription		= parser.GetString("RoomDescription");
	obj->m_Description			= parser.GetString("Description");
	
	//	Known work-around to modify a shared string directly
	Lexi::String &str = obj->m_Name.GetStringRef();
	if (!str_cmp(fname(str.c_str()), "a") || !str_cmp(fname(str.c_str()), "an") || !str_cmp(fname(str.c_str()), "the"))
		str[0] = tolower(str[0]);
	
	obj->m_Type					= parser.GetEnum("Type", ITEM_UNDEFINED);
	obj->extra					= parser.GetFlags("ExtraFlags", extra_bits);
	obj->wear					= parser.GetFlags("WearFlags", wear_bits);
	obj->m_AffectFlags			= parser.GetBitFlags<AffectFlag>("AffectFlags");

	obj->weight					= parser.GetInteger("Weight");
	obj->cost					= parser.GetInteger("Cost");
	obj->m_Timer				= parser.GetInteger("Timer");
	
	obj->m_RaceRestriction		= parser.GetBitFlags("Restrictions/Race", race_types);
	obj->m_MinimumLevelRestriction = parser.GetInteger("Restrictions/Level", 0);
	obj->m_ClanRestriction		= parser.GetInteger("Restrictions/Clan", 0);


	const ObjectDefinition * definition = ObjectDefinition::Get(GET_OBJ_TYPE(obj));
	
	if (definition)
	{
		PARSER_SET_ROOT(parser, "Values");
		
		FOREACH_CONST(ObjectDefinition::ValueMap, definition->values, iter)
		{
			const ObjectDefinition::Value &value = *iter;
			ObjectTypeValue	slot = value.GetSlot();
			
			switch (value.GetType())
			{
				case ObjectDefinition::Value::TypeInteger:
					obj->SetValue(slot, parser.GetInteger(value.GetName(), value.intDefault));
					break;
				case ObjectDefinition::Value::TypeEnum:
					obj->SetValue(slot, parser.GetEnum(value.GetName(), const_cast<char **>(value.nametable), value.intDefault));
					break;
				case ObjectDefinition::Value::TypeFlags:
					obj->SetValue(slot, parser.GetFlags(value.GetName(), const_cast<char **>(value.nametable)));
					break;
				case ObjectDefinition::Value::TypeSkill:
					obj->SetValue(slot, find_skill(parser.GetString(value.GetName())));
					break;
				
				case ObjectDefinition::Value::TypeFloat:
					obj->m_FValue[slot] = parser.GetFloat(value.GetName(), value.floatDefault);
					break;
				
				case ObjectDefinition::Value::TypeVirtualID:
					obj->m_VIDValue[slot] = parser.GetVirtualID(value.GetName(), obj->GetVirtualID().zone);
					break;
			}
		}
	}


	int numExtraDescs = parser.NumSections("ExtraDescs");
	for (int i = 0; i < numExtraDescs; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "ExtraDescs", i);
		
		const char *	keywords		= parser.GetString("Keywords");
		const char *	description		= parser.GetString("Description");
		
		if (!*keywords || !*description)
			continue;
		
		obj->m_ExtraDescriptions.push_back(ExtraDesc(keywords, description));
	}

	int numAffects = MIN((int)parser.NumSections("Affects"), MAX_OBJ_AFFECT);
	for (int i = 0; i < numAffects; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Affects", i);
		
		obj->affect[i].location			= -find_skill(parser.GetString("Location"));
		if (obj->affect[i].location == 0)
			obj->affect[i].location			= parser.GetEnum("Location", APPLY_NONE);
		if (obj->affect[i].location != 0)
			obj->affect[i].modifier			= parser.GetInteger("Modifier");
	}
	
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && parser.DoesSectionExist("Gun"))
	{
		GET_GUN_INFO(obj) = new GunData();
		GET_GUN_DAMAGE(obj)				= parser.GetInteger("Gun/Damage");
		GET_GUN_DAMAGE_TYPE(obj)		= parser.GetEnum("Gun/DamageType", damage_types);
		GET_GUN_RATE(obj)				= parser.GetInteger("Gun/Rate", 1);
		GET_GUN_ATTACK_TYPE(obj)		= parser.GetEnum("Gun/AttackType", attack_types);	//	TEMP FIX
		GET_GUN_RANGE(obj)				= parser.GetInteger("Gun/Range");
		GET_GUN_OPTIMALRANGE(obj)		= parser.GetInteger("Gun/OptimalRange");

		GET_GUN_AMMO_TYPE(obj)			= parser.GetEnum("Gun/Ammo", ammo_types, -1);
		GET_GUN_MODIFIER(obj)			= parser.GetInteger("Gun/Modifier");
		GET_GUN_SKILL(obj)				= find_skill(parser.GetString("Gun/Skill"));
		GET_GUN_SKILL2(obj)				= find_skill(parser.GetString("Gun/Skill2"));
	}

	int numSets = parser.NumFields("ScriptSets");
	for (int i = 0; i < numSets; ++i)
	{
		BehaviorSet *set = BehaviorSet::Find(parser.GetIndexedString("ScriptSets", i));
		if (set)			proto->m_BehaviorSets.push_back(set);
	}

	int numScripts = parser.NumFields("Scripts");
	for (int i = 0; i < numScripts; ++i)
	{
		VirtualID	script = parser.GetIndexedVirtualID("Scripts", i, vid.zone);
		if (script.IsValid())	proto->m_Triggers.push_back(script);
	}
	
	int numVariables = parser.NumSections("Variables");
	for (int i = 0; i < numVariables; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Variables", i);
		
		ScriptVariable *var = ScriptVariable::Parse(parser);
		
		if (var)			proto->m_InitialGlobals.Add(var);
	}
	
	//	Fixups
	if (obj->GetType() == ITEM_DRINKCON || obj->GetType() == ITEM_FOUNTAIN)
	{
		if (obj->weight < obj->GetValue(OBJVAL_FOODDRINK_CONTENT))
			obj->weight = obj->GetValue(OBJVAL_FOODDRINK_CONTENT);
	}
	obj->totalWeight = obj->weight;
}


void ObjData::Write(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Virtual", GetVirtualID(), GetVirtualID().zone);
	
	file.WriteString("Keywords", m_Keywords);
	file.WriteString("Name", m_Name);
	file.WriteString("RoomDescription", m_RoomDescription);
	file.WriteLongString("Description", m_Description);
	
	file.WriteEnum("Type", m_Type, ITEM_UNDEFINED);
	file.WriteFlags("ExtraFlags", GET_OBJ_EXTRA(this), extra_bits);
	file.WriteFlags("WearFlags", GET_OBJ_WEAR(this), wear_bits);
	file.WriteFlags("AffectFlags", m_AffectFlags);
	
	file.WriteInteger("Weight", GET_OBJ_WEIGHT(this), 0);
	file.WriteInteger("Cost", GET_OBJ_COST(this), 0);
	file.WriteInteger("Timer", GET_OBJ_TIMER(this), 0);
	
	file.BeginSection("Restrictions");
	{
		file.WriteFlags("Race", GetRaceRestriction(), race_types);
		file.WriteInteger("Level", GetMinimumLevelRestriction(), 0);
		file.WriteInteger("Clan", GetClanRestriction(), 0);
	}
	file.EndSection();
	

	const ObjectDefinition * definition = ObjectDefinition::Get(m_Type);
	
	file.BeginSection("Values");
	if (definition)
	{
		FOREACH_CONST(ObjectDefinition::ValueMap, definition->values, iter)
		{
			const ObjectDefinition::Value &value = *iter;
			ObjectTypeValue	slot = value.GetSlot();
			
			switch (value.GetType())
			{
				case ObjectDefinition::Value::TypeInteger:
					file.WriteInteger(value.GetName(), GetValue(slot), value.intDefault);
					break;
				case ObjectDefinition::Value::TypeEnum:
					file.WriteEnum(value.GetName(), GetValue(slot), const_cast<char **>(value.nametable));
					break;
				case ObjectDefinition::Value::TypeFlags:
					if (GetValue(slot) != value.intDefault)
						file.WriteFlags(value.GetName(), GetValue(slot), const_cast<char **>(value.nametable));
					break;
				case ObjectDefinition::Value::TypeSkill:
					if (GetValue(slot) != value.intDefault)
						file.WriteString(value.GetName(), skill_name(GetValue(slot)), skill_name(0));
					break;
				case ObjectDefinition::Value::TypeVirtualID:
					file.WriteVirtualID(value.GetName(), m_VIDValue[slot], GetVirtualID().zone);
					break;
				case ObjectDefinition::Value::TypeFloat:
					file.WriteFloat(value.GetName(), GetFloatValue(slot), value.floatDefault);
					break;
				default:
					;	//	Do nothing
			}
		}
	}
	file.EndSection();


	file.BeginSection("ExtraDescs");
	{
		//	Home straight, just deal with extra descriptions.
		int	count = 0;
		FOREACH(ExtraDesc::List, m_ExtraDescriptions, extradesc)
		{
			if (!extradesc->keyword.empty() && !extradesc->description.empty())
			{
				file.BeginSection(Lexi::Format("ExtraDesc %d", ++count));
				{
					file.WriteString("Keywords", extradesc->keyword);
					file.WriteLongString("Description", extradesc->description);
				}
				file.EndSection();
			}
		}
	}
	file.EndSection();
	
	file.BeginSection("Affects");
	{
		for (int aff = 0; aff < MAX_OBJ_AFFECT; ++aff)
		{
			if (this->affect[aff].location == 0 || this->affect[aff].modifier == 0)
				continue;
			
			file.BeginSection(Lexi::Format("Affect %d", aff + 1));
			{
				if (this->affect[aff].location < 0)
					file.WriteString("Location", skill_name(-this->affect[aff].location));
				else
					file.WriteEnum("Location", (AffectLoc)this->affect[aff].location);
				file.WriteInteger("Modifier", this->affect[aff].modifier);
			}
			file.EndSection();
		}
	}
	file.EndSection();

	file.BeginSection("Gun");
	if (GetType() == ITEM_WEAPON && IS_GUN(this))
	{
		file.WriteInteger("Damage", GET_GUN_DAMAGE(this), 0);
		file.WriteEnum("DamageType", GET_GUN_DAMAGE_TYPE(this), damage_types);
		file.WriteInteger("Rate", GET_GUN_RATE(this), 1);
		file.WriteEnum("AttackType", GET_GUN_ATTACK_TYPE(this), attack_types);
		file.WriteInteger("Range", GET_GUN_RANGE(this), 0);
		file.WriteInteger("OptimalRange", GET_GUN_OPTIMALRANGE(this), 0);
		
		file.WriteEnum("Ammo", GET_GUN_AMMO_TYPE(this), ammo_types, -1);
		file.WriteInteger("Modifier", GET_GUN_MODIFIER(this), 0);
		if (GET_GUN_SKILL(this))	file.WriteString("Skill", skill_name(GET_GUN_SKILL(this)));
		if (GET_GUN_SKILL2(this))	file.WriteString("Skill2", skill_name(GET_GUN_SKILL2(this)));
	}
	file.EndSection();


	file.BeginSection("ScriptSets");
	{
		BehaviorSets	&	sets = GetPrototype()->m_BehaviorSets;
		int count = 0;
		FOREACH(BehaviorSets, sets, iter)
		{
//				BehaviorSet *set = BehaviorSet::Find(*iter);
			BehaviorSet *set = *iter;
			if (set)	file.WriteString(Lexi::Format("Set %d", ++count).c_str(), set->GetName());
		}
	}
	file.EndSection();
	
	
	file.BeginSection("Scripts");
	{
		int count = 0;
		FOREACH(ScriptVector, GetPrototype()->m_Triggers, iter)
		{
			file.WriteVirtualID(Lexi::Format("Script %d", ++count).c_str(), *iter, GetPrototype()->GetVirtualID().zone);
		}
	}
	file.EndSection();


	file.BeginSection("Variables");
	{
		ScriptVariable::List &variables = GetPrototype()->m_InitialGlobals;
		int count = 0;
		FOREACH(ScriptVariable::List, variables, var)
		{
			(*var)->Write(file, Lexi::Format("Variable %d", ++count), false);
		}
	}
	file.EndSection();
}


void ObjData::ToWorld()
{
	if (GetPrototype())	GetPrototype()->m_Count++;
	object_list.push_front(this);
	IDManager::Instance()->Register(this);

	if (OBJ_FLAGGED(this, ITEM_PERSISTENT))
	{
		persistentObjects.push_front(this);
	}
	
//	FOREACH(ObjectList, contents, iter)
//		(*iter)->ToWorld();
}


void ObjData::FromWorld()
{
//	FOREACH(ObjectList, contents, iter)
//		(*iter)->FromWorld();

	//	Assume it's in the list
	persistentObjects.remove_one(this);
	
	
	object_list.remove_one(this);	//	Faster with the optimized Lexi::List::remove
	
	IDManager::Instance()->Unregister(this);
	
	if (GetPrototype())	GetPrototype()->m_Count--;
}


/* create an object, and add it to the object list */
ObjData *ObjData::Clone(ObjData *orig)
{
	ObjData *obj = new ObjData(*orig);
	
	IDManager::Instance()->GetAvailableIDNum(obj);
	
	/* add triggers */
	FOREACH(TrigData::List, SCRIPT(orig)->m_Triggers, iter)
	{
		TrigData *trig = *iter;
		if (!trig->GetVirtualID().IsValid())
			continue;
		
		trig = TrigData::Create(trig);
		obj->GetScript()->AddTrigger(trig);
	}
	
	SCRIPT(obj)->m_Globals.Add(SCRIPT(orig)->m_Globals);
	
	obj->ToWorld();
	
	return obj;
}


/* returns the real room number that the object or object's carrier is in */
RoomData *ObjData::Room(void) {
	if (InRoom())						return InRoom();
	else if (CarriedBy())				return CarriedBy()->InRoom();
	else if (WornBy())					return WornBy()->InRoom();
	else if (Inside())					return Inside()->Room();
	else								return NULL;
}


ObjData *ObjData::Find(IDNum id)
{
	Entity *e = IDManager::Instance()->Find(id);
	return dynamic_cast<ObjData *>(e);
}



ObjectDefinition::Map	ObjectDefinition::ms_Definitions;

const ObjectDefinition *ObjectDefinition::Get(ObjectType t)
{
	ObjectDefinition::Map::const_iterator iter = ms_Definitions.find(t);
	return (iter != ms_Definitions.end()) ? &iter->second : NULL;
}


namespace 
{
	class ObjectDefinitionFactory : public ObjectDefinition
	{
	public:	ObjectDefinitionFactory();
	} objectDefinitionFactory;


	ObjectDefinitionFactory::ObjectDefinitionFactory()
	{
		ObjectDefinition *			definition;
		
		definition = &ms_Definitions[ITEM_LIGHT];
		definition->name = "Light";
		definition->Add(OBJVAL_LIGHT_HOURS, "Hours")			.IntegerValue(Range<int>(-1, 5000), -1).SetSavable();
		definition->Add(OBJVAL_LIGHT_RADIUS, "Radius")			.IntegerValue(Range<int>(0, 1), 0);
		
		definition = &ms_Definitions[ITEM_WEAPON];
		definition->name = "Weapon";
		definition->Add(OBJVAL_WEAPON_DAMAGE, "Damage")			.IntegerValue(Range<int>(0, 1000));
		definition->Add(OBJVAL_WEAPON_DAMAGETYPE, "DamageType")	.EnumValue(damage_types);
		definition->Add(OBJVAL_WEAPON_ATTACKTYPE, "AttackType")	.EnumValue(attack_types);
		definition->Add(OBJVAL_WEAPON_SKILL, "Skill")			.SkillValue();
		definition->Add(OBJVAL_WEAPON_SKILL2, "Skill2")			.SkillValue();
		definition->Add(OBJVAL_WEAPON_RATE, "Rate")				.IntegerValue(Range<int>(0, 1000));
		definition->Add(OBJFVAL_WEAPON_SPEED, "Speed")			.FloatValue(Range<float>(0.1, 10), 1);
		definition->Add(OBJFVAL_WEAPON_STRENGTH, "Strength")	.FloatValue(Range<float>(0, 5), 1);
		
		definition = &ms_Definitions[ITEM_AMMO];
		definition->name = "Ammo";
		definition->Add(OBJVAL_AMMO_TYPE, "AmmoType")			.EnumValue(ammo_types, -1);
		definition->Add(OBJVAL_AMMO_AMOUNT, "AmmoAmount")		.IntegerValue().SetSavable();
		
		definition = &ms_Definitions[ITEM_THROW];
		definition->name = "Throw";
		definition->Add(OBJVAL_THROWABLE_DAMAGE, "Damage")		.IntegerValue(Range<int>(0, 1000));
		definition->Add(OBJVAL_THROWABLE_DAMAGETYPE, "DamageType").EnumValue(damage_types);
		definition->Add(OBJVAL_THROWABLE_SKILL, "Skill")		.SkillValue();
		definition->Add(OBJVAL_THROWABLE_RANGE, "Range")		.IntegerValue(Range<int>(1, 5), 1);
		definition->Add(OBJFVAL_THROWABLE_SPEED, "Speed")		.FloatValue(Range<float>(0.1, 10), 1);
		definition->Add(OBJFVAL_THROWABLE_STRENGTH, "Strength")	.FloatValue(Range<float>(0, 5), 1);
		//	Replicate THROW to BOOMERANG
		ms_Definitions[ITEM_BOOMERANG] = ms_Definitions[ITEM_THROW];
		ms_Definitions[ITEM_BOOMERANG].name = "Boomerang";
		
		definition = &ms_Definitions[ITEM_GRENADE];
		definition->name = "Grenade";
		definition->Add(OBJVAL_GRENADE_DAMAGE, "Damage")		.IntegerValue(Range<int>(0, 1000));
		definition->Add(OBJVAL_GRENADE_DAMAGETYPE, "DamageType").EnumValue(damage_types, -1);
		definition->Add(OBJVAL_GRENADE_TIMER, "Timer")			.IntegerValue(Range<int>(0, 3600));

		definition = &ms_Definitions[ITEM_ARMOR];
		definition->name = "Armor";
		definition->Add(OBJVAL_ARMOR_DAMAGETYPE0, damage_types[0]).IntegerValue(Range<int>(0, 100));
		definition->Add(OBJVAL_ARMOR_DAMAGETYPE1, damage_types[1]).IntegerValue(Range<int>(0, 100));
		definition->Add(OBJVAL_ARMOR_DAMAGETYPE2, damage_types[2]).IntegerValue(Range<int>(0, 100));
		definition->Add(OBJVAL_ARMOR_DAMAGETYPE3, damage_types[3]).IntegerValue(Range<int>(0, 100));
		definition->Add(OBJVAL_ARMOR_DAMAGETYPE4, damage_types[4]).IntegerValue(Range<int>(0, 100));
		definition->Add(OBJVAL_ARMOR_DAMAGETYPE5, damage_types[5]).IntegerValue(Range<int>(0, 100));
		definition->Add(OBJVAL_ARMOR_DAMAGETYPE6, damage_types[6]).IntegerValue(Range<int>(0, 100));

		definition = &ms_Definitions[ITEM_CONTAINER];
		definition->name = "Container";
		definition->Add(OBJVAL_CONTAINER_CAPACITY, "Capacity")	.IntegerValue(Range<int>(0, 1000));
		definition->Add(OBJVAL_CONTAINER_FLAGS, "ContainerFlags").FlagsValue(container_bits).SetSavable();
		definition->Add(OBJVAL_CONTAINER_CORPSE, "Corpse")		.BooleanValue().SetUneditable().SetSavable();
		definition->Add(OBJVIDVAL_CONTAINER_KEY, "Key")			.VirtualIDValue(Entity::TypeObject);

		definition = &ms_Definitions[ITEM_FOOD];
		definition->name = "Food";
		definition->Add(OBJVAL_FOODDRINK_FILL, "Fill")			.IntegerValue(Range<int>(0, 1000)).SetSavable();
		definition->Add(OBJVAL_FOODDRINK_POISONED, "Poisoned")	.BooleanValue().SetSavable();

		definition = &ms_Definitions[ITEM_DRINKCON];
		definition->name = "Drink";
		definition->Add(OBJVAL_FOODDRINK_FILL, "Capacity")		.IntegerValue(Range<int>(0, 1000));
		definition->Add(OBJVAL_FOODDRINK_CONTENT, "Contents")	.IntegerValue(Range<int>(0, 1000)).SetSavable();
		definition->Add(OBJVAL_FOODDRINK_TYPE, "Type")			.EnumValue(drinks).SetSavable();
		definition->Add(OBJVAL_FOODDRINK_POISONED, "Poisoned")	.BooleanValue().SetSavable();
		ms_Definitions[ITEM_FOUNTAIN] = ms_Definitions[ITEM_DRINKCON];
		ms_Definitions[ITEM_FOUNTAIN].name = "Fountain";

		definition = &ms_Definitions[ITEM_VEHICLE];
		definition->name = "Vehicle";
		definition->Add(OBJVAL_VEHICLE_FLAGS, "VehicleFlags")		.FlagsValue(vehicle_bits);
		definition->Add(OBJVAL_VEHICLE_SIZE, "Size")				.IntegerValue(Range<int>(0, 32000));
		definition->Add(OBJVIDVAL_VEHICLE_ENTRY, "Entry")			.VirtualIDValue(Entity::TypeRoom);
//		definition->Add(OBJVAL_VEHICLE_STARTROOM, "StartRoom")		.VirtualIDValue(Entity::TypeRoom);
//		definition->Add(OBJVAL_VEHICLE_ENDROOM, "EndRoom")			.VirtualIDValue(Entity::TypeRoom);

		definition = &ms_Definitions[ITEM_VEHICLE_HATCH];
		definition->name = "VehicleHatch";
		definition->Add(OBJVIDVAL_VEHICLEITEM_VEHICLE, "Vehicle")	.VirtualIDValue(Entity::TypeObject);

		definition = &ms_Definitions[ITEM_VEHICLE_CONTROLS];
		definition->name = "VehicleControls";
		definition->Add(OBJVIDVAL_VEHICLEITEM_VEHICLE, "Vehicle")	.VirtualIDValue(Entity::TypeObject);

		definition = &ms_Definitions[ITEM_VEHICLE_WINDOW];
		definition->name = "VehicleWindow";
		definition->Add(OBJVIDVAL_VEHICLEITEM_VEHICLE, "Vehicle")	.VirtualIDValue(Entity::TypeObject);

		definition = &ms_Definitions[ITEM_VEHICLE_WEAPON];
		definition->name = "VehicleWeapon";
		definition->Add(OBJVIDVAL_VEHICLEITEM_VEHICLE, "Vehicle")	.VirtualIDValue(Entity::TypeObject);

		definition = &ms_Definitions[ITEM_BED];
		definition->name = "Bed";
		definition->Add(OBJVAL_FURNITURE_CAPACITY, "Capacity")		.IntegerValue(Range<int>(1, 20), 1);

		definition = &ms_Definitions[ITEM_CHAIR];
		definition->name = "Chair";
		definition->Add(OBJVAL_FURNITURE_CAPACITY, "Capacity")		.IntegerValue(Range<int>(1, 20), 1);

		definition = &ms_Definitions[ITEM_BOARD];
		definition->name = "Board";
		definition->Add(OBJVAL_BOARD_READLEVEL, "ReadLevel")		.IntegerValue(Range<int>(0, LVL_CODER));
		definition->Add(OBJVAL_BOARD_WRITELEVEL, "WriteLevel")		.IntegerValue(Range<int>(0, LVL_CODER));
		definition->Add(OBJVAL_BOARD_REMOVELEVEL, "RemoveLevel")	.IntegerValue(Range<int>(0, LVL_CODER));

		definition = &ms_Definitions[ITEM_INSTALLABLE];
		definition->name = "Installable";
		definition->Add(OBJVAL_INSTALLABLE_TIME, "Time")			.IntegerValue(Range<int>(0, 32000));
		definition->Add(OBJVAL_INSTALLABLE_SKILL, "Skill")			.SkillValue();
		definition->Add(OBJVIDVAL_INSTALLABLE_INSTALLED, "Installed").VirtualIDValue(Entity::TypeObject);

		definition = &ms_Definitions[ITEM_INSTALLED];
		definition->name = "Installed";
		definition->Add(OBJVAL_INSTALLED_TIME, "Time")				.IntegerValue(Range<int>(0, 32000));
		definition->Add(OBJVAL_INSTALLED_PERMANENT, "Permanent")	.BooleanValue();

		definition = &ms_Definitions[ITEM_TOKEN];
		definition->name = "Token";
		definition->Add(OBJVAL_TOKEN_VALUE, "TokenValue")			.IntegerValue(Range<int>(0, 500));
		definition->Add(OBJVAL_TOKEN_SET, "TokenSet")				.EnumValue(token_sets);
		definition->Add(OBJVAL_TOKEN_SIGNER, "Signer")				.IntegerValue(Range<int>(0, 0));
		
		ms_Definitions[ITEM_TREASURE].name = "Treasure";
		ms_Definitions[ITEM_OTHER].name = "Other";
		ms_Definitions[ITEM_TRASH].name = "Trash";
		ms_Definitions[ITEM_NOTE].name = "Note";
		ms_Definitions[ITEM_KEY].name = "Key";
		ms_Definitions[ITEM_PEN].name = "Pen";
		ms_Definitions[ITEM_BOAT].name = "Boat";
		ms_Definitions[ITEM_ATTACHMENT].name = "Attachment";
	}

}


static void DoLoadTriggers(ObjData *obj)
{
	extern void load_otrigger(ObjData *obj);
	
	FOREACH(ObjectList, obj->contents, iter)
	{
		DoLoadTriggers(*iter);
		
		if (PURGED(obj))
			return;
	}
	
	load_otrigger(obj);
}


static bool IsInRoom(ObjData *obj)
{
	return obj->InRoom() != NULL;
}


static const char *PERSISTENT_OBJECTS = ETC_PREFIX "persistobjs";
static void SaveObject(Lexi::OutputParserFile &file, ObjData *obj, int level, int &num, SaveObjectFilePredicate predicate);

void SavePersistentObjects()
{
	extern void store_otrigger(ObjData *obj);
	
	Lexi::String			objectsFilename = PERSISTENT_OBJECTS;
	
	if (persistentObjects.empty())
	{
		remove(objectsFilename.c_str());
		return;
	}
	
	FOREACH(ObjectList, persistentObjects, obj)
	{
		if ((*obj)->InRoom())
			store_otrigger(*obj);
	}
	
	Lexi::String			objectsTempFilename = objectsFilename + ".new";
	Lexi::OutputParserFile	file(objectsTempFilename.c_str());
	if (file.fail())
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write persistent objects to disk: unable to open temporary file %s",
			objectsTempFilename.c_str());
		return;
	}
	
	ObjectList	persistentObjectsToSave = persistentObjects;
	
	int	num = 0;
	
	//SaveObjectFile(file, persistentObjects, IsInRoom);	//	The predicate applies to contents too
	FOREACH(ObjectList, persistentObjects, obj)
	{
		if ((*obj)->InRoom())
			SaveObject(file, *obj, 0, num, NULL);
	}
	
	file.close();
		
	remove(objectsFilename.c_str());
	if (rename(objectsTempFilename.c_str(), objectsFilename.c_str()))
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write persistent objects to disk: unable to replace file %s with temporary file %s",
			objectsFilename.c_str(),
			objectsTempFilename.c_str());
		return;
	}
}


void LoadPersistentObjects()
{
	LoadPersistentObjectFile(PERSISTENT_OBJECTS, NULL);
}


void LoadPersistentObjectFile(const Lexi::String &filename, RoomData *defaultRoom)
{
	Lexi::BufferedFile	objectFile(filename.c_str());
	Lexi::Parser		objectParser;
	ObjectList			objects;
	
	if (objectFile.bad())
		return;
	
	objectParser.Parse(objectFile);
	
	LoadObjectFile(objectParser, objects);
	
	FOREACH(ObjectList, objects, obj)
	{
		if ((*obj)->InRoom() == NULL)
		{
			if (defaultRoom != NULL)
			{
				(*obj)->ToRoom(defaultRoom, true);
			}
			else
			{
				(*obj)->Extract();
			}
		}
	
		DoLoadTriggers(*obj);
	}
}



bool IsRentable(ObjData *obj)
{
	return !OBJ_FLAGGED(obj, ITEM_NORENT);
}


void SavePlayerObjectFile(CharData *ch, SavePlayerObjectsMethod method)
{
//	if (method == SavePlayerBackup)	Crash_crashsave(ch);
//	else							Crash_rentsave(ch);	
//	return;
	
	Lexi::String		playerObjectFilename = ch->GetObjectFilename();
	Lexi::String		playerObjectTempFilename = playerObjectFilename + ".new";
	Lexi::OutputParserFile	file(playerObjectTempFilename.c_str());
	
	if (file.fail())
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write player '%s' inventory to disk: unable to open temporary file %s",
			ch->GetName(), playerObjectTempFilename.c_str());
		return;
	}
	
	SaveObjectFilePredicate predicate = NULL;
	if (method == SavePlayerLogout)
		predicate = IsRentable;

	int num = 0;
	for (int j = 0; j < NUM_WEARS; j++)
	{
		if (GET_EQ(ch,j))
		{
			SaveObject(file, GET_EQ(ch,j), 0, num, predicate);
			if (method == SavePlayerLogout)	GET_EQ(ch,j)->Extract();
		}
	}
	
	FOREACH(ObjectList, ch->carrying, iter)
	{
		SaveObject(file, *iter, 0, num, predicate);
		if (method == SavePlayerLogout)	(*iter)->Extract();
	}
	
	file.close();
	
	REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
	
	remove(playerObjectFilename.c_str());
	if (rename(playerObjectTempFilename.c_str(), playerObjectFilename.c_str()))
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write player '%s' inventory to disk: unable to replace file %s with temporary file %s",
			ch->GetName(),
			playerObjectFilename.c_str(),
			playerObjectTempFilename.c_str());
	}
}


void LoadPlayerObjectFile(CharData *ch)
{
	Lexi::String		filename = ch->GetObjectFilename();
	Lexi::BufferedFile	objectFile(filename.c_str());
	Lexi::Parser		objectParser;
	ObjectList			objects;
	
	if (!objectFile.good())
	{
		FILE *fl = fopen(filename.c_str(), "r");
		if (!fl && errno != ENOENT) {	/* if it fails, NOT because of no file */
			log("SYSERR: READING OBJECT FILE %s (5)", filename.c_str());
			send_to_char("\n********************* NOTICE *********************\n"
							"There was a problem loading your objects from disk.\n"
							"Contact Staff for assistance.\n", ch);
		}
		if (fl)	fclose(fl);
		mudlogf(NRM, ch->GetPlayer()->m_StaffInvisLevel, TRUE,  "%s entering game with no equipment.", ch->GetName());
		
		return;
	}
	
	if (isdigit(*objectFile.peek()))
	{
		objectFile.close();
		
		//	Classic object file system...
		Crash_load(ch);
		return;
	}

	if (ch->desc)	mudlogf(NRM, ch->GetPlayer()->m_StaffInvisLevel, TRUE, "%s entering game.", ch->GetName());

	objectParser.Parse(objectFile);
	
	//	LOAD GLOBAL DATA HERE
	time_t savedOn = objectParser.GetInteger("Saved");
	
	LoadObjectFile(objectParser, objects);
	
	FOREACH(ObjectList, objects, iter)
	{
		ObjData *obj = *iter;
		
		if (obj->InRoom() == NULL)
		{
			void auto_equip(CharData *ch, ObjData *obj, int locate);

			int location = obj->WornOn();
			obj->m_WornOn = -1;
			auto_equip(ch, obj, location);	//	Change this later on
		}
	}
	
	//	Load triggers handled externally
}


void ShowPlayerObjectFile(CharData *ch, const char *name)
{
	BUFFER(buf, MAX_BUFFER_LENGTH);
	int len = 0;
	Lexi::String		filename = CharData::GetObjectFilename(name);
	Lexi::BufferedFile	objectFile(filename.c_str());
	Lexi::Parser		parser;
	
	if (!objectFile.good())
	{
		ch->Send("%s has no rent file.\n", name);
		return;
	}
	
	if (isdigit(*objectFile.peek()))
	{
		objectFile.close();
		
		//	Classic object file system...
		Crash_listrent(ch, name);
		return;
	}
	
	parser.Parse(objectFile);
	
	//	LOAD GLOBAL DATA HERE
	time_t savedOn = parser.GetInteger("Saved");	
	
	int	numRootObjects = parser.NumSections("");
	for (int i = 0; i < numRootObjects; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "", i);
		
		VirtualID	vid		= parser.GetVirtualID("Virtual");
		int			inside	= parser.GetInteger("Inside", 0);
		int			worn	= parser.GetEnum("Worn", eqpos, -1);
		const char *name	= parser.GetString("Name", NULL);
		
		if (vid.IsValid() && !name)
		{
			ObjectPrototype *proto = obj_index.Find(vid);
			if (proto)
			{
				name = proto->m_pProto->m_Name.c_str();	
			}
		}
				
		if (!name)
		{
			name = "Empty";
		}
		
		Lexi::String insideString;
		
		if (inside > 0)
		{
			while (inside > 0)
			{
				insideString += "| ";
				--inside;
			}
		}
		
		if (worn != -1)		len += sprintf_cat(buf, " [%10s] <%s> %s\n", vid.IsValid() ? vid.Print().c_str() : "", eqpos[worn], name);
		else				len += sprintf_cat(buf, " [%10s] %s%s\n", vid.IsValid() ? vid.Print().c_str() : "", insideString.c_str(), name);
		
		if (len > MAX_BUFFER_LENGTH - 256)
		{
			sprintf_cat(buf, "*OVERFLOW*\n");
			break;
		}
	}
	
	if (numRootObjects == 0)
	{
		strcat(buf, "Nothing in inventory.\n");
	}
	
	page_string(ch->desc, buf);
}


//	WARNING
//	BIG WARNING: This uses ObjData::Create which automatically puts the object into the world!
//	WARNING
void LoadObjectFile(Lexi::Parser &parser, ObjectList &rootObjects)
{
	ObjectList			stack;
	ObjData *			prevObject = NULL;
	
	int	numRootObjects = parser.NumSections("");
	
	for (int i = 0; i < numRootObjects; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "", i);
		
		VirtualID	vid			= parser.GetVirtualID("Virtual");
		int			inside		= parser.GetInteger("Inside", 0);
		
		ObjectPrototype *proto	= obj_index.Find(vid);
		
		ObjData * obj = ObjData::Create(proto);
		if (!obj->GetPrototype())
		{
			//	continue?  Treat the object like it's gone...
			obj->m_Keywords = "Empty.";
			obj->m_Name = "Empty.";
			obj->m_RoomDescription = "Empty.";
		}
		
		obj->Load(parser);
		
		if (inside > 0)
		{
			if (inside > stack.size())
			{
				if (prevObject && GET_OBJ_TYPE(prevObject) == ITEM_CONTAINER)
					stack.push_front(prevObject);
			}
			else
			{
				while (inside < stack.size())
				{
					stack.pop_front();
				}
			}
			
			if (stack.size())	obj->ToObject(stack.front(), true);
			else				rootObjects.push_back(obj);
		}
		else
		{
			stack.clear();
			
			if (parser.DoesFieldExist("Room"))
			{
				RoomData *room		= world.Find(parser.GetString("Room"));
				if (room)	obj->ToRoom(room, true);
			}
			else
			{
				obj->m_WornOn	= parser.GetEnum("Worn", eqpos, -1);
			}
			
			rootObjects.push_back(obj);
		}
		
		prevObject = obj;
	}
}


static void SaveObject(Lexi::OutputParserFile &file, ObjData *obj, int level, int &num, SaveObjectFilePredicate predicate)
{
	if (predicate && !predicate(obj))
	{
		return;
	}
	
	file.BeginSection(Lexi::Format("Object %d", ++num));
	{
		obj->Save(file, level);
	}
	file.EndSection();
	
	FOREACH(ObjectList, obj->contents, iter)
	{
		SaveObject(file, *iter, level + 1, num, predicate);
	}
}


void SaveObjectFile(Lexi::OutputParserFile &file, ObjectList &rootObjects, SaveObjectFilePredicate predicate)
{
	int	num = 0;
	
	for (ObjectList::iterator obj = rootObjects.begin(); obj != rootObjects.end(); ++obj)
	{
		SaveObject(file, *obj, 0, num, predicate);
	}
}


void ObjData::Load(Lexi::Parser &parser)
{
	const char *	replacement;
	replacement					= parser.GetString("Keywords", NULL);
	if (replacement)	m_Keywords = replacement;
	
	replacement					= parser.GetString("Name", NULL);
	if (replacement)
	{
		m_Name = replacement;
		
		//	Known work-around to modify a shared string directly
		Lexi::String &str = m_Name.GetStringRef();
		if (!str_cmp(fname(str.c_str()), "a") || !str_cmp(fname(str.c_str()), "an") || !str_cmp(fname(str.c_str()), "the"))
			str[0] = tolower(str[0]);		
	}
	replacement					= parser.GetString("RoomDescription", NULL);
	if (replacement)	m_RoomDescription = replacement;
	replacement					= parser.GetString("Description", NULL);
	if (replacement)	m_Description = replacement;
	
	this->m_Type				= parser.GetEnum("Type", m_Type);
	this->extra					= parser.GetFlags("ExtraFlags", extra_bits, this->extra);
	this->wear					= parser.GetFlags("WearFlags", wear_bits, this->wear);
	m_AffectFlags				= parser.GetBitFlags<AffectFlag>("AffectFlags", m_AffectFlags);

	//	Total weight will equal this, below
	this->weight				= parser.GetInteger("Weight", this->weight);
	//	CONSTANT
//	this->cost					= parser.GetInteger("Cost", this->cost);
	this->m_Timer				= parser.GetInteger("Timer", this->m_Timer);

	//	CONSTANT	
//	this->m_RaceRestriction		= parser.GetFlagsFromTable("Restrictions/Race", race_types, this->m_RaceRestriction);
//	this->m_MinimumLevelRestriction = parser.GetInteger("Restrictions/Level", this->m_MinimumLevelRestriction);
	this->m_ClanRestriction		= parser.GetInteger("Restrictions/Clan", this->m_ClanRestriction);

	this->owner					= parser.GetInteger("Ownership/Owner");
	this->buyer					= parser.GetInteger("Ownership/Buyer");
	this->bought				= parser.GetInteger("Ownership/Bought");
	
	
	const ObjectDefinition * definition = ObjectDefinition::Get(GET_OBJ_TYPE(this));
	
	if (definition)
	{
		PARSER_SET_ROOT(parser, "Values");
		
		FOREACH_CONST(ObjectDefinition::ValueMap, definition->values, iter)
		{
			const ObjectDefinition::Value &value = *iter;
			ObjectTypeValue	slot = value.GetSlot();
			
			if (!parser.DoesFieldExist(value.GetName()) || !value.CanSave())
				continue;
			
			switch (value.GetType())
			{
				case ObjectDefinition::Value::TypeInteger:
					SetValue(slot, parser.GetInteger(value.GetName(), GetValue(slot)));
					break;
				case ObjectDefinition::Value::TypeEnum:
					SetValue(slot, parser.GetEnum(value.GetName(), const_cast<char **>(value.nametable), GetValue(slot)));
					break;
				case ObjectDefinition::Value::TypeFlags:
					SetValue(slot, parser.GetFlags(value.GetName(), const_cast<char **>(value.nametable), GetValue(slot)));
					break;
				case ObjectDefinition::Value::TypeSkill:
					{
						const char *	skillname = parser.GetString(value.GetName(), NULL);
						if (skillname)	SetValue(slot, find_skill(skillname));
					}
					break;
				case ObjectDefinition::Value::TypeVirtualID:
					m_VIDValue[slot] = parser.GetVirtualID(value.GetName(), GetVirtualID().zone);
					continue;	//	No SetValue
				
				case ObjectDefinition::Value::TypeFloat:
					m_FValue[slot] = parser.GetFloat(value.GetName(), GetFloatValue(slot));
					continue;	//	No SetValue
				
				default:
					//	Do nothing
					break;
			}
		}
	}
	

	//	CONSTANT
/*	int numExtraDescs = parser.NumSections("ExtraDescs");
	for (int i = 0; i < numExtraDescs; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "ExtraDescs", i);
		
		const char *	keywords		= parser.GetString("Keywords");
		const char *	description		= parser.GetString("Description");
		
		if (!*keywords || !*description)
			continue;
		
		ExtraDesc *extradesc = new ExtraDesc;
		extradesc->keyword = keywords;
		extradesc->description = description;
		this->m_ExtraDescriptions.push_back(extradesc);
	}

	int numAffects = MIN(parser.NumSections("Affects"), MAX_OBJ_AFFECT);
	for (int i = 0; i < numAffects; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Affects", i);
		
		obj->affect[i].location			= -find_skill(parser.GetString("Location"));
		if (obj->affect[i].location == 0)
			obj->affect[i].location		= parser.GetIntegerFromTable("Location", apply_types, APPLY_NONE);
		obj->affect[i].modifier			= parser.GetInteger("Modifier");
	}
*/	
	
	if (GET_OBJ_TYPE(this) == ITEM_WEAPON && IS_GUN(this) && parser.DoesSectionExist("Gun"))
	{
//		if (!IS_GUN(this))
//			GET_GUN_INFO(this) = new GunData();

		GET_GUN_AMMO(this)				= parser.GetInteger("Gun/Ammo/Amount");
		GET_GUN_AMMO_VID(this)			= parser.GetVirtualID("Gun/Ammo/Virtual");
		GET_GUN_AMMO_FLAGS(this)		= parser.GetInteger("Gun/Ammo/AmmoFlags");

		//	CONSTANT
/*		GET_GUN_DAMAGE(this)			= parser.GetInteger("Gun/Damage");
		GET_GUN_DAMAGE_TYPE(this)		= parser.GetIntegerFromTable("Gun/DamageType", damage_types);
		GET_GUN_RATE(this)				= parser.GetInteger("Gun/Rate");
		GET_GUN_ATTACK_TYPE(this)		= parser.GetIntegerFromTable("Gun/AttackType", attack_types);
		GET_GUN_RANGE(this)				= parser.GetInteger("Gun/Range");
		GET_GUN_OPTIMALRANGE(this)		= parser.GetInteger("Gun/OptimalRange");

		GET_GUN_AMMO_TYPE(this)			= parser.GetIntegerFromTable("Gun/Ammo", ammo_types, -1);
		GET_GUN_MODIFIER(this)			= parser.GetInteger("Gun/Modifier");
		GET_GUN_SKILL(this)				= find_skill(parser.GetString("Gun/Skill"));
		GET_GUN_STRENGTH(this)			= parser.GetInteger("Gun/Strength");
*/	}

	//	CONSTANT
/*	int numScripts = parser.NumFields("Scripts");
	for (int i = 0; i < numScripts; ++i)
	{
		VNum	script = parser.GetIndexedInteger("Scripts", i, -1);
		if (script != -1)
			obj_index[rnum_obj]->m_Triggers.push_back(script);
	}
*/	
	int numVariables = parser.NumSections("Variables");
	for (int i = 0; i < numVariables; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Variables", i);
		
		ScriptVariable *var = ScriptVariable::Parse(parser);
		if (var)
		{
			const ScriptVariable *preexistingVar = SCRIPT(this)->m_Globals.Find(var);
			if (preexistingVar
				&& !strcmp(preexistingVar->GetValue(), var->GetValue()))
				continue;	//	SKIP IT!

			SCRIPT(this)->m_Globals.Add(var);
		}
	}
	
	this->totalWeight = this->weight;
	
	SET_BIT(GET_OBJ_EXTRA(this), ITEM_RELOADED);
}



void ObjData::Save(Lexi::OutputParserFile &file, int level)
{
	ObjData *			proto = GetPrototype() ? GetPrototype()->m_pProto : NULL;
	
	file.WriteVirtualID("Virtual", GetVirtualID());
	file.WriteEnum("Worn", WornOn(), eqpos, -1);
	file.WriteInteger("Inside", level, 0);
	if (InRoom())	file.WriteVirtualID("Room", InRoom()->GetVirtualID());
	
	file.WriteString("Keywords", m_Keywords.c_str(), proto ? proto->m_Keywords.c_str() : "");
	file.WriteString("Name", m_Name.c_str(), proto ? proto->m_Name.c_str() : "");
	file.WriteString("RoomDescription", m_RoomDescription.c_str(), proto ? proto->m_RoomDescription.c_str() : "");
	file.WriteLongString("Description", m_Description.c_str(), proto ? proto->m_Description.c_str() : "");
	
	if (!proto)
	{
		//	These fields are constant for non-dynamic objects
		file.WriteEnum("Type", m_Type, ITEM_UNDEFINED);
		file.WriteFlags("ExtraFlags", this->extra & ~ITEM_RELOADED, extra_bits);
		file.WriteFlags("WearFlags", this->wear, wear_bits);
		file.WriteFlags("AffectFlags", m_AffectFlags);
	}
	
	file.WriteInteger("Weight", this->weight, proto ? proto->weight : 0);
	file.WriteInteger("Timer", m_Timer, proto ? proto->m_Timer : 0);

	file.BeginSection("Restrictions");
	{
		file.WriteInteger("Clan", this->m_ClanRestriction, proto ? proto->m_ClanRestriction : 0);	
	}
	file.EndSection();
	
	file.BeginSection("Ownership");
	{
		if (this->owner)
		{
			const char *ownerName = get_name_by_id(this->owner);
			if (ownerName)	file.WriteString("Owner", Lexi::Format("%u %s", this->owner, ownerName).c_str());
		}
		if (this->buyer)
		{
			const char *buyerName = get_name_by_id(this->buyer);
			if (buyerName)	file.WriteString("Buyer", Lexi::Format("%u %s", this->buyer, buyerName).c_str());
		}
		file.WriteDate("Bought", this->bought);
	}
	file.EndSection();
	

	const ObjectDefinition * definition = ObjectDefinition::Get(GET_OBJ_TYPE(this));
	
	if (definition)
	{
		file.BeginSection("Values");
		
		FOREACH_CONST(ObjectDefinition::ValueMap, definition->values, iter)
		{
			const ObjectDefinition::Value &value = *iter;
			ObjectTypeValue	slot = value.GetSlot();
			
			if (!value.CanSave())
				continue;
			
			//	Do not save values that match the prototype
			if (proto)
			{
				if (value.GetType() == ObjectDefinition::Value::TypeFloat)
				{
					if (GetFloatValue(slot) == proto->GetFloatValue(slot))
						continue;
				}
				else if (value.GetType() == ObjectDefinition::Value::TypeVirtualID)
				{
					if (m_VIDValue[slot] == proto->m_VIDValue[slot])
						continue;
				}
				else
				{
					if (GetValue(slot) == proto->GetValue(slot))
						continue;
				}
			}
			
			switch (value.GetType())
			{
				case ObjectDefinition::Value::TypeInteger:
					file.WriteInteger(value.GetName(), GetValue(slot));
					break;
					
				case ObjectDefinition::Value::TypeEnum:
					file.WriteEnum(value.GetName(), GetValue(slot), const_cast<char **>(value.nametable));
					break;
					
				case ObjectDefinition::Value::TypeFlags:
					file.WriteFlagsAlways(value.GetName(), GetValue(slot), const_cast<char **>(value.nametable));
					break;
					
				case ObjectDefinition::Value::TypeSkill:
					file.WriteString(value.GetName(), skill_name(GetValue(slot)), skill_name(0));
					break;
					
				case ObjectDefinition::Value::TypeVirtualID:
					file.WriteVirtualID(value.GetName(), m_VIDValue[slot], GetVirtualID().zone);
					break;
				
				case ObjectDefinition::Value::TypeFloat:
					file.WriteFloat(value.GetName(), GetFloatValue(slot));
				
				default:
					continue;	//	Don't do anything with this one
			}
		}
		
		file.EndSection();
	}

	
	if (GET_OBJ_TYPE(this) == ITEM_WEAPON && m_pGun && proto && proto->m_pGun)
	{
		file.BeginSection("Gun/Ammo");
		{
			file.WriteInteger("Amount", m_pGun->ammo.m_Amount, 0);
			file.WriteVirtualID("Virtual", m_pGun->ammo.m_Virtual);
			file.WriteInteger("AmmoFlags", m_pGun->ammo.m_Flags, 0);
		}
		file.EndSection();
	}
	

	if (OBJ_FLAGGED(this, ITEM_SAVEVARIABLES))
	{
		file.BeginSection("Variables");
		{
			ScriptVariable::List &variables = GetScript()->m_Globals;
			int i = 0;
			for (ScriptVariable::List::iterator iter = variables.begin(); iter != variables.end(); ++iter)
			{
				ScriptVariable *var = *iter;
				
				if (GetPrototype())
				{
//					if (var->IsShared())
//						continue;
					
					const ScriptVariable *protoVar = GetPrototype()->m_InitialGlobals.Find(var);
					if (protoVar && protoVar->GetValueString() == var->GetValueString())
						continue;	//	Skip this variable for sure!

	//	TODO: When the new variable save system is in place!
//					if (!OBJ_FLAGGED(this, ITEM_SAVEVARIABLES)
//						&& (!protoVar || !IS_SET(protoVar->GetFlags(), VAR_SAVE)))
//						continue;
				}
				
				var->Write(file, Lexi::Format("Variable %d", ++i), /*bSaving:*/true);
			}
		}
		file.EndSection();
	}
}
