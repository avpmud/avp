/* ************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

#define __SHOP_C__


#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "files.h"


/* External variables */
extern struct TimeInfoData time_info;
extern char *extra_bits[];
extern char *drinks[];

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
char *fname(char *namelist);
void sort_keeper_objs(CharData * keeper, ShopData *shop);
int can_take_obj(CharData * ch, ObjData * obj);


/* Local variables */
ShopMap	shop_index;


int is_ok_char(CharData * keeper, CharData * ch, ShopData *shop);
int is_open(CharData * keeper, ShopData *shop, int msg);
int is_ok(CharData * keeper, CharData * ch, ShopData *shop);
void push(struct stack_data * stack, int pushval);
int top(struct stack_data * stack);
int pop(struct stack_data * stack);
void evaluate_operation(struct stack_data * ops, struct stack_data * vals);
int find_oper_num(char token);
int evaluate_expression(ObjData * obj, const char *expr);
int trade_with(ObjData * item, ShopData *shop);
bool same_obj(ObjData * obj1, ObjData * obj2);
bool shop_producing(ObjData * item, ShopData *shop);
int transaction_amt(char *arg);
Lexi::String times_message(ObjData * obj, char *name, int num);
int buy_price(ObjData * obj, ShopData *shop);
int sell_price(CharData * ch, ObjData * obj, ShopData *shop);
char *list_object(ObjData * obj, int cnt, int index, ShopData *shop);
int ok_shop_room(ShopData *shop, RoomData *room);
int ok_damage_shopkeeper(CharData * ch, CharData * victim);
void read_line(FILE * shop_f, char *string, Ptr data);
void boot_the_shops(FILE * shop_f, const char *filename);
void assign_the_shopkeepers(void);
char *customer_string(ShopData *shop, int detailed);
void list_all_shops(CharData * ch);
void handle_detailed_list(char *buf, char *buf1, CharData * ch);
void list_detailed_shop(CharData * ch, ShopData *shop);
void show_shops(CharData * ch, char *arg);
SPECIAL(shop_keeper);
ObjData *get_slide_obj_vis(CharData * ch, char *name, ObjectList &list);
ObjData *get_hash_obj_vis(CharData * ch, char *name, ObjectList &list);
ObjData *get_purchase_obj(CharData * ch, char *arg, CharData * keeper, ShopData *shop, int msg);
void shopping_buy(char *arg, CharData * ch, CharData * keeper, ShopData *shop);
ObjData *get_selling_obj(CharData * ch, char *name, CharData * keeper, ShopData *shop, int msg);
ObjData *slide_obj(ObjData * obj, CharData * keeper, ShopData *shop);
void shopping_sell(char *arg, CharData * ch, CharData * keeper, ShopData *shop);
void shopping_value(char *arg, CharData * ch,  CharData * keeper, ShopData *shop);
void shopping_list(char *arg, CharData * ch, CharData * keeper, ShopData *shop);
void read_objects(FILE * shop_f, std::vector<ObjectPrototype *> &list);
void read_rooms(FILE * shop_f, ShopData::RoomVector &list);
void read_type_list(FILE * shop_f, ShopBuyDataList &list);

void load_otrigger(ObjData *ch);
int get_otrigger(ObjData *obj, CharData *actor);

const char *operator_str[] = {
	"[({",
	"])}",
	"|+",
	"&*",
	"^'"
} ;
/* Constant list for printing out who we sell to */
char *trade_letters[] = {
	"Human",		/* Then the class based ones */
	"Predator",
	"Alien",
	"\n"
} ;
char *shop_bits[] = {
	"WILL_FIGHT",
	"USES_BANK",
	"\n"
} ;



int is_ok_char(CharData * keeper, CharData * ch, ShopData *shop) {
	if (keeper->CanSee(ch) != VISIBLE_FULL) {
		do_say(keeper, MSG_NO_SEE_CHAR, "say", 0);
		return (FALSE);
	}
  
	if (NO_STAFF_HASSLE(ch))
		return (TRUE);

/*  if (IS_NPC(ch) ||)
	return (TRUE);*/

	if ((IS_MARINE(ch) && NOTRADE_HUMAN(shop)) ||
			(IS_PREDATOR(ch) && NOTRADE_PREDATOR(shop)) ||
			(IS_ALIEN(ch) && NOTRADE_ALIEN(shop))) {
		BUFFER(buf, 256);
		sprintf(buf, "%s %s", ch->GetName(), MSG_NO_SELL_CLASS);
		do_tell(keeper, buf, "tell", 0);
		return (FALSE);
	}
	return (TRUE);
}


int is_open(CharData * keeper, ShopData *shop, int msg) {
	BUFFER(buf, 256);

	if (SHOP_OPEN1(shop) > time_info.hours)
		strcpy(buf, MSG_NOT_OPEN_YET);
	else if (SHOP_CLOSE1(shop) < time_info.hours)
		if (SHOP_OPEN2(shop) > time_info.hours)
			strcpy(buf, MSG_NOT_REOPEN_YET);
		else if (SHOP_CLOSE2(shop) < time_info.hours)
			strcpy(buf, MSG_CLOSED_FOR_DAY);

	if (!(*buf)) {
		return (TRUE);
	}
	if (msg)
		do_say(keeper, buf, "say", 0);
	return (FALSE);
}


int is_ok(CharData * keeper, CharData * ch, ShopData *shop) {
	if (is_open(keeper, shop, TRUE))		return (is_ok_char(keeper, ch, shop));
	else									return (FALSE);
}


void push(struct stack_data * stack, int pushval)
{
  S_DATA(stack, S_LEN(stack)++) = pushval;
}


int top(struct stack_data * stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, S_LEN(stack) - 1));
  else
    return (NOTHING);
}


int pop(struct stack_data * stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, --S_LEN(stack)));
  else {
    log("Illegal expression %d in shop keyword list", S_LEN(stack));
    return (0);
  }
}


void evaluate_operation(struct stack_data * ops, struct stack_data * vals)
{
  int oper;

  if ((oper = pop(ops)) == OPER_NOT)
    push(vals, !pop(vals));
  else if (oper == OPER_AND)
    push(vals, pop(vals) && pop(vals));
  else if (oper == OPER_OR)
    push(vals, pop(vals) || pop(vals));
}


int find_oper_num(char token)
{
  int index;

  for (index = 0; index <= MAX_OPER; index++)
    if (strchr(operator_str[index], token))
      return (index);
  return (NOTHING);
}


int evaluate_expression(ObjData * obj, const char *expr)
{
  struct stack_data ops, vals;
  const char *ptr, *end;
  int temp, index;

	if (!expr)
		return TRUE;

	if (!isalpha(*expr))
		return TRUE;

	ops.len = vals.len = 0;
	ptr = expr;
	BUFFER(name, 256);
	while (*ptr) {
		if (isspace(*ptr))
			ptr++;
		else {
			if ((temp = find_oper_num(*ptr)) == NOTHING) {
				end = ptr;
				while (*ptr && !isspace(*ptr) && (find_oper_num(*ptr) == NOTHING))
					ptr++;
				strncpy(name, end, ptr - end);
				name[ptr - end] = 0;
				for (index = 0; *extra_bits[index] != '\n'; index++)
					if (!str_cmp(name, extra_bits[index])) {
						push(&vals, OBJ_FLAGGED(obj, 1 << index));
						break;
					}
				if (*extra_bits[index] == '\n')
					push(&vals, isname(name, obj->GetAlias()));
			} else {
				if (temp != OPER_OPEN_PAREN)
					while (top(&ops) > temp)
						evaluate_operation(&ops, &vals);

				if (temp == OPER_CLOSE_PAREN) {
					if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
						log("Illegal parenthesis in shop keyword expression.");
						return (FALSE);
					}
				} else
					push(&ops, temp);
				ptr++;
			}
		}
	}
	while (top(&ops) != NOTHING)
		evaluate_operation(&ops, &vals);
	temp = pop(&vals);
	if (top(&vals) != NOTHING) {
		log("Extra operands left on shop keyword expression stack.");
		return (FALSE);
	}
	return (temp);
}


int trade_with(ObjData * item, ShopData *shop)
{
	//	Is it worth anything?  --  get_selling_obj needs this check
	if (item->TotalCost() < 1)
		return (OBJECT_NOVAL);

	//	IS it a no-sell item?
	if (OBJ_FLAGGED(item, ITEM_NOSELL))
		return (OBJECT_NOTOK);

	//	Makes sure it doesn't contain a no-sell item
	//	Moved up so that it can't be overridden by a buyword, because
	//	if it is, it would either return OBJECT_CONT_NOTOK, or OBJECT_NOTOK
	//	which is pretty useless.
	FOREACH(ObjectList, item->contents, iter)
		if (trade_with(*iter, shop) != OBJECT_OK)
			return OBJECT_CONT_NOTOK;
	
	ShopBuyDataList::iterator buydata = shop->type.begin(),
							  end =  shop->type.end();
	for (; buydata != end; ++buydata)
	{
		if (buydata->type == GET_OBJ_TYPE(item))
			if (evaluate_expression(item, buydata->keywords.c_str()))
				return (OBJECT_OK);
	}
	

	return (OBJECT_NOTOK);
}


bool same_obj(ObjData * obj1, ObjData * obj2) {
	int index;

	if (!obj1 || !obj2)
		return (obj1 == obj2);

	if (obj1->GetPrototype() != obj2->GetPrototype())
		return false;

	if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
		return false;

	if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
		return false;

	if (GET_OBJ_TOTAL_WEIGHT(obj1) != GET_OBJ_TOTAL_WEIGHT(obj2))
		return false;
	
	for (index = 0; index < 8; index++)
		if (obj1->GetValue((ObjectTypeValue)index) != obj2->GetValue((ObjectTypeValue)index))
			return false;
	
	if (obj1->m_Keywords != obj2->m_Keywords)
		return false;
	
	if (obj1->m_Name != obj2->m_Name)
		return false;
	
	if (obj1->m_RoomDescription != obj2->m_RoomDescription)
		return false;
	
//	if (obj1->m_Description != obj2->m_Description)
//		return false;
	
	
	for (index = 0; index < MAX_OBJ_AFFECT; index++)
		if ((obj1->affect[index].location != obj2->affect[index].location) ||
				(obj1->affect[index].modifier != obj2->affect[index].modifier))
			return false;

	return true;
}


bool shop_producing(ObjData * item, ShopData *shop) {
	int counter;

	if (!item->GetPrototype())
		return false;

	for (counter = 0; counter < shop->producing.size(); counter++)
		if (same_obj(item, SHOP_PRODUCT(shop, counter)->m_pProto))
			return true;

	return false;
}


int transaction_amt(char *arg) {
	int num;
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	one_argument(arg, buf);
	if (*buf)
		if ((is_number(buf))) {
			num = atoi(buf);
			strcpy(arg, arg + strlen(buf) + 1);
			return (num);
		}
	return (1);
}


Lexi::String times_message(ObjData * obj, char *name, int num)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	char *ptr;

	if (obj)
		strcpy(buf, obj->GetName());
	else {
		if ((ptr = strchr(name, '.')) == NULL)	ptr = name;
		else									ptr++;
		sprintf(buf, "%s %s", AN(ptr), ptr);
	}

	if (num > 1)
		sprintf_cat(buf, " (x %d)", num);

	return buf;
}


ObjData *get_slide_obj_vis(CharData * ch, char *name, ObjectList &list) {
  ObjData *last_match = 0;
  int j, number;
  BUFFER(tmpname, MAX_INPUT_LENGTH);
  char *tmp;

  strcpy(tmpname, name);
  tmp = tmpname;
  if (!(number = get_number(tmp))) {
    return NULL;
  }

	j = 1;
	FOREACH(ObjectList, list, iter)
	{
		ObjData *i = *iter;
		if (j > number)
			break;
		if (ch->CanSee(i) && isname(tmp, i->GetAlias()) && !same_obj(last_match, i)) {
			if (j == number) {
				
				return i;
			}
			last_match = i;
			j++;
		}
	}
	return NULL;
}


ObjData *get_hash_obj_vis(CharData * ch, char *name, ObjectList &list) {
	ObjData *last_obj = 0;
	int index;

	if ((is_number(name + 1)))		index = atoi(name + 1);
	else							return NULL;

	FOREACH(ObjectList, list, iter)
	{
		ObjData *loop = *iter;
		if (ch->CanSee(loop) && (loop->TotalCost() > 0) && !same_obj(last_obj, loop)) {
			if (--index == 0) {
				
				return loop;
			}
			last_obj = loop;
		}
	}
	return NULL;
}


ObjData *get_purchase_obj(CharData * ch, char *arg, CharData * keeper, ShopData *shop, int msg) {
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(name, MAX_INPUT_LENGTH);
	ObjData *obj;

	one_argument(arg, name);
	do {
		if (*name == '#')	obj = get_hash_obj_vis(ch, name, keeper->carrying);
		else				obj = get_slide_obj_vis(ch, name, keeper->carrying);
		if (!obj) {
			if (msg) {
				sprintf(buf, shop->no_such_item1.c_str(), ch->GetName());
				do_tell(keeper, buf, "tell", 0);
			}
			return NULL;
		}
		if (obj->TotalCost() <= 0) {
			obj->Extract();
			obj = NULL;
		}
	} while (!obj);
	return (obj);
}


int buy_price(ObjData * obj, ShopData *shop) {
	return ((int) (obj->TotalCost() * SHOP_BUYPROFIT(shop)));
}


void shopping_buy(char *arg, CharData * ch, CharData * keeper, ShopData *shop) {
	ObjData *obj, *last_obj = NULL;
	int goldamt = 0, buynum, bought = 0;

	if (!(is_ok(keeper, ch, shop)))
		return;

	if (SHOP_SORT(shop) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop);

	BUFFER(buf, MAX_STRING_LENGTH);

	if ((buynum = transaction_amt(arg)) < 0) {
		sprintf(buf, "%s A negative amount?  Try selling me something.", ch->GetName());
		do_tell(keeper, buf, "tell", 0);
		return;
	}
	if (!(*arg) || !(buynum)) {
		sprintf(buf, "%s What do you want to buy??", ch->GetName());
		do_tell(keeper, buf, "tell", 0);
		return;
	}
	if (!(obj = get_purchase_obj(ch, arg, keeper, shop, TRUE))) {
		return;
	}  


	if ((buy_price(obj, shop) > GET_MISSION_POINTS(ch)) && !NO_STAFF_HASSLE(ch)) {
		sprintf(buf, shop->missing_cash2.c_str(), ch->GetName());
		do_tell(keeper, buf, "tell", 0);

		switch (SHOP_BROKE_TEMPER(shop)) {
			case 0:
				do_action(keeper, const_cast<char *>(ch->GetName()), "puke", 0);
				break;
			case 1:
				do_echo(keeper, "smokes on his joint.", "emote", SCMD_EMOTE);
				break;
			default:
				break;
		}
		return;
	}
/*
  if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))) {
    sprintf(buf, "%s: You can't carry any more items.\n", fname(SSData(obj->name)));
    send_to_char(buf, ch);
    return;
  }
  if ((IS_CARRYING_W(ch) + GET_OBJ_TOTAL_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
    sprintf(buf, "%s: You can't carry that much weight.\n", fname(SSData(obj->name)));
    send_to_char(buf, ch);
    return;
  }
*/
	if (!can_take_obj(ch, obj)) {
		return;
	}
	ObjData *original = obj;
	while ((obj) && ((GET_MISSION_POINTS(ch) >= buy_price(obj, shop)) || NO_STAFF_HASSLE(ch))
			&& (bought < buynum))
	{
		if (!get_otrigger(obj, ch) || PURGED(obj) || obj->CarriedBy() != keeper)
			return;
		
		bought++;
		
		/* Test if producing shop ! */
		if (shop_producing(obj, shop))
		{
//			obj = create_obj(obj);
			obj = ObjData::Create(original->GetVirtualID());
			load_otrigger(obj);
		}
		else
		{
			obj->FromChar();
			SHOP_SORT(shop)--;
		}
		obj->ToChar(ch);
		obj->SetBuyer(ch);

		goldamt += buy_price(obj, shop);
		if (!NO_STAFF_HASSLE(ch))
			GET_MISSION_POINTS(ch) -= buy_price(obj, shop);

		last_obj = obj;
		obj = get_purchase_obj(ch, arg, keeper, shop, FALSE);
		if (!same_obj(obj, last_obj))
			break;
	}

	if (bought < buynum) {
		if (!obj || !same_obj(last_obj, obj))
			sprintf(buf, "%s I only have %d to sell you.", ch->GetName(), bought);
		else if (GET_MISSION_POINTS(ch) < buy_price(obj, shop))
			sprintf(buf, "%s You can only afford %d.", ch->GetName(), bought);
		else
			sprintf(buf, "%s Something screwy only gave you %d.", ch->GetName(), bought);
		do_tell(keeper, buf, "tell", 0);
	}
//  if (!NO_STAFF_HASSLE(ch))
//    GET_MISSION_POINTS(keeper) += goldamt;
	BUFFER(tempstr, MAX_STRING_LENGTH);

	sprintf(tempstr, times_message(ch->carrying.front(), 0, bought).c_str());
	sprintf(buf, "$n buys %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, shop->message_buy.c_str(), ch->GetName(), goldamt);
	do_tell(keeper, buf, "tell", 0);
	sprintf(buf, "You now have %s.\n", tempstr);
	send_to_char(buf, ch);

	if (ch->GetLevel() >= 101)
		mudlogf(NRM, LVL_SRSTAFF, TRUE, "(GC) %s purchased %s.", ch->GetName(), ch->carrying.front()->GetName());

//  if (SHOP_USES_BANK(shop_nr))
//    if (GET_MISSION_POINTS(keeper) > MAX_OUTSIDE_BANK) {
//      SHOP_BANK(shop_nr) += (GET_MISSION_POINTS(keeper) - MAX_OUTSIDE_BANK);
//      GET_MISSION_POINTS(keeper) = MAX_OUTSIDE_BANK;
//    }
}


ObjData *get_selling_obj(CharData * ch, char *name, CharData * keeper, ShopData *shop, int msg)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	ObjData		*obj;
	int			result;

	if (!(obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
		if (msg) {
			sprintf(buf, shop->no_such_item2.c_str(), ch->GetName());
			do_tell(keeper, buf, "tell", 0);
		}
		return NULL;
	}
	if ((result = trade_with(obj, shop)) == OBJECT_OK)
		return (obj);

	switch (result) {
		case OBJECT_NOVAL:
			sprintf(buf, "%s You've got to be kidding, that thing is worthless!", ch->GetIDString());
			break;
		case OBJECT_NOTOK:
			sprintf(buf, shop->do_not_buy.c_str(), ch->GetIDString());
			break;
		case OBJECT_DEAD:
			sprintf(buf, "%s %s", ch->GetIDString(), MSG_NO_USED_WANDSTAFF);
			break;
		case OBJECT_CONT_NOTOK:
			sprintf(buf, "%s I won't buy the contents of it!", ch->GetIDString());
			break;
		default:
			log("Illegal return value of %d from trade_with() (" __FILE__ ")", result);
			sprintf(buf, "%s An error has occurred.", ch->GetIDString());
			break;
	}
	if (msg)	do_tell(keeper, buf, "tell", 0);
	return NULL;
}


int sell_price(CharData * ch, ObjData * obj, ShopData *shop) {
	return ((int) (obj->TotalCost(ch) * SHOP_SELLPROFIT(shop)));
}


//	Finally rewrote to sort.  With the InsertBefore() function added to
//	the LList template, it became MUCH easier!
ObjData *slide_obj(ObjData * obj, CharData * keeper, ShopData *shop) {
	if (SHOP_SORT(shop) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop);

	/* Extract the object if it is identical to one produced */
	if (shop_producing(obj, shop))
	{
		ObjData *temp = obj->GetPrototype()->m_pProto;
		obj->Extract();
		return temp;
	}
	SHOP_SORT(shop)++;
	obj->ToChar(keeper);			//	Move it to the keeper...
	keeper->carrying.remove(obj);	//	But we want to re-order it

	FOREACH(ObjectList, keeper->carrying, iter)
	{
		ObjData *loop = *iter;
		if (same_obj(obj, loop)) {
			keeper->carrying.insert(iter, obj);
			return obj;
		}
	}
	keeper->carrying.push_front(obj);
	return obj;
}


void sort_keeper_objs(CharData * keeper, ShopData *shop) {
	ObjData				*temp;
	ObjectList			list;
	
	while (SHOP_SORT(shop) < IS_CARRYING_N(keeper)) {
		temp = keeper->carrying.front();
		temp->FromChar();
		list.push_front(temp);
	}

	while (!list.empty())
	{
		temp = list.front();
		list.pop_front();
		if ((shop_producing(temp, shop)) && !(get_obj_in_list_num(temp->GetVirtualID(), keeper->carrying)))
		{
			temp->ToChar(keeper);
			SHOP_SORT(shop)++;
		}
		else
			slide_obj(temp, keeper, shop);
	}
}


void shopping_sell(char *arg, CharData * ch, CharData * keeper, ShopData *shop)
{
	ObjData *obj, *tag = 0;
	int sellnum, sold = 0, goldamt = 0;

	if (!(is_ok(keeper, ch, shop)))
		return;

	BUFFER(buf, SMALL_BUFSIZE);
	if ((sellnum = transaction_amt(arg)) < 0) {
		sprintf(buf, "%s A negative amount?  Try buying something.", ch->GetName());
		do_tell(keeper, buf, "tell", 0);
		return;
	}
	if (!*arg || !sellnum) {
		sprintf(buf, "%s What do you want to sell??", ch->GetName());
		do_tell(keeper, buf, "tell", 0);
		return;
	}
	BUFFER(name, MAX_INPUT_LENGTH);
	one_argument(arg, name);

	obj = get_selling_obj(ch, name, keeper, shop, TRUE);
	if (!obj) {
		return;
	} /*else if (!ch->CanUse(obj)) {
		act("$p: you can't sell something you don't understand.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}*/
	
//	if (GET_MISSION_POINTS(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, obj, shop_nr)) {
//		sprintf(buf, shop_index[shop_nr].missing_cash1, ch->GetName());
//		do_tell(keeper, buf, cmd_tell, 0);
//		return;
//	}
	while ((obj) && /* (GET_MISSION_POINTS(keeper) + SHOP_BANK(shop_nr) >=
			sell_price(ch, obj, shop_nr)) && */ (sold < sellnum)) {
		sold++;

		goldamt += sell_price(ch, obj, shop);
//		GET_MISSION_POINTS(keeper) -= sell_price(ch, obj, shop_nr);

		obj->FromChar();
		tag = slide_obj(obj, keeper, shop);
		obj = get_selling_obj(ch, name, keeper, shop, FALSE);
	}

	if (sold < sellnum) {
		if (!obj)
			sprintf(buf, "%s You only have %d of those.", ch->GetName(), sold);
//		else if (GET_MISSION_POINTS(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, obj, shop_nr))
//			sprintf(buf, "%s I can only afford to buy %d of those.", ch->GetName(), sold);
		else
			sprintf(buf, "%s Something really screwy made me buy %d.", ch->GetName(), sold);

		do_tell(keeper, buf, "tell", 0);
	}
	BUFFER(tempstr, MAX_STRING_LENGTH);

	GET_MISSION_POINTS(ch) += goldamt;
	strcpy(tempstr, times_message(0, name, sold).c_str());
	sprintf(buf, "$n sells %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, shop->message_sell.c_str(), ch->GetName(), goldamt);
	do_tell(keeper, buf, "tell", 0);
	sprintf(buf, "The shopkeeper now has %s.\n", tempstr);
	send_to_char(buf, ch);

//	if (GET_MISSION_POINTS(keeper) < MIN_OUTSIDE_BANK) {
//		goldamt = MIN(MAX_OUTSIDE_BANK - GET_MISSION_POINTS(keeper), SHOP_BANK(shop_nr));
//		SHOP_BANK(shop_nr) -= goldamt;
//		GET_MISSION_POINTS(keeper) += goldamt;
//	}
}


void shopping_value(char *arg, CharData * ch, CharData * keeper, ShopData *shop)
{
  BUFFER(buf, MAX_INPUT_LENGTH);
  ObjData *obj;

  if (!(is_ok(keeper, ch, shop)))
    return;

  if (!(*arg)) {
    sprintf(buf, "%s What do you want me to valuate??", ch->GetName());
    do_tell(keeper, buf, "tell", 0);
    return;
  }
  one_argument(arg, buf);
  obj = get_selling_obj(ch, buf, keeper, shop, TRUE);
  if (!obj)
    return;

  sprintf(buf, "%s I'll give you %d MP for that!", ch->GetName(), sell_price(ch, obj, shop));
  do_tell(keeper, buf, "tell", 0);

  return;
}


char *list_object(ObjData * obj, int cnt, int index, ShopData *shop)
{
	static char buf[256];
	BUFFER(buf2, MAX_STRING_LENGTH);
	BUFFER(buf3, MAX_STRING_LENGTH);
	int		contentCount;

	if (obj->GetMinimumLevelRestriction() <= 1)		strcpy(buf2, "       ");
	else								sprintf(buf2, "%3d    ", obj->GetMinimumLevelRestriction());
	snprintf(buf, sizeof(buf), " %2d) %s", index, buf2);

	/* Compile object name and information */
	strcpy(buf3, obj->GetName());
	if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && obj->GetValue(OBJVAL_FOODDRINK_CONTENT))
		sprintf_cat(buf3, " of %s", drinks[obj->GetValue(OBJVAL_FOODDRINK_TYPE)]);
	else if ((contentCount = obj->contents.size()))
		sprintf_cat(buf3, " (%d content%s)", contentCount, ((contentCount > 1) ? "s" : ""));

	/* FUTURE: */
	/* Add glow/hum/etc */

	int len = cstrnlen_for_sprintf(buf3, 52);
	sprintf(buf2, "%-*.*s`n %6d\n", len, len, buf3, buy_price(obj, shop));
	strncat(buf, CAP(buf2), sizeof(buf) - strlen(buf));
	buf[sizeof(buf) - 1] = 0;

	return (buf);
}


void shopping_list(char *arg, CharData * ch, CharData * keeper, ShopData *shop) {
	ObjData *last_obj = NULL;
	int cnt = 0, index = 0;

	if (!(is_ok(keeper, ch, shop)))
		return;
    

	if (SHOP_SORT(shop) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop);

	BUFFER(buf, MAX_STRING_LENGTH * 4);
	BUFFER(name, MAX_STRING_LENGTH);
  
	one_argument(arg, name);

	strcpy(buf, "\n ##   Lvl   Item                                                Cost (MP)\n");
	strcat(buf, "---------------------------------------------------------------------------\n");
	FOREACH(ObjectList, keeper->carrying, iter)
	{
		ObjData *obj = *iter;
		
		if (ch->CanSee(obj) && (obj->TotalCost() > 0)) {
			if (!last_obj) {
				last_obj = obj;
				cnt = 1;
			} else if (same_obj(last_obj, obj))
				cnt++;
			else {
				index++;
				if (!(*name) || isname(name, last_obj->GetAlias()))
					strcat(buf, list_object(last_obj, cnt, index, shop));
				cnt = 1;
				last_obj = obj;
			}
		}
		
		if (strlen(buf) > ((MAX_STRING_LENGTH * 4) - 1024))
		{
			strcat(buf, "***OVERFLOW***\n");
			break;
		}
	}
	index++;
	if (!last_obj) {
		if (*name)	strcpy(buf, "Presently, none of those are for sale.\n");
		else		strcpy(buf, "Currently, there is nothing for sale.\n");
	} else if (!(*name) || isname(name, last_obj->GetAlias()))
		strcat(buf, list_object(last_obj, cnt, index, shop));

	page_string(ch->desc, buf);
}


int ok_shop_room(ShopData *shop, RoomData *room)
{
	FOREACH(ShopData::RoomVector, shop->in_room, iter)
		if (*iter == room->GetVirtualID())
			return (TRUE);
	
	return (FALSE);
}


SPECIAL(shop_keeper)
{
	CharData *keeper = (CharData *) me;
	
	ShopData *shop = NULL;
	for (int shop_nr = 0; shop_nr < shop_index.size(); ++shop_nr)
	{
		if (SHOP_KEEPER(shop_index[shop_nr]) == keeper->GetPrototype())
		{
			shop = shop_index[shop_nr];
			break;
		}
	}

	if (!shop)
 		return (FALSE);

//  if (SHOP_FUNC(shop_nr))	/* Check secondary function */
//    if ((SHOP_FUNC(shop_nr)) (ch, me, cmd, NULL))
//      return (TRUE);

  if (keeper == ch) {
    if (cmd)
      SHOP_SORT(shop) = 0;	/* Safety in case "drop all" */
    return (FALSE);
  }
  if (!ok_shop_room(shop, ch->InRoom()))
    return (0);

  if (!AWAKE(keeper))
    return (FALSE);

  if (CMD_IS("buy")) {
    shopping_buy(argument, ch, keeper, shop);
    return (TRUE);
  } else if (CMD_IS("sell")) {
    shopping_sell(argument, ch, keeper, shop);
    return (TRUE);
  } else if (CMD_IS("value")) {
    shopping_value(argument, ch, keeper, shop);
    return (TRUE);
  } else if (CMD_IS("list")) {
    shopping_list(argument, ch, keeper, shop);
    return (TRUE);
  }
  return (FALSE);
}


int ok_damage_shopkeeper(CharData * ch, CharData * victim) {
//	if (IS_NPC(victim) && (mob_index[GET_MOB_RNUM(victim)].func == shop_keeper))
//		return (FALSE);
	return (TRUE);
}


void read_line(FILE * shop_f, char *string, Ptr data)
{
  BUFFER(buf, MAX_STRING_LENGTH);
  if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
//    log("Error in shop #%d", shop_index.back()->vnum);
//    exit(1);
  }
}


void read_objects(FILE * shop_f, std::vector<ObjectPrototype *> &list)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	while (1)
	{
		*buf = 0;
		read_line(shop_f, "%s", buf);
				
		if (!str_cmp(buf, "-1"))
			break;
		
		ObjectPrototype *proto = obj_index.Find(buf);
		if (!proto)
			continue;
		
		list.push_back(proto);
	}
}

void read_rooms(FILE * shop_f, ShopData::RoomVector &list)
{
	BUFFER(buf, MAX_STRING_LENGTH);
		
	while (1)
	{
		*buf = 0;
		read_line(shop_f, "%s", buf);
		
		if (!str_cmp(buf, "-1"))
			break;
		
		VirtualID	vid(buf);
		
		if (world.Find(vid))
			list.push_back(vid);
	}
}


void read_type_list(FILE * shop_f, ShopBuyDataList & list)
{
	char *ptr;

	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(arg, MAX_STRING_LENGTH);
	while (1)
	{
		fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
		if ((ptr = strchr(buf, ';')) != NULL)	*ptr = 0;
		else									*(END_OF(buf) - 1) = 0;
		
		char *argument = any_one_arg(buf, arg);
		int type = GetEnumByName<ObjectType>(arg);
		
		ptr = buf;
		if (type == -1)
		{
			type = atoi(buf);
			while (!isdigit(*ptr))	ptr++;
			while (isdigit(*ptr))	ptr++;
			
			if ((unsigned int)type >= NUM_ITEM_TYPES)	break;
		}
		else
		{
			ptr = argument;		
		}
		
		while (isspace(*ptr))				ptr++;
		while (isspace(*(END_OF(ptr) - 1)))	*(END_OF(ptr) - 1) = 0;
		
		if (type != -1)
		{
			list.push_back(ShopBuyData((ObjectType)type, ptr));
		}
	}
}


void boot_the_shops(FILE * shop_f, const char *filename)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(buf2, MAX_STRING_LENGTH);
	int done = 0;

	sprintf(buf2, "beginning of shop file %s", filename);

	while (!done) {
		Lexi::String str = fread_lexistring(shop_f, buf2, filename);
		if (*str.c_str() == '#') {		/* New shop */
			sscanf(str.c_str(), "#%s\n", buf);
			sprintf(buf2, "shop %s in shop file %s", buf, filename);

			ShopData *shop = new ShopData(VirtualID(buf));
			shop_index.insert(shop);
			
			read_objects(shop_f, shop->producing);
			
			read_line(shop_f, "%f", &shop->profit_buy);
			read_line(shop_f, "%f", &shop->profit_sell);

			read_type_list(shop_f, shop->type);

			shop->no_such_item1 = fread_lexistring(shop_f, buf2, filename);
			shop->no_such_item2 = fread_lexistring(shop_f, buf2, filename);
			shop->do_not_buy = fread_lexistring(shop_f, buf2, filename);
			shop->missing_cash1 = fread_lexistring(shop_f, buf2, filename);
			shop->missing_cash2 = fread_lexistring(shop_f, buf2, filename);
			shop->message_buy = fread_lexistring(shop_f, buf2, filename);
			shop->message_sell = fread_lexistring(shop_f, buf2, filename);
			read_line(shop_f, "%d", &shop->temper1);
			read_line(shop_f, "%d", &shop->bitvector);
			
			read_line(shop_f, "%s", buf);
			shop->keeper = mob_index.Find(buf);
			
			read_line(shop_f, "%d", &shop->with_who);

			read_rooms(shop_f, shop->in_room);

			read_line(shop_f, "%d", &shop->open1);
			read_line(shop_f, "%d", &shop->close1);
			read_line(shop_f, "%d", &shop->open2);
			read_line(shop_f, "%d", &shop->close2);

			shop->bankAccount = 0;
			shop->lastsort = 0;
//			SHOP_FUNC(top_shop) = 0;
		}
		else if (*str.c_str() == '$')		/* EOF */
			done = TRUE;
	}
}


void assign_the_shopkeepers(void)
{
  int index;

  for (index = 0; index < shop_index.size(); index++) {
    if (SHOP_KEEPER(shop_index[index]) == NULL)
      continue;
//    if (mob_index[SHOP_KEEPER(index)]->m_Function)
//      SHOP_FUNC(index) = mob_index[SHOP_KEEPER(index)]->m_Function;
    SHOP_KEEPER(shop_index[index])->m_Function = shop_keeper;
  }
}


char *customer_string(ShopData *shop, int detailed) {
	int index, cnt = 1;
	static char buf[256];

	*buf = '\0';
	for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
		if (!(SHOP_TRADE_WITH(shop) & cnt))
			if (detailed) {
				if (*buf)
					strcat(buf, ", ");
				strcat(buf, trade_letters[index]);
			} else
				sprintf_cat(buf, "%c", *trade_letters[index]);
		else if (!detailed)
			strcat(buf, "_");

	return (buf);
}


void list_all_shops(CharData * ch)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	int itemsPerPage = ch->GetPlayer()->m_PageLength - 2;
	if (itemsPerPage < 0)	itemsPerPage = INT_MAX;

	int shop_nr = 0;
	FOREACH(ShopMap, shop_index, iter)
	{
		ShopData *shop = *iter;
		
		if ((shop_nr % itemsPerPage) == 0)
		{
			strcat(buf, " ##   Virtual      Where        Keeper       Buy   Sell   Customers\n");
			strcat(buf, "-------------------------------------------------------------------\n");
		}
		sprintf_cat(buf, "%3d   %10s   %10s   %10s   %3.2f   %3.2f    %s\n",
				++shop_nr,
				shop->GetVirtualID().Print().c_str(),
				shop->in_room.empty() ? "<NOWHERE>" : shop->in_room.front().Print().c_str(),
				SHOP_KEEPER(shop) ? SHOP_KEEPER(shop)->GetVirtualID().Print().c_str() : "<NONE>",
				SHOP_SELLPROFIT(shop), SHOP_BUYPROFIT(shop),
				customer_string(shop, FALSE));
	}
	/*
	* There is a reason for putting these releases above the page_string.
	* If page_string needs memory, we can use these instead of causing
	* it to potentially have to create another buffer.
	*/
	page_string(ch->desc, buf);
}


void handle_detailed_list(char *buf, char *buf1, CharData * ch)
{
  if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
    strcat(buf, buf1);
  else {
    strcat(buf, "\n");
    send_to_char(buf, ch);
    sprintf(buf, "            %s", buf1);
  }
}


void list_detailed_shop(CharData * ch, ShopData *shop)
{
  ObjData *obj;
  CharData *k;
  int index;
  BUFFER(buf, MAX_STRING_LENGTH);
  BUFFER(buf1, MAX_STRING_LENGTH);

  sprintf(buf, "Virtual:    [%10s]\n", shop->GetVirtualID().Print().c_str());
  send_to_char(buf, ch);

  strcpy(buf, "Rooms:      ");
  FOREACH(ShopData::RoomVector, shop->in_room, iter)
  {
    if (iter == shop->in_room.begin())
      strcat(buf, ", ");
    RoomData *room = world.Find(*iter);
    if (room)
      sprintf(buf1, "%s [%s]", room->GetName(), room->GetVirtualID().Print().c_str());
    else
      sprintf(buf1, "<UNKNOWN> [%s]", iter->Print().c_str());
    handle_detailed_list(buf, buf1, ch);
  }
  if (shop->in_room.empty())
    send_to_char("Rooms:      None!\n", ch);
  else {
    strcat(buf, "\n");
    send_to_char(buf, ch);
  }

  strcpy(buf, "Shopkeeper: ");
  if (SHOP_KEEPER(shop)) {
    sprintf_cat(buf, "%s [%s], Special Function: %s\n",
	    SHOP_KEEPER(shop)->m_pProto->GetName(),
	SHOP_KEEPER(shop)->GetVirtualID().Print().c_str(), YESNO(/*SHOP_FUNC(shop)*/ 0));
    if ((k = get_char_num(SHOP_KEEPER(shop)->GetVirtualID()))) {
      send_to_char(buf, ch);
      sprintf(buf, "Coins:      [%9d], Bank: [%9d] (Total: %d)\n",
	 GET_MISSION_POINTS(k), SHOP_BANK(shop), GET_MISSION_POINTS(k) + SHOP_BANK(shop));
    }
  } else
    strcat(buf, "<NONE>\n");
  send_to_char(buf, ch);

  strcpy(buf1, customer_string(shop, TRUE));
  sprintf(buf, "Customers:  %s\n", (*buf1) ? buf1 : "None");
  send_to_char(buf, ch);

  strcpy(buf, "Produces:   ");
  for (index = 0; index < shop->producing.size(); index++) {
    obj = SHOP_PRODUCT(shop, index)->m_pProto;
    if (index)
      strcat(buf, ", ");
    sprintf(buf1, "%s [%s]", obj->GetName(), SHOP_PRODUCT(shop, index)->GetVirtualID().Print().c_str());
    handle_detailed_list(buf, buf1, ch);
  }
  if (!index)
    send_to_char("Produces:   Nothing!\n", ch);
  else {
    strcat(buf, "\n");
    send_to_char(buf, ch);
  }

	strcpy(buf, "Buys:       ");
	ShopBuyDataList::iterator	begin = shop->type.begin(),
								end = shop->type.end();
	for (ShopBuyDataList::iterator buydata = begin; buydata != end; ++buydata)
	{
		if (buydata != begin)	strcat(buf, ", ");
		sprintf(buf1, "%s ", GetEnumName(buydata->type));
		if (!buydata->keywords.empty())		sprintf_cat(buf1, "[%s]", buydata->keywords.c_str());
		else								strcat(buf1, "[all]");
		handle_detailed_list(buf, buf1, ch);
	}
	if (begin == end)
		send_to_char("Buys:       Nothing!\n", ch);
	else {
		strcat(buf, "\n");
		send_to_char(buf, ch);
	}

  sprintf(buf, "Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]%s",
     SHOP_SELLPROFIT(shop), SHOP_BUYPROFIT(shop), SHOP_OPEN1(shop),
   SHOP_CLOSE1(shop), SHOP_OPEN2(shop), SHOP_CLOSE2(shop), "\n");

  send_to_char(buf, ch);

  sprintbit((long) SHOP_BITVECTOR(shop), shop_bits, buf1);
  sprintf(buf, "Bits:       %s\n", buf1);
  send_to_char(buf, ch);
}


void show_shops(CharData * ch, char *arg)
{
  int shop_nr;

  if (!*arg)
    list_all_shops(ch);
  else {
    if (!str_cmp(arg, ".")) {
      for (shop_nr = 0; shop_nr < shop_index.size(); shop_nr++)
	if (ok_shop_room(shop_index[shop_nr], ch->InRoom()))
	  break;

      if (shop_nr == shop_index.size()) {
	send_to_char("This isn't a shop!\n", ch);
	return;
      }
    } else if (is_number(arg))
      shop_nr = atoi(arg) - 1;
    else
      shop_nr = -1;

    if ((shop_nr < 0) || (shop_nr >= shop_index.size())) {
      send_to_char("Illegal shop number.\n", ch);
      return;
    }
    list_detailed_shop(ch, shop_index[shop_nr]);
  }
}



ShopData::ShopData(VirtualID v)
:	m_Virtual(v)
,	profit_buy(1.0f)
,	profit_sell(1.0f)
,	no_such_item1("%s Sorry, I don't stock that item.")
,	no_such_item2("%s You don't seem to have that.")
,	missing_cash1("%s I can't afford that!")
,	missing_cash2("%s You are too poor!")
,	do_not_buy("%s I don't trade in such items.")
,	message_buy("%s That'll be %d coins, thanks.")
,	message_sell("%s I'll give you %d coins for that.")
,	temper1(0)
,	bitvector(0)
,	keeper(NULL)
,	with_who(0)
,	open1(0)
,	open2(0)
,	close1(28)
,	close2(0)
,	bankAccount(0)
,	lastsort(0)
{
}

#if 0
void ShopData::Parse(Lexi::Parser &parser, VirtualID nr)
{
	ShopData *shop = new ShopData(nr);
	
	shop_index.insert(shop);
	
	//	read_objects
	
	shop->profit_buy			= parser.GetFloat("BuyMultiplier");
	shop->profit_buy			= parser.GetFloat("SellMultiplier");
	
	//	read_type_list
	shop->no_such_item1			= parser.GetString("Messages/NoSuchItem1");
	shop->no_such_item2			= parser.GetString("Messages/NoSuchItem2");
	shop->do_not_buy			= parser.GetString("Messages/DoNotBuy");
	shop->missing_cash1			= parser.GetString("Messages/MissingCash1");
	shop->missing_cash2			= parser.GetString("Messages/MissingCash2");
	shop->message_buy			= parser.GetString("Messages/Buy");
	shop->message_sell			= parser.GetString("Messages/Sell");

	shop->temper1				= parser.GetInteger("Temper");
	shop->bitvector				= parser.GetFlags("Flags", shop_bits);
	shop->keeper				= mob_index.Find(parser.GetString("Keeper"));
	shop->with_who				= parser.GetInteger("NoTrade");
}
#endif


ShopMap::ShopMap()
:	m_Allocated(0)
,	m_Size(0)
,	m_bDirty(false)
,	m_Index(NULL)
{
}


ShopMap::~ShopMap()
{
	delete [] m_Index;
}


bool ShopMap::SortFunc(value_type lhs, value_type rhs)
{
//	if (lhs->GetVirtualID() < rhs->GetVirtualID())	return true;
//	if (lhs->GetVirtualID() == rhs->GetVirtualID())	return lhs->GetID() < rhs->GetID();
//	return false;
	return lhs->GetVirtualID() < rhs->GetVirtualID();
}


ShopMap::value_type ShopMap::Find(VirtualID vid)
{
	if (m_Size == 0)		return NULL;
	if (!vid.IsValid())		return NULL;
	
	if (m_bDirty)		Sort();
	
//	unsigned long long ull = vid.as_ull();
	
	int	 bot = 0;
	int top = m_Size - 1;

	do
	{
		int mid = (bot + top) / 2;
		
		value_type i = m_Index[mid];
//		unsigned long long	vull = i->GetVirtualID().as_ull();
		const VirtualID &ivid = i->GetVirtualID();
		
//		if (vull == ull)	return i;
		if (ivid == vid)	return i;
		if (bot >= top)		break;
//		if (vull > ull)		top = mid - 1;
		if (ivid > vid)		top = mid - 1;
		else				bot = mid + 1;
	} while (1);
	
	ZoneData *z = VIDSystem::GetZoneFromAlias(vid.zone);
	if (z && z->GetHash() != vid.zone)
	{
		vid.zone = z->GetHash();
		return Find(vid);
	}

	return NULL;
}


ShopMap::value_type ShopMap::Find(const char *name)
{
	return IsVirtualID(name) ? Find(VirtualID(name)) : NULL;
}


static bool ShopMapLowerBoundFunc(const ShopData *s, Hash zone)
{
	return (s->GetVirtualID().zone < zone);
}


static bool ShopMapUpperBoundFunc(Hash zone, const ShopData *s)
{
	return (zone < s->GetVirtualID().zone);
}


ShopMap::iterator ShopMap::lower_bound(Hash zone)
{
//	if (m_Size == 0)	return m_Index;
	
	if (m_bDirty)		Sort();
	
	return std::lower_bound(m_Index, m_Index + m_Size, zone, ShopMapLowerBoundFunc);
}


ShopMap::iterator ShopMap::upper_bound(Hash zone)
{
//	if (m_Size == 0)	return m_Index;
	
	if (m_bDirty)		Sort();
	
	return std::upper_bound(m_Index, m_Index + m_Size, zone, ShopMapUpperBoundFunc);
}


void ShopMap::renumber(Hash oldHash, Hash newHash)
{
	if (m_bDirty)		Sort();
	
	FOREACH_BOUNDED(ShopMap, *this, oldHash, i)
		(*i)->m_Virtual.zone = newHash;
	
	m_bDirty = true;
	Sort();
}


void ShopMap::insert(value_type i)
{
	if (m_Size == m_Allocated)
	{
		//	Grow: increase alloc size, alloc new, copy old to new, delete old, reassign
		m_Allocated += msGrowSize;
		value_type *newIndex = new value_type[m_Allocated];
		std::copy(m_Index, m_Index + m_Size, newIndex);
		delete [] m_Index;
		m_Index = newIndex;
	}
	
	//	Mark as dirty if the item being added would fall before the end of the entries
	if (!m_bDirty
		&& m_Size > 0
		&& i->GetVirtualID() < m_Index[m_Size - 1]->GetVirtualID())
	{
		m_bDirty = true;
	}

	//	Add new item to end
	m_Index[m_Size] = i;
	++m_Size;
}


void ShopMap::Sort()
{
	if (!m_bDirty)	return;
	
	std::sort(m_Index, m_Index + m_Size, SortFunc);
	
	m_bDirty = false;
}
