#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

//	Character Information
extern char *faction_types[];
extern char *race_types[];
extern char *race_abbrevs[];
extern char *relation_colors;
extern int max_hit_base[NUM_RACES];
extern int max_hit_bonus[NUM_RACES];
extern VNum start_rooms[NUM_RACES];

//	Date and Time
extern char *weekdays[];
extern char *month_name[];

//	Informational and Help
extern Lexi::String help;
extern Lexi::String credits;
extern Lexi::String news;
extern Lexi::String info;
extern Lexi::String motd;
extern Lexi::String imotd;
extern Lexi::String wizlist;
extern Lexi::String immlist;
extern Lexi::String policies;
extern Lexi::String handbook;
extern Lexi::String background;
extern Lexi::String login;
extern Lexi::String welcomeMsg;
extern Lexi::String startMsg;
extern char *MENU;
extern char *race_menu;

//	Rooms
extern int rev_dir[];
extern char *dirs[];
extern char *dir_text_the[];
extern char *dir_text_to_the[];
extern char *dir_text_to_the_of[];
extern char *cmd_door[];
extern int movement_loss[];
extern int movement_time[];

//	Combat
extern char *combat_modes[];
extern char *damage_types[];
extern char *damage_type_abbrevs[];
extern char *attack_types[];
extern char *times_string[];
extern char *damage_locations[];

//	Items
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern int wear_to_armor_loc[NUM_WEARS];
extern char *drinks[];
extern char *drinknames[];
extern int drink_aff[][3];
extern char *equipment_types[];
extern char *ammo_types[];
extern char *eqpos[];
extern char *vehicle_bits[];

//	Teams
#define MAX_TEAMS 10
extern char *team_names[];
extern char *team_titles[];
extern RoomData *team_recalls[MAX_TEAMS + 1];

//	System
extern char *circlemud_version;
extern int pk_allowed;
extern int auto_save;
extern int max_npc_corpse_time, max_pc_corpse_time;
extern int no_specials;
extern int max_bad_pws;

//	Bits
extern char *action_bits[];
extern char *wear_bits[];
extern char *extra_bits[];
extern char *container_bits[];
extern char *stat_types[];
extern char *token_sets[];
extern char *trade_letters[];
extern char *shop_bits[];
extern char *ammo_bits[];
extern char *player_bits[];
extern char *preference_bits[];
extern char *staff_bits[];
extern char *restriction_bits[];
extern char *channel_bits[];

extern char *zonecmd_types[];
extern char *hive_types[];

//	Scripts
extern char *trig_data_types[];
extern char *mtrig_types[];
extern char *otrig_types[];
extern char *wtrig_types[];
extern char *gtrig_types[];

#endif
