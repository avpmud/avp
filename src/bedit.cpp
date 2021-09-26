/*************************************************************************
*   File: bedit.c++                  Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for characters                                    *
*************************************************************************/



#include "scripts.h"
#include "descriptors.h"
#include "characters.h"
#include "olc.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "buffer.h"


void bedit_parse(DescriptorData *d, const char *arg);
void bedit_setup_new(DescriptorData *d, const char *name);
void bedit_setup_existing(DescriptorData *d, BehaviorSet *set);
void bedit_save_internally(DescriptorData *d);
void bedit_disp_var_menu(DescriptorData * d);
void bedit_disp_menu(DescriptorData *d);

void bedit_setup_new(DescriptorData *d, const char *name) {
	OLC_BEHAVIOR(d) = new BehaviorSet(name);
	OLC_VAL(d) = 0;
	
	bedit_disp_menu(d);
}


void bedit_setup_existing(DescriptorData *d, BehaviorSet *set) {
	OLC_BEHAVIOR(d) = new BehaviorSet(*set);
	OLC_VAL(d) = 0;
	
	bedit_disp_menu(d);
}


void bedit_save_internally(DescriptorData *d)
{
	BehaviorSet::SaveSet(*OLC_BEHAVIOR(d));
}


void bedit_disp_var_menu(DescriptorData * d)
{
	ScriptVariable *vd = OLC_TRIGVAR(d);

	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Variable menu\n"
			"`gN`n) Name         : `y%s\n"
			"`gV`n) Value        : `y%s\n"
			"`gC`n) Context      : `c%d\n"
			"`gQ`n) Quit Variable Setup\n"
			"Enter choice : ",
			vd->GetName(),
			vd->GetValue(),
			vd->GetContext());

	OLC_MODE(d) = BEDIT_TRIGGER_VARIABLE;
}


void bedit_disp_menu(DescriptorData *d)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	BehaviorSet *set = OLC_BEHAVIOR(d);
	
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Behavior Set: `c%s`n\n"
				"`GN`n) Name      : `y%s`n\n",
				OLC_BEHAVIOR(d)->GetName(),
				OLC_BEHAVIOR(d)->GetName());
	
	int counter = 0;
	
	if (!set->GetScripts().empty())
	{
		d->m_Character->Send("`y-- Scripts (%d):`n\n", set->GetScripts().size());

		FOREACH(ScriptVector, set->GetScripts(), i)
		{
			ScriptPrototype *proto = trig_index.Find(*i);
			
			d->m_Character->Send("`g%2d`n) [`c%10s`n] %s\n", ++counter, i->Print().c_str(), proto ? proto->m_pProto->GetName() : "(Uncreated)");
		}
	}
	else
		send_to_char("`y-- No scripts`n\n", d->m_Character);
	
	if (!set->GetVariables().empty())
	{
		d->m_Character->Send("`y-- Global Variables (%d):`n\n", set->GetVariables().size());
		FOREACH(ScriptVariable::List, set->GetVariables(), iter)
		{
			ScriptVariable *var = *iter;
			strcpy(buf, var->GetName());
			if (var->GetContext())		sprintf_cat(buf, " {%d}", var->GetContext());
			d->m_Character->Send("`g%2d`n) %-30.30s: %s\n", ++counter, buf, var->GetValue());
		}
	}
	else
		send_to_char("`y-- No Variables`n\n", d->m_Character);

	send_to_char(	"`GA`n) Add Script\n"
					"`GV`n) Add Variable\n"
					"`GE`n) Edit Variable\n"
					"`GR`n) Remove Script/Variable\n"
					"`GQ`n) Quit\n"
					"Enter choice:  ", d->m_Character);

	OLC_MODE(d) = BEDIT_MAIN_MENU;
}


void bedit_parse(DescriptorData *d, const char *arg)
{
	switch (OLC_MODE(d)) {
		case BEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
					send_to_char("Saving changes to behavior.\n", d->m_Character);
					mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s edits behavior set %s", d->m_Character->GetName(), OLC_BEHAVIOR(d)->GetName());
					bedit_save_internally(d);
					// Fall through
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
				default:
				send_to_char("Invalid choice!\n"
							 "Do you wish to save the clan? ", d->m_Character);
			}
			return;
		case BEDIT_MAIN_MENU:
			switch (*arg)
			{
				case 'q':
				case 'Q':
					if (OLC_VAL(d))
					{
						send_to_char("Do you wish to save this behavior set? ", d->m_Character);
						OLC_MODE(d) = BEDIT_CONFIRM_SAVESTRING;
					}
					else
						cleanup_olc(d);
					return;
				case 'n':
				case 'N':
					send_to_char("Re-enter the name.  It must match the original name, but it can be case sensitive.", d->m_Character);
					OLC_MODE(d) = BEDIT_NAME;
					return;
				case 'a':
				case 'A':
					send_to_char("Enter the VNUM of script to attach: ", d->m_Character);
					OLC_MODE(d) = BEDIT_TRIGGER_ADD_SCRIPT;
					return;
				case 'v':
				case 'V':
					OLC_TRIGVAR(d) = OLC_BEHAVIOR(d)->GetVariables().AddBlank();
					bedit_disp_var_menu(d);
					return;
				case 'e':
				case 'E':
					send_to_char("Enter item in list to edit: ", d->m_Character);
					OLC_MODE(d) = BEDIT_TRIGGER_CHOOSE_VARIABLE;
					return;
				case 'r':
				case 'R':
					send_to_char("Enter item in list to remove: ", d->m_Character);
					OLC_MODE(d) = BEDIT_TRIGGER_REMOVE;
					return;
				default:
					bedit_disp_menu(d);
					return;
			}
			break;
		
		case BEDIT_NAME:
			if (!str_cmp(OLC_BEHAVIOR(d)->GetName(), arg))	OLC_BEHAVIOR(d)->SetName(arg);
			OLC_VAL(d) = 1;
			bedit_disp_menu(d);
			break;
			
		case BEDIT_TRIGGER_ADD_SCRIPT:
			if (IsVirtualID(arg))
			{
				OLC_BEHAVIOR(d)->GetScripts().push_back(VirtualID(arg));
				send_to_char("Script attached.\n", d->m_Character);
				OLC_VAL(d) = 1;
			} else
				send_to_char("Virtual IDs only, please.\n", d->m_Character);
			bedit_disp_menu(d);
			return;
			
		case BEDIT_TRIGGER_REMOVE:
			if (is_number(arg))
			{
				int number = atoi(arg) - 1;
				
				if (number < 0)
					send_to_char("Invalid number.\n", d->m_Character);
				else
				{
					if (number < OLC_BEHAVIOR(d)->GetScripts().size())
					{
						OLC_BEHAVIOR(d)->GetScripts().erase(OLC_BEHAVIOR(d)->GetScripts().begin() + number);
						OLC_VAL(d) = 1;
						send_to_char("Script detached.\n", d->m_Character);
					}
					else
					{
						number -= OLC_BEHAVIOR(d)->GetScripts().size();
			
						if (number >= OLC_BEHAVIOR(d)->GetVariables().size())
							send_to_char("Invalid number.\n", d->m_Character);
						else
						{
							ScriptVariable::List::iterator var = OLC_BEHAVIOR(d)->GetVariables().begin();
							std::advance(var, number);
						
							d->m_Character->Send("Variable \"%s\" removed.\n", (*var)->GetName());
							delete *var;
							OLC_BEHAVIOR(d)->GetVariables().erase(var);
							OLC_VAL(d) = 1;
						}
					}
				}
			}
			else
				send_to_char("Numbers only, please.\n", d->m_Character);
			bedit_disp_menu(d);
			return;
		
		case BEDIT_TRIGGER_CHOOSE_VARIABLE:
			if (is_number(arg))
			{
				int number = atoi(arg) - 1;
				
				number -= OLC_BEHAVIOR(d)->GetScripts().size();
				
				if (number < 0)
					send_to_char("Variables only.\n", d->m_Character);
				else if (number < OLC_BEHAVIOR(d)->GetVariables().size())
				{
					ScriptVariable::List::iterator var = OLC_BEHAVIOR(d)->GetVariables().begin();
					std::advance(var, number);
					
					OLC_TRIGVAR(d) = *var;
					bedit_disp_var_menu(d);
					return;
				}
				else
					send_to_char("Invalid number.\n", d->m_Character);
			}
			else
				send_to_char("Numbers only, please.\n", d->m_Character);
			bedit_disp_menu(d);
			return;
		
		case BEDIT_TRIGGER_VARIABLE:
			switch (*arg)
			{
				case 'n':
				case 'N':
					send_to_char("Name: ", d->m_Character);
					OLC_MODE(d) = BEDIT_TRIGGER_VARIABLE_NAME;
					break;
				case 'v':
				case 'V':
					send_to_char("Value: ", d->m_Character);
					OLC_MODE(d) = BEDIT_TRIGGER_VARIABLE_VALUE;
					break;
				case 'c':
				case 'C':
					send_to_char("Context (0 = default): ", d->m_Character);
					OLC_MODE(d) = BEDIT_TRIGGER_VARIABLE_CONTEXT;
					break;
				case 'q':
				case 'Q':
					if (OLC_TRIGVAR(d)->GetNameString().empty())
					{
						ScriptVariable::List::iterator iter = Lexi::Find(OLC_BEHAVIOR(d)->GetVariables(), OLC_TRIGVAR(d));
						if (iter != OLC_BEHAVIOR(d)->GetVariables().end())
							OLC_BEHAVIOR(d)->GetVariables().erase(iter);
						delete OLC_TRIGVAR(d);
						OLC_TRIGVAR(d) = NULL;
					}
					
					bedit_disp_menu(d);
					break;
				default:
					bedit_disp_var_menu(d);
					break;
			}
			return;
		
		case BEDIT_TRIGGER_VARIABLE_NAME:
			if (*arg)
			{
				ScriptVariable var(arg, OLC_TRIGVAR(d)->GetContext());
				var.SetValue(OLC_TRIGVAR(d)->GetValue());
				
				*OLC_TRIGVAR(d) = var;
				OLC_BEHAVIOR(d)->GetVariables().Sort();
				
				OLC_VAL(d) = 1;
			}
			bedit_disp_var_menu(d);
			return;
			
		case BEDIT_TRIGGER_VARIABLE_VALUE:
			if (*arg)
			{
				OLC_TRIGVAR(d)->SetValue(arg);
				OLC_VAL(d) = 1;
			}
			bedit_disp_var_menu(d);
			return;

		case BEDIT_TRIGGER_VARIABLE_CONTEXT:
			if (is_number(arg))
			{
				ScriptVariable var(OLC_TRIGVAR(d)->GetName(), atoi(arg));
				var.SetValue(OLC_TRIGVAR(d)->GetValue());
				
				*OLC_TRIGVAR(d) = var;
				OLC_BEHAVIOR(d)->GetVariables().Sort();
				
				OLC_VAL(d) = 1;
			}
			bedit_disp_var_menu(d);
			return;
			
		default:
			mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: bedit_parse(): Reached default case!  Case: %d", OLC_MODE(d));
			send_to_char("Oops...\n", d->m_Character);
			break;
	}
	OLC_VAL(d) = 1;
	bedit_disp_menu(d);
}
