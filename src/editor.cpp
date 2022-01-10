#include <list>

#include "types.h"
#include "lexistring.h"
#include "buffer.h"
#include "descriptors.h"
#include "characters.h"
#include "utils.h"
#include "interpreter.h"

//	Editor-specific stuff
#include "rooms.h"
#include "zones.h"
#include "constants.h"
#include "staffcmds.h"
#include "objects.h"
#include "scripts.h"
#include "clans.h"
#include "characters.h"


template <class RepresentedType>
class NumberWrapper
{
private:
						NumberWrapper();
	void				operator=(const NumberWrapper &nw);
						NumberWrapper(const NumberWrapper &nw);

	struct NumberImplementationBase
	{
//		virtual void		Set(RepresentedType) = 0;
//		virtual RepresentedType	Get() = 0;
		virtual void		operator=(RepresentedType) = 0;
		virtual				operator RepresentedType() = 0;
		virtual bool		operator==(RepresentedType) = 0;
		virtual bool		operator!=(RepresentedType) = 0;
		virtual bool		operator>=(RepresentedType) = 0;
		virtual bool		operator<=(RepresentedType) = 0;
		virtual bool		operator>(RepresentedType) = 0;
		virtual bool		operator<(RepresentedType) = 0;
	};

	template<class WrappedType>
	struct NumberImplementation : public NumberImplementationBase
	{
							NumberImplementation(WrappedType &num) : m_Val(num) {}
//		virtual void		Set(RepresentedType n) { m_Val = n; }
//		virtual RepresentedType	Get() { return m_Val; }
		virtual void		operator=(RepresentedType n) { m_Val = (WrappedType)n; }
		virtual 			operator RepresentedType() { return m_Val; }
		virtual bool		operator==(RepresentedType n) { return m_Val == n; }
		virtual bool		operator!=(RepresentedType n) { return m_Val != n; }
		virtual bool		operator>=(RepresentedType n) { return m_Val >= n; }
		virtual bool		operator<=(RepresentedType n) { return m_Val <= n; }
		virtual bool		operator>(RepresentedType n) { return m_Val > n; }
		virtual bool		operator<(RepresentedType n) { return m_Val < n; }
		
	private:
		WrappedType &		m_Val;
	};
	
	std::auto_ptr<NumberImplementationBase>	m_Impl;
	
public:
	template<class WrappedType>
						NumberWrapper(WrappedType &num) : m_Impl(new NumberImplementation<WrappedType>(num)) { }
						NumberWrapper(NumberWrapper &nw) : m_Impl(nw.m_Impl) {}
						
//	void				Set(RepresentedType n) { m_Impl->Set(n); }
//	RepresentedType		Get() { return m_Impl->Get(); }
	void				operator=(RepresentedType n) { *m_Impl = n; }
//	explicit			operator RepresentedType() const { return *m_Impl; }
	RepresentedType		get() const { return *m_Impl; }
	bool				operator==(RepresentedType n) { return *m_Impl == n; }
	bool				operator!=(RepresentedType n) { return *m_Impl != n; }
	bool				operator>=(RepresentedType n) { return *m_Impl >= n; }
	bool				operator<=(RepresentedType n) { return *m_Impl <= n; }
	bool				operator>(RepresentedType n) { return *m_Impl > n; }
	bool				operator<(RepresentedType n) { return *m_Impl < n; }
};


/*
template <class RepresentedType>
class NumberWrapperBase
{
public:
	virtual void		Set(RepresentedType value) = 0;
	virtual RepresentedType		Get() = 0;
};


template <class RepresentedType, class WrappedType>
class NumberWrapper : public NumberWrapperBase<RepresentedType>
{
public:
						NumberWrapper(WrappedType &i) : m_Value(i) {}

	virtual void			Set(RepresentedType value) { m_Value = value; }
	virtual RepresentedType	Get() { return m_Value; }
private:						
	WrappedType &		m_Value;
};
*/


class Editor;
class EditorStackConnectionState;


class Editor : public Lexi::Shareable
{
public:
	Editor()
	:	m_Stack(NULL)
	{}

	typedef Lexi::SharedPtr<Editor>	Ptr;
	
	virtual void		Display() = 0;
	
	//	Callbacks
	virtual void		Enter() { Display(); }
	virtual void		Exit() {}
	virtual void		Pause() {}
	virtual void		Resume() { Display(); }
	virtual void		Parse(const char *arg) = 0;
	
	virtual void		OnEditorSave();
	virtual void		OnEditorAbort();
	virtual void		OnEditorFinished();
	
	//	Communication
	int					Send(const char *format, ...) __attribute__ ((format (printf, 2, 3)));
	
	void				Push(Ptr state);
	void				Pop();
	void				Swap(Ptr state);
	
protected:
	friend class EditorStackConnectionState;
	
	void				SetStack(EditorStackConnectionState *stack) { m_Stack = stack; }
	DescriptorData *	GetDesc() const;
	
	void				SetHasChanged();
	bool				HasChanged() const;
	
	void				ClearScreen() const;
	
	virtual				~Editor() {}
	
private:
	EditorStackConnectionState *m_Stack;
	
	Editor(const Editor &);
};


class EditorStackConnectionState : public ConnectionState
{
public:
	EditorStackConnectionState()
	:	m_bChanged(false)
	{}
	
	virtual void		Enter() {}
	virtual void		Exit() {}
	virtual void		Pause() {}
	virtual void		Resume() {}
	virtual void		Parse(const char *arg);
	
	virtual bool		IsInGame() const { return true; }
	
	void				PushEditor(Editor::Ptr pEditor);
	void				PopEditor();
	void				SwapEditor(Editor::Ptr pEditor);
	
	void				SetHasChanged() { m_bChanged = true; }
	bool				HasChanged()	{ return m_bChanged; }
	
	DescriptorData *	GetDesc() { return ConnectionState::GetDesc(); }
	
private:
	Lexi::List<Editor::Ptr>	m_Stack;
	
	bool				m_bChanged;
	
	EditorStackConnectionState(const EditorStackConnectionState &);
};



void Editor::Push(Ptr state)	{ m_Stack->PushEditor(state); }
void Editor::Pop()				{ m_Stack->PopEditor(); }
void Editor::Swap(Ptr state)	{ m_Stack->SwapEditor(state); }

void Editor::OnEditorSave()		{ m_Stack->OnEditorSave(); }
void Editor::OnEditorAbort()	{ m_Stack->OnEditorAbort(); }
void Editor::OnEditorFinished()	{ m_Stack->OnEditorFinished(); }

DescriptorData * Editor::GetDesc() const { return m_Stack->GetDesc(); }

void Editor::SetHasChanged()	{ m_Stack->SetHasChanged(); }
bool Editor::HasChanged() const	{ return m_Stack->HasChanged(); }


void EditorStackConnectionState::Parse(const char *arg)
{
	if (!m_Stack.empty())
	{
		Editor::Ptr editor = m_Stack.front();
		editor->Parse(arg);
//		if (!m_Stack.empty() && editor == m_Stack.front())
//			editor->Display();	//	Force a re-display
	}
}



void EditorStackConnectionState::PushEditor(Editor::Ptr pEditor)
{
	if (!m_Stack.empty())
	{
		m_Stack.front()->Pause();
	}
	
	m_Stack.push_front(pEditor);
	pEditor->SetStack( this );
	pEditor->Enter();
}


void EditorStackConnectionState::PopEditor()
{
	if (!m_Stack.empty())	m_Stack.front()->Exit();
	m_Stack.pop_front();
	if (!m_Stack.empty())	m_Stack.front()->Enter();
	else					Pop();	//	We're done!
}


void EditorStackConnectionState::SwapEditor(Editor::Ptr pEditor)
{
	if (!m_Stack.empty())	m_Stack.front()->Exit();
	m_Stack.pop_front();
	m_Stack.push_front(pEditor);
	pEditor->SetStack( this );
	pEditor->Enter();
}


int Editor::Send(const char *messg, ...)
{
	int	length = 0;

	if (!messg || !*messg)
		return 0;
		
 	BUFFER(buf, MAX_STRING_LENGTH);

	va_list args;
	va_start(args, messg);
	length += vsnprintf(buf, MAX_STRING_LENGTH, messg, args);
	buf[MAX_STRING_LENGTH - 1] = '\0';
	va_end(args);
	
	GetDesc()->Write(buf);
	
	return length;
}


void Editor::ClearScreen() const
{
#if defined(CLEAR_SCREEN)
	Send("\x1B[H\x1B[J");
#endif
}



class BaseFlagEditor : public Editor
{
public:
	BaseFlagEditor(const Lexi::String &type)
	:	m_Type(type)
	{}
	
	virtual void		Display();
	virtual void		Parse(const char *arg);

	virtual bool		test(unsigned int flag) const = 0;
	virtual void		toggle(unsigned int flag) = 0;
	
	virtual unsigned int size() const = 0;
	virtual const char *name(unsigned int flag) const = 0;
	virtual Lexi::String print() const = 0;
	
private:
	Lexi::String		m_Type;
};


void BaseFlagEditor::Display()
{
	BUFFER(buf, MAX_STRING_LENGTH);

	int			counter, num;

	ClearScreen();
	for (counter = 0, num = size(); num; ++counter)
	{
		Send("`g%2d`n) %-20.20s %s",
			counter + 1,
			name(counter),
			(counter & 1) == 0 ? "" : "\n");
	}
	
	Send(
			"\n%s"
			"%s flags: `c%s`n\n"
			"Enter flags, 0 to quit : ",
			(counter & 1) == 0 ? "" : "\n",
			m_Type.c_str(),
			print().c_str());
}


void BaseFlagEditor::Parse(const char *arg)
{
	int number = atoi(arg);
	
	if (!is_number(arg) || (number < 0) || (number >= size()))
		Send("That is not a valid choice!\n");
	else if (number == 0)
		Pop();
	else
	{
		toggle(number - 1);
		SetHasChanged();
	}
}


class FlagEditor : public BaseFlagEditor
{
public:
	FlagEditor(const Lexi::String &type, NumberWrapper<Flags> flags, char *bits[]);

	virtual bool		test(unsigned int flag) const	{ Flags f = m_Flags.get(); return IS_SET(f, 1 << flag); }
	virtual void		toggle(unsigned int flag)		{ Flags f = m_Flags.get(); TOGGLE_BIT(f, 1 << flag); m_Flags = f;	}
	
	virtual unsigned int size() const					{ return m_NumBits;					}
	virtual const char *name(unsigned int flag) const	{ return m_Bits[flag];				}
	virtual Lexi::String print() const;
	
private:
	NumberWrapper<Flags>	m_Flags;
	char **				m_Bits;
	int					m_NumBits;
};


FlagEditor::FlagEditor(const Lexi::String &type, NumberWrapper<Flags> flags, char **bits)
:	BaseFlagEditor(type)
,	m_Flags(flags)
,	m_Bits(bits)
,	m_NumBits(0)
{
	int count = 0;

	while (*m_Bits[count] != '\n')
		++count;
	
	m_NumBits = count;
}


Lexi::String FlagEditor::print() const
{
	BUFFER(buf, MAX_STRING_LENGTH);
	sprintbit(m_Flags.get(), m_Bits, buf);
	return buf;
}


template <class BF>
class BitFlagEditor : public BaseFlagEditor
{
public:
	BitFlagEditor(const Lexi::String &type, BF & flags)
	:	BaseFlagEditor(type)
	,	m_Flags(flags)
	{}

	virtual bool		test(unsigned int flag) const	{ return m_Flags.test((BF::value_type)flag);	}
	virtual void		toggle(unsigned int flag)		{ m_Flags.flip((BF::value_type)flag);			}
	
	virtual unsigned int size() const					{ return m_Flags.size();		}
	virtual const char *name(unsigned int flag) const	{ return GetEnumName<BF::value_type>(flag);	}
	virtual Lexi::String print() const					{ return m_Flags.print();		}
	
private:
	BF &				m_Flags;
};



class EnumPickerBaseEditor : public Editor
{
public:
	EnumPickerBaseEditor(const Lexi::String &type, NumberWrapper<unsigned int> item)
	:	m_Type(type)
	,	m_Item(item)
	{}
	
	virtual void		Display();
	virtual void		Parse(const char *arg);

	virtual unsigned int	size() const = 0;
	virtual const char *	name(unsigned int i) const = 0;
	
private:
	Lexi::String		m_Type;
	NumberWrapper<unsigned int>	m_Item;
};


void EnumPickerBaseEditor::Display()
{
	int			counter;

	ClearScreen();
	
	for (counter = 0; counter < size(); ++counter)
	{
		Send("`g%2d`n) %-20.20s %s",
			counter + 1,
			name(counter),
			(counter & 1) == 0 ? "" : "\n");
	}
	
	Send(	"\n%s"
			"Enter %s : ",
			(counter & 1) == 0 ? "" : "\n",
			m_Type.c_str());
}


void EnumPickerBaseEditor::Parse(const char *arg)
{
	unsigned int number = atoi(arg) - 1;
	
	if (number >= size())
	{
		Send("That is not a valid choice!\n");
	}
	else
	{
		m_Item = number;
		SetHasChanged();
	}
	
	Pop();
}



class NamedItemPickerEditor : public EnumPickerBaseEditor
{
public:
						NamedItemPickerEditor(const Lexi::String &type, NumberWrapper<unsigned int> item, char *choices[]);
	
	virtual unsigned int	size() const 				{ return m_NumChoices; }
	virtual const char *	name(unsigned int i) const	{ return i < m_NumChoices ? m_Choices[i] : "<ERROR>"; }
	
private:
	char **				m_Choices;
	int					m_NumChoices;
};


NamedItemPickerEditor::NamedItemPickerEditor(const Lexi::String &type, NumberWrapper<unsigned int> item, char *choices[])
:	EnumPickerBaseEditor(type, item)
,	m_Choices(choices)
,	m_NumChoices(0)
{
	while (*m_Choices[m_NumChoices] != '\n')
		++m_NumChoices;
}


class EnumPickerEditor : public EnumPickerBaseEditor
{
public:
	template<typename Enum>
	EnumPickerEditor(const Lexi::String &type, Enum &item)
	:	EnumPickerBaseEditor(type, item)
	,	m_NameResolver(GetEnumNameResolver<Enum>())
	{
	}
	
	virtual unsigned int	size() const 				{ return m_NameResolver.GetNameCount(); }
	virtual const char *	name(unsigned int i) const	{ return m_NameResolver.GetName(i); }
	
private:
	Lexi::String		m_Type;
	const EnumNameResolverBase &m_NameResolver;
};


class VirtualIDChooserEditor : public Editor
{
public:
	enum Type
	{
		None,
		Mobile,
		Object,
		Room,
		Trigger,
		Clan
	};
	
						VirtualIDChooserEditor(const Lexi::String &prompt, VirtualID &vid, bool bAllowNone, Type type = None);
	
	virtual void		Display();
	virtual void		Parse(const char *arg);

private:
	Lexi::String		m_Prompt;
	VirtualID &			m_Virtual;
	bool				m_bAllowNone;
	Type				m_Type;
};


VirtualIDChooserEditor::VirtualIDChooserEditor(const Lexi::String &prompt, VirtualID &vid, bool bAllowNone, Type type)
:	m_Prompt(prompt)
,	m_Virtual(vid)
,	m_bAllowNone(bAllowNone)
,	m_Type(type)
{
}


void VirtualIDChooserEditor::Display()
{
	Send("%s", m_Prompt.c_str());
}


void VirtualIDChooserEditor::Parse(const char *arg)
{
	VirtualID vid(arg);
	bool	bValid = false;
	
	if (vid.IsValid())
	{
		switch (m_Type)
		{
			case Mobile:
				bValid = mob_index.Find(vid);
				break;
			case Object:
				bValid = obj_index.Find(vid);
				break;
			case Room:
				bValid = world.Find(vid);
				break;
			case Trigger:
				bValid = trig_index.Find(vid);
				break;
			case Clan:
				bValid = Clan::GetClan(atoi(arg));
				break;
		}
	}
	
	if (bValid || m_bAllowNone)
	{
		m_Virtual = vid;
		SetHasChanged();
		Pop();
	}
	else
		Send("That does not exist, try again.");
}


class ItemChooserBaseEditor : public Editor
{
public:
	enum OptionsBits
	{
		AllowEmpty,
		ExitOnFail,
		NumOptions
	};
	
	typedef Lexi::BitFlags<NumOptions, OptionsBits> Options;
	
	ItemChooserBaseEditor(const Lexi::String &prompt, Hash zone, Options options = Options())
	:	m_Prompt(prompt)
	,	m_Zone(zone)
	,	m_Options(options)
	{}

	virtual void		Display() { Send("%s: ", m_Prompt.c_str()); }
	virtual void		Parse(const char *arg);
	
	virtual bool		find(VirtualID vid) const = 0;
	virtual void		set(VirtualID vid) = 0;
	virtual void		clear() = 0;

private:
	Lexi::String		m_Prompt;
	Hash				m_Zone;
	Options				m_Options;
};


void ItemChooserBaseEditor::Parse(const char *arg)
{
	VirtualID vid(arg, m_Zone);

	if (atoi(arg) == -1 && m_Options.test(AllowEmpty))
	{
		clear();
	}
	else if (vid.IsValid() && find(vid))
	{
		set(vid);
	}
	else if (!m_Options.test(ExitOnFail))
	{
		Send("That does not exist.\n");
		Display();
		return;
	}
	
	Pop();
}


template<class T, class Map>
class ItemChooserEditor : public ItemChooserBaseEditor
{
public:
	ItemChooserEditor(const Lexi::String &prompt, T *&item, Map &m, Hash zone, ItemChooserBaseEditor::Options options)
	:	ItemChooserBaseEditor(prompt, zone, options)
	,	m_Item(item)
	,	m_Map(m)
	{}
	
	virtual bool		find(VirtualID vid) const	{ return m_Map.Find(vid) != NULL; }
	virtual void		set(VirtualID vid)			{ m_Item = m_Map.Find(vid); }
	virtual void		clear()						{ m_Item = NULL; }
	
private:
	T * &				m_Item;
	Map &				m_Map;
};






class StringEditor : public Editor
{
public:
						StringEditor(const Lexi::String &prompt, Lexi::String &str, const Lexi::String &defaultString = "");
	
	virtual void		Display();
	virtual void		Parse(const char *arg);

protected:
	Lexi::String		m_Prompt;
	Lexi::String &		m_String;
	Lexi::String		m_Default;
};


StringEditor::StringEditor(const Lexi::String &prompt, Lexi::String &str,  const Lexi::String &defaultString) :
	m_Prompt(prompt),
	m_String(str),
	m_Default(defaultString)
{ }


void StringEditor::Display()
{
	Send("%s", m_Prompt.c_str());
}


void StringEditor::Parse(const char *arg)
{
	m_String = *arg ? arg : m_Default;
	SetHasChanged();
	Pop();
}



class ComplexStringEditor : public Editor
{
public:
						ComplexStringEditor(const Lexi::String &prompt, Lexi::String &str, int maxLength);
	
	virtual void		Display();
	virtual void		Parse(const char *arg);

protected:
	Lexi::String		m_Prompt;
	Lexi::String &		m_String;
	int					m_MaxLength;
};



class RoomExitEditor : public Editor
{
public:
						RoomExitEditor(ManagedPtr<RoomExit> &exit, Direction dir, Hash zone);
	
	virtual void		Display();
	virtual void		Parse(const char *arg);

private:
	ManagedPtr<RoomExit> &	m_Exit;
	Direction			m_Direction;
	Hash				m_Zone;
};


RoomExitEditor::RoomExitEditor(ManagedPtr<RoomExit> &exit, Direction dir, Hash zone)
:	m_Exit(exit)
,	m_Direction(dir)
,	m_Zone(zone)
{
	if (!m_Exit)
	{
		m_Exit = new RoomExit;
	}
}


void RoomExitEditor::Display()
{
	ClearScreen();
	Send(	"`c   %s`n\n"
			"`g1`n) Exit to     : `c%s\n"
			"`g2`n) Description :-\n`y%s\n"
			"`g3`n) Door name   : `y%s\n"
			"`g4`n) Key         : `c%s\n"
			"`g5`n) Door flags  : `c%s\n"
			"`g6`n) Purge exit.\n"
			"Enter choice, 0 to quit : ",
			GetEnumName(m_Direction),
			m_Exit->ToRoom() ? m_Exit->ToRoom()->GetVirtualID().Print().c_str() : "<NOWHERE>",
			*m_Exit->GetDescription() ? m_Exit->GetDescription() : "<NONE>",
			m_Exit->GetKeyword(),
			m_Exit->key.Print().c_str(),
			m_Exit->m_Flags.any() ? m_Exit->m_Flags.print().c_str( ): "None");
}


void RoomExitEditor::Parse(const char *arg)
{
	switch (*arg)
	{
		case '0':
			Pop();
			break;
		case '1':
			Push(new ItemChooserEditor<RoomData, RoomMap>(
				"Exit to room number: ",
				m_Exit->room,
				world,
				m_Zone,
				MAKE_BITSET(ItemChooserBaseEditor::Options, ItemChooserBaseEditor::AllowEmpty)));
			break;
		case '2':
			Push(new ComplexStringEditor(
				"Enter exit description",
				m_Exit->m_pDescription,
				/*MAX_EXIT_DESC*/ 256));
			break;
		case '3':
			Push(new StringEditor(
				"Enter keywords: ",
				m_Exit->m_pKeyword,
				"door"));
			break;
		case '4':
			Push(new VirtualIDChooserEditor(
				"Enter key number: ",
				m_Exit->key,
				true,
				VirtualIDChooserEditor::Object));
			break;
		case '5':
			Push(new BitFlagEditor<ExitFlags>("Exit", m_Exit->m_Flags));
			break;
		case '6':
			/*
			 * Delete an exit.
			 */
			m_Exit = NULL;
			SetHasChanged();
			Pop();
			break;
		default:
			Send("Try again: ");
			break;
	}
}





class RoomEditor : public Editor
{
public:
						RoomEditor(VirtualID vnum, ZoneData *zone);
	
	virtual void		Display();
	virtual void		Parse(const char *arg);

private:
	VirtualID			m_Virtual;
	ZoneData *			m_Zone;
	ManagedPtr<RoomData>m_Room;
};


RoomEditor::RoomEditor(VirtualID vid, ZoneData *zone)
:	m_Virtual(vid)
,	m_Zone(zone)
//,	m_Room(NULL)	//	ManagedPtr auto null
{
	RoomData *room = world.Find(vid);
	
	if (room == NULL)
	{
		m_Room = new RoomData(vid);
		m_Room->SetName("An unfinished room");
		m_Room->SetDescription("You are in an unfinished room.\n");
	}
	else
	{
		m_Room = new RoomData(*room);
	}
}


void RoomEditor::Display()
{
	ClearScreen();
	Send(	"-- Room number : [`c%s`n]\n"
			"`G1`n) Name        : `y%s\n"
			"`G2`n) Description :\n`y%s",
			m_Virtual.Print().c_str(),
			m_Room->GetName(),
			m_Room->GetDescription());
				
	
	if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN )
		|| (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLC)
			&& m_Zone->IsBuilder(GetDesc()->m_Character, m_Virtual.ns)))
	{
		Send(
			"`G3`n) Room flags  : `c%s\n"
			"`G4`n) Sector type : `c%s\n"
			"`G5`n) Exit north  : `c%s\n"
			"`G6`n) Exit east   : `c%s\n"
			"`G7`n) Exit south  : `c%s\n"
			"`G8`n) Exit west   : `c%s\n"
			"`G9`n) Exit up     : `c%s\n"
			"`GA`n) Exit down   : `c%s\n",
			m_Room->m_Flags.print().c_str(),
			GetEnumName(m_Room->m_SectorType),
			m_Room->m_Exits[NORTH]	&& m_Room->m_Exits[NORTH]->ToRoom()	? m_Room->m_Exits[NORTH]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
			m_Room->m_Exits[EAST]	&& m_Room->m_Exits[EAST]->ToRoom()	? m_Room->m_Exits[EAST]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
			m_Room->m_Exits[SOUTH]	&& m_Room->m_Exits[SOUTH]->ToRoom()	? m_Room->m_Exits[SOUTH]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
			m_Room->m_Exits[WEST]	&& m_Room->m_Exits[WEST]->ToRoom()	? m_Room->m_Exits[WEST]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>",
			m_Room->m_Exits[UP]		&& m_Room->m_Exits[UP]->ToRoom()	? m_Room->m_Exits[UP]->ToRoom()->GetVirtualID().Print().c_str()		: "<NOWHERE>",
			m_Room->m_Exits[DOWN]	&& m_Room->m_Exits[DOWN]->ToRoom()	? m_Room->m_Exits[DOWN]->ToRoom()->GetVirtualID().Print().c_str()	: "<NOWHERE>");
	}
	
	Send(	"`GB`n) Extra descriptions menu\n"
			"`GQ`n) Quit\n"
			"Enter choice : ");
}


void RoomEditor::Parse(const char *arg)
{
	switch (toupper(*arg))
	{
		case 'Q':
			if (HasChanged())
			{
				Send("Do you wish to save this room internally? ");
//				SwitchEditor();
			}
			else
			{
				Pop();
			}
			return;
		case '1':
			Push(new StringEditor(
				"Enter room name:-\n|",
				m_Room->m_pDescription,
				"A blank room."));
			return;
		case '2':
//			SEND_TO_Q("Enter room description: (/s saves /h for help)\n\n", d);
			Push(new ComplexStringEditor(
				"Enter room description",
				m_Room->m_pDescription,
				/*MAX_ROOM_DESC*/ 1024));
			return;
		case '3':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new BitFlagEditor<RoomFlags>("Room", m_Room->m_Flags));
			}
			return;
		case '4':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new EnumPickerEditor("sector type", m_Room->m_SectorType));
			}
			return;
		case '5':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new RoomExitEditor(m_Room->m_Exits[NORTH], NORTH, m_Zone->GetHash()));
			}
			return;
		case '6':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new RoomExitEditor(m_Room->m_Exits[EAST], EAST, m_Zone->GetHash()));
			}
			return;
		case '7':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new RoomExitEditor(m_Room->m_Exits[SOUTH], SOUTH, m_Zone->GetHash()));
			}
			return;
		case '8':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new RoomExitEditor(m_Room->m_Exits[WEST], WEST, m_Zone->GetHash()));
			}
			return;
		case '9':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new RoomExitEditor(m_Room->m_Exits[UP], UP, m_Zone->GetHash()));
			}
			return;
		case 'A':
			if (STF_FLAGGED(GetDesc()->m_Character, STAFF_OLCADMIN | STAFF_OLC))
			{
				Push(new RoomExitEditor(m_Room->m_Exits[DOWN], DOWN, m_Zone->GetHash()));
			}
			return;
		case 'B':
//			Push(new ExtraDescriptionListEditor(m_Room->ex_description));
			return;
	}
	
	Send("Invalid choice!");
	Display();
}


//	WARNING: Need to be able to access the Room being edited via Descriptor
//	to update changes to room exit RNums!!!
