/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2006-2007        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : house.h                                                        [*]
[*] Usage: Player housing                                                 [*]
\***************************************************************************/


#ifndef __HOUSE_H__
#define __HOUSE_H__


#include "types.h"

#include "lexistring.h"

#include <map>
#include <vector>

namespace Lexi
{
	class Parser;
	class OutputParserFile;
}

class RoomData;


class House
{
public:
	enum SecurityLevel
	{
		AccessInvalid	= -1,
		AccessOpen		= 0,
		AccessClan,
		AccessGuest,
		AccessFriend,
		AccessOwner,
		
		NUM_SECURITY_LEVELS,
		
		DefaultSecurity = AccessGuest	//	Don't change this!  Existing rooms, houses, and guests may change type!
	};
	
	enum PaymentType
	{
		PolicyNone		= -1,
		PolicyRent		= 0,
		PolicyMortgage,
		PolicyOwned,
		
		NUM_PAYMENT_TYPES,
		
		DefaultPaymentType = PolicyRent	//	Don't change this!  Existing houses may change types!
	};
	
	struct Room
	{
		Room(RoomData *r, SecurityLevel s) : room(r), security(s) {}
		
		RoomData *		room;
		SecurityLevel	security;
	};
	
	typedef std::map<IDNum, SecurityLevel>	GuestMap;
	typedef std::vector<Room> RoomList;
	
	
						House();
						~House();
	
	IDNum				GetID() const { return m_ID; }
	
	IDNum				GetOwner() const { return m_Owner; }
	void				SetOwner(IDNum owner) { m_Owner = owner; }
	
	time_t				GetCreated() const { return m_Created; }
	const RoomList &	GetRooms() const { return m_Rooms; }
	
	const GuestMap &	GetGuests() const { return m_Guests; }
	bool				IsGuest(IDNum guest);
	void				AddGuest(IDNum guest, SecurityLevel security);
	void				SetGuestSecurity(IDNum guest, SecurityLevel security);
	SecurityLevel		GetGuestSecurity(IDNum guest);
	void				RemoveGuest(IDNum guest);
	bool				IsGuestAnOwner(IDNum guest) { return GetGuestSecurity(guest) == AccessOwner; }

	void				AddRoom(RoomData *room, SecurityLevel security = AccessOwner);
	void				SetRoomSecurity(RoomData *room, SecurityLevel security);
	SecurityLevel		GetRoomSecurity(RoomData *room);
	void				RemoveRoom(RoomData *room);
	
	SecurityLevel		GetSecurityLevel() const { return m_SecurityLevel; }
	void				SetSecurityLevel(SecurityLevel security) { m_SecurityLevel = security; }
	
	RoomData *			GetRecall() const { return m_Recall; }
	void				SetRecall(RoomData * room) { m_Recall = room; }
	
	PaymentType			GetPaymentType() const { return m_PaymentType; }
	unsigned int		GetWeeklyPayment() const { return m_WeeklyPayment; }
	unsigned int		GetAmountOwed() const { return m_AmountOwed; }
	unsigned int		GetAmountPaid() const { return m_AmountPaid; }
	time_t				GetNextPaymentDue() const { return m_NextPaymentDue; }
	time_t				GetLastPayment() const { return m_LastPayment; }
	void				ProcessPayment(CharData *ch, unsigned int amount);

	void				SetPaymentType(PaymentType type) { m_PaymentType = type; }
	void				SetWeeklyPayment(unsigned int amount) { m_WeeklyPayment = amount; }
	void				SetAmountOwed(unsigned int amount) { m_AmountOwed = amount; }
	void				SetAmountPaid(unsigned int amount) { m_AmountPaid = amount; }
	void				ResetNextPaymentDue() { m_NextPaymentDue = 0; }
	void				MarkNextPaymentMade() { m_NextPaymentDue = time(0); }
	void				ResetLastPayment() { m_LastPayment = 0; }
	
	time_t				GetNextPaymentPayableOn() const;
	time_t				GetPaymentOverdueOn() const;
	
	Lexi::String		GetNextPaymentDueString() const { return CreateDateString(m_NextPaymentDue); }
	Lexi::String		GetNextPaymentPayableOnString() const { return CreateDateString(GetNextPaymentPayableOn()); }
	Lexi::String		GetLastPaymentString() const { return CreateDateString(m_LastPayment); }

	bool				IsPaidUp();
	
	unsigned int		CalculateInterestDue() const;
	unsigned int		CalculatePayoffAmount() const;
	unsigned int		CalculatePaymentDue() const;
	
	void				Show(CharData *ch) const;
	void				ListGuests(CharData *ch, SecurityLevel security) const;
	
	void				SaveContents();
	
	static bool			CanEnter(CharData *ch, RoomData *room);
	static House *		GetHouseForRoom(RoomData *room);
	static House *		GetHouseForOwner(IDNum owner);
	static House *		GetHouseForAnyOwner(IDNum id);
	static House *		GetHouseByID(IDNum id);
	
	static void			SaveAllHousesContents();
	static void			SaveContents(RoomData *room);
	static void			List(CharData *ch);

	static void			Load();
	static void			Save();
	
	static Lexi::String	GetRoomFilename(RoomData *room);
	
private:
	bool				CheckSecurity(CharData *ch, RoomData *room);	//	Not "can enter" because it'll be a naming conflict

	static Lexi::String	CreateDateString(time_t date);
	static Lexi::String	CreateFormalDateString(time_t date);
	static Lexi::String	GetOldRoomFilename(RoomData *room);

	void				Parse(Lexi::Parser &parser);
	void				Save(Lexi::OutputParserFile &file);
	
	
	IDNum				m_ID;
	
	//	Owner, Creation
	IDNum				m_Owner;
	time_t				m_Created;
	
	//	House rooms
	RoomList			m_Rooms;
	RoomData *			m_Recall;
	
	//	Guests
	SecurityLevel		m_SecurityLevel;
	GuestMap			m_Guests;
	
	//	Payment
	PaymentType			m_PaymentType;
	unsigned int		m_WeeklyPayment;
	unsigned int		m_AmountOwed;
	unsigned int		m_AmountPaid;
	time_t				m_NextPaymentDue;
	time_t				m_LastPayment;
	
	//	House Control
	typedef std::list<House *>	HouseList;
	typedef std::multimap<IDNum, House *>HouseMap;
	typedef std::map<RoomData *, House *>RoomToHouseMap;
	
	static HouseList		ms_Houses;
	static RoomToHouseMap	ms_RoomMap;
	
	static IDNum			ms_LastHouseID;
	
	static const unsigned		OneDay = (60 * 60 * 24);
	static const unsigned		OneWeek = (OneDay * 7);
private:
	House(const House &);
	void operator=(const House &);
};

#endif
