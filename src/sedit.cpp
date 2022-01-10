/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - sedit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "buffer.h"
#include "constants.h"
#include "interpreter.h"

/*
 * Handy macros.
 */
#define S_NUM(i)		((i)->vnum)
#define S_KEEPER(i)		((i)->keeper)
#define S_OPEN1(i)		((i)->open1)
#define S_CLOSE1(i)		((i)->close1)
#define S_OPEN2(i)		((i)->open2)
#define S_CLOSE2(i)		((i)->close2)
#define S_BANK(i)		((i)->bankAccount)
#define S_BROKE_TEMPER(i)	((i)->temper1)
#define S_BITVECTOR(i)		((i)->bitvector)
#define S_NOTRADE(i)		((i)->with_who)
#define S_SORT(i)		((i)->lastsort)
#define S_BUYPROFIT(i)		((i)->profit_buy)
#define S_SELLPROFIT(i)		((i)->profit_sell)
#define S_FUNC(i)		((i)->func)

#define S_ROOMS(i)		((i)->in_room)
#define S_PRODUCTS(i)		((i)->producing)
#define S_NAMELISTS(i)		((i)->type)
#define S_PRODUCT(i, num)	((i)->producing[(num)])
#define S_BUYTYPE(i, num)	(BUY_TYPE((i)->type[(num)]))
#define S_BUYWORD(i, num)	(BUY_WORD((i)->type[(num)]))

#define S_NOITEM1(i)		((i)->no_such_item1)
#define S_NOITEM2(i)		((i)->no_such_item2)
#define S_NOCASH1(i)		((i)->missing_cash1)
#define S_NOCASH2(i)		((i)->missing_cash2)
#define S_NOBUY(i)		((i)->do_not_buy)
#define S_BUY(i)		((i)->message_buy)
#define S_SELL(i)		((i)->message_sell)

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
void sedit_setup(DescriptorData *d, ShopData *shop);
void sedit_parse(DescriptorData *d, const char *arg);
void sedit_disp_menu(DescriptorData *d);
void sedit_namelist_menu(DescriptorData *d);
void sedit_types_menu(DescriptorData *d);
void sedit_products_menu(DescriptorData *d);
void sedit_rooms_menu(DescriptorData *d);
void sedit_compact_rooms_menu(DescriptorData *d);
void sedit_shop_flags_menu(DescriptorData *d);
void sedit_no_trade_menu(DescriptorData *d);
void sedit_save_internally(DescriptorData *d);
void sedit_save_to_disk(ZoneData *zone);
void sedit_remove_from_type_list(ShopBuyDataList &list, int num);
void sedit_modify_string(Lexi::String &str, const char *new_t);

/*
 * External functions.
 */
SPECIAL(shop_keeper);

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

void sedit_setup(DescriptorData *d, ShopData *shop)
{
	if (shop)		OLC_SHOP(d) = new ShopData(*shop);
	else			OLC_SHOP(d) = new ShopData(OLC_VID(d));
	sedit_disp_menu(d);
}


/*-------------------------------------------------------------------*/


void sedit_remove_from_type_list(ShopBuyDataList &list, int num) {
	/*. Count number of entries .*/
	if (num < 0)
		return;

	ShopBuyDataList::iterator	buydata = list.begin(),
								end = list.end();
	
	while (num >= 0 && buydata != end)
	{
		++buydata;
		--num;
	}
	
	if (buydata != end)
		list.erase(buydata);
}


//	Generic string modifier for shop keeper messages
void sedit_modify_string(Lexi::String &str, const char *new_t) {
	BUFFER(pointer, MAX_STRING_LENGTH);
	char *orig = pointer;
	
	/*. Check the '%s' is present, if not, add it .*/
	if (*new_t != '%') {
		strcpy(pointer, "%s ");
		strcat(pointer, new_t);
//		pointer = buf;
	} else
		pointer = const_cast<char *>(new_t);
	str = pointer;
}

/*-------------------------------------------------------------------*/

void sedit_save_internally(DescriptorData *d)
{
	ShopData *origShop = shop_index.Find(OLC_VID(d));
	ShopData *shop = OLC_SHOP(d)/*.Get()*/;

	if(origShop)		//	The shop already exists, just update it
	{
		*origShop = *shop;
	}
	else
	{
		shop_index.insert(new ShopData(*shop));
	}
	
	if (S_KEEPER(shop))
	{
		S_KEEPER(shop)->m_Function = shop_keeper;
	}
	
	sedit_save_to_disk(OLC_SOURCE_ZONE(d));
}

        
void sedit_save_to_disk(ZoneData *zone)
{
	FILE *shop_file;
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(fname, MAX_STRING_LENGTH);

	sprintf(fname, SHP_PREFIX "%s.new", zone->GetTag());

	if(!(shop_file = fopen(fname, "w"))) {
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open shop file %s!", fname);
		return;
	}
	if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0) {
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot write to shop file %s!", fname);
		fclose(shop_file);
		return;
	}

	/*. Search database for shops in this zone .*/
	FOREACH_BOUNDED(ShopMap, shop_index, zone->GetHash(), i)
	{
		ShopData *shop = *i;
	
		fprintf(shop_file, "#%s~\n", shop->GetVirtualID().Print().c_str());

		/*
		 * Save products.
		 */
		for (int j = 0; j < shop->producing.size(); ++j)
			fprintf(shop_file, "%s\n", S_PRODUCT(shop, j)->GetVirtualID().Print().c_str());
		fprintf(shop_file, "-1\n");

		/*
		 * Save rates.
		 */
		fprintf(shop_file,	"%1.2f\n"
							"%1.2f\n",
				S_BUYPROFIT(shop), S_SELLPROFIT(shop));

		/*
		 * Save buy types and namelists.
		 */
		FOREACH(ShopBuyDataList, shop->type, buydata)
		{
			fprintf(shop_file, "%d%s\n", buydata->type, buydata->keywords.c_str());
		}
		fprintf(shop_file, "-1\n");

		/*
		 * Save messages'n'stuff
		 * Added some small'n'silly defaults as sanity checks.
		 */
		fprintf(shop_file,	"%s~\n"
							"%s~\n"
							"%s~\n"
							"%s~\n"
							"%s~\n"
							"%s~\n"
							"%s~\n"
							"%d\n"
							"%d\n"
							"%s\n"
							"%d\n",
			!S_NOITEM1(shop).empty() ? S_NOITEM1(shop).c_str() : "%s Ke?!",
			!S_NOITEM2(shop).empty() ? S_NOITEM2(shop).c_str() : "%s Ke?!",
			!S_NOBUY(shop).empty() ? S_NOBUY(shop).c_str() : "%s Ke?!",
			!S_NOCASH1(shop).empty() ? S_NOCASH1(shop).c_str() : "%s Ke?!",
			!S_NOCASH2(shop).empty() ? S_NOCASH2(shop).c_str() : "%s Ke?!",
			!S_BUY(shop).empty() ? S_BUY(shop).c_str() : "%s Ke?! %d?",
			!S_SELL(shop).empty() ? S_SELL(shop).c_str() : "%s Ke?! %d?",
			S_BROKE_TEMPER(shop),
			S_BITVECTOR(shop),
			S_KEEPER(shop) ? S_KEEPER(shop)->GetVirtualID().Print().c_str() : "-1",
			S_NOTRADE(shop));

		/*
		 * Save the rooms.
		 */
		FOREACH(ShopData::RoomVector, shop->in_room, iter)
			fprintf(shop_file, "%s\n", iter->Print().c_str());
		fprintf(shop_file, "-1\n");

		/*
		 * Save open/close times.
		 */
		fprintf(shop_file, "%d\n"
							"%d\n"
							"%d\n"
							"%d\n",
				S_OPEN1(shop),
				S_CLOSE1(shop),
				S_OPEN2(shop),
				S_CLOSE2(shop));
	}
	fprintf(shop_file, "$~\n");
	fclose(shop_file);
	
	sprintf(buf, SHP_PREFIX "%s" SHP_SUFFIX, zone->GetTag());
	/*
	 * We're fubar'd if we crash between the two lines below.
	 */
	remove(buf);
	rename(fname, buf);
}


/**************************************************************************
 Menu functions 
 **************************************************************************/
  
void sedit_products_menu(DescriptorData *d) {
	ShopData *shop = OLC_SHOP(d)/*.Get()*/;

#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	send_to_char("##     VNUM     Product\n", d->m_Character);
			
	for(int i = 0; i < shop->producing.size(); i++) {
		d->m_Character->Send("%2d - [`c%10s`n] - `y%s`n\n",  
				i, S_PRODUCT(shop, i)->GetVirtualID().Print().c_str(),
				S_PRODUCT(shop, i)->m_pProto->GetName());
	}
	send_to_char("\n"
				 "`gA`n) Add a new product.\n"
				 "`gD`n) Delete a product.\n"
				 "`gQ`n) Quit\n"
				 "Enter choice : ",
			d->m_Character);
	OLC_MODE(d) = SEDIT_PRODUCTS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_compact_rooms_menu(DescriptorData *d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	
	ShopData *shop = OLC_SHOP(d)/*.Get()*/;

#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	
	int count = 0;
	FOREACH(ShopData::RoomVector, shop->in_room, iter)
	{
		int i = count;
		sprintf(buf, "%2d - [`c%10s`n]  | %s",  
				i, iter->Print().c_str(), !(++count % 5) ? "\n" : "");
		send_to_char(buf, d->m_Character);
	}
	
	send_to_char("\n"
				 "`gA`n) Add a new room.\n"
				 "`gD`n) Delete a room.\n"
				 "`gL`n) Long display.\n"
				 "`gQ`n) Quit\n"
				 "Enter choice : ",
			d->m_Character);
 
	OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_rooms_menu(DescriptorData *d) {
	ShopData *shop = OLC_SHOP(d)/*.Get()*/;

#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	send_to_char("##     VNUM     Room\n\n", d->m_Character);
	int i = 0;
	FOREACH(ShopData::RoomVector, shop->in_room, iter)
	{
		d->m_Character->Send("%2d - [`c%10s`n] - `y%s`n\n",  
				i++, iter->Print().c_str(), world.Find(*iter)->GetName());
	}
	send_to_char("\n"
				 "`gA`n) Add a new room.\n"
				 "`gD`n) Delete a room.\n"
				 "`gC`n) Compact Display.\n"
				 "`gQ`n) Quit\n"
				 "Enter choice : ",
			d->m_Character);
 
	OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_namelist_menu(DescriptorData *d)
{
	ShopData *shop = OLC_SHOP(d)/*.Get()*/;

#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	send_to_char("##              Type   Namelist\n\n", d->m_Character);
	
	int i = 0;
	FOREACH(ShopBuyDataList, shop->type, buydata)
	{
		d->m_Character->Send("%2d - `c%15s`n - `y%s`n\n",
				i++, GetEnumName(buydata->type),
				!buydata->keywords.empty() ? buydata->keywords.c_str() : "<None>");
	}
	send_to_char("\n"
				 "`gA`n) Add a new entry.\n"
				 "`gD`n) Delete an entry.\n"
				 "`gQ`n) Quit\n"
				 "Enter choice : ",
			d->m_Character);
	OLC_MODE(d) = SEDIT_NAMELIST_MENU;
}
  
/*-------------------------------------------------------------------*/

void sedit_shop_flags_menu(DescriptorData *d) {
	BUFFER(buf, MAX_STRING_LENGTH);

#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for(int i = 0, count = 0; i < NUM_SHOP_FLAGS; i++) {
		d->m_Character->Send("`g%2d`n) %-20.20s   %s", i+1, shop_bits[i],
				!(++count % 2) ? "\n" : "");
	}
	sprintbit(S_BITVECTOR(OLC_SHOP(d)), shop_bits, buf);
	d->m_Character->Send("\nCurrent Shop Flags : `c%s`n\n"
						"Enter choice : ", buf);
	OLC_MODE(d) = SEDIT_SHOP_FLAGS;
}

/*-------------------------------------------------------------------*/

void sedit_no_trade_menu(DescriptorData *d)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for(int i = 0, count = 0; i < NUM_TRADERS; i++) {
		d->m_Character->Send("`g%2d`n) %-20.20s   %s", i+1, trade_letters[i],
				!(++count % 2) ? "\n" : "");
	}
	
	sprintbit(S_NOTRADE(OLC_SHOP(d)), trade_letters, buf);
	d->m_Character->Send("\nCurrently won't trade with: `c%s`n\n"
						"Enter choice : ", buf);
	OLC_MODE(d) = SEDIT_NOTRADE;
}

/*-------------------------------------------------------------------*/

void sedit_types_menu(DescriptorData *d)
{
#if defined(CLEAR_SCREEN)
	d->m_Character->Send("\x1B[H\x1B[J");
#endif
	for(int i = 0, count = 0;  i < NUM_ITEM_TYPES; i++) {
		d->m_Character->Send("`g%2d`n) `c%-20s  %s", i, GetEnumName((ObjectType)i),
				!(++count % 3) ? "\n" : "");
	}
	d->m_Character->Send("`nEnter choice : ");
	OLC_MODE(d) = SEDIT_TYPE_MENU;
}
    

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void sedit_disp_menu(DescriptorData *d) {
	ShopData *shop;
	BUFFER(buf1, MAX_INPUT_LENGTH);
	BUFFER(buf2, MAX_INPUT_LENGTH);

	shop = OLC_SHOP(d)/*.Get()*/;

	sprintbit(S_NOTRADE(shop), trade_letters, buf1); 
	sprintbit(S_BITVECTOR(shop), shop_bits, buf2); 
	
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Shop Number : [`c%s`n]\n"
				"`g0`n) Keeper      : [`c%s`n] `y%s\n"
				"`g1`n) Open 1      : `c%4d`n          `g2`n) Close 1     : `c%4d\n"
				"`g3`n) Open 2      : `c%4d`n          `g4`n) Close 2     : `c%4d\n"
				"`g5`n) Sell rate   : `c%1.2f`n          `g6`n) Buy rate    : `c%1.2f\n"
				"`g7`n) Keeper no item : `y%s\n"
				"`g8`n) Player no item : `y%s\n"
				"`g9`n) Keeper no cash : `y%s\n"
				"`gA`n) Player no cash : `y%s\n"
				"`gB`n) Keeper no buy  : `y%s\n"
				"`gC`n) Buy sucess     : `y%s\n"
				"`gD`n) Sell sucess    : `y%s\n"
				"`gE`n) No Trade With  : `c%s\n"
				"`gF`n) Shop flags     : `c%s\n"
				"`gR`n) Rooms Menu\n"
				"`gP`n) Products Menu\n"
				"`gT`n) Accept Types Menu\n"
				"`gQ`n) Quit\n"
				"Enter Choice : ",
			OLC_VID(d).Print().c_str(),
			S_KEEPER(shop) ? S_KEEPER(shop)->GetVirtualID().Print().c_str() : "<NOBODY>", 
			S_KEEPER(shop) ? S_KEEPER(shop)->m_pProto->GetName() : "None",
			S_OPEN1(shop),		S_CLOSE1(shop),
			S_OPEN2(shop),		S_CLOSE2(shop),
			S_BUYPROFIT(shop),	S_SELLPROFIT(shop),
			S_NOITEM1(shop).c_str(),
			S_NOITEM2(shop).c_str(),
			S_NOCASH1(shop).c_str(),
			S_NOCASH2(shop).c_str(),
			S_NOBUY(shop).c_str(),
			S_BUY(shop).c_str(),
			S_SELL(shop).c_str(),
			buf1,
			buf2);

	OLC_MODE(d) = SEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
 **************************************************************************/

void sedit_parse(DescriptorData * d, const char *arg) {
	int i;

	if (OLC_MODE(d) > SEDIT_NUMERICAL_RESPONSE) {
		if(!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
			send_to_char("Field must be numerical, try again : ", d->m_Character);
			return;
		}
	}

  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case SEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      send_to_char("Saving changes to shop - no need to 'sedit save'!\n", d->m_Character);
      sedit_save_internally(d);
      mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s edits shop %s", d->m_Character->GetName(), OLC_VID(d).Print().c_str());
      //	Fall through
    case 'n':
    case 'N':
      cleanup_olc(d);
      return;
    default:
      send_to_char(	"Invalid choice!\n"
      				"Do you wish to save this shop? ", d->m_Character);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case SEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) 
    { case 'q':
      case 'Q':
        if (OLC_VAL(d)) /*. Anything been changed? .*/
        { send_to_char("Do you wish to save this shop? ", d->m_Character);
          OLC_MODE(d) = SEDIT_CONFIRM_SAVESTRING;
        } else
          cleanup_olc(d);
        return;
      case '0':
        OLC_MODE(d) = SEDIT_KEEPER;
        send_to_char("Enter virtual number of shop keeper : ", d->m_Character);
        return;
      case '1':
        OLC_MODE(d) = SEDIT_OPEN1;
        i++;
        break;
      case '2':
        OLC_MODE(d) = SEDIT_CLOSE1;
        i++;
        break;
      case '3':
        OLC_MODE(d) = SEDIT_OPEN2;
        i++;
        break;
      case '4':
        OLC_MODE(d) = SEDIT_CLOSE2;
        i++;
        break;
      case '5':
        OLC_MODE(d) = SEDIT_BUY_PROFIT;
        i++;
        break;
      case '6':
        OLC_MODE(d) = SEDIT_SELL_PROFIT;
        i++;
        break;
      case '7':
        OLC_MODE(d) = SEDIT_NOITEM1;
        i--;
        break;
      case '8':
        OLC_MODE(d) = SEDIT_NOITEM2;
        i--;
        break;
      case '9':
        OLC_MODE(d) = SEDIT_NOCASH1;
        i--;
        break;
      case 'a':
      case 'A':
        OLC_MODE(d) = SEDIT_NOCASH2;
        i--;
        break;
      case 'b':
      case 'B':
        OLC_MODE(d) = SEDIT_NOBUY;
        i--;
        break;
      case 'c':
      case 'C':
        OLC_MODE(d) = SEDIT_BUY;
        i--;
        break;
      case 'd':
      case 'D':
        OLC_MODE(d) = SEDIT_SELL;
        i--;
        break;
      case 'e':
      case 'E':
        sedit_no_trade_menu(d);
        return;
      case 'f':
      case 'F':
        sedit_shop_flags_menu(d);
        return;
      case 'r':
      case 'R':
        sedit_rooms_menu(d);
        return;
      case 'p':
      case 'P':
        sedit_products_menu(d);
        return;
      case 't':
      case 'T':
        sedit_namelist_menu(d);
        return;
      default:
        sedit_disp_menu(d);
	return;
    }

			if (i != 0) {
				send_to_char(i == 1 ? "\nEnter new value : " : (i == -1 ?
						"\nEnter new text :\n] " : "Oops...\n"), d->m_Character);
				return;
			}
    break; 
/*-------------------------------------------------------------------*/
  case SEDIT_NAMELIST_MENU:
    switch (*arg) 
    { case 'a':
      case 'A':
        sedit_types_menu(d);
        return;
      case 'd':
      case 'D':
        send_to_char("\nDelete which entry? : ", d->m_Character);
        OLC_MODE(d) = SEDIT_DELETE_TYPE;
        return;
      case 'q':
      case 'Q':
        break;
    }
    break;
/*-------------------------------------------------------------------*/
  case SEDIT_PRODUCTS_MENU:
    switch (*arg) 
    { case 'a':
      case 'A':
        send_to_char("\nEnter new product virtual number : ", d->m_Character);
        OLC_MODE(d) = SEDIT_NEW_PRODUCT;
        return;
      case 'd':
      case 'D':
        send_to_char("\nDelete which product? : ", d->m_Character);
        OLC_MODE(d) = SEDIT_DELETE_PRODUCT;
        return;
      case 'q':
      case 'Q':
        break;
    }
    break;
/*-------------------------------------------------------------------*/
  case SEDIT_ROOMS_MENU:
    switch (*arg) 
    { case 'a':
      case 'A':
        send_to_char("\nEnter new room virtual number : ", d->m_Character);
        OLC_MODE(d) = SEDIT_NEW_ROOM;
        return;
      case 'c':
      case 'C':
        sedit_compact_rooms_menu(d);
        return;
      case 'l':
      case 'L':
        sedit_rooms_menu(d);
        return;
      case 'd':
      case 'D':
        send_to_char("\nDelete which room? : ", d->m_Character);
        OLC_MODE(d) = SEDIT_DELETE_ROOM;
        return;
      case 'q':
      case 'Q':
        break;
    }
    break;
/*-------------------------------------------------------------------*/
  /*. String edits .*/
  case SEDIT_NOITEM1:
    sedit_modify_string(S_NOITEM1(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOITEM2:
    sedit_modify_string(S_NOITEM2(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOCASH1:
    sedit_modify_string(S_NOCASH1(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOCASH2:
    sedit_modify_string(S_NOCASH2(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOBUY:
    sedit_modify_string(S_NOBUY(OLC_SHOP(d)), arg);
    break;
  case SEDIT_BUY:
    sedit_modify_string(S_BUY(OLC_SHOP(d)), arg);
    break;
  case SEDIT_SELL:
    sedit_modify_string(S_SELL(OLC_SHOP(d)), arg);
    break;
		case SEDIT_NAMELIST:
		{
			ShopBuyData	buydata;
			buydata.type = (ObjectType)OLC_VAL(d);
			buydata.keywords = arg;
			S_NAMELISTS(OLC_SHOP(d)).push_back(buydata);
			sedit_namelist_menu(d);
		}
		return;

/*-------------------------------------------------------------------*/
  /*. Numerical responses .*/

		case SEDIT_KEEPER:
			S_KEEPER(OLC_SHOP(d)) = NULL;
			if (*arg)
			{
				S_KEEPER(OLC_SHOP(d)) = mob_index.Find(arg);
				if (!S_KEEPER(OLC_SHOP(d)))
				{
					send_to_char("That mobile does not exist, try again : ", d->m_Character);
					return;
				}
			}
			break;
  case SEDIT_OPEN1:
    S_OPEN1(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_OPEN2:
    S_OPEN2(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_CLOSE1:
    S_CLOSE1(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_CLOSE2:
    S_CLOSE2(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_BUY_PROFIT:
    sscanf(arg, "%f", &S_BUYPROFIT(OLC_SHOP(d)));
    break;
  case SEDIT_SELL_PROFIT:
    sscanf(arg, "%f", &S_SELLPROFIT(OLC_SHOP(d)));
    break;
  case SEDIT_TYPE_MENU:
    OLC_VAL(d) = MAX(0, MIN(NUM_ITEM_TYPES - 1, atoi(arg)));
    send_to_char("Enter namelist (return for none) :-\n| ", d->m_Character);
    OLC_MODE(d) = SEDIT_NAMELIST;
    return;
  case SEDIT_DELETE_TYPE:
    sedit_remove_from_type_list(S_NAMELISTS(OLC_SHOP(d)), atoi(arg));
    sedit_namelist_menu(d);
    return;
  case SEDIT_NEW_PRODUCT:
	{
		if (is_number(arg))
		{
		  	ObjectPrototype *proto = is_number(arg) ? obj_index.Find(arg) : NULL;
			if (proto == NULL)
			{
				send_to_char("That object does not exist, try again : ", d->m_Character);
				return;
			}
			S_PRODUCTS(OLC_SHOP(d)).push_back(proto);
		}
		sedit_products_menu(d);
		return;
	}

		case SEDIT_DELETE_PRODUCT:
		{
			int num = atoi(arg);
			if (num >= 0 && num < S_PRODUCTS(OLC_SHOP(d)).size())
				S_PRODUCTS(OLC_SHOP(d)).erase(S_PRODUCTS(OLC_SHOP(d)).begin() + num);
		}
		sedit_products_menu(d);
		return;
  case SEDIT_NEW_ROOM:
    if (atoi(arg) != -1)
    { RoomData *room = world.Find(arg);
      if (room == NULL)
      { send_to_char("That room does not exist, try again : ", d->m_Character);
        return;
      }
      else
      S_ROOMS(OLC_SHOP(d)).push_back(room->GetVirtualID());
    }
    sedit_rooms_menu(d);
    return;
		case SEDIT_DELETE_ROOM:
			{
				int num = atoi(arg);
				if (num >= 0 && num < S_ROOMS(OLC_SHOP(d)).size())
					S_ROOMS(OLC_SHOP(d)).erase(S_ROOMS(OLC_SHOP(d)).begin() + num);
			}
			sedit_rooms_menu(d);
			return;
  case SEDIT_SHOP_FLAGS:
		if ((i = MAX(0, MIN(NUM_SHOP_FLAGS, atoi(arg)))) > 0) {
			TOGGLE_BIT(S_BITVECTOR(OLC_SHOP(d)), 1 << (i - 1));
			sedit_shop_flags_menu(d);
			return;
		}
    break;
  case SEDIT_NOTRADE:
    if ((i = MAX(0, MIN(NUM_TRADERS, atoi(arg)))) > 0) {
      TOGGLE_BIT(S_NOTRADE(OLC_SHOP(d)), 1 << (i - 1));
      sedit_no_trade_menu(d);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  default:
    /*. We should never get here .*/
    cleanup_olc(d);
    mudlog("SYSERR: OLC: sedit_parse(): Reached default case!",BRF,LVL_BUILDER,TRUE);
    send_to_char("Oops...\n", d->m_Character);
    break;
  }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
    If we get here, we have probably changed something, and now want to
    return to main menu.  Use OLC_VAL as a 'has changed' flag .*/

  OLC_VAL(d) = 1;
  sedit_disp_menu(d);
}
