/* ************************************************************************
*   File: shop.h                                        Part of CircleMUD *
*  Usage: shop file definitions, structures, constants                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __SHOP_H__
#define __SHOP_H__

#include "objects.h"

struct ShopBuyData
{
					ShopBuyData(ObjectType t = ITEM_UNDEFINED) : type(t) {}
					ShopBuyData(ObjectType t, const char *key) : type(t), keywords(key) {}
   ObjectType		type;
   Lexi::String		keywords;
};

typedef std::list<ShopBuyData> ShopBuyDataList;

#define BUY_TYPE(i)		((i).type)
#define BUY_WORD(i)		((i).keywords)


class ShopData
{
public:
						ShopData(VirtualID v);

	VirtualID			GetVirtualID() const { return m_Virtual; }
	
private:
	friend class ShopMap;
	VirtualID			m_Virtual;

public:
	
	std::vector<ObjectPrototype *>	producing;		/* Which item to produce (virtual)	*/
	float				profit_buy;		/* Factor to multiply cost with		*/
	float				profit_sell;		/* Factor to multiply cost with		*/
	ShopBuyDataList		type;	/* Which items to trade			*/
	Lexi::String		no_such_item1;		/* Message if keeper hasn't got an item	*/
	Lexi::String		no_such_item2;		/* Message if player hasn't got an item	*/
	Lexi::String		missing_cash1;		/* Message if keeper hasn't got cash	*/
	Lexi::String		missing_cash2;		/* Message if player hasn't got cash	*/
	Lexi::String		do_not_buy;		/* If keeper dosn't buy such things	*/
	Lexi::String		message_buy;		/* Message when player buys item	*/
	Lexi::String		message_sell;		/* Message when player sells item	*/
	int					temper1;		/* How does keeper react if no money	*/
	int					bitvector;		/* Can attack? Use bank? Cast here?	*/
	CharacterPrototype *keeper;		/* The mobil who owns the shop (virtual)*/
	int					with_who;		/* Who does the shop trade with?	*/
	typedef std::vector<VirtualID>	RoomVector;
	RoomVector			in_room;		/* Where is the shop?			*/
	int					open1, open2;		/* When does the shop open?		*/
	int					close1, close2;	/* When does the shop close?		*/
	int					bankAccount;		/* Store all gold over 15000 (disabled)	*/
	int					lastsort;		/* How many items are sorted in inven?	*/
//	SPECIAL (*func);		/* Secondary spec_proc for shopkeeper	*/
};


class ShopMap
{
public:
	typedef ShopData	data_type;
	typedef data_type *	value_type;
	typedef value_type *pointer;
	typedef value_type &reference;
	typedef pointer		iterator;
	typedef const pointer const_iterator;
	
	ShopMap();
	~ShopMap();
	
	size_t				size() { return m_Size; }
	bool				empty() { return m_Size == 0; }
	
	void				insert(value_type i);
	value_type			Find(VirtualID vid);
	value_type			Find(const char *arg);
	value_type			Get(size_t i)		{ return i < m_Size ? m_Index[i] : NULL; }
		
	value_type			operator[] (size_t i) { return Get(i); }
	
	iterator			begin()			{ return m_Index; }
	iterator			end()			{ return m_Index + m_Size; }
	const_iterator		begin() const	{ return m_Index; }
	const_iterator		end() const 	{ return m_Index + m_Size; }
	
	iterator			lower_bound(Hash zone);
	iterator			upper_bound(Hash zone);
		
	void				renumber(Hash old, Hash newHash);

private:
	unsigned int		m_Allocated;
	unsigned int		m_Size;
	bool				m_bDirty;
	value_type *		m_Index;
	
	static const int	msGrowSize = 64;
	
	void				Sort();
	static bool			SortFunc(value_type lhs, value_type rhs);
	
	ShopMap(const ShopMap &);
	ShopMap &operator=(const ShopMap &);
};


extern ShopMap		shop_index;


#define VERSION3_TAG	"v3.0"	/* The file has v3.0 shops in it!	*/


/* Possible states for objects trying to be sold */
#define OBJECT_DEAD			0
#define OBJECT_NOTOK		1
#define OBJECT_OK			2
#define OBJECT_NOVAL		3
#define OBJECT_CONT_NOTOK	4

/* Types of lists to read */
#define LIST_PRODUCE		0
#define LIST_TRADE			1
#define LIST_ROOM			2


/* Whom will we not trade with (bitvector for SHOP_TRADE_WITH()) */
#define TRADE_NO_HUMAN			1
#define TRADE_NO_PREDATOR		2
#define TRADE_NO_ALIEN			4


struct stack_data {
   int data[100];
   int len;
} ;

#define S_DATA(stack, index)	((stack)->data[(index)])
#define S_LEN(stack)		((stack)->len)


/* Which expression type we are now parsing */
#define OPER_OPEN_PAREN		0
#define OPER_CLOSE_PAREN	1
#define OPER_OR			2
#define OPER_AND		3
#define OPER_NOT		4
#define MAX_OPER		4

#define SHOP_NUM(i)			((i)->vnum)
#define SHOP_KEEPER(i)		((i)->keeper)
#define SHOP_OPEN1(i)		((i)->open1)
#define SHOP_CLOSE1(i)		((i)->close1)
#define SHOP_OPEN2(i)		((i)->open2)
#define SHOP_CLOSE2(i)		((i)->close2)
#define SHOP_BUYTYPE(i, num)	(BUY_TYPE((i)->type[(num)]))
#define SHOP_BUYWORD(i, num)	(BUY_WORD((i)->type[(num)]))
#define SHOP_PRODUCT(i, num)	((i)->producing[(num)])
#define SHOP_BANK(i)		((i)->bankAccount)
#define SHOP_BROKE_TEMPER(i)	((i)->temper1)
#define SHOP_BITVECTOR(i)	((i)->bitvector)
#define SHOP_TRADE_WITH(i)	((i)->with_who)
#define SHOP_SORT(i)		((i)->lastsort)
#define SHOP_BUYPROFIT(i)	((i)->profit_buy)
#define SHOP_SELLPROFIT(i)	((i)->profit_sell)
//#define SHOP_FUNC(i)		((i)->func)

#define NOTRADE_HUMAN(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NO_HUMAN))
#define NOTRADE_PREDATOR(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NO_PREDATOR))
#define NOTRADE_ALIEN(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NO_ALIEN))


#define	WILL_START_FIGHT	1
#define WILL_BANK_MONEY		2

#define SHOP_KILL_CHARS(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_START_FIGHT))
#define SHOP_USES_BANK(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_BANK_MONEY))


#define MIN_OUTSIDE_BANK	5000
#define MAX_OUTSIDE_BANK	15000

#define MSG_NOT_OPEN_YET	"Come back later!"
#define MSG_NOT_REOPEN_YET	"Sorry, we have closed, but come back later."
#define MSG_CLOSED_FOR_DAY	"Sorry, come back tomorrow."
#define MSG_NO_SEE_CHAR		"I don't trade with someone I can't see!"
#define MSG_NO_SELL_RACE	"Get out of here before I call the guards!"
#define MSG_NO_SELL_CLASS	"We don't serve your kind here!"
#define MSG_NO_USED_WANDSTAFF	"I don't buy used up wands or staves!"
#define MSG_CANT_KILL_KEEPER	"Get out of here before I call the guards!"

#endif // __SHOP_H__
