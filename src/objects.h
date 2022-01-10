/*************************************************************************
*   File: objects.h                  Part of Aliens vs Predator: The MUD *
*  Usage: Header file for objects                                        *
*************************************************************************/

#ifndef	__OBJECTS_H__
#define __OBJECTS_H__

#include <float.h>
#include <list>
#include <map>

#include "types.h"
#include "lexilist.h"
#include "lexistring.h"
#include "entity.h"
#include "index.h"
#include "event.h"
#include "extradesc.h"
#include "affects.h"
#include "races.h"

#define NUM_OBJ_VALUES	8
#define	NUM_OBJ_FVALUES	2
#define NUM_OBJ_VIDVALUES 1


class ObjData;
typedef Lexi::List<ObjData *>			ObjectList;
typedef IndexMap<ObjData>				ObjectPrototypeMap;
typedef ObjectPrototypeMap::data_type	ObjectPrototype;


//	WARNING: Any changes here MUST also update:
//		objectValueDefinitions
//		item_types
enum ObjectType
{
	ITEM_UNDEFINED		= 0,
	ITEM_LIGHT,			//	Light source
	ITEM_WEAPON,		//	Weapon
	ITEM_AMMO,			//	Ammo
	ITEM_THROW,			//	Throwable Weapon
	ITEM_GRENADE,		//	Grenade
	ITEM_BOOMERANG,
	ITEM_TREASURE,		//	Item is a treasure
	ITEM_ARMOR,
	ITEM_OTHER,			//	Misc object
	ITEM_TRASH,			//	Trash - shopkeeps won't buy
	ITEM_CONTAINER,		//	Container
	ITEM_NOTE,			//	A note
	ITEM_KEY,			//	Item is a key
	ITEM_PEN,
	ITEM_BOAT,
	ITEM_FOOD,
	ITEM_DRINKCON,
	ITEM_FOUNTAIN,
	ITEM_VEHICLE,
	ITEM_VEHICLE_HATCH,
	ITEM_VEHICLE_CONTROLS,
	ITEM_VEHICLE_WINDOW,
	ITEM_VEHICLE_WEAPON,
	ITEM_BED,
	ITEM_CHAIR,
	ITEM_BOARD,
	ITEM_INSTALLABLE,
	ITEM_INSTALLED,
	ITEM_ATTACHMENT,
	ITEM_TOKEN,
	
	NUM_ITEM_TYPES
};


enum ObjectTypeValue
{
	OBJVAL_FIRST_FLOAT			= NUM_OBJ_VALUES,
	OBJVAL_FIRST_VID			= OBJVAL_FIRST_FLOAT + NUM_OBJ_FVALUES,
	OBJVAL_MAX					= OBJVAL_FIRST_VID + NUM_OBJ_VIDVALUES,
	
	//
	OBJVAL_LIGHT_HOURS			= 0,
	OBJVAL_LIGHT_RADIUS			= 1,
	
	OBJVAL_WEAPON_DAMAGE		= 0,
	OBJVAL_WEAPON_DAMAGETYPE,
	OBJVAL_WEAPON_ATTACKTYPE,
	OBJVAL_WEAPON_SKILL,
	OBJVAL_WEAPON_SKILL2,
	OBJVAL_WEAPON_RATE,
	OBJFVAL_WEAPON_SPEED		= 0,	//	Float
	OBJFVAL_WEAPON_STRENGTH,			//	Float
	
	OBJVAL_AMMO_TYPE			= 0,
	OBJVAL_AMMO_AMOUNT,
	OBJVAL_AMMO_FLAGS,
	
	//	THROW and BOOMERANG
	OBJVAL_THROWABLE_DAMAGE		= 0,
	OBJVAL_THROWABLE_DAMAGETYPE,
	OBJVAL_THROWABLE_SKILL,
	OBJVAL_THROWABLE_RANGE,
	OBJFVAL_THROWABLE_SPEED		= 0,	//	Float
	OBJFVAL_THROWABLE_STRENGTH,			//	Float
	
	OBJVAL_GRENADE_DAMAGE		= 0,
	OBJVAL_GRENADE_DAMAGETYPE,
	OBJVAL_GRENADE_TIMER,

	OBJVAL_ARMOR_DAMAGETYPE0	= 0,
	OBJVAL_ARMOR_DAMAGETYPE1,
	OBJVAL_ARMOR_DAMAGETYPE2,
	OBJVAL_ARMOR_DAMAGETYPE3,
	OBJVAL_ARMOR_DAMAGETYPE4,
	OBJVAL_ARMOR_DAMAGETYPE5,
	OBJVAL_ARMOR_DAMAGETYPE6,
	
	OBJVAL_CONTAINER_CAPACITY	= 0,
	OBJVAL_CONTAINER_FLAGS,
	OBJVAL_CONTAINER_CORPSE,
	OBJVIDVAL_CONTAINER_KEY		= 0,	//	VID
	
	//	FOOD, DRINKCON, and FOUNTAIN
	OBJVAL_FOODDRINK_FILL		= 0,	//	Food, Drink, Fountain
	OBJVAL_FOODDRINK_CONTENT,			//	Drink, Fountain
	OBJVAL_FOODDRINK_TYPE,				//	Drink, Fountain
	OBJVAL_FOODDRINK_POISONED,			//	Food, Drink, Fountain

	OBJVAL_VEHICLE_FLAGS		= 0,
	OBJVAL_VEHICLE_SIZE,
	OBJVIDVAL_VEHICLE_ENTRY		= 0,	//	VID

	OBJVIDVAL_VEHICLEITEM_VEHICLE	= 0,	//	Hatch, Controls, Window, Weapon
	
	OBJVAL_FURNITURE_CAPACITY	= 0,	//	Bed, chair
	
	OBJVAL_BOARD_READLEVEL		= 0,
	OBJVAL_BOARD_WRITELEVEL,
	OBJVAL_BOARD_REMOVELEVEL,
	
	OBJVAL_INSTALLABLE_TIME		= 0,
	OBJVAL_INSTALLABLE_SKILL,
	OBJVIDVAL_INSTALLABLE_INSTALLED= 0,	//	VID
	
	OBJVAL_INSTALLED_TIME		= 0,
	OBJVAL_INSTALLED_PERMANENT,
	OBJVAL_INSTALLED_DISABLED,
	
	OBJVAL_TOKEN_VALUE			= 0,
	OBJVAL_TOKEN_SET,
	OBJVAL_TOKEN_SIGNER
};


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/
#define ITEM_WEAR_EYES		(1 << 15) /* Can be worn on eyes */
#define ITEM_WEAR_TAIL		(1 << 16)
/*
enum ItemWearBits
{
	ITEM_WEAR_TAKE,
	ITEM_WEAR_FINGER,
	ITEM_WEAR_NECK,
	ITEM_WEAR_BODY,
	ITEM_WEAR_HEAD,
	ITEM_WEAR_LEGS,
	ITEM_WEAR_FEET,
	ITEM_WEAR_HANDS,
	ITEM_WEAR_ARMS,
	ITEM_WEAR_SHIELD,
	ITEM_WEAR_ABOUT,
	ITEM_WEAR_WAIST,
	ITEM_WEAR_WRIST,
	ITEM_WEAR_WIELD,
	ITEM_WEAR_HOLD,
	ITEM_WEAR_EYES,
	ITEM_WEAR_TAIL,
	
	NUM_ITEM_WEARS
};

typedef Lexi::BitFlags<NUM_ITEM_WEARS, ItemWearBits> ItemWearFlags;
*/
//	#include "objectflags.h"

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW			(1 << 0)	/* Item is glowing		*/
#define ITEM_HUM			(1 << 1)	/* Item is humming		*/
#define ITEM_NORENT			(1 << 2)	/* Item cannot be rented	*/
#define ITEM_NODONATE		(1 << 3)	/* Item cannot be donated	*/
#define ITEM_LOG			(1 << 4)	/* Item cannot be made invis	*/
#define ITEM_INVISIBLE		(1 << 5)	/* Item is invisible		*/
#define ITEM_NODROP			(1 << 6)	/* Item is cursed: can't drop	*/
#define ITEM_AUTOSNIPE		(1 << 7)	/* Weapon allows auto-shoot */
#define ITEM_PERSISTENT		(1 << 8)	/* Persistent object */
#define ITEM_NOSTACK		(1 << 9)	/* Don't stack in room */
#define ITEM_VIEWABLE		(1 << 10)	/* visible at a distance	*/
#define ITEM_NOSELL			(1 << 11)	/* Shopkeepers won't touch it	*/
#define ITEM_NOLOSE			(1 << 12)	/* Don't lose it when you die */
#define ITEM_MOVEABLE		(1 << 13)	/* Can move it */
#define ITEM_MISSION		(1 << 14)	/* Missoin item - I thought this already existed! */
#define ITEM_TWO_HAND		(1 << 15)
#define ITEM_SPRAY			(1 << 16)
#define ITEM_NOSHOOT		(1 << 17)
#define ITEM_STAFFONLY		(1 << 18)
#define ITEM_SAVEVARIABLES	(1 << 19)
#define ITEM_DEATHPURGE		(1 << 20)
#define ITEM_NOMELEE		(1 << 21)
#define ITEM_NOPUT			(1 << 22)
#define ITEM_SPECIAL_MISSION	(1 << 23)
#define ITEM_HALF_MELEE_DAMAGE (1 << 24)
#define ITEM_LEVELBASED		(1 << 25)
#define ITEM_SCRIPTEDCOMBAT	(1 << 26)

#define ITEM_RELOADED		(1 << 30)
#define ITEM_DELETED		(1 << 31)	/* Item has been DELETED */

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0

#define MAX_OBJ_AFFECT		8	/* Used in obj_file_elem *DO*NOT*CHANGE* */




struct ObjAffectedType {
	ObjAffectedType() : location(0), modifier(0) { }
			
	short				location;				// Which ability to change (APPLY_XXX)
	short				modifier;				// How much it changes by
};

//	For Guns
struct GunData {
						GunData();
	
	char				skill, skill2;			// Skill used
	char				attack;					// Attack type
	char				damagetype;				// Type of damage
	int					damage;
	int					strength;				// Real strength for multiple attacks
	int					rate;					// Rate of fire
	int					range;					// Range of gun
	int					optimalrange;
	int					modifier;
	struct {
		unsigned int		m_Type;				// Type of ammo used
		VirtualID			m_Virtual;			// VNUM of ammo currently loaded
		unsigned int		m_Amount;			// Amount of ammo currently loaded
		Flags				m_Flags;
	} ammo;
};


namespace Lexi {
	class Parser;
}




class ObjData : public Entity
{
public:
						ObjData(void);
						ObjData(const ObjData &obj);
						~ObjData(void);
	
	static ObjData *	Create(ObjData *obj = NULL);
	static ObjData *	Create(ObjectPrototype *proto);
	static ObjData *	Create(VirtualID vnum);
	static ObjData *	Clone(ObjData *obj);
	
	static ObjData *	Find(IDNum n);
	
	static void			Parse(Lexi::Parser &parser, VirtualID nr);
	void				Write(Lexi::OutputParserFile &file);
	
private:
	void				operator=(const ObjData &obj);
public:	
	//	Entity
	virtual	void		Extract();
	virtual Entity::Type	GetEntityType() { return Entity::TypeObject; }

	//	Accessors
	const char *		GetAlias() const { return m_Keywords.c_str(); }
	const char *		GetName() const { return m_Name.c_str(); }
	const char *		GetRoomDescription() const { return m_RoomDescription.c_str(); }
	const char *		GetDescription() const { return m_Description.c_str(); }
	
	void				ToWorld();
	void				FromWorld();
	
	void				ToChar(CharData *ch, bool append = false);
	void				FromChar(void);
	void				ToRoom(RoomData *room, bool append = false);
	void				FromRoom(void);
	void				ToObject(ObjData *obj_to, bool append = false);
	void				FromObject(void);
	void				unequip(void);
	void				equip(CharData *ch);
	
	void				update(unsigned int use);
	
	bool				load(FILE *fl, const char *filename);
//	void				save(FILE *fl, int location);
	
	void				Load(Lexi::Parser &parser);
	void				Save(Lexi::OutputParserFile &file, int depth);

	RoomData *			Room(void);
	
	int					TotalCost();
	int					TotalCost(CharData *ch);
	
	void				SetBuyer(CharData *ch);
	
	void				AddOrReplaceEvent(Event *event);
	
//	UNIMPLEMENTED:	Replace bool same_obj(ObjData *, ObjData *) with this
//	bool				operator==(const ObjData &) const;
	
	inline ObjectType	GetType() const { return m_Type; }
	
//	inline const ItemWearFlags &GetWearFlags() const { return m_WearFlags; }
	
	inline int			GetValue(ObjectTypeValue index) const { return m_Value[index]; }
	inline void			SetValue(ObjectTypeValue index, int v) { m_Value[index] = v; }
	inline float		GetFloatValue(ObjectTypeValue index) const { return m_FValue[index]; }
	inline VirtualID	GetVIDValue(ObjectTypeValue index) const { return m_VIDValue[index]; }
	
	inline int			GetMinimumLevelRestriction() const { return m_MinimumLevelRestriction; }
	inline const RaceFlags &GetRaceRestriction() const { return m_RaceRestriction; }
	inline IDNum		GetClanRestriction() const { return m_ClanRestriction; }
	
	
public:
	ObjectPrototype *	GetPrototype() { return m_Prototype; }
	void				SetPrototype(ObjectPrototype *proto) { m_Prototype = proto; }
	
	VirtualID			GetVirtualID() { return GetPrototype() ? GetPrototype()->GetVirtualID() : VirtualID(); }
	
	RoomData *			InRoom()					{ return m_InRoom; }
	ObjData *			Inside()					{ return m_Inside; }
	CharData *			CarriedBy()					{ return m_CarriedBy; }
	CharData *			WornBy()					{ return m_WornBy; }
	int					WornOn()					{ return m_WornOn; }
	
private:
	ObjectPrototype *	m_Prototype;
	RoomData *			m_InRoom;
	ObjData *			m_Inside;
public:
	CharData *			m_CarriedBy;
	CharData *			m_WornBy;
	int					m_WornOn;
	
public:
	Lexi::SharedString	m_Keywords;				// Title of object: get, etc
	Lexi::SharedString	m_Name;					// when worn/carry/in cont.
	Lexi::SharedString	m_RoomDescription;		// When in room
	Lexi::SharedString	m_Description;			// What to write when used
	ExtraDesc::List		m_ExtraDescriptions;	// extra descriptions

	ObjectList			contents;				// Contains objects
	
	Event::List			m_Events;					//	List of events

	//	Object information
//	union {
		int					m_Value[NUM_OBJ_VALUES];	// Values of the item
		float				m_FValue[NUM_OBJ_FVALUES];
		VirtualID			m_VIDValue[NUM_OBJ_VIDVALUES];
//		ItemProperties		Properties;
//	};
	
	unsigned int		cost;					// Cost of the item
	int					weight, totalWeight;	// Weight of the item
	unsigned int		m_Timer;				// Timer
	ObjectType			m_Type;					// Type of item
	Flags				wear;
	//ItemWearFlags		m_WearFlags;			// Wear flags
	//	To replace EXTRA
//	typedef BitFlags<NUM_OBJECT_FLAGS, ObjectFlags>	ObjectBits;
//	ObjectBits			m_Flags;
	Flags				extra;					// Extra flags (glow, hum, etc)
	Affect::Flags		m_AffectFlags;			// Affect flags

	int					m_MinimumLevelRestriction;					//	Maximum level
	RaceFlags			m_RaceRestriction;
	IDNum				m_ClanRestriction;
	
	time_t				bought;
	IDNum				buyer;
	IDNum				owner;
	
	GunData *			m_pGun;					//	Ranged weapon data
	
	ObjAffectedType		affect[MAX_OBJ_AFFECT];  /* affects */
};


extern ObjectList				object_list;
extern ObjectPrototypeMap		obj_index;



struct WeaponProperties
{
	int				damage;
	int				damageType;
	int				skill;
	int				attackType;
	int				unused;
	int				rateOfFire;
};



struct ObjectDefinition
{
	struct Value
	{
		enum Type
		{
			TypeUnused,
			TypeInteger,
			TypeFloat,
			TypeEnum,
			TypeFlags,
			TypeSkill,
			TypeVirtualID
		};
		
		
		
		Value(ObjectTypeValue s, const char *n)
		:	slot(s)
		,	type(TypeUnused)
		,	name(n)
		,	bCanEdit(true)
		,	bCanSave(false)
		,	intDefault(0)
		,	floatDefault(0)
		,	intRange(INT_MIN, INT_MAX)
		,	floatRange(FLT_MIN, FLT_MAX)
		,	nametable(NULL)
		{
		}
		
private:
		Value();	//	No default construction allowed
		
		ObjectTypeValue		slot;
		Type				type;
		/*Lexi::String*/ const char *		name;
		bool				bCanEdit;
		bool				bCanSave;
		
public:
		int					intDefault;
		float				floatDefault;
			
		Range<int>			intRange;
		Range<float>		floatRange;
		
		const char **		nametable;
		
		ObjectTypeValue		GetSlot() const { return slot; }
		Type				GetType() const { return type; }
		const char *		GetName() const { return name; /*.c_str();*/ }
		bool				CanEdit() const { return bCanEdit; }
		bool				CanSave() const { return bCanSave; }
		
		Value &				SetUneditable() { bCanEdit = false; return *this; }
		Value &				SetSavable() { bCanSave = true; return *this; }
		
		//	Pseudo-Constructors
		Value &				IntegerValue(int def = 0)
		{
			type = TypeInteger;
			intRange = Range<int>(INT_MIN, INT_MAX);
			intDefault = def;
			return *this;
		}
		
		Value &				IntegerValue(Range<int> range, int def = 0)
		{
			type = TypeInteger;
			intRange = range;
			intDefault = def;
			return *this;
		}
		
		Value &				FloatValue(Range<float> range, float def = 0)
		{
			type = TypeFloat;
			floatRange = range;
			floatDefault = def;
			return *this;
		}
		
		Value &				FloatValue(float def = 0)
		{
			type = TypeFloat;
			floatRange = Range<float>(FLT_MIN, FLT_MAX);
			floatDefault = def;
			return *this;
		}

		Value &				EnumValue(char **table, int def = 0)
		{
			type = TypeEnum;
			nametable = const_cast<const char **>(table);
			intDefault = def;
			return *this;
		}
		
		Value &				FlagsValue(char **table)
		{
			type = TypeFlags;
			nametable = const_cast<const char **>(table);
			return *this;
		}
	
		Value &				BooleanValue(bool def = false)
		{
			type = TypeInteger;
			intRange = Range<int>(0, 1);
			intDefault = def ? 1 : 0;
			return *this;
		}

		Value &				SkillValue()
		{
			type = TypeSkill;
			return *this;
		}
		
		Value &				VirtualIDValue(Entity::Type t)
		{
			type = TypeVirtualID;
			intDefault = t;
			return *this;
		}


	};
	

	static const ObjectDefinition *Get(ObjectType t);
//	const Value *			GetValue(ObjectTypeValue v);
//	const Value *			GetFValue(ObjectTypeValue v);
	
	Lexi::String			name;
	typedef std::pair<ObjectTypeValue, Value> 	TypeValuePair;
	typedef std::vector< Value >				ValueMap;
	ValueMap				values;
	
	Value &					Add(ObjectTypeValue t, const char *n) 
	{
		values.push_back( /*TypeValuePair( t,*/ Value(t,n) /*)*/ );
		return values.back();
	}
	

	typedef std::map<ObjectType, ObjectDefinition>	Map;
	static Map			ms_Definitions;
};


void equip_char(CharData * ch, ObjData * obj, unsigned int pos);
ObjData *unequip_char(CharData * ch, unsigned int pos);

//	Object Save Predicate
typedef bool (*SaveObjectFilePredicate)(ObjData *obj);

//	Predicate Functions
bool IsRentable(ObjData *obj);

//	Persistence
void LoadPersistentObjectFile(const Lexi::String &filename, RoomData *defaultRoom = NULL);
void SavePersistentObjects();
void LoadPersistentObjects();

//	Object Load/Save
void LoadObjectFile(Lexi::Parser &parser, ObjectList &rootObjects);
void SaveObjectFile(Lexi::OutputParserFile &file, ObjectList &rootObjects, SaveObjectFilePredicate predicate);

enum SavePlayerObjectsMethod
{
	SavePlayerBackup,
	SavePlayerLogout
};

void LoadPlayerObjectFile(CharData *ch);
void SavePlayerObjectFile(CharData *ch, SavePlayerObjectsMethod method = SavePlayerBackup);
void ShowPlayerObjectFile(CharData *ch, const char *name);

#endif	// __OBJECTS_H__
