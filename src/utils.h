/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* external declarations and prototypes **********************************/
#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"
#include "lexistring.h"

#if defined(WIN32)
#include <windows.h>
#endif

#undef log

#define mudlog(a,b,c,d)	mudlogf(b,c,d,a)

/* public functions in utils.c */
char *		str_dup(const char *source);

#define		str_cmp(a, b)	strcasecmp(a, b)
#define		strn_cmp(a, b, c) strncasecmp(a, b, c)
const char *str_str(const char *cs, const char *ct);
inline char *str_str(char *cs, const char *ct) { return const_cast<char *>(str_str(static_cast<const char *>(cs), ct)); }
int			sprintf_cat(char *str, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
int			snprintf_cat(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
int			str_cmp_numeric(const char *arg1, const char *arg2);
int str_prefix(const char *astr, const char *bstr);

#if defined(_WIN32)
char *		strsep(char **sp, const char *sep);
#endif
int			cstr_cmp(const char *a, const char *b);
size_t		cstrlen(const char *str);
size_t		cstrnlen(const char *str, size_t n);
size_t		cstrnlen_for_sprintf(const char *str, size_t n);
Lexi::String	strip_color (const char *txt);
int		touch(char *path);
int		Number(int from, int to);
int		dice(int number, int size);
float	BellCurve();
float	LinearCurve();
float	SkillRoll(float stat);
float	LinearSkillRoll(float stat);
void	sprintbit(Flags vektor, char *names[], char *result);
void	sprinttype(int type, char **names, char *result);
const char *findtype(int type, char **names);
int		get_line(FILE *fl, char *buf);
int		get_any_line(FILE * fl, char *buf, int length);
struct TimeInfoData age(CharData *ch);
void	core_dump_func(const char *who, int line);
#define core_dump()	core_dump_func(__PRETTY_FUNCTION__, __LINE__)

/* random functions in random.c */
void circle_srandom(unsigned int initial_seed);
unsigned int circle_random(void);


/* in log.c */
/*
 * The attribute specifies that mudlogf() is formatted just like printf() is.
 * 4,5 means the format string is the fourth parameter and start checking
 * the fifth parameter for the types in the format string.  Not certain
 * if this is a GNU extension so if you want to try it, put #if 1 below.
 */
void	log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

void    mudlogf(int type, int level, bool file, const char *format, ...) __attribute__ ((format (printf, 4, 5)));


/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#ifdef RANGE
#undef RANGE
#endif


inline int MIN(int a, int b) {	return a < b ? a : b; }
inline int MAX(int a, int b) { return a > b ? a : b; }
inline int RANGE(int var, int low, int high) { return MAX(low, MIN(high, var)); }
inline unsigned int MIN(unsigned int a, unsigned int b) {	return a < b ? a : b; }
inline unsigned int MAX(unsigned int a, unsigned int b) { return a > b ? a : b; }
inline unsigned int RANGE(unsigned int var, unsigned int low, unsigned int high) { return MAX(low, MIN(high, var)); }
inline float MIN(float a, float b) { return a < b ? a : b; }
inline float MAX(float a, float b) { return a > b ? a : b; }
inline float RANGE(float var, float low, float high) { return MAX(low, MIN(high, var)); }

/* in magic.c */
int	circle_follow(CharData *ch, CharData * victim);

/* in act.informative.c */
void	look_at_room(CharData *ch, bool ignore_brief = false);
void	look_at_room(CharData *ch, RoomData * room, bool ignore_brief = false);

/* in act.movmement.c */

enum
{
	MOVE_FOLLOWING		= 1 << 0,
	MOVE_QUIET			= 1 << 1,
	MOVE_FLEEING		= 1 << 2
};

int	do_simple_move(CharData *ch, int nDir, Flags flags);
int	perform_move(CharData *ch, int nDir, Flags flags);

/* in limits.c */
int	hit_gain(CharData *ch);
int	move_gain(CharData *ch);
int endurance_gain(CharData *ch);
void	advance_level(CharData *ch);
void	gain_condition(CharData *ch, int condition, int value);
void	check_idling(CharData *ch);
void	point_update(void);


/* various constants *****************************************************/


/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* mud-life time */
//#define SECS_PER_MUD_HOUR	75
#define SECS_PER_MUD_HOUR	60
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(30*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(12*SECS_PER_MUD_MONTH)

#define PULSES_PER_MUD_HOUR     (SECS_PER_MUD_HOUR*PASSES_PER_SEC)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
extern char *CAP(char *in);
extern const char *AN(const char *in);

//#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

#define END_OF(buffer)		((buffer) + strlen((buffer)))

/* memory utils **********************************************************/


#if !defined(__STRING)
#define __STRING(x)	#x
#endif

#define CREATE(result, type, number)  do {\
	if ((number * sizeof(type)) <= 0) {\
	mudlogf(BRF, 102, TRUE, "CODEERR: Attempt to alloc 0 at %s:%d", __PRETTY_FUNCTION__, __LINE__);\
	}\
	if (!((result) = (type *) ALLOC ((number ? number : 1), sizeof(type))))\
		{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
	if ((result) == NULL && (number) == 0)\
		(result) = NULL;\
	else if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } } while(0)
/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) ^= (bit))

#define MOB_FLAGS(ch)	((ch)->m_ActFlags)
#define PLR_FLAGS(ch)	((ch)->m_ActFlags)
#define PRF_FLAGS(ch)	((ch)->GetPlayer()->m_Preferences)
#define AFF_FLAGS(ch)	((ch)->m_AffectFlags)
#define CHN_FLAGS(ch)	((ch)->GetPlayer()->m_Channels)
#define STF_FLAGS(ch)	((ch)->GetPlayer()->m_StaffPrivileges)

#define IS_NPC(ch)		(IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)		(IS_NPC(ch) && (ch)->GetPrototype())

#define MOB_FLAGGED(ch, flag)		(IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag)		AFF_FLAGS(ch).test(flag)
#define PRF_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(PRF_FLAGS(ch), (flag)))
#define CHN_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(CHN_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag)		(loc)->GetFlags().test(flag)
#define EXIT_FLAGGED(exit, flag)		((exit)->GetFlags().test(flag))
#define OBJWEAR_FLAGGED(obj, flag)	(IS_SET((obj)->wear, (flag)))
#define OBJ_FLAGGED(obj, flag)		(IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define STF_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(STF_FLAGS(ch), (flag)))

#define PLR_TOG_CHK(ch,flag)	((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag)	((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))


/* room utils ************************************************************/

#define GET_ROOM_SPEC(room) (room ? room->func : NULL)


/* char utils ************************************************************/

#define IN_ROOM(e)			((e)->InRoom())
#define GET_AGE(ch)			(age(ch).year)

/*
 * I wonder i
f this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
	(ch->desc && ch->desc->m_OrigCharacter ? ch->desc->m_OrigCharacter->GetLevel() : ch->GetLevel())

#define GET_STR(ch)			((ch)->aff_abils.Strength)
#define GET_HEA(ch)			((ch)->aff_abils.Health)
#define GET_COO(ch)			((ch)->aff_abils.Coordination)
#define GET_AGI(ch)			((ch)->aff_abils.Agility)
#define GET_PER(ch)			((ch)->aff_abils.Perception)
#define GET_KNO(ch)			((ch)->aff_abils.Knowledge)

#define GET_REAL_STR(ch)			((ch)->real_abils.Strength)
#define GET_REAL_HEA(ch)			((ch)->real_abils.Health)
#define GET_REAL_COO(ch)			((ch)->real_abils.Coordination)
#define GET_REAL_AGI(ch)			((ch)->real_abils.Agility)
#define GET_REAL_PER(ch)			((ch)->real_abils.Perception)
#define GET_REAL_KNO(ch)			((ch)->real_abils.Knowledge)

#define MAX_PC_STAT	210
#define MAX_STAT	630
#define MAX_EFFECTIVE_STAT	35

//extern int EffectiveStat[MAX_STAT + 1];
//extern float EffectiveStatF[MAX_STAT + 1];
//extern int RealStat[MAX_EFFECTIVE_STAT + 1];

//#define GET_EFFECTIVE(theStat)			(EffectiveStatF[(theStat)])
extern float GetInverseTriangularNumber(int stat);
#define GET_EFFECTIVE(theStat)			GetInverseTriangularNumber(theStat)
//#define GET_REAL_STAT(theStat)			(RealStat[(theStat)])

#define GET_EFFECTIVE_STR(ch)			(GET_EFFECTIVE(GET_STR(ch)))
#define GET_EFFECTIVE_HEA(ch)			(GET_EFFECTIVE(GET_HEA(ch)))
#define GET_EFFECTIVE_COO(ch)			(GET_EFFECTIVE(GET_COO(ch)))
#define GET_EFFECTIVE_AGI(ch)			(GET_EFFECTIVE(GET_AGI(ch)))
#define GET_EFFECTIVE_PER(ch)			(GET_EFFECTIVE(GET_PER(ch)))
#define GET_EFFECTIVE_KNO(ch)			(GET_EFFECTIVE(GET_KNO(ch)))


/*
#define GET_MAX_SKILL_ALLOWED(ch) \
	(MIN(GET_REAL_STAT(((ch->GetLevel() + 9) / 10) + 5), MAX_SKILL_LEVEL))
*/
#define GET_MAX_SKILL_ALLOWED(ch) \
	(ch->GetLevel() < 30 ? 55 : (ch->GetLevel() < 60 ? 91 : 120))

//1-30 = 55
//31-60 = 91
//61+ = 120

#define GET_ROLL_STEALTH(ch)		((ch)->points.curStealth)
#define GET_ROLL_NOTICE(ch)			((ch)->points.curNotice)
#define GET_ROLL_NOTICE_TIME(ch)	((ch)->m_LastNoticeRollTime)

#define GET_EXTRA_ARMOR(ch)			((ch)->points.extraArmor)
#define GET_ARMOR(ch, loc, type)	((ch)->points.armor[loc][type])
#define GET_HIT(ch)			((ch)->points.hit)
#define GET_MAX_HIT(ch)		((ch)->points.max_hit)
#define GET_MOVE(ch)		((ch)->points.move)
#define GET_MAX_MOVE(ch)		((ch)->points.max_move)
#define GET_ENDURANCE(ch)	((ch)->points.endurance)
#define GET_MAX_ENDURANCE(ch)	((ch)->points.max_endurance)
#define GET_MISSION_POINTS(ch)	((ch)->points.mp)
#define GET_LIFEMP(ch)		((ch)->points.lifemp)
#define GET_HITROLL(ch)		((ch)->points.hitroll)
#define GET_DAMROLL(ch)		((ch)->points.damroll)

#define GET_POS(ch)			((ch)->m_Position)
#define IS_CARRYING_W(ch)	((ch)->m_CarriedWeight)
#define IS_CARRYING_N(ch)	((ch)->m_NumCarriedItems)

#define FIGHTING(ch)		((ch)->fighting)
#define GET_LAST_ATTACKER(ch)	((ch)->m_LastAttacker)

#define GET_COND(ch, i)		((ch)->general.conditions[(i)])

#define GET_REAL_SKILL(ch, i)	((ch)->GetPlayer()->m_RealSkills[i])
#define GET_SKILL(ch, i)		((ch)->GetPlayer()->m_Skills[i])

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_WAIT(ch)	((ch)->mob->m_WaitState)

#define GET_MOB_BASE_HP(ch)	((ch)->mob->m_BaseHP)
#define GET_MOB_VARY_HP(ch)	((ch)->mob->m_VaryHP)

#define GET_CLAN(ch)		((ch)->m_Clan)
#define GET_TEAM(ch)		((ch)->m_Team)

#define GET_COMBAT_MODE(ch)	((ch)->m_CombatMode)

#define GET_DEFAULT_POS(ch)	((ch)->mob->m_DefaultPosition)

//#define CAN_CARRY_W(ch)		(GET_EFFECTIVE_STR(ch) * 8) /* 1000*/	/*(GET_STR(ch) * 1.5)*/
#define CAN_CARRY_W(ch)		GET_STR(ch)
#define CAN_CARRY_N(ch)		25		/*(5 + (GET_AGI(ch) / 10))*/
#define AWAKE(ch)			(GET_POS(ch) > POS_SLEEPING)

#define CHAR_WATCHING(ch)	((ch)->m_WatchingDirection)


#define NO_STAFF_HASSLE(ch)	(/*IS_STAFF(ch) && */PRF_FLAGGED(ch, PRF_NOHASSLE))

/* descriptor-based utils ************************************************/


#define WAIT_STATE(ch, cycle) { \
	if ((ch)->desc) (ch)->desc->wait = (cycle); \
	else if (IS_NPC(ch)) GET_MOB_WAIT(ch) = (cycle); }

#define CHECK_WAIT(ch)	(((ch)->desc) ? ((ch)->desc->wait > 1) : 0)

#define MOVE_WAIT_STATE(ch, cycle) { \
	if ((ch)->desc) (ch)->move_wait = (cycle); \
	else if (IS_NPC(ch)) GET_MOB_WAIT(ch) = (cycle); }
#define CHECK_MOVE_WAIT(ch)	(((ch)->move_wait) ? ((ch)->move_wait > 1) : 0)

/* object utils **********************************************************/


#define GET_OBJ_TYPE(obj)	((obj)->GetType())
#define GET_OBJ_COST(obj)	((obj)->cost)
#define GET_OBJ_EXTRA(obj)	((obj)->extra)
#define GET_OBJ_WEAR(obj)	((obj)->wear)
#define GET_OBJ_TOTAL_WEIGHT(obj)	((obj)->totalWeight)
#define GET_OBJ_WEIGHT(obj)	((obj)->weight)
#define GET_OBJ_TIMER(obj)	((obj)->m_Timer)


#define IS_GUN(obj)					((obj)->m_pGun)
#define GET_GUN_INFO(obj)			((obj)->m_pGun)
#define GET_GUN_DAMAGE(obj)			(GET_GUN_INFO(obj)->damage)
#define GET_GUN_RATE(obj)			(GET_GUN_INFO(obj)->rate)
#define GET_GUN_RANGE(obj)			(GET_GUN_INFO(obj)->range)
#define GET_GUN_OPTIMALRANGE(obj)	(GET_GUN_INFO(obj)->optimalrange)
#define GET_GUN_MODIFIER(obj)		(GET_GUN_INFO(obj)->modifier)
#define GET_GUN_AMMO(obj)			(GET_GUN_INFO(obj)->ammo.m_Amount)
#define GET_GUN_AMMO_TYPE(obj)		(GET_GUN_INFO(obj)->ammo.m_Type)
#define GET_GUN_AMMO_VID(obj)		(GET_GUN_INFO(obj)->ammo.m_Virtual)
#define GET_GUN_AMMO_FLAGS(obj)		(GET_GUN_INFO(obj)->ammo.m_Flags)
#define GET_GUN_ATTACK_TYPE(obj)	(GET_GUN_INFO(obj)->attack)
#define GET_GUN_SKILL(obj)			(GET_GUN_INFO(obj)->skill)
#define GET_GUN_SKILL2(obj)			(GET_GUN_INFO(obj)->skill2)
#define GET_GUN_DAMAGE_TYPE(obj)	(GET_GUN_INFO(obj)->damagetype)

#define GET_OBJ_SPEC(obj) ((obj)->GetPrototype() ? (obj)->GetPrototype()->m_Function : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->wear, (part)))

#define IS_SITTABLE(obj)	((GET_OBJ_TYPE(obj) == ITEM_CHAIR) || (GET_OBJ_TYPE(obj) == ITEM_BED))
#define IS_SLEEPABLE(obj)	((GET_OBJ_TYPE(obj) == ITEM_BED))

#define IS_MOUNTABLE(obj)	((GET_OBJ_TYPE(obj) == ITEM_VEHICLE) && !(obj)->GetVIDValue(OBJVIDVAL_VEHICLE_ENTRY).IsValid())

#define NO_LOSE(obj, ch)	(OBJ_FLAGGED(obj, ITEM_NOLOSE | ITEM_MISSION))
#define NO_DROP(obj)		(OBJ_FLAGGED(obj, ITEM_NODROP | ITEM_MISSION))

/* compound utilities and other macros **********************************/


//#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
//#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
//#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->GetAlias()) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->GetAlias()) ? "an" : "a")


#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_TOTAL_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    (ch)->CanSee(obj))


//#define PERS(ch, vict)   ((vict)->CanSee(ch, true) == VISIBLE_FULL ? (ch)->GetName() : "someone")
#define SHIMMER_NAME "shimmer faint"
const char *PERS(CharData *ch, CharData *vict);
const char *PERSS(CharData *ch, CharData *vict);
const char *HMHR(CharData *ch, CharData *vict);
const char *HMHR(CharData *ch);
const char *HSHR(CharData *ch, CharData *vict);
const char *HSHR(CharData *ch);
const char *HSSH(CharData *ch, CharData *vict);
const char *HSSH(CharData *ch);

#define SHPS(shp, vict)	((vict)->CanSee(shp) ? (shp)->name : "something")

#define OBJS(obj, vict) ((vict)->CanSee(obj) ? (obj)->GetName()  : "something")

#define OBJN(obj, vict) ((vict)->CanSee(obj) ? fname((obj)->GetAlias()) : "something")


#define IS_HUMAN(ch)		(ch->GetRace() == RACE_HUMAN)
#define IS_SYNTHETIC(ch)	(ch->GetRace() == RACE_SYNTHETIC)
#define IS_PREDATOR(ch)		(ch->GetRace() == RACE_PREDATOR)
#define IS_ALIEN(ch)		(ch->GetRace() == RACE_ALIEN)

#define IS_MARINE(ch)					(ch->GetRace() <= RACE_SYNTHETIC)

#define IS_STAFF(ch)					(!IS_NPC(ch) && ((ch)->GetLevel() >= LVL_IMMORT))

#define OUTSIDE(ch)						(!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS) && (ch)->InRoom()->GetZone())


//#define PURGED(i)	((i)->purged)
//#define GET_ID(i)	((i)->id)

#define PURGED(i)	((i)->IsPurged())
#define GET_ID(i)	((i)->GetID())


#define DAMAGE_DICE_SIZE /*50*/ 30


#define FOREACH(Type, List, Iter) \
	for (Type::iterator Iter = (List).begin(), Iter##EndIter = (List).end(); Iter != Iter##EndIter; ++Iter)

#define FOREACH_CONST(Type, List, Iter) \
	for (Type::const_iterator Iter = (List).begin(), Iter##EndIter = (List).end(); Iter != Iter##EndIter; ++Iter)

#define FOREACH_BOUNDED(Type, List, Param, Iter) \
	for (Type::iterator Iter = (List).lower_bound(Param), Iter##EndIter = (List).upper_bound(Param); Iter != Iter##EndIter; ++Iter)

namespace Lexi
{
//	template<class Container>
//	typename Container::iterator Find(Container &container, const typename Container::value_type &value)
//	{
//		return std::find(container.begin(), container.end(), value);
//	}

	template<class Container, class T>
	typename Container::iterator Find(Container &container, const T &value)
	{
		return std::find(container.begin(), container.end(), value);
	}

	template<class Container, class T>
	typename Container::const_iterator Find(const Container &container, const T &value)
	{
		return std::find(container.begin(), container.end(), value);
	}
	
	template<class Container, class Predicate>
	typename Container::iterator FindIf(Container &container, Predicate pred)
	{
		return std::find_if(container.begin(), container.end(), pred);
	}
	
	template<class Container, class Predicate>
	typename Container::const_iterator FindIf(const Container &container, Predicate pred)
	{
		return std::find_if(container.begin(), container.end(), pred);
	}
	
	template<class Container>
	bool IsInContainer(Container &container, const typename Container::value_type &value)
	{
		return Find(container, value) != container.end();
	}
	
	template<class Container, class Predicate>
	void Sort(Container &container, Predicate pred)
	{
		std::sort(container.begin(), container.end(), pred);
	}

	template<class Container, class Predicate>
	void ForEach(Container &container, Predicate pred)
	{
		std::for_each(container.begin(), container.end(), pred);
	}

	template<class Container, class T>
	void Replace(Container &container, T oldValue, T newValue)
	{
		std::replace(container.begin(), container.end(), oldValue, newValue);
	}

	void tolower(char *str);
	
	Lexi::String	CreateDateString(time_t date, const char *format = "%c");
}




class PerfTimer
{
public:
	PerfTimer() {}
	
	void				Start();
	
	double				GetTimeElapsed();
	
private:
#if defined(_WIN32)
	class Frequency
	{
	public:
		Frequency() { QueryPerformanceFrequency(&freq); }
		LONGLONG Get() { return freq.QuadPart; }
	private:
		LARGE_INTEGER	freq;
	};

	LARGE_INTEGER		m_Start;
	static Frequency	ms_Frequency;
#else
	timeval				m_Start;
#endif
};

#endif	//	__UTILS_H__

