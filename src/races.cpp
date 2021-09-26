

#include "races.h"

#include "structs.h"
#include "buffer.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "constants.h"

#include "lexistring.h"
#include "lexiparser.h"

/* prototypes */
long find_race_bitvector(char arg);
void roll_real_abils(CharData * ch);
void do_start(CharData * ch);
void advance_level(CharData * ch);
void CheckRegenRates(CharData *ch);
char *skill_name(int skill);
extern int max_hit_base[NUM_RACES];


class Faction
{
public:
						Faction();
	
	int					GetIndex() const { return m_Index; }					
	const char *		GetName() const { return m_Name.c_str(); }
	
	//	Faction Relations
	enum
	{
		Friendly		= 0x00,
		Neutral			= 0x01,
		Hostile			= 0x02,
		
		HostilityMask	= 0x03
	};

	static Flags		GetRelations(const Faction *f1, const Faction *f2);
	static inline bool	IsHostile(const Faction *f1, const Faction *f2) { return (GetRelations(f1, f2) & HostilityMask) == Hostile; }
	
	
private:
	int					m_Index;
	Lexi::String		m_Name;
	
	typedef std::list<Faction *>	FactionList;
	static FactionList	ms_Factions;
//	static int			ms_NumFactions;
	static Flags *		ms_Relations;
	
public:
	static void			GenerateRelationsTable();
	static const Faction *Find(const char *name);
};


const Faction *Faction::Find(const char *name)
{
	FOREACH(FactionList, ms_Factions, iter)
		if ((*iter)->m_Name == name)
			return *iter;
	
	return NULL;
}

//Flags *Faction::ms_Relations = NULL;
std::list<Faction *>	Faction::ms_Factions;
//int			Faction::ms_NumFactions;

#if 0
Flags Faction::ms_Relations[] =
{
		/*M*/		/*Y*/		/*A*/
/*M*/	Friendly,
/*Y*/	Hostile,	Friendly,
/*A*/	Hostile,	Hostile,	Friendly
};
#endif
Flags *	Faction::ms_Relations = NULL;

void Faction::GenerateRelationsTable()
{
	int			numRelationFlags = (ms_Factions.size() * (ms_Factions.size() + 1)) / 2;
	
	delete ms_Relations;
	ms_Relations = new Flags[numRelationFlags];
	memset(ms_Relations, 0, sizeof(Flags) * numRelationFlags);
	
	for (int i = 0; i < ms_Factions.size(); ++i)
	{
		int	base = ((i * (i + 1)) / 2);
		
		ms_Relations[base + i] = Friendly;
		
		for (int n = 0; n < i; ++n)
		{
			int index = base + n;
			
			//	Figure out relations here.
			Flags	relation = Hostile;
			
			ms_Relations[index] = relation;
		}
	}
}
		

/*
	0  1  2  3  4  5
0	0
1	1  2
2	3  4  5
3	6  7  8  9
4	10 11 12 13 14
5	15 16 17 18 19 20

0 * 1 = 0 / 2 = 0
1 * 2 = 2 / 2 = 1
2 * 3 = 6 / 2 = 3
3 * 4 = 12 / 2 = 6
4 * 5 = 20 / 2 = 10
5 * 6 = 30 / 2 = 15
*/


Flags Faction::GetRelations(const Faction *f1, const Faction *f2)
{
	/* ms_Relations is numFactions * (numFactions - 1) */
	
	/* 0 = 0,0
	   1 = 1,0	2 = 1,1,
	   3 = 2,0	4 = 2,1  5 = 2,2
	   6 = 3,0  7 = 3,1  8 = 3,2  9 = 3,3
	 */
	
	int low = f1->GetIndex();
	int high = f2->GetIndex();
	
	if (low > high)
		std::swap(low, high);
	
	int index = ((high * (high + 1)) / 2) + low;
	
	if (index < 0 || index >= ((ms_Factions.size() * (ms_Factions.size() + 1)) / 2))
		return 0;	//	Bad...
	
	return ms_Relations[index];
}




class RaceData
{
public:
	const char *		GetAbbrev() const { return m_Abbrev.c_str(); }
	const char *		GetName() const { return m_Name.c_str(); }
	const char *		GetProperName() const { return m_ProperName.c_str(); }
	char				GetLetter() const { return m_Letter; }
	
	const Faction *		GetDefaultFaction() const { return m_DefaultFaction; }
	
	enum
	{
		ClanOnlyGrouping	= 1 << 0,
	};
	
	Flags				GetAttributes() const { return m_Attributes; }
	Flags				HasAttributes(Flags attributes) const { return m_Attributes & attributes; }	
	
	VirtualID			GetStartRoom() const { return m_StartRoom; }
	
private:
	Lexi::String		m_Abbrev;
	Lexi::String		m_Name;
	Lexi::String		m_ProperName;
	char				m_Letter;
	
	Flags				m_Attributes;
	
	std::vector<Lexi::String>	m_Titles[2];	//	0 = male, 1 = female
	
	const Faction *		m_DefaultFaction;
	VirtualID			m_StartRoom;
	
	typedef std::vector<RaceData *>	RaceVector;
	static RaceVector	ms_Races;
//	static int			ms_NumRaces;

public:
	static int			GetNumRaces() { return ms_Races.size(); }
	static const RaceData *	GetRace(int n) { return ms_Races[n]; }
	
	static const RaceData *FindByName(const char *name);
	static const RaceData *FindByAbbrev(const char *name);
	static const RaceData *FindByLetter(const char *name);
};


std::vector<RaceData *>	RaceData::ms_Races;
//int 	Race::ms_NumRaces = 0;



const RaceData *RaceData::FindByName(const char *name)
{
	for (RaceVector::iterator iter = ms_Races.begin(), end = ms_Races.end(); iter != end; ++iter)
		if ((*iter)->m_Name == name)
			return *iter;
	
	return NULL;
}


char *race_abbrevs[] = { 
  "Hum",
  "Syn",
  "Prd",
  "Aln",
  "Oth",
  "\n"
};


char *race_types[] = {
  "human",
  "synthetic",
  "predator",
  "alien",
  "other",
  "\n"
};


char *faction_types[] = {
  "Marines",
  "Marines",
  "Predators",
  "Aliens",
  "Other",
  "\n"
};


/* The menu for choosing a race in interpreter.c: */
char *race_menu =
"\n"
"Select a Species:\n"
"  (H)uman             - The natural soldier\n"
"  (S)ynthetic Human   - The artificial soldier\n"
"  (A)lien             - The natural killer\n"
"  (P)redator          - The natural hunter\n"
"\n"
"  The US Colonial Marine Corps is based out of Camp Pendleton, on Earth.\n"
"They are equipped with ranged weaponry, armor, and vehicles.  Mobility\n"
"and heavy fire power is key to a fast response.  Poor at close combat.\n"
"\n"
"  The Aliens consist of a series of hives and swarms spread haphazardly\n"
"throughout the galaxy.  They are an evolutionary nightmare, highly efficient\n"
"killers whose sole purpose is to spread and consume.  Close combat only.\n"
"\n"
"  The Predators - Yautja - are a nomadic race.  The Hunter caste's sole\n"
"purpose is to hunt down dangerous life forms and face them in Honorable\n"
"combat.  Stealth suits, limited ranged weaponry, limited armor, limited\n"
"transportation.\n"
//  WARNING: PREDATORS REQUIRE UNDERSTANDING OF HOW THE GAME\n"
//"AND OTHER RACES WORK TO BE PLAYBLE.  PREDATORS ARE THE WEAKEST STARTING RACE.\n"
//"IF YOU ARE NEW TO THE GAME, PICK ANOTHER RACE.\n"
"\n";

/*
 * The code to interpret a race letter (used in interpreter.c when a
 * new character is selecting a race).
 */
Race ParseRace(const char *arg)
{
	return ParseRace(*arg);
}


Race ParseRace(char arg) {
	switch (tolower(arg)) {
		case 'h':	return RACE_HUMAN;
		case 's':	return RACE_SYNTHETIC;
		case 'p':	return RACE_PREDATOR;
		case 'a':	return RACE_ALIEN;
		default:	return RACE_UNDEFINED;
	}
}



RaceFlags GetRaceFlag(char arg)
{
	RaceFlags flags;
	Race race = ParseRace(arg);
	if (race != RACE_UNDEFINED)
		flags.set(race);
	return flags;
}


RaceFlags GetRaceFlag(const char *arg)
{
	RaceFlags flags;
	Race race = ParseRace(arg);
	if (race != RACE_UNDEFINED)
		flags.set(race);
	return flags;
}


Lexi::String GetDisplayRaceRestrictionFlags(RaceFlags flags)
{
	return flags.print(restriction_bits);
}


int stat_base[NUM_RACES][7] =
{		/*	STR		HEA		COO		AGI		PER		KNO */
/* HUM */ {	70,		70,		70,		70,		70,		70, 470 },
/* SYN */ {	70,		70,		70,		70,		70,		70, 470 },
/* PRD */ {	70,		70,		70,		70,		70,		70, 470 },
/* ALN */ {	70,		70,		70,		70,		70,		70, 470 },
/* ANM */ {  1,		1,		1,		1,		1,		1, 10 }
};


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(CharData * ch) {
	const int r = ch->GetRace();
	
	ch->real_abils.Strength		= stat_base[r][0];
	ch->real_abils.Health		= stat_base[r][1];
	ch->real_abils.Coordination	= stat_base[r][2];
	ch->real_abils.Agility		= stat_base[r][3];
	ch->real_abils.Perception	= stat_base[r][4];
	ch->real_abils.Knowledge	= stat_base[r][5];
	
	ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
void do_start(CharData * ch) {
	ch->m_Level = 1;

	ch->SetTitle(NULL);

	ch->points.max_hit = max_hit_base[ch->GetRace()] + (ch->real_abils.Health * max_hit_bonus[ch->GetRace()]);
	//ch->points.max_endurance = 20;

	advance_level(ch);
	
	GET_HIT(ch) = GET_MAX_HIT(ch);
	GET_MOVE(ch) = GET_MAX_MOVE(ch);
	//GET_ENDURANCE(ch) = GET_MAX_ENDURANCE(ch);

//	GET_COND(ch, THIRST) = -1;
//	GET_COND(ch, FULL) = -1;
//	GET_COND(ch, DRUNK) = -1;

	ch->GetPlayer()->m_TotalTimePlayed = 0;
	ch->GetPlayer()->m_LoginTime = ch->GetPlayer()->m_LastLogin = time(0);
	
	SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);
	SET_BIT(PRF_FLAGS(ch), PRF_AUTOEXIT | PRF_COLOR);
	
	ch->GetPlayer()->m_Version = CURRENT_VERSION;
}



/*
 * This function controls the change to maxmove, and maxhp for
 * each race every time they gain a level.
 */
void advance_level(CharData * ch) {
	int add_hp = 0, add_move = 0 /*, i*/;

	if (IS_STAFF(ch))
	{
//		for (i = 0; i < 3; i++)
//			GET_COND(ch, i) = (char) -1;
		SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
	}
	
	CheckRegenRates(ch);
	
	ch->GetPlayer()->m_SkillPoints += 12;
	ch->GetPlayer()->m_LifetimeSkillPoints += 12;
	
	ch->Save();
	
//	mudlogf(CMP, LVL_STAFF, FALSE,  "%s advanced to level %d", ch->GetName(), ch->GetLevel());
}


//	5 skills to (10) = 255
//	max 2 stats (15-20) = 180
//	total = 435... * 1.5 = 650... / 100 = 6.5... 6 stats...  Effective Knowledge / 2?

/* NEW SYSTEM */
/* Names of class/levels and exp required for each level */

struct title_type titles[NUM_RACES][16] = {
		/* Humans */
	{	{"Civilian", "Civilian"},						/* 0 */
		{"Private",		"Private"},						/* 1-10 */
		{"Corporal",	"Corporal"},					/* 11-20 */
		{"Sergeant",	"Sergeant"},					/* 21-30 */
		{"Sergeant Major", "Sergeant Major"},			/* 31-40 */
		{"Warrant Officer", "Warrant Officer"},			/* 41-50 */
		{"Lieutenant",	"Lieutenant"},					/* 51-60 */
		{"Captain",		"Captain"},						/* 61-70 */
		{"Major",		"Major"},						/* 71-80 */
		{"Colonel",		"Colonel"},						/* 81-90 */
		{"General",		"General"},						/* 91-100 */
		{"Veteran",		"Veteran"},						/* 101 */
		{"the Dishonorably Discharged", "the Dishonorably Discharged"},
		{"the Court Martialed", "the Court Martialed"},
		{"is M.I.A.", "is M.I.A."},
		{"the Implementor", "Implementor"}				/* 15 */
	},
		/* Synthetics */
	{	{"Civilian",	"Civilian"},					/* 0 */
		{"Private",		"Private"},						/* 1-10 */
		{"Corporal",	"Corporal"},					/* 11-20 */
		{"Sergeant",	"Sergeant"},					/* 21-30 */
		{"Sergeant Major", "Sergeant Major"},			/* 31-40 */
		{"Warrant Officer", "Warrant Officer"},			/* 41-50 */
		{"Lieutenant",	"Lieutenant"},					/* 51-60 */
		{"Captain",		"Captain"},						/* 61-70 */
		{"Major",		"Major"},						/* 71-80 */
		{"Colonel",		"Colonel"},						/* 81-90 */
		{"General",		"General"},						/* 91-100 */
		{"Veteran",		"Veteran"},						/* 101 */
		{"the Dishonorably Discharged", "the Dishonorably Discharged"},
		{"the Court Marshalled", "the Court Marshalled"},
		{"is M.I.A.", "is M.I.A."},
		{"the Implementor", "Implementor"}					/* 15 */
	},

		/* PREDATORS */
	{	{"Suckling", 	"Suckling"},					/* 0 */
		{"Unblooded",	"Unblooded"},					/* 1-10 */
		{"Young Blood",	"Young Blood"},					/* 11-20 */
		{"Blooded",		"Blooded"},						/* 21-30 */
		{"Warrior",		"Warrior"},						/* 31-40 */
		{"Hunter",		"Hunter"},						/* 41-50 */
		{"Skulltaker",	"Skulltaker"},					/* 51-60 */
		{"Champion",	"Champion"},					/* 61-70 */
		{"Elder",	 	"Elder"},						/* 71-80 */
		{"Pack Leader", "Pack Leader"},					/* 81-90 */
		{"Ancient", "Ancient"},							/* 91-100 */
		{"Veteran", "Veteran"},							/* 101 */
		{"the Outcast", "the Outcast"},
		{"the Chickenshit", "the Chickenshit"},
		{"the Fang-Face", "the Fang-Face"},
		{"the Implementor", "the Implementress"}			/* 15 */
	},

		/* ALIENS */
	{	{"Egg",			"Egg"},								/* 0 */
		{"Chestburster","Chestburster"},					/* 1-10 */
		{"Drone",		"Drone"},							/* 11-20 */
		{"Hive Builder","Hive Builder"},					/* 21-30 */
		{"Scout",		"Scout"},							/* 31-40 */
		{"Sentry",		"Sentry"},							/* 41-50 */
		{"Hunter",		"Hunter"},							/* 51-60 */
		{"Warrior",		"Warrior"},							/* 61-70 */
		{"Stalker",		"Stalker"},							/* 71-80 */
		{"Royal Guard", "Royal Guard"},						/* 81-90 */
		{"Queen",		"Queen"},							/* 91-100 */
		{"Empress", "Empress"},								/* 101 */
		{"the Squashed Bug", "the Squashed Bug"},
		{"the Trophy", "the Trophy"},
		{"the Rogue", "the Bitch"},
		{"the Implementor", "the Implementress"}			/* 15 */
	}
};



							//	  M  S  P  A  O
#define	BASE_COST				{ 1, 1, 1, 1, 1 }
#define	COMBAT_COST				{ 1, 1, 1, 2, 2 }
#define	SPECIAL_COMBAT_COST		
#define NO_PREREQS				0

#define UNAVAILABLE_SKILL		{ 101, 101, 101, 101, 101 }, BASE_COST

#define	RACES_M					{ 1, 1, 101, 101, 1 }
#define	RACES_P					{ 101, 101, 1, 101, 1 }
#define	RACES_A					{ 101, 101, 101, 1, 1 }
#define	RACES_MP				{ 1, 1, 1, 101, 1 }
#define	RACES_MA				{ 1, 1, 101, 1, 1 }
#define	RACES_PA				{ 101, 101, 1, 1, 1 }
#define	RACES_MPA				{ 1, 1, 1, 1, 1 }

struct skill_info_type skill_info[NUM_SKILLS] =
{
	{	"" },	//	0
	{	"Unarmed Combat",		"Using your body as a weapon",
		STAT_COO,	RACES_MPA,	BASE_COST	},			//	1
	{	"Melee Weapons",		"Hand to hand combat weapons",
		STAT_COO,	RACES_MPA,	COMBAT_COST	},			//	2
	{	"Small Arms",			"Small guns such as pistols",
		STAT_COO,	RACES_M,	COMBAT_COST	},	//	3
	{	"Rifles",				"General two-handed guns",
		STAT_COO,	RACES_MP,	COMBAT_COST	},	//	4
	{	"Special Weapons",		"Weaponry requiring special training",
		STAT_COO,	RACES_MP,	COMBAT_COST	},			//	5
	{	"Heavy Weapons",		"Heavy duty weapons",
		STAT_COO,	RACES_MPA,	COMBAT_COST	},	//	6
	{	"Throwing",				"Thrown weapons",
		STAT_COO,	RACES_MP,	BASE_COST	},			//	7
	{	"!Shields", "",			STAT_COO,	UNAVAILABLE_SKILL	},		//	8
	{	"Dodge",				"Use of agility to avoid being hit",
		STAT_AGI,	RACES_MPA,	{ 3, 3, 3, 2, 3 }	},			//	9
	{	"Natural Abilities",	"Natural abilities of your race",
		STAT_STR,	RACES_A,	BASE_COST },		//	10
	{	"Healing",				"Hit Point recovery",
		STAT_HEA,	RACES_MPA,	{ 3, 1, 3, 3, 3 } },		//	11
	{	"!Running", "",			STAT_AGI,	UNAVAILABLE_SKILL	},		//	12
	{	"Stealth",				"Avoiding detection while moving and hiding",
		STAT_PER,	RACES_MPA,	{ 3, 3, 2, 2, 3 } },			//	13
	{	"!Analysis", "",		STAT_PER,	UNAVAILABLE_SKILL	},		//	14
	{	"!Antipicate Attack", "",STAT_PER,	UNAVAILABLE_SKILL	},		//	15
	{	"!Hiding", "Use of surroundings to hide", STAT_PER,	UNAVAILABLE_SKILL },		//	16
	{	"!Listen", "",			STAT_PER,	UNAVAILABLE_SKILL	},		//	17
	{	"Notice",				"Situational awareness",
		STAT_PER,	RACES_MPA,	{ 3, 3, 2, 2, 3 }	},			//	18
	{	"!Research", "",		STAT_PER,	UNAVAILABLE_SKILL	},		//	19
	{	"!Search", "",			STAT_PER,	UNAVAILABLE_SKILL	},		//	20
	{	"!Surveillance", "",	STAT_PER,	UNAVAILABLE_SKILL	},		//	21
	{	"!Tracking", "",		STAT_PER,	UNAVAILABLE_SKILL	},		//	22
	{	"Traps",				"Creation and use of traps",
		STAT_KNO,	RACES_PA,	BASE_COST	},			//	23
	{	"Demolitions",			"Creation and use of explosives",
		STAT_KNO,	RACES_M,	BASE_COST	},			//	24
	{	"Engineering",			"Set up and maintenance of devices",
		STAT_KNO,	RACES_MP,	BASE_COST	},			//	25
	{	"!Flee", "Escaping from combat situations", STAT_AGI, UNAVAILABLE_SKILL	},		//	26

	{	"Medical",				"Ability to provide medical treatment",
		STAT_KNO,	RACES_MP,	BASE_COST	},		//	26

//	{	"!First Aid",			STAT_KNO,	UNAVAILABLE_SKILL	},		//	26
//	{	"!Security",			STAT_KNO,	UNAVAILABLE_SKILL	},		//	27
};



char *skill_name(int skill)
{
	if (skill <= 0 || skill >= NUM_SKILLS)	return "<INVALID>";
	return skill_info[skill].name;
}


int find_skill(const char *name)
{
	if (!*name)
		return 0;

	for (int index = 1; index < NUM_SKILLS; ++index)
	{
		if (*skill_info[index].name == '!')
			continue;
		
		if (!str_cmp(name, skill_info[index].name))
			return index;
	}

	return 0;
}


int find_skill_abbrev(const char *name)
{
	int index = 0, ok;
	char *temp, *temp2;
	
	if (!*name)
		return 0;
	
	BUFFER(first, MAX_INPUT_LENGTH);
	BUFFER(first2, MAX_INPUT_LENGTH);

	for (index = 1; index < NUM_SKILLS; ++index)
	{
		if (*skill_info[index].name == '!')
			continue;
		
		if (is_abbrev(name, skill_info[index].name)) {
			break;
		}

		ok = 1;
		temp = any_one_arg(skill_info[index].name, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!is_abbrev(first2, first))
				ok = 0;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
			break;
	}

	return index < NUM_SKILLS ? index : 0;
}
