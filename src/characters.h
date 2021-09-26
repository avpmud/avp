/*************************************************************************
*   File: characters.h               Part of Aliens vs Predator: The MUD *
*  Usage: Header file for characters                                     *
*************************************************************************/


#ifndef	__CHARACTERS_H__
#define __CHARACTERS_H__

#include "types.h"

#include "lexistring.h"
#include "lexisharedstring.h"
#include "lexilist.h"
#include <list>
#include <vector>
#include <map>

#include "entity.h"

#include "event.h"

#include "opinion.h"
#include "alias.h"
#include "clans.h"
#include "races.h"
#include "timercommand.h"

#include "affects.h"
//#include "PersonalPtr.h"


class	Affect;
class	Path;

class CharData;
typedef Lexi::List<CharData *>	CharacterList;

typedef IndexMap<CharData>		CharacterPrototypeMap;
typedef CharacterPrototypeMap::data_type		CharacterPrototype;


//	Sex
enum Sex {
	SEX_NEUTRAL = 0,
	SEX_MALE,
	SEX_FEMALE,
	NUM_SEXES
};

typedef Lexi::BitFlags<NUM_SEXES, Sex>	SexFlags;


//	Positions
enum Position
{
	POS_DEAD = 0,			//	dead
	POS_MORTALLYW,			//	mortally wounded
	POS_INCAP,				//	incapacitated
	POS_STUNNED,			//	stunned
	POS_SLEEPING,			//	sleeping
	POS_RESTING,			//	resting
	POS_SITTING,			//	sitting
	POS_FIGHTING,			//	fighting
	POS_STANDING,			//	standing
	NUM_POSITIONS
};



/* Player flags: used by char_data.char_specials.act */
#define PLR_PKONLY		(1 << 0)   /* Player is a traitor			*/
#define PLR_unused1		(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN		(1 << 2)   /* Player is frozen				*/
#define PLR_DONTSET		(1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING		(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING		(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH		(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK		(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT		(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE		(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED		(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_unused12	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/

#define PLR_SKILLPOINT_INCREASE	(1 << 30)	//	add: 11-01-03, remove: 12-20-03
#define PLR_SKILLCOST_ADJUST	(1 << 31)	//	add: 12-20-03, remove: 12-29-03

/* Mobile flags: used by char_data.char_specials.act */

#define MOB_SPEC			(1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL		(1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER		(1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC			(1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE			(1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE		(1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE		(1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY			(1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_ALL		(1 << 8)  /* auto attack everything		*/
#define MOB_MEMORY			(1 << 9) /* remember attackers if attacked	*/
#define MOB_HELPER			(1 << 10) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM			(1 << 11) /* Mob can't be charmed		*/
#define MOB_NOSNIPE			(1 << 12) /* Mob can't be summoned		*/
#define MOB_NOSLEEP			(1 << 13) /* Mob can't be slept		*/
#define MOB_NOBASH			(1 << 14) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND			(1 << 15) /* Mob can't be blinded		*/
#define MOB_ACIDBLOOD		(1 << 16) /* MOB has acid blood (splash) */
#define MOB_PROGMOB			(1 << 17) /* Mob is a Program MOB */
#define MOB_AI				(1 << 18) /* Mob is a Program MOB */
#define MOB_MECHANICAL		(1 << 19)
#define MOB_NOMELEE			(1 << 20)
#define MOB_HALFSNIPE		(1 << 21)
#define MOB_FRIENDLY		(1 << 22)  /* Mob has a callable spec-proc	*/

#define MOB_DELETED			(1 << 31) /* Mob is deleted */

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF		(1 << 0)		//	Room descs won't normally be shown
#define PRF_COMPACT		(1 << 1)		//	No extra CRLF pair before prompts
#define PRF_PK			(1 << 2)		//	Can PK/be PK'd
//#define PRF_AUTOZLIB	(1 << 3) 		//	MCCP Support
//#define PRF_SCMUD_REFUGEE	(1 << 4)  	//	Player came from SCMud, can't transfer MEQ
//#define PRF_NOCLAN		(1 << 5)  /* Display mana points in prompt	*/
#define PRF_NOAUTOFOLLOW	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)		//	Display exits in a room
#define PRF_NOHASSLE	(1 << 8)		//	Staff are immortal
//#define PRF_MISSION		(1 << 9)  /* On quest				*/
//#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR		(1 << 13) /* Color (low bit)			*/
//#define PRF_unused14	(1 << 14) /* Color (high bit)			*/
//#define PRF_NOWIZ		(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1		(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2		(1 << 17) /* On-line System Log (high bit)	*/
//#define PRF_NOMUSIC		(1 << 18) /* Can't hear music channel		*/
//#define PRF_NOGOSS		(1 << 19) /* Can't hear gossip channel		*/
//#define PRF_NOGRATZ		(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
//#define PRF_unused22	(1 << 22) /* Can hear clan talk */
//#define PRF_AFK			(1 << 23)
#define PRF_AUTOSWITCH	(1 << 25)
//#define PRF_NOINFO		(1 << 26)


class Channel {
public:
	enum {
		NoShout		= (1 << 0),
		NoTell		= (1 << 1),
		NoChat		= (1 << 2),
		Mission		= (1 << 3),
		NoMusic		= (1 << 4),
		NoGratz		= (1 << 5),
		NoInfo		= (1 << 6),
		NoWiz		= (1 << 7),
		NoRace		= (1 << 8),
		NoClan		= (1 << 9),
		NoBitch		= (1 << 10),
		NoNewbie	= (1 << 11)
	};
};


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
enum WearPosition
{
	WEAR_NOT_WORN = -1,
	WEAR_FINGER_R = 0,
	WEAR_FINGER_L,
	WEAR_NECK,
	WEAR_BODY,
	WEAR_HEAD,
	WEAR_LEGS,
	WEAR_FEET,
	WEAR_HANDS,
	WEAR_ARMS,
	WEAR_ABOUT,
	WEAR_WAIST,
	WEAR_WRIST_R,
	WEAR_WRIST_L,
	WEAR_EYES,
	WEAR_TAIL,
	WEAR_HAND_R,
	WEAR_HAND_L,

	NUM_WEARS
};


enum ExtendedWearPosition
{
/* position of messages in the list for WEAR_HAND_x, depending on object */
	POS_WIELD_TWO		= NUM_WEARS,
	POS_HOLD_TWO,
	POS_WIELD,
	POS_WIELD_OFF,
	POS_LIGHT,
	POS_HOLD
};

enum
{
	ARMOR_LOC_BODY = 0,
	ARMOR_LOC_HEAD,
//	ARMOR_LOC_ARMS,
//	ARMOR_LOC_HANDS,
//	ARMOR_LOC_LEGS,
//	ARMOR_LOC_FEET,
	ARMOR_LOC_NATURAL,
	ARMOR_LOC_MISSION,
	NUM_ARMOR_LOCS	
};

/*
#define ARMOR_LOC_BODY_FLAG		(1 << ARMOR_LOC_BODY)
#define ARMOR_LOC_HEAD_FLAG		(1 << ARMOR_LOC_HEAD)
#define ARMOR_LOC_ARMS_FLAG		(1 << ARMOR_LOC_ARMS)
#define	ARMOR_LOC_HANDS_FLAG	(1 << ARMOR_LOC_HANDS)
#define ARMOR_LOC_LEGS_FLAG		(1 << ARMOR_LOC_LEGS)
#define ARMOR_LOC_FEET_FLAG		(1 << ARMOR_LOC_FEET)
*/
enum DamageType
{
	DAMAGE_UNKNOWN = -1,
	DAMAGE_BLUNT = 0,
	DAMAGE_SLASH,
	DAMAGE_IMPALE,
	DAMAGE_BALLISTIC,
	DAMAGE_FIRE,
	DAMAGE_ENERGY,
	DAMAGE_ACID,
	NUM_DAMAGE_TYPES
};


enum {
	LVL_IMMORT	= 101,
	LVL_STAFF	= 102,
	LVL_SRSTAFF	= 103,
	LVL_ADMIN	= 104,
	LVL_CODER	= 105,

	LVL_FREEZE	= LVL_ADMIN
};



//	Player conditions
enum { DRUNK, FULL, THIRST };

const int	MAX_NAME_LENGTH		= 12;
const int	MAX_PWD_LENGTH		= 20;
const int	MAX_TITLE_LENGTH	= 80;
const int	EXDSCR_LENGTH		= 480;
const int	MAX_SKILLS			= 32;	//	adjust if need be later
//const int	MAX_AFFECT			= 32;
const int	MAX_ICE_LENGTH		= 160;


#include "relation.h"

enum Substate
{
	SUB_NONE,
	SUB_TIMER_DO_ABORT = 128,
	SUB_TIMER_CANT_ABORT
};


enum CombatMode
{
	COMBAT_MODE_DEFENSIVE,
	COMBAT_MODE_NORMAL,
	COMBAT_MODE_OFFENSIVE
};

enum Visibility
{
	VISIBLE_NONE,
	VISIBLE_SHIMMER,
	VISIBLE_FULL
};


#define DEFAULT_PAGE_LENGTH     22



struct title_type {
	char *	title_m;
	char *	title_f;
};
extern struct title_type titles[NUM_RACES][16];


	
struct PlayerIMCData
{
						PlayerIMCData();
	
	
	Lexi::String		m_Listen;
	Lexi::String		m_Reply;
	Lexi::String		m_ReplyName;
	Flags				m_Deaf;		//	Deaf-to-channel
	Flags				m_Allow;		//	Allow overrides
	Flags				m_Deny;		//	Deny overrides
	
private:
						PlayerIMCData(const PlayerIMCData &);
	void				operator=(const PlayerIMCData &);
};


struct PlayerIMC2Data
{
						PlayerIMC2Data()
						:	m_Flags(0)
						,	m_Perm(0)
						{}
						
	Lexi::StringList	m_Ignores;
	Lexi::String		m_RReply;
//	Lexi::String		m_RReplyName;
	Lexi::String		m_Listen;
	Lexi::String		m_Denied;
	Lexi::StringList	m_TellHistory;
	int					m_Flags;
	int					m_Perm;
	
	Lexi::String		m_Comment;
};



class PlayerData {
public:
						PlayerData();
						~PlayerData();
	
	//	Internal
	int					m_Version;
	
	//	Login information
	char				m_Password[MAX_PWD_LENGTH+1];
	Lexi::String		m_Host;
	Lexi::String		m_MaskedHost;
	int					m_BadPasswordAttempts;
	
	time_t				m_Creation;			//	Moment of character's creation
	time_t				m_LoginTime;		//	Time of login for current session
	time_t				m_LastLogin;		//	Last time logged on
	time_t				m_TotalTimePlayed;	//	Total time played
	
	int					m_IdleTime;			//	Minutes idle
	VirtualID			m_LastRoom;			//	Last room before idling to void

	//	Input
	Lexi::String		m_Prompt;	
	Alias::List			m_Aliases;
	
	//	Communication
	Flags				m_Channels;
	
	Lexi::String		m_AFKMessage;
	Lexi::StringList	m_InfoBuffer;
	Lexi::StringList	m_ChatBuffer;
	Lexi::StringList	m_TellBuffer;
	Lexi::StringList	m_WizBuffer;
	Lexi::StringList	m_SayBuffer;
	std::list<IDNum>	m_IgnoredPlayers;
	IDNum				m_LastTellReceived, m_LastTellSent;
	
	//	Staff
	Lexi::String		m_PoofInMessage;
	Lexi::String		m_PoofOutMessage;
	Lexi::String		m_Pretitle;
	Flags				m_StaffPrivileges;
	int					m_StaffInvisLevel;
	
	//	Preferences
	int					m_PageLength;
	VirtualID			m_LoadRoom;
	Flags				m_Preferences;
	
	//	Security
	int					m_Karma;
	int					m_FreezeLevel;
	time_t				m_MutedAt;
	int					m_MuteLength;
	
	//	Skills
	unsigned short		m_RealSkills[MAX_SKILLS+1];
	unsigned short		m_Skills[MAX_SKILLS+1];
	int					m_SkillPoints;
	unsigned int		m_LifetimeSkillPoints;
	
	//	Misc
	Clan::Member *		m_pClanMembership;
	
	unsigned int		m_PlayerKills;
	unsigned int		m_MobKills;
	unsigned int		m_PlayerDeaths;
	unsigned int		m_MobDeaths;
	
//	PlayerIMCData		m_IMC;
	PlayerIMC2Data		m_IMC2;
	
private:
						PlayerData(const PlayerData&);
	PlayerData &		operator=(const PlayerData&);
};





struct MobEquipment
{
					MobEquipment() : m_Position(-1) {}
					
	VirtualID		m_Virtual;
	int				m_Position;
};

typedef std::vector<MobEquipment> MobEquipmentList;


//Aggressiveness - close combat
//Aggressiveness - ranged (
//Aggressiveness - loyalty
//Awareness - Surroundings
//Awareness - Distance
//Movement

class MobData
{
public:
					MobData();
	
	//	Load Time Only
	unsigned int		m_BaseHP;
	unsigned int		m_VaryHP;
	MobEquipmentList	m_StartingEquipment;
	
	//	Misc
	Position		m_DefaultPosition;
	
	//	Combat
	float			m_AttackSpeed;
	int				m_AttackRate;
	int				m_AttackDamage;
	int				m_AttackDamageType;
	int				m_AttackType;
	
	//	AI
	int				m_WaitState;
	unsigned int	m_UpdateTick;

	int				m_AIAggrRoom;
	int				m_AIAggrRanged;
	int				m_AIAggrLoyalty;
	int				m_AIAwareRoom;
	int				m_AIAwareRange;
	int				m_AIMoveRate;
	
	Opinion			m_Hates;
	Opinion			m_Fears;
};


/* Char's abilities. */
struct AbilityData {
	AbilityData() : Strength(0), Health(0), Coordination(0),
					Agility(0), Perception(0), Knowledge(0) { }
					
	unsigned short	Strength,
					Health,
					Coordination,
					Agility,
					Perception,
					Knowledge;
};


struct PointData {
			PointData() : hit(0), max_hit(0), move(0), max_move(100), /*endurance(20), max_endurance(0),*/ mp(0),
					lifemp(0), extraArmor(0), curStealth(0), curNotice(0)
					{ memset(armor, 0, sizeof(armor)); }
			
	int				hit;
	int				max_hit;
	int				move;
	int				max_move;
//	int				endurance;
//	int				max_endurance;

	int				mp, lifemp;

	int				extraArmor;
	unsigned char	armor[NUM_ARMOR_LOCS][NUM_DAMAGE_TYPES];
	
	float			curStealth, curNotice;
};




/* Abstract base class */
class ActionTimerCommand : public TimerCommand
{
public:
	static const char * Type;
	
						ActionTimerCommand(int time, CharData *ch, ActionCommandFunction func, int newstate, void *buf1, void * /*buf2*/);
	virtual				~ActionTimerCommand();
							
	virtual int			Run();							//	Return 0 if ran, time if should run again
	virtual bool		Abort(CharData *ch, const char *str = NULL);//	Return false if failed to abort
	
	void *				GetBuffer() { return m_pBuffer; }
	
	CharData *			m_pCharacter;
	ActionCommandFunction	m_pFunction;
	int					m_NewState;
	void *				m_pBuffer;
};



enum RestrictionType
{
	NotRestricted,
	RestrictedByRace,
	RestrictedByLevel,
	RestrictedByClan
};


enum
{
	NotGloballyVisible = false,
	GloballyVisible = true,
};
	
/* ================== Structure for player/non-player ===================== */
class CharData : public Entity
{
public:
						CharData();
						CharData(const CharData &ch);
						~CharData();
	
	static CharData *	Create(CharacterPrototype *proto);
	static CharData *	Create(CharData *ch);
	
	static CharData *	Find(IDNum id);

	static void			Parse(Lexi::Parser &parser, VirtualID nr);
	void				Write(Lexi::OutputParserFile &file);
	
private:
	CharData &			operator=(const CharData &);

public:
	//	Entity
	virtual	void		Extract();
	virtual Entity::Type	GetEntityType() { return Entity::TypeCharacter; }

//	Accessor Functions
	const char *		GetAlias() const;
	const char *		GetName() const { return m_Name.c_str(); }
	const char *		GetTitle() const { return m_Title.c_str(); }
	const char *		GetRoomDescription() const { return m_RoomDescription.c_str(); }
	const char *		GetDescription() const { return m_Description.c_str(); }
	
	Sex					GetSex() const { return m_Sex; }
	Race				GetRace() const { return m_Race; }
	Race				GetFaction() const;
	int					GetLevel() const { return m_Level; }
	
	int					GetHeight() const { return m_Height; }
	int					GetWeight() const { return m_Weight; }
	
//	Game Functions
	void				ToWorld();
	void				FromWorld();
	
	void				OldSave();
	bool				OldLoad(const char *name);

	void				Save();
	bool				Load(const char *name);
	
//	void				set_name(char *name);
//	void				set_short(char *short_descr = NULL);
	void				SetTitle(const char *title = NULL);
	void				SetLevel(int level);
	
	void				EventCleanup();
	
	void				FromRoom();
	void				ToRoom(RoomData *room);
	
	void				update_pos();
	void				update_objects();
	void				AffectTotal();
	
	void				UpdateWeight();
	
	void				die_follower();
	void				stop_follower();

	void				stop_fighting();
	void				redirect_fighting();
	void				die(CharData * killer);
	
	Relation			GetRelation(CharData *target);
	RoomData *			StartRoom();
	RoomData *			LastRoom();

	void				AddTimer(int time, ActionCommandFunction func, int newstate, void *buf1 = NULL, void *buf2 = NULL);
	void				ExtractTimer();
	bool				AbortTimer(const char *str = NULL) { return m_pTimerCommand ? m_pTimerCommand->ExecuteAbort(this, str) : true; }
	
	void				AddOrReplaceEvent(Event *event);
	
	//	Skill system
	float				GetEffectiveSkill(int skillnum);
	float				ModifiedStealth();
	float				ModifiedNotice(CharData *target);
	
//	Utility Classes, mostly former Macros
	bool				LightOk();
//	bool				LightOk(CharData *target);
	bool				LightOk(RoomData *room);
	Visibility			InvisOk(CharData *target);
	Visibility			CanSee(CharData *target, bool ignoreLight = NotGloballyVisible);
	bool				CanSee(ObjData *target);
//	bool				CanSee(ShipData *target);
	
	void				Appear(Affect::Flags flags = Affect::AFF_STEALTH_FLAGS);

	RestrictionType		CanUse(ObjData *obj);
//	bool				CanUse(ShipData *ship);
	
	int					Send(const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
	Lexi::String		SendAndCopy(const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
	void				AddChatBuf(const char *buf);
	void				AddTellBuf(const char *buf);
	void				AddWizBuf(const char *buf);
	void				AddSayBuf(const char *buf);
	void				AddInfoBuf(const char *buf);
	
	bool				IsIgnoring(IDNum id);
	bool				IsIgnoring(CharData *ch) { return IsIgnoring(ch->GetID()); }
	void				AddIgnore(IDNum id);
	void				RemoveIgnore(IDNum id);
	
	ObjData *			GetWeapon();
	ObjData *			GetSecondaryWeapon();
	ObjData *			GetGun();
	
	
	PlayerData *		GetPlayer() { return m_pPlayer; }
	const PlayerData *	GetPlayer() const { return m_pPlayer; }
	
public:
	CharacterPrototype *GetPrototype() { return m_Prototype; }
	void				SetPrototype(CharacterPrototype *proto) { m_Prototype = proto; }

	VirtualID 			GetVirtualID() { return GetPrototype() ? GetPrototype()->GetVirtualID() : VirtualID(); }
	
	RoomData *			InRoom()						{ return m_InRoom; }
	RoomData *			WasInRoom()						{ return m_WasInRoom; }
	void				SetWasInRoom(RoomData * room)	{ m_WasInRoom = room; }

private:
	
	CharacterPrototype *m_Prototype;
	RoomData *			m_InRoom;			//	Location (real room number)
	RoomData *			m_WasInRoom;		//	location for linkdead people
	RoomData *			m_PreviouslyInRoom;	//	Internal: For debugging purposes

public:
	//	Name and Description
	Lexi::SharedString	m_Name;
	Lexi::SharedString	m_Keywords;
	Lexi::SharedString	m_RoomDescription;
	Lexi::SharedString	m_Description;
	Lexi::String		m_Title;
	
	//	Vital Data
	Sex					m_Sex;
	Race				m_Race;
	int					m_Level;

	int					m_Height;
	int					m_Weight;
	
	//	Attributes
	AbilityData			real_abils;			//	Unmodified Abilities
	AbilityData			aff_abils;			//	Current Abililities
	
	PlayerData *		m_pPlayer;			//	PC specials
	MobData *			mob;				//	NPC specials
	
	//	Connection
	DescriptorData *	desc;				//	NULL for mobiles
	
	//	Affiliations
	Clan *				m_Clan;
	int					m_Team;
	
	//	Items and Equipment
	ObjData *			equipment[NUM_WEARS];			//	Equipment array
	Lexi::List<ObjData *>carrying;			//	List of carried items
	int					m_NumCarriedItems;	//	used instead of carrying.count() because some items should not be counted
	int					m_CarriedWeight;

	//	Statuses
	Lexi::List<Affect *>m_Affects;			//	affects
	Affect::Flags		m_AffectFlags;	
	PointData			points;				//	Points
	
	
	//	Game Runtime Data
	
	//	General Runtime Data
	Position			m_Position;
	Flags				m_ActFlags;	//	MOB_FLAGS and PLR_FLAGS
	
	time_t				m_LastNoticeRollTime;
	int					m_WatchingDirection;
	
	//	Combat Runtime Data
	CharData *			fighting;
	IDNum				m_LastAttacker;
	CombatMode			m_CombatMode;
	
	
	//	Following
	CharacterList		m_Followers;
	CharData *			m_Following;		//	Who is char following?
	
	//	Events
	Event::List			m_Events;
	Event *				points_event[3];
	
	TimerCommand *		m_pTimerCommand;
	int					m_Substate;
	int					m_SubstateNewTime;
	Lexi::String		m_ScriptPrompt;
	
	int					move_wait;
	
	//	Misc
	Path *				path;
	ObjData *			sitting_on;
	
public:
	Lexi::String		GetFilename() const { return GetFilename(GetName()); }
	Lexi::String		GetObjectFilename() const { return GetObjectFilename(GetName()); }
	
	static Lexi::String	GetFilename(const char *name);
	static Lexi::String	GetObjectFilename(const char *name);
};


extern CharacterList		character_list;
extern CharacterList		combat_list;
extern CharacterPrototypeMap mob_index;

typedef std::map<IDNum, Lexi::String> PlayerTable;
extern PlayerTable	player_table;

void finish_load_mob(CharData *mob);

#define GET_POINTS_EVENT(ch, i) ((ch)->points_event[i])




//#define VERSION_ELIMINATE_PLR_FLAG_VERSIONING			1	//	12-29-03
//#define VERSION_PREDATOR_MARINE_SKILL_COST_REDUCTION	2	//	3-23-04
//#define VERSION_ELIMINATE_KNOWLEDGE_SP_BONUS			3	//	12-10-04
//#define VERSION_REDUCE_SKILL_COSTS_4_1_05				4	//	3-31-05
//#define VERSION_ALTER_SKILL_COSTS_8_28_05				5	//	8-28-05
//#define VERSION_DECREASE_PREDATOR_UNARMED_COST_11_15_05	6	//	11-15-05
//#define VERSION_DECREASE_SYNTHETIC_HEALING_1_29_06		7	//	1-29-06
		//	New version format: YYYYMMDD
//#define VERSION_PREDATOR_SKILLPOINT_ADJUSTMENT_2_6_06	20060206	//	1-29-06


//	Switch to Versioning System				1		(12-29-03)
//	Predator Marine skill cost reduction	2		( 3-23-04)
//	Eliminate Knowledge SP Bonus			3		(12-10-04)
//	Reduce skill costs						4		( 3-31-05)
//	Alter skill costs						5		( 8-28-05)
//	Decrease pred unarmed cost				6		(11-15-05)
//	Decrease synthetic healing cost			7		( 1-29-06)
//	Predator skillpoint adj					8		( 2- 6-06)
//	Predator skillpoint adj #2				9		( 2-13-06)
//	Predator skillpoint adj #3				10		( 2-26-06)
//	Unnecessary skills removed				11		( 3- 1-06)
//	Fix issue with players having trained skills that were removed	12
//		Todo: new versioning system?  YYYYMMDD
//	Alien skillpoint adj					13		( 9-15-07)

#define VERSION_LAST_SKILLPOINT_CHANGE		13
#define CURRENT_VERSION						13


#endif	// __CHARACTERS_H__
