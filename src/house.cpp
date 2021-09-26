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
[*] File : house.cp                                                       [*]
[*] Usage: Player housing                                                 [*]
\***************************************************************************/




#include "house.h"

#include "buffer.h"
#include "lexifile.h"
#include "lexiparser.h"

#include "descriptors.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "db.h"
#include "utils.h"
#include "staffcmds.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "olc.h"

#include "files.h"


ACMD(do_house);
ACMD(do_houseedit);




House::HouseList		House::ms_Houses;
House::RoomToHouseMap	House::ms_RoomMap;
IDNum					House::ms_LastHouseID = 0;

const float				INTEREST_RATE = 0.05f;


ENUM_NAME_TABLE_NS(House, SecurityLevel, House::NUM_SECURITY_LEVELS)
{
	{ House::AccessOpen,	"Open"		},
	{ House::AccessClan,	"Clan"		},
	{ House::AccessGuest,	"Guest"		},
	{ House::AccessFriend,	"Friend"	},
	{ House::AccessOwner,	"Owner"		}
};


ENUM_NAME_TABLE_NS(House, PaymentType, House::NUM_PAYMENT_TYPES)
{
	{ House::PolicyRent,	"Rent"		},
	{ House::PolicyMortgage,"Mortgage"	},
	{ House::PolicyOwned,	"Owned"		}
};



House::House() :
	m_ID(++ms_LastHouseID)
,	m_Owner(0)
,	m_Created(time(0))
,	m_Recall(NULL)
,	m_SecurityLevel(DefaultSecurity)
,	m_PaymentType(DefaultPaymentType)
,	m_WeeklyPayment(0)
,	m_AmountOwed(0)
,	m_AmountPaid(0)
,	m_NextPaymentDue(0)
,	m_LastPayment(0)
{
	ms_Houses.push_back(this);
}



House::~House()
{
	ms_Houses.remove(this);
	
	for (RoomList::iterator room = m_Rooms.begin(); room != m_Rooms.end(); ++room)
	{
		room->room->GetFlags().clear(ROOM_HOUSE).clear(ROOM_HOUSE_CRASH);
		ms_RoomMap.erase(room->room);
	}
}


bool House::CanEnter(CharData *ch, RoomData *room)
{
	if (!room || !ROOM_FLAGGED(room, ROOM_HOUSE)
		|| STF_FLAGGED(ch, STAFF_ADMIN | STAFF_SECURITY | STAFF_HOUSES))
		return true;

	House *house = GetHouseForRoom(room);
	if (!house)	return true;
	
	return house->CheckSecurity(ch, room);
}


House *House::GetHouseForRoom(RoomData *room)
{
	if (!room || !ROOM_FLAGGED(room, ROOM_HOUSE))
		return NULL;
	
	RoomToHouseMap::iterator iter = ms_RoomMap.find(room);
	if (iter == ms_RoomMap.end())
		return NULL;
	
	return iter->second;
}


House *House::GetHouseForOwner(IDNum id)
{
	if (!id)	return NULL;
	
	for (HouseList::iterator iter = ms_Houses.begin(); iter != ms_Houses.end(); ++iter)
	{
		House *house = *iter;
		if (house->GetOwner() == id)
			return house;
	}

	return NULL;
}


House *House::GetHouseForAnyOwner(IDNum id)
{
	if (!id)	return NULL;
	
	for (HouseList::iterator iter = ms_Houses.begin(); iter != ms_Houses.end(); ++iter)
	{
		House *house = *iter;
		if (house->IsGuestAnOwner(id))
			return house;
	}

	return NULL;
}


House *House::GetHouseByID(IDNum id)
{
	if (!id)	return NULL;
	
	for (HouseList::iterator iter = ms_Houses.begin(); iter != ms_Houses.end(); ++iter)
	{
		House *house = *iter;
		if (house->GetID() == id)
			return house;
	}

	return NULL;
}


bool House::IsGuest(IDNum guest)
{
	return m_Guests.find(guest) != m_Guests.end();	
}


void House::AddGuest(IDNum guest, SecurityLevel security)
{
	m_Guests[guest] = security;	
}


void House::SetGuestSecurity(IDNum guest, SecurityLevel security)
{
	m_Guests[guest] = security;	
}


House::SecurityLevel House::GetGuestSecurity(IDNum guest)
{
	if (guest == m_Owner)	return AccessOwner;
	
	GuestMap::iterator iter = m_Guests.find(guest);
	
	return iter != m_Guests.end() ? iter->second : AccessOpen;	
}


void House::RemoveGuest(IDNum guest)
{
	m_Guests.erase(guest);
}


void House::AddRoom(RoomData *room, SecurityLevel security)
{
	m_Rooms.push_back(Room(room, security));
	ms_RoomMap[room] = this;
	
	room->GetFlags().set( ROOM_HOUSE ).set( ROOM_HOUSE_CRASH );
}


void House::SetRoomSecurity(RoomData *room, SecurityLevel security)
{
	for (RoomList::iterator iter = m_Rooms.begin(); iter != m_Rooms.end(); ++iter)
	{
		if (iter->room == room)
		{
			iter->security = security;
			return;
		}
	}
}


House::SecurityLevel House::GetRoomSecurity(RoomData *room)
{
	for (RoomList::iterator iter = m_Rooms.begin(); iter != m_Rooms.end(); ++iter)
	{
		if (iter->room == room)
		{
			return iter->security;
		}
	}
	
	return AccessInvalid;
}


void House::RemoveRoom(RoomData *room)
{
	room->GetFlags().clear(ROOM_HOUSE).clear(ROOM_HOUSE_CRASH);
	
	remove(House::GetRoomFilename(room).c_str());
	remove(House::GetOldRoomFilename(room).c_str());
	ms_RoomMap.erase(room);
	
	for (RoomList::iterator iter = m_Rooms.begin(); iter != m_Rooms.end(); ++iter)
	{
		if (iter->room == room)
		{
			m_Rooms.erase(iter);
			break;
		}
	}
	
	if (m_Recall == room)
	{
		m_Recall = NULL;
	}
}


bool House::IsPaidUp()
{
	return (time(0) < (m_NextPaymentDue + OneDay)) || (CalculatePaymentDue() == 0);
}

	

bool House::CheckSecurity(CharData *ch, RoomData *room)
{
	if (m_Owner == NoID)	return true;
	
	if (!IsPaidUp())
	{
		return false;
	}
	
	SecurityLevel chSecurity = GetGuestSecurity(ch->GetID());
	
	if (chSecurity == AccessOpen
		&& GET_CLAN(ch)
		&& GET_CLAN(ch)->IsMember(m_Owner))
	{
		chSecurity = AccessClan;
	}
	
	//	Test house security first
	if (chSecurity < m_SecurityLevel)
		return false;
	
	for (RoomList::iterator iter = m_Rooms.begin(), end = m_Rooms.end(); iter != end; ++iter)
	{
		if (iter->room == room)
		{
			return chSecurity >= iter->security;
		}
	}
	
	//	We didn't find the room... assume they can!
	mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: '%s' entering room '%s', supposedly in house '%d', not found in house room list.", ch->GetName(), room->GetVirtualID().Print().c_str(), m_ID);
	
	return true;
}


unsigned int House::CalculateInterestDue() const
{
	return (unsigned)(m_AmountOwed * INTEREST_RATE);//5% interest on remaining amount each week
}


unsigned int House::CalculatePayoffAmount() const
{
	return m_AmountOwed + CalculateInterestDue();
}


unsigned int House::CalculatePaymentDue() const
{
	if (m_PaymentType == PolicyOwned)		return 0;
	else if (m_PaymentType == PolicyRent)	return m_WeeklyPayment;
	
	unsigned	minimumPayment		= m_WeeklyPayment;	//	Start with this
	unsigned	mortgageInterest	= CalculateInterestDue();
	unsigned	mortgagePayoff		= CalculatePayoffAmount();
	
	if (mortgageInterest > minimumPayment)		minimumPayment = mortgageInterest;
	else if (minimumPayment > mortgagePayoff)	minimumPayment = mortgagePayoff;
	
	return minimumPayment;
}


time_t House::GetNextPaymentPayableOn() const
{
	return m_NextPaymentDue - (OneWeek - OneDay);
}


time_t House::GetPaymentOverdueOn() const
{
	return m_NextPaymentDue + OneDay;
}


void House::Show(CharData *ch) const
{
	const char *paymentTypes[] = { "rented", "mortgaged", "owned" };
	const char *accessColors[] = { "`c", "`g", "`y", "`r" };
		
	ch->Send(	 "==== House %d %s by %s", m_ID, paymentTypes[m_PaymentType], get_name_by_id(m_Owner, "<UNKNOWN>"));
	
	if (IS_STAFF(ch))
		ch->Send(" (player #%d)", m_Owner);
	
	ch->Send( 	 "\n"
			 	 "Built on       : %s\n"
			 	 "Access Level   : %s%s`n\n",
		CreateFormalDateString( m_Created ).c_str(),
		m_SecurityLevel != AccessInvalid ? accessColors[m_SecurityLevel] : "`n",
		GetEnumName(m_SecurityLevel));
	
	if (m_Recall)
	{
		ch->Send("Recall         : ");
		if (IS_STAFF(ch))	ch->Send("[`c%10s`n] ", m_Recall->GetVirtualID().Print().c_str());
		ch->Send("%-40.40s\n", m_Recall->GetName());	
	}
	
	if (m_PaymentType != PolicyOwned)
	{
		ch->Send("Weekly payment : %d\n", CalculatePaymentDue());
		if (m_PaymentType == PolicyMortgage)
		{
			ch->Send(
				 "   Mortgage    : %d has been paid out of %d total\n"
				 "   Interest    : %d of the next payment will be interest\n",
				 m_AmountPaid, m_AmountPaid + m_AmountOwed,
				 CalculateInterestDue());
			if (CalculatePaymentDue() < m_WeeklyPayment)
				ch->Send("                 The next payment is the final payment due.");
		}
		if (m_LastPayment)	ch->Send("   Last payment was made on %s.\n", GetLastPaymentString().c_str());
		else				ch->Send("   No payments on record.\n");
		
		ch->Send("   Next payment due on %s.", GetNextPaymentDueString().c_str());
		
		time_t now = time(0);
		if (now > GetPaymentOverdueOn())
			ch->Send("  The payment is extremely late, the house is locked.");
		else if (now > GetNextPaymentDue())
			ch->Send("  The payment is overdue.");
		else if (now < GetNextPaymentPayableOn())
			ch->Send("  It can be paid on %s.", GetNextPaymentPayableOnString().c_str());
		
		ch->Send("\n");
	}
	
	ch->Send("%d Room%s\n", m_Rooms.size(), m_Rooms.size() == 1 ? "" : "s");
	int counter = 0;
	for (RoomList::const_iterator room = m_Rooms.begin(); room != m_Rooms.end(); ++room)
	{
		ch->Send("  `g%2d`n) ", ++counter);
		if (IS_STAFF(ch))	ch->Send("[`c%10s`n] ", room->room->GetVirtualID().Print().c_str());
		ch->Send("%-40.40s [%s%s`n]\n",
			room->room->GetName(), room->security != AccessInvalid ? accessColors[room->security] : "`n",
				GetEnumName(room->security));
	}
		
	ListGuests(ch, AccessGuest);
	ListGuests(ch, AccessFriend);
	ListGuests(ch, AccessOwner);
}


void House::ListGuests(CharData *ch, SecurityLevel security) const
{
	int numGuests = 0;
	for (GuestMap::const_iterator guest = m_Guests.begin(); guest != m_Guests.end(); ++guest)
		if (guest->second == security)
			++numGuests;
	ch->Send("%d %s%s\n",
		numGuests,
		GetEnumName(security), numGuests == 1 ? "" : "s");


	int counter = 0, columns = 0;
	for (GuestMap::const_iterator guest = m_Guests.begin(); guest != m_Guests.end(); ++guest)
	{
		if (guest->second == security)
		{
			const char *guestName = get_name_by_id(guest->first);
			if (!guestName)	continue;
			ch->Send("  `g%2d`n) %-20.20s%s", ++counter, guestName, (++columns % 2) == 0 ? "\n" : "   ");
		}
	}
	if (columns % 2)	ch->Send("\n");
}


Lexi::String House::CreateDateString(time_t date)
{
	return Lexi::CreateDateString(date, "%A, %B %e");
}


Lexi::String House::CreateFormalDateString(time_t date)
{
	return Lexi::CreateDateString(date, "%b %e %Y");
}


Lexi::String House::GetRoomFilename(RoomData *room)
{
	if (!room->GetVirtualID().IsValid())	return "<INVALID>";
	BUFFER(buf, MAX_STRING_LENGTH);
	sprintf(buf, HOUSE_DIR "%s.objs", room->GetVirtualID().Print().c_str());
	while (1)
	{
		char *colon = strchr(buf, ':');
		if (colon)	*colon = '_';
		else		break;
	}
	
	return buf;
}


Lexi::String House::GetOldRoomFilename(RoomData *room)
{
	if (!room->GetVirtualID().IsValid())	return Lexi::String();
	
	VNum	vnum = VNumLegacy::GetVirtualNumber(room->GetVirtualID());
	if (vnum == -1)	return Lexi::String();

	BUFFER(buf, MAX_STRING_LENGTH);
	sprintf(buf, HOUSE_DIR "%d.objs", vnum);
	return buf;
}


//			ch->Send("Your minimum payment this week has been adjusted to %d MP, because the interest on this week's payment is greater than the weekly minimum.  If you only pay this amount, you will only cover the interest.\n");

void House::ProcessPayment(CharData *ch, unsigned int amount)
{
	unsigned	amountOwed = CalculatePaymentDue();
	time_t		now = time(0);
	
	//	Mortgage interest adjustment:
	//	mortgage is ascumulated only once per week
	if (m_PaymentType == PolicyMortgage)
	{
		amount = MIN(amount, CalculatePayoffAmount());
	}
	else if (m_PaymentType == PolicyRent && amount > amountOwed)			amount = amountOwed;
	
	if (amountOwed <= 0)				ch->Send("There appears to be a problem with your house and the amount you owe.\n");
	else if (m_PaymentType == PolicyOwned)	ch->Send("You do not owe anything on your house.\n");
	else if (amount <= 0)				ch->Send("That is not a valid amount to pay.\n");
	else if (amount < amountOwed)		ch->Send("You must pay at least %d.\n", amountOwed);
	else if (now < GetNextPaymentPayableOn())	ch->Send("You cannot pay more than 6 days in advance, sorry.\n");
	else if (amount > GET_MISSION_POINTS(ch))	ch->Send("You do not have enough Mission Points to pay that amount.\n");
	else
	{
		int amountPaid = amount;
		PaymentType oldPaymentType = m_PaymentType;
		
		ch->Send("You make your weekly payment of %d on this house.\n", amountPaid);
		
		if (m_PaymentType == PolicyMortgage)
		{
			unsigned int	interest = CalculateInterestDue();
			unsigned int	principalApplied = amount - interest;
			
			m_AmountOwed -= principalApplied;	//	then apply remainder to value and reduce amount owed
			m_AmountPaid += principalApplied;
			
			ch->Send("%d MP covered the interest, and the remaining %d went to principal.\n", interest, principalApplied);

			if (m_AmountOwed == 0)
			{
				ch->Send("Congratulations!  Your house is now paid off in full!\n");
				m_PaymentType = PolicyOwned;
			}
			else						ch->Send("The remaining principal owed on your mortgage is %d MP.\n", m_AmountOwed);
		}

		m_LastPayment = now;
		
		if (m_PaymentType != PolicyOwned)
		{
			if (m_NextPaymentDue == 0)							//	First payment...
				m_NextPaymentDue = now + OneWeek;			//	You are paid up minus 1 day
			else if (now > GetPaymentOverdueOn())				//	If you more than 1 day overdue...
				m_NextPaymentDue = now + OneWeek - OneDay;	//	You are paid up minus 1 day
			else
				m_NextPaymentDue = m_NextPaymentDue + OneWeek;	//	Otherwise you are paid up 
			
			
			ch->Send("You are paid up through %s.\n", GetNextPaymentDueString().c_str() );
		}
		
		//	Reduce MPs
		GET_MISSION_POINTS(ch) -= amountPaid;
		
    	mudlogf( NRM, LVL_IMMORT, TRUE,  "HOUSE: %s paid %d towards %s on house owned by %s [%d].",
    		ch->GetName(), amountPaid, GetEnumName(oldPaymentType),
    		get_name_by_id(m_Owner, "<UNKNOWN>"), m_Owner);
    	
		House::Save();
		ch->Save();
	}
}


void House::List(CharData *ch)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	strcat(buf, "ID   Owner         Build Date   Rooms  Guests  Policy    Last Paymt\n"
			 	"---  ------------  -----------  -----  ------  --------  -----------\n");

	for (HouseList::iterator iter = ms_Houses.begin(); iter != ms_Houses.end(); ++iter)
	{
		House *house = *iter;
		
		sprintf_cat(buf, "%3d  %-12.12s  %-11.11s  %5d  %6d  %-8.8s  %-11.11s\n",
			house->GetID(),
			house->GetOwner() != NoID ? get_name_by_id(house->GetOwner(), "<UNKNOWN>") : "",
			CreateFormalDateString(house->GetCreated()).c_str(),
			house->GetRooms().size(),
			house->GetGuests().size(),
			GetEnumName(house->GetPaymentType()),
			CreateFormalDateString(house->GetLastPayment()).c_str());
	}
	
	page_string(ch->desc, buf);
}



void House::Load()
{
	Lexi::BufferedFile	file(HOUSE_FILE);
	
	if (file.bad())
	{
		log("Unable to read housing file.");
		tar_restore_file(HOUSE_FILE);
	}
	
	Lexi::Parser		parser;
	parser.Parse(file);
	
	int numHouses = parser.NumSections("Houses");
	for (int i = 0; i < numHouses; ++i)
	{
		House *house = new House;
		
		PARSER_SET_ROOT_INDEXED(parser, "Houses", i);
		house->Parse(parser);
		
		for (RoomList::iterator room = house->m_Rooms.begin(); room != house->m_Rooms.end(); ++room)
		{
			Lexi::String oldFilename = GetOldRoomFilename(room->room);
			Lexi::String filename = GetRoomFilename(room->room);
			
			if (!oldFilename.empty() && Lexi::InputFile(oldFilename.c_str()).good())
			{
				rename(oldFilename.c_str(), filename.c_str());
			}
			
			LoadPersistentObjectFile(filename, room->room);
		}
	}
}



void House::Save()
{
	Lexi::OutputParserFile	file(HOUSE_FILE ".new");

	if (file.fail())
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write houses to disk: unable to open temporary file.");
		return;
	}
	
	file.BeginParser();
	
	int count = 0;
	for (HouseList::iterator iter = ms_Houses.begin(); iter != ms_Houses.end(); ++iter)
	{
		House *house = *iter;
		
		file.BeginSection(Lexi::Format("Houses/House %d", ++count));
		{
			file.WriteInteger("ID", house->m_ID);
			file.WriteString("Owner", Lexi::Format("%u %s", house->m_Owner, get_name_by_id(house->m_Owner, "")).c_str());
			file.WriteInteger("Created", house->m_Created);
			file.WriteEnum("Security", house->m_SecurityLevel, DefaultSecurity);
			if (house->m_Recall)	file.WriteVirtualID("Recall", house->m_Recall->GetVirtualID());
			
			int roomCount = 0;
			for (House::RoomList::iterator room = house->m_Rooms.begin(), end = house->m_Rooms.end();
				room != end; ++room)
			{
				file.BeginSection(Lexi::Format("Rooms/%d", ++roomCount));
				{
					file.WriteVirtualID("Virtual", room->room->GetVirtualID());
					file.WriteEnum("Security", room->security, DefaultSecurity);
				}
				file.EndSection();
			}
			
			int guestCount = 0;
			for (House::GuestMap::iterator guest = house->m_Guests.begin(), end = house->m_Guests.end();
				guest != end; ++guest)
			{
				IDNum			id = guest->first;
				SecurityLevel	security = guest->second;
				
				file.BeginSection(Lexi::Format("Guests/%d", ++guestCount));
				{
					file.WriteString("ID", Lexi::Format("%u %s", id, get_name_by_id(id, "")).c_str());
					file.WriteEnum("Security", security, DefaultSecurity);
				}
				file.EndSection();
			}
			
			file.BeginSection("Payment");
			{
				file.WriteEnum("Type", house->m_PaymentType);
				file.WriteDate("DueDate", house->m_NextPaymentDue);
				file.WriteDate("LastPayment", house->m_LastPayment);
				file.WriteInteger("Weekly", house->m_WeeklyPayment, 0);
				file.WriteInteger("Owed", house->m_AmountOwed, 0);
				file.WriteInteger("Paid", house->m_AmountPaid, 0);
			}
			file.EndSection();
		}
		file.EndSection();
	}
	
	file.EndParser();
	file.close();

	remove(HOUSE_FILE);
	if (rename(HOUSE_FILE ".new", HOUSE_FILE))
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write houses to disk: unable to replace file with temporary");
		return;
	}
}


void House::SaveAllHousesContents()
{
	for (HouseList::iterator iter = ms_Houses.begin(); iter != ms_Houses.end(); ++iter)
	{
		House *house = *iter;
		house->SaveContents();
	}
}


void House::SaveContents(RoomData *room)
{
	House *house = GetHouseForRoom(room);
	if (house)	house->SaveContents();
}


void House::Parse(Lexi::Parser &parser)
{
	m_ID = parser.GetInteger("ID");
	if (ms_LastHouseID < m_ID)	ms_LastHouseID = m_ID;
	
	m_Owner = parser.GetInteger("Owner");
	m_Created = parser.GetInteger("Created");
	
	m_SecurityLevel = parser.GetEnum("Security", DefaultSecurity);

	int numRooms = parser.NumSections("Rooms");
	for (int i = 0; i < numRooms; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Rooms", i);
		
		RoomData *		realRoom = world.Find(parser.GetString("Virtual", parser.GetString("VNum")));
		SecurityLevel	security = parser.GetEnum("Security", DefaultSecurity);
		
		if (realRoom == NULL)
			continue;
		
		AddRoom(realRoom, security);
	}
	
	RoomData *recallRoom = world.Find(parser.GetVirtualID("Recall"));
	if (recallRoom != NULL && GetRoomSecurity(recallRoom) != AccessInvalid)
	{
		m_Recall = recallRoom;
	}
	
	int numGuests = parser.NumSections("Guests");
	for (int i = 0; i < numGuests; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Guests", i);

		IDNum			guest = parser.GetInteger("ID");
		SecurityLevel	security = parser.GetEnum("Security", DefaultSecurity);
		
		if (get_name_by_id(guest))
			m_Guests[guest] = security;
	}
	
	m_PaymentType	= parser.GetEnum("Payment/Type", PolicyRent);
	m_NextPaymentDue= parser.GetInteger("Payment/DueDate");
	m_LastPayment	= parser.GetInteger("Payment/LastPayment");
	m_WeeklyPayment	= parser.GetInteger("Payment/Weekly");
	m_AmountOwed	= parser.GetInteger("Payment/Owed");
	m_AmountPaid	= parser.GetInteger("Payment/Paid");
}


void House::SaveContents()
{
	for (RoomList::iterator room = m_Rooms.begin(); room != m_Rooms.end(); ++room)
	{
		//	Nothing has changed
		if (!ROOM_FLAGGED(room->room, ROOM_HOUSE_CRASH))
			continue;
		
		Lexi::String	roomContentsFilename = GetRoomFilename(room->room);
		ObjectList		roomContents;
		
		FOREACH(ObjectList, room->room->contents, obj)
		{
			if (IsRentable(*obj) && !OBJ_FLAGGED(*obj, ITEM_PERSISTENT))
				roomContents.push_back(*obj);
		}
		
		if (roomContents.empty())
		{
			remove(roomContentsFilename.c_str());
			continue;
		}
		
		Lexi::String			roomContentsTempFilename = roomContentsFilename + ".new";
		Lexi::OutputParserFile	file(roomContentsTempFilename.c_str());
		if (file.fail())
		{
			mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write room %s contents to disk: unable to open temporary file %s",
				room->room->GetVirtualID().Print().c_str(), roomContentsTempFilename.c_str());
			continue;
		}
		
		SaveObjectFile(file, roomContents, IsRentable);
		
		file.close();
		
		remove(roomContentsFilename.c_str());
		if (rename(roomContentsTempFilename.c_str(), roomContentsFilename.c_str()))
		{
			mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write room %s contents to disk: unable to replace file %s with temporary file %s",
				room->room->GetVirtualID().Print().c_str(),
				roomContentsFilename.c_str(),
				roomContentsTempFilename.c_str());
			continue;
		}
		
		room->room->GetFlags().clear(ROOM_HOUSE_CRASH);
	}
}


ACMD(do_house)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	argument = one_argument(argument, arg);
	
	bool bInHouse = true;
	House *	house = NULL;
	
	if (*arg && is_number(arg))
	{
		house = House::GetHouseByID(atoi(arg));
		if (!house)
		{
			ch->Send("No such house found.\n");
			return;
		}
		if (!house->IsGuestAnOwner(ch->GetID()))
		{
			ch->Send("You are not an owner of that house.\n");
			return;
		}
		
		argument = one_argument(argument, arg);
	
		bInHouse = house->GetRoomSecurity(ch->InRoom()) == House::AccessOwner;
	}
	else
	{
		house = House::GetHouseForRoom(ch->InRoom());
		if (!house && ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
			ch->Send("System error: you appear to be in a house, but it does not exist in the system!\n");
		else if (!house || !house->IsGuestAnOwner(ch->GetID()))
		{
			house = House::GetHouseForOwner(ch->GetID());
			bInHouse = false;
		}
	}
	
	if (!house)
		ch->Send("You do not own a house.  If you are not the primary owner of the house, you must be in it to use any house commands.\n");
	else if (!str_cmp(arg, "show"))		//	house show
	{
		house->Show(ch);
	}
	else if (!str_cmp(arg, "add"))		//	house add <guest name> <security>
	{
		two_arguments(argument, arg, arg2);
		
		IDNum id = get_id_by_name(arg);
		House::SecurityLevel security =
			*arg2 ? (House::SecurityLevel)GetEnumByName<House::SecurityLevel>(arg2) : House::AccessGuest;
		
		if (id == NoID)							ch->Send("No such player found.\n");
		else if (id == ch->GetID())				ch->Send("That's you... idiot.\n");
		else if (house->IsGuest(id))			ch->Send("They are already a guest.\n");
		else if (security <= House::AccessOpen)	ch->Send("Invalid security level.  Please choose from 'Guest', 'Friend', or 'Owner'.\n");
		else
		{
			house->AddGuest(id, security);
			ch->Send("'%s' added to the guest list as %s.\n", CAP(arg), GetEnumName(security));
			House::Save();
		}
	}
	else if (!str_cmp(arg, "remove"))	//	house remove <guest name>
	{
		one_argument(argument, arg);
		
		IDNum id = get_id_by_name(arg);
		
		if (id == NoID)					ch->Send("No such player found.\n");
		else if (id == ch->GetID())		ch->Send("That's you... idiot.\n");
		else if (!house->IsGuest(id))	ch->Send("No such guest.\n");
		else
		{
			house->RemoveGuest(id);
			ch->Send("'%s' removed from the guest list.\n", CAP(arg));
			House::Save();
		}
	}
	else if (!str_cmp(arg, "guestaccess"))	//	house change <guest name> <security>
	{
		two_arguments(argument, arg, arg2);
		
		IDNum id = get_id_by_name(arg);
		House::SecurityLevel security = (House::SecurityLevel)GetEnumByName<House::SecurityLevel>(arg2);
		
		if (id == NoID)							ch->Send("No such player found.\n");
		else if (id == ch->GetID())				ch->Send("That's you... idiot.\n");
		else if (!house->IsGuest(id))			ch->Send("No such guest.\n");
		else if (security <= House::AccessOpen)	ch->Send("Invalid security level.  Please choose from 'Guest', 'Friend', or 'Owner'.\n");
		else
		{
			House::SecurityLevel oldSecurity = house->GetGuestSecurity(id);
			if (security == oldSecurity)	ch->Send("'%s' was already a %s.  No change made.\n", CAP(arg), GetEnumName(security));
			else
			{
				house->SetGuestSecurity(id, security);
				ch->Send("'%s' changed from %s to %s.\n", CAP(arg), GetEnumName(oldSecurity), GetEnumName(security));
				House::Save();
			}
		}
	}
	else if (!str_cmp(arg, "roomaccess"))	//	house roomaccess <security>
	{
		one_argument(argument, arg);
		
		House::SecurityLevel security = (House::SecurityLevel)GetEnumByName<House::SecurityLevel>(arg);
		House::SecurityLevel oldSecurity = house->GetRoomSecurity(ch->InRoom());
		
		if (!bInHouse)							ch->Send("This room is not part of your house!\n");
		else if (security < House::AccessOpen)	ch->Send("Invalid access level.  Please choose from 'Open', 'Guest', 'Friend', or 'Owner'.\n");
		else if (security == oldSecurity)		ch->Send("This room is already set to %s access.  No change made.\n", GetEnumName(security));
		else
		{
			house->SetRoomSecurity(ch->InRoom(), security);
			ch->Send("This room has been changed to %s access.\n", GetEnumName(security));
			House::Save();
		}
	}
	else if (!str_cmp(arg, "setrecall"))	//	house setrecall
	{
		if (!bInHouse)							ch->Send("This room is not part of your house!\n");
		else
		{
			house->SetRecall(ch->InRoom());
			ch->Send("This room is now the house recall.  Friends can recall to here if the house and room is to Friends, by using \"recall house %d\".\n", house->GetID());
			House::Save();
		}
	}
	else if (!str_cmp(arg, "roomedit"))		//	house roomedit
	{
		one_argument(argument, arg);
		
		House::SecurityLevel roomSecurity = house->GetRoomSecurity(ch->InRoom());
		
		if (!bInHouse)	ch->Send("This room is not part of your house!");
		else
		{
			/*. Check whatever it is isn't already being edited .*/
			FOREACH(DescriptorList, descriptor_list, iter)
			{
				DescriptorData *d = *iter;
	            if (typeid(*d->GetState()) == typeid(RoomEditOLCConnectionState)
	            	&& d->olc && OLC_VID(d) == ch->InRoom()->GetVirtualID())
	            {
					act("This room is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
	                return;
	            }
	        }
			
			ch->desc->olc = new olc_data;
			OLC_VID(ch->desc) = ch->InRoom()->GetVirtualID();
			OLC_SOURCE_ZONE(ch->desc) = ch->InRoom()->GetZone();
			REdit::setup_existing(ch->desc, IN_ROOM(ch));
			ch->desc->GetState()->Push(new RoomEditOLCConnectionState);

			act("$n starts re-decorating the room.", TRUE, ch, 0, 0, TO_ROOM);
//			mudlogf(NRM, ch->GetPlayer()->m_StaffInvisLevel, FALSE, "OLC: %s starts using OLC.", ch->GetName());
//			SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
		}
	}
	else if (!str_cmp(arg, "access"))		//	house access <security>
	{
		one_argument(argument, arg);
		
		House::SecurityLevel security = (House::SecurityLevel)GetEnumByName<House::SecurityLevel>(arg);
		
		if (security < House::AccessOpen)				ch->Send("Invalid access level.  Please choose from 'Open', 'Guest', 'Friend', or 'Owner'.\n");
		else if (security == house->GetSecurityLevel())	ch->Send("House was already set to %s access.  No change made.\n", GetEnumName(security));
		else
		{
			house->SetSecurityLevel(security);
			ch->Send("House changed to %s access.\n", GetEnumName(security));
			House::Save();
		}
	}
	else if (!str_cmp(arg, "boot") || !str_cmp(arg, "kick"))	//	house (boot|kick) <guest name>
	{
		one_argument(argument, arg);
		
		CharData *victim = get_player_vis(ch, arg);
		if (!victim)									ch->Send("No such player found.\n");
		else if (ch == victim)								ch->Send("That's you, you idiot!\n");
		else if (House::GetHouseForRoom(victim->InRoom()) != house)
			ch->Send("%s is not in your house.\n", victim->GetName());
		else
		{
			act("$N is kicked out of your house.", TRUE, ch, 0, victim, TO_CHAR);
			act("$N is kicked out of $n's house.", TRUE, ch, 0, victim, TO_NOTVICT);
			victim->FromRoom();
			victim->ToRoom(victim->StartRoom());
			act("$N has kicked you out of $s house.", TRUE, ch, 0, victim, TO_VICT);
			act("$n appears in a bright flash of light.", TRUE, victim, 0, 0, TO_ROOM);
		}
	}
	else if (!str_cmp(arg, "pay"))		//	house pay [amount]
	{
		if (house->GetPaymentType() == House::PolicyOwned)
		{
			ch->Send("You do not owe anything on your house.\n");
		}
		else if (!*argument)
		{
			time_t	now = time(0);
			
			unsigned int	amountOwed = house->CalculatePaymentDue();
			
			if (house->GetNextPaymentDue() == 0)			ch->Send("Your first payment of %d is due.  Your house is not accessible until the payment is made.\n", amountOwed);
			else if (now > house->GetPaymentOverdueOn())	ch->Send("Your next payment of %d is overdue.  Your house is not accessible until the payment is made.\n", amountOwed);
			else if (now > house->GetNextPaymentDue())		ch->Send("Your next payment of %d is overdue.  Pay promptly to prevent being locked out of your house!\n", amountOwed);
			else if (now < house->GetNextPaymentPayableOn())ch->Send("Your next payment of %d will be payable on %s.", amountOwed, house->GetNextPaymentPayableOnString().c_str());
			else											ch->Send("You may make your next payment of %d now.  It is due by %s.", amountOwed, house->GetNextPaymentDueString().c_str());
			
			if (house->GetPaymentType() == House::PolicyMortgage)
			{
				ch->Send("%d MP of your payment will cover the interest on your mortgage.\n", house->CalculateInterestDue());
			}
		}
		else
		{
			int payment = atoi(argument);
			house->ProcessPayment(ch, payment);
		}
	}
	else
	{
		ch->Send(
			"Usage:\n"
			"house show                           Show house information\n"
			"house add <guest> <access>           Add guest as: Guest, Friend, Owner\n"
			"house remove <guest>                 Remove guest\n"
			"house guestaccess <guest> <access>   Change guest access\n"
			"house roomaccess <guest>             Change room access\n"
			"house access <access>                Change house access\n"
			"house boot <guest>                   Boot guest out of house\n"
			"house pay                            View payment information\n"
			"house pay <amount>                   Pay rent/mortgage\n"
			"house roomedit                       Edit room\n"
			"house setrecall                      Set the current room as the recall\n"
			"If you are not in the house you wish to control, you can still use the house commands if you specify your house ID number immediately the house command.\n"
			"For example: house <id> pay <guest>\n");
	}
}



ACMD(do_houseedit)
{
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	BUFFER(arg3, MAX_INPUT_LENGTH);
	
	argument = three_arguments(argument, arg1, arg2, arg3);
	
	House *house = NULL;
	
	if (!str_cmp(arg1, "list"))					//	house list
	{
		House::List(ch);
	}
	else if (!str_cmp(arg1, "show"))			//	house show <#>
	{
		if (*arg2)	house = House::GetHouseByID(atoi(arg2));
		if (!house)	house = House::GetHouseForRoom(ch->InRoom());
		
		if (!house)	ch->Send("No house found, and you are not in a house.\n");
		else		house->Show(ch);
	}
	else if (!STF_FLAGGED(ch, STAFF_HOUSES))
	{
		ch->Send("Usage:\n"
				 "houseedit list                       List all houses\n"
				 "houseedit show <#>                   Show information about the house\n"
				 "You do not have permission to access the rest of houseedit.\n");
	}
	else if (!str_cmp(arg1, "create"))			//	house create [<owner>]
	{
		IDNum id = 0;
		if (*arg2)
		{
			id = get_id_by_name(arg2);
			if (id == NoID)
			{
				ch->Send("No such player.\n");
				return;
			}
		}
		
		house = new House;
		
		ch->Send("House %d created.\n", house->GetID());
		if (id != NoID)
		{
			house->SetOwner(id);
			
			const char *ownerName = get_name_by_id(id, "<UNKNOWN>");
			ch->Send("Owner set to %s.\n", ownerName);	
			
			mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s created the house with owner %s.", house->GetID(), ch->GetName(), ownerName);
		}
		else
		{
			mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s created the house with no owner.", house->GetID(), ch->GetName());
		}

		House::Save();
	}
	else
	{
		if (*arg1)	house = House::GetHouseByID(atoi(arg1));
		
		//	Reuse arg1 now
		one_argument(argument, arg1);
				
		if (!house)								ch->Send("No house specified.\n");
		else if (!str_cmp(arg2, "set"))			//	houseedit <#> set <what ...>
		{
			if (house->GetRoomSecurity(ch->InRoom()) == House::AccessInvalid)
				ch->Send("As a safety precaution, you must actually be in the house to use this command.\n");
			else if (!str_cmp(arg3, "owner"))	//	houseedit <#> set owner <name>
			{
				IDNum id = get_id_by_name(arg1);
				if (!str_cmp(arg1, "0"))
				{
					house->SetOwner(0);
					house->ResetLastPayment();
					ch->Send("House %d is no longer owned by anyone.\n", house->GetID());

					mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s removed the owner.", house->GetID(), ch->GetName());

					House::Save();
				}
				else if (id == NoID)			ch->Send("No such player.\n");
				else
				{
					house->SetOwner(id);
					house->ResetLastPayment();
					house->RemoveGuest(id);
					
					const char *ownerName = get_name_by_id(id, "<UNKNOWN>");
					
					ch->Send("House %d is now owned by %s.\n", house->GetID(), ownerName);
					
					mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s changed the owner to %s.", house->GetID(), ch->GetName(), ownerName);

					House::Save();
				}
			}
			else if (!str_cmp(arg3, "policy"))	//	houseedit <#> set policy <type>
			{
				one_argument(argument, arg1);
				
				House::PaymentType policy = (House::PaymentType)GetEnumByName<House::PaymentType>(arg1);
				
				if (policy == House::PolicyNone)			ch->Send("Valid policy types: Rent, Mortgage, Owned\n");
				else if (house->GetPaymentType() == policy)	ch->Send("The house policy is already set to %s.  No change made.\n", GetEnumName(policy));
				else
				{
					house->SetPaymentType(policy);
					ch->Send("Policy on house %d changed to %s.\n", house->GetID(), GetEnumName(policy) );
					
					mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s changed the payment policy to %s.", house->GetID(), ch->GetName(), GetEnumName(policy));

					if (policy == House::PolicyMortgage)
						ch->Send("Please verify the weekly payment, amount due, and amount paid to date on this house.\n");
					else if (policy == House::PolicyRent)
						ch->Send("Please verify the weekly payment of this house.\n");
					else if (policy == House::PolicyOwned)
						ch->Send("Please verify the amount paid on this house.\n");
					
					House::Save();
				}
			}
			else if (!str_cmp(arg3, "payment"))	//	houseedit <#> set payment <amount>
			{
				one_argument(argument, arg1);
				
				int amount = atoi(arg1);
				
				if (!*arg1 || !is_number(arg1))	ch->Send("Usage: houseedit <#> set payment <amount>\n");
				else if (amount < 0)		ch->Send("Please specify a non-negative amount.\n");
				else
				{
					house->SetWeeklyPayment(amount);
					ch->Send("House %d's weekly payment changed to %d.\n", house->GetID(), amount);
					
					mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s changed the weekly payment to %d.", house->GetID(), ch->GetName(), amount);

					House::Save();
				}
			}
			else if (!str_cmp(arg3, "owed"))	//	houseedit <#> set owed <amount>
			{
				one_argument(argument, arg1);
				
				int amount = atoi(arg1);
				
				if (!*arg1 || !is_number(arg1))	ch->Send("Usage: houseedit <#> set owed <amount>\n");
				else if (amount < 0)		ch->Send("Please specify a non-negative amount.\n");
				else
				{
					house->SetAmountOwed(amount);
					ch->Send("House %d's total amount owed changed to %d.\n", house->GetID(), amount);
					
					mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s changed the amount owed to %d.", house->GetID(), ch->GetName(), amount);

					House::Save();
				}
			}
			else if (!str_cmp(arg3, "paid"))	//	house <#> set paid <amount>
			{
				one_argument(argument, arg1);
				
				int amount = atoi(arg1);
				
				if (!*arg1 || !is_number(arg1))	ch->Send("Usage: houseedit <#> set paid <amount>\n");
				else if (amount < 0)		ch->Send("Please specify a non-negative amount.\n");
				else
				{
					house->SetAmountPaid(amount);
					ch->Send("House %d's total amount paid changed to %d.\n", house->GetID(), amount);

					mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s changed the amount paid to %d.", house->GetID(), ch->GetName(), amount);
					
					House::Save();
				}
			}
			else
			{
				ch->Send("Usage:\n"
						 "houseedit <#> set owner <owner>      Set owner\n"
						 "houseedit <#> set policy <type>      Set policy to Rent, Mortage, Owned\n"
						 "houseedit <#> set payment <amount>   Set minimum weekly payment\n"
						 "houseedit <#> set owed <amount>      Set remaining amount owed\n"
						 "houseedit <#> set paid <amount>      Set amount paid to date\n");
			}
		}
		else if (!str_cmp(arg2, "addroom"))
		{
			RoomData *room = ch->InRoom();
			
			if (*arg3)	room = world.Find(arg3);
			
			if (*arg3 && !is_number(arg3))	ch->Send("Usage: houseedit <#> room add <vnum>\n");
			else if (room == NULL)			ch->Send("Room %s not found.\n", arg3);
			else if (House::GetHouseForRoom(room))	ch->Send("Room %s is already part of a house.\n", room->GetVirtualID().Print().c_str());
			else
			{
				house->AddRoom(room);
				ch->Send("House %d has had room %s added.\n", house->GetID(), room->GetVirtualID().Print().c_str());
					
				mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s added room %s.", house->GetID(), ch->GetName(), room->GetVirtualID().Print().c_str());

				House::Save();
			}
		}
		else if (!str_cmp(arg2, "removeroom"))
		{
			RoomData *room = ch->InRoom();
			
			if (*arg3)	room = world.Find(arg3);
			
			if (*arg3 && !is_number(arg3))	ch->Send("Usage: houseedit <#> room remove <vnum>\n");
			else if (room == NULL)			ch->Send("Room %s not found.\n", arg3);
			else if (House::GetHouseForRoom(room) != house)	ch->Send("Room %s is part of another house.\n", room->GetVirtualID().Print().c_str());
			else
			{
				house->RemoveRoom(room);
				ch->Send("House %d has had room %s removed.\n", house->GetID(), room->GetVirtualID().Print().c_str());
				
				mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s removed room %s.", house->GetID(), ch->GetName(), room->GetVirtualID().Print().c_str());

				House::Save();
			}
		}
		else if (!str_cmp(arg2, "resetpaymentdue"))		//	house <#> resetpaymentdue
		{
			if (house->GetRoomSecurity(ch->InRoom()) == House::AccessInvalid)
				ch->Send("As a safety precaution, you must actually be in the house to use this command.\n");
			else
			{
				house->ResetNextPaymentDue();
				ch->Send("House %d's next payment is now due immediately.\n", house->GetID());
						
				mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s reset the next payment due date.", house->GetID(), ch->GetName());

				House::Save();
			}
		}
		else if (!str_cmp(arg2, "markpaymentpaid"))		//	house <#> markpaymentpaid
		{
			if (house->GetRoomSecurity(ch->InRoom()) == House::AccessInvalid)
				ch->Send("As a safety precaution, you must actually be in the house to use this command.\n");
			else
			{
				house->MarkNextPaymentMade();
				ch->Send("House %d's next payment is marked as paid.\n", house->GetID());

				mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s marked the next payment as paid.", house->GetID(), ch->GetName());
				
				House::Save();
			}
		}
		else if (!str_cmp(arg2, "delete"))		//	house <#> delete <code>
		{
			Lexi::String code = Lexi::Format("%p", house);
			
			if (arg3 != code)
				ch->Send("`rWARNING: Before deleting a house, please make sure to notify the player, and reimburse them if necessary.`n\n"
						 "To delete house %d, type the following command: houseedit %d delete %s\n",
						 	house->GetID(), house->GetID(), code.c_str());
			else
			{
				const char *ownerName = get_name_by_id(house->GetOwner());
				
				ch->Send("House %d deleted.\n", house->GetID());
				
				if (ownerName && house->GetOwner())
				{
					BUFFER(buf, MAX_STRING_LENGTH);
					strcpy(buf, ownerName);
					ch->Send("Player %s had invested %d into the house.\n", CAP(buf), house->GetAmountPaid());
				}

				mudlogf(NRM, LVL_STAFF, TRUE, "(GC) HOUSE %d: %s deleted this house, which was owned by %s.", house->GetID(), ch->GetName(), ownerName ? ownerName : "<UNKNOWN>");
				
				delete house;
					
				House::Save();
			}
		}
		else
		{
			ch->Send("Usage:\n"
					 "houseedit list                       List all houses\n"
					 "houseedit show <#>                   Show information about the house\n"
					 "houseedit <#> set ...                Set various attributes of the house\n"
					 "houseedit <#> addroom <room>         Add the room to the house\n"
					 "houseedit <#> removeroom <room>      Remove the room from the house\n"
					 "houseedit <#> resetpaymentdue        Make the next payment due now\n"
					 "houseedit <#> markpaymentpaid        Mark that the next payment was paid\n"
					 "houseedit <#> delete                 Delete the house\n");

		}
	}
}
