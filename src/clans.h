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
[*] File : clans.h                                                        [*]
[*] Usage: Class and function declarations for clans                      [*]
\***************************************************************************/

#ifndef __CLANS_H__
#define __CLANS_H__

#include "index.h"

#include <list>
#include "lexistring.h"
#include "bitflags.h"

#include "relation.h"

namespace Lexi {
	class BufferedFile;
}

class CharData;

enum
{
	MAX_RANKS			= 10,
	MAX_RANK_NAME_LENGTH = 20,
	LEADER_PRIVILEGES	= 0xFFFFFFFF
};

class Clan {
public:
						Clan(IDNum vn);
						
	
	enum RankPermissions
	{
		Permission_ShowRank,
//		Permission_Vote,
		
		Permission_EditMOTD,
		Permission_EditRanks,
		Permission_EditEnlist,
		Permission_Enlist,
		Permission_Boot,
		Permission_Ban,
		Permission_Promote,
		Permission_Demote,
		Permission_Allow,
		
		Permission_War,
		Permission_Ally,
		
		Permission_Bank_View,
		Permission_Bank_Deposit,
		Permission_Bank_Withdraw,
		
		NumPermissions
	};
	typedef Lexi::BitFlags<NumPermissions, RankPermissions>	RankPermissionBits;
	static const RankPermissionBits	Permission_Bank_Mask;

	
	class Rank
	{
	public:
		Rank() : m_Order(0) {}
		
		const char *		GetName() { return m_Name.c_str(); }

		Lexi::String		m_Name;
		int					m_Order;
		RankPermissionBits	m_Privileges;
		
		static bool SortFunc(Rank *a, Rank *b) { return a->m_Order < b->m_Order; }
	};
	
	class Member
	{
	public:
		Member(IDNum id) : m_ID(id), m_Character(NULL), m_pRank(NULL), m_LastLogin(0) {}
		
		IDNum				GetID() { return m_ID; }
		const char *		GetName() { return m_Name.c_str(); }

		const IDNum			m_ID;
		Lexi::String		m_Name;
		CharData *			m_Character;
		Rank *				m_pRank;
		RankPermissionBits	m_Privileges;
		time_t				m_LastLogin, m_PreviousLogin;
	};


	IDNum				GetID() const { return m_ID; }
	
	const char *		GetName() const { return m_Name.c_str(); }
	const char *		GetTag() const { return m_Tag.c_str(); }

	
	Clan::Member *		AddMember(IDNum id);
	void				RemoveMember(Clan::Member *member);
	bool				IsMember(IDNum id) const	{ return GetMember(id) != NULL; }
	
	void				AddApplicant(IDNum id) { if (!IsApplicant(id)) m_Applicants.push_back(id); }
	void				RemoveApplicant(IDNum id) { m_Applicants.remove(id); }
	bool				IsApplicant(IDNum id) const;
	
	void				AddBan(IDNum id) { if (!IsBanned(id)) m_Bans.push_back(id); }
	void				RemoveBan(IDNum id) { m_Bans.remove(id); }
	bool				IsBanned(IDNum id) const;
	
	void				AddEnemy(IDNum id) { if (!IsEnemy(id)) m_Enemies.push_back(id); }
	void				AddEnemy(const Clan *clan) { if (!IsEnemy(clan->GetID())) m_Enemies.push_back(clan->GetID()); }
	void				RemoveEnemy(IDNum id) { m_Enemies.remove(id); }
	void				RemoveEnemy(const Clan *clan) { m_Enemies.remove(clan->GetID()); }
	bool				IsEnemy(IDNum id) const;
	bool				IsEnemy(const Clan *pClan) const { return IsEnemy(pClan->GetID()); }
	
	void				AddAlly(IDNum id) { if (!IsAlly(id)) m_Allies.push_back(id); }
	void				AddAlly(const Clan *clan) { if (!IsAlly(clan->GetID())) m_Allies.push_back(clan->GetID()); }
	void				RemoveAlly(IDNum id) { m_Allies.remove(id); }
	void				RemoveAlly(const Clan *clan) { m_Allies.remove(clan->GetID()); }
	bool				IsAlly(IDNum id) const;
	bool				IsAlly(const Clan *pClan) const { return IsAlly(pClan->GetID()); }
	
	static bool			AreAllies(const Clan *clan1, const Clan *clan2) { return clan1->IsAlly(clan2) && clan2->IsAlly(clan1); }
	static bool			AreEnemies(const Clan *clan1, const Clan *clan2) { return clan1->IsEnemy(clan2) || clan2->IsEnemy(clan1); }
	
	VirtualID			GetRoom() const { return m_Room; }
	
	int					GetNumMembersOnline() const { return m_NumMembersOnline; }
	Member *			GetMember(IDNum id) const;
	Member *			GetMember(const char *name) const;
	Rank *				GetRank(const char * name) const;
	Rank *				GetRank(int num) const;
	
	void				CreateDefaultRanks();
	void				SortRanks() { m_Ranks.sort(Rank::SortFunc); }
	void				SortMembers() {}
	
	void				OnPlayerToWorld(CharData *ch);			//	We need a callback system, maybe
	void				OnPlayerFromWorld(CharData *ch);		//	We need a callback system, maybe
	void				OnPlayerLoginMessages(CharData *ch);	//	...
	
	bool				CanAutoEnlist(CharData *ch);
	
	typedef std::list<Rank *>		RankList;
	typedef std::list<Member *>		MemberList;
	typedef std::list<IDNum>		EnemyList;
	typedef std::list<IDNum>		AllyList;
	typedef std::list<IDNum>		ApplicantList;
	typedef std::list<IDNum>		BanList;
	
	IDNum				m_ID;
	unsigned int		m_RealNumber;
	
	//	Settings
	Lexi::String		m_Name;
	Lexi::String		m_Tag;
	
	IDNum				m_Leader;
	VirtualID			m_Room;
	Flags				m_Races;
	
	//	Membership
	unsigned int		m_MaxMembers;
	MemberList			m_Members;
	ApplicantList		m_Applicants;
	BanList				m_Bans;
	
	//	Preferences
	RankList			m_Ranks;
	Rank *				m_pDefaultRank;

	//	Autoenlist
	enum ApplicationMode
	{
		Applications_Deny,
		Applications_Allow,
		Applications_AutoAccept,
		NumApplicationModes
	}					m_ApplicationMode;
	unsigned int		m_AutoEnlistMinLevel;
	unsigned int		m_AutoEnlistMinPKs;
	unsigned int		m_AutoEnlistMinDaysSinceCreation;
	unsigned int		m_AutoEnlistMinHoursPlayed;
	float				m_AutoEnlistMinPKPDRatio;
	float				m_AutoEnlistMinPKMKRatio;
	
	EnemyList			m_Enemies;
	AllyList			m_Allies;
	
	unsigned int		m_InactivityThreshold;
	Lexi::String		m_MOTD;
	time_t				m_MOTDChangedOn;
	enum ShowMOTD
	{
		ShowMOTD_Never,
		ShowMOTD_HasChanged,
		ShowMOTD_WhenChanged,
		ShowMOTD_Always,
		NumShowMOTDOptions
	}					m_ShowMOTD;
	
	//	Clan Scores
	int					m_Savings;
	
	//	Runtime Data
	int					m_NumMembersOnline;

	static void			Load();
	static void			Save();
	
	static Clan **		Index;
	static unsigned int	IndexSize;
	
	static Clan *		GetClan(IDNum id);
	
	static bool			ms_SaveClans;
	static bool			MarkedForSave() { return ms_SaveClans; }
	static void			MarkForSave()  { ms_SaveClans = true; }


	static void Parse(Lexi::BufferedFile & clan_f, IDNum id);

	//	Inter-clan relationships
	bool				HasSharedFaction(Clan *clan2);
	Relation			GetRelation(Clan *clan2);
	static void			RebuildRelations();
	
	static Relation *	ms_Relations;
	
	
//	static bool			IsMember(CharData *ch);
	static Member *		GetMember(CharData *ch);
	static void			SetMember(CharData *ch, Member *member);
	static Rank *		GetMembersRank(CharData *ch);
	static bool			HasPermission(CharData *ch, RankPermissions permission);
	static bool			HasAnyPermission(CharData *ch, RankPermissionBits permissions);
	static inline bool	IsHigherRank(Rank *r1, Rank *r2)
	{
		return r1->m_Order < r2->m_Order;
	}
	static inline bool	IsHigherRank(CharData *ch, CharData *vict)
	{
		return IsHigherRank(GetMembersRank(ch), GetMembersRank(vict));
	}
};


#endif
