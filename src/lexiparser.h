/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2005-2007        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : lexiparser.h                                                   [*]
[*] Usage: Lexi::Parser parses Lexi's special file format for easy        [*]
[*]        reading by client code.  It relies on the Lexi::File.          [*]
[*]        The Lexi file format is heavily derived from 
\***************************************************************************/


#ifndef __FILEPARSER_H__
#define __FILEPARSER_H__

#include <vector>

#include "lexifile.h"
#include "bitflags.h"
#include "virtualid.h"


namespace Lexi {
	class Parser;
}


class Lexi::Parser
{
public:
						Parser() : m_Root(""), m_CurrentRoot(&m_Root) { m_RootHistory.reserve(8); }
						~Parser() {}
	
	void				Parse(Lexi::BufferedFile & file);
	
/*	class BitFlags : public Lexi::BitFlags<256>
	{
	public:
		BitFlags() {}
		
		template<typename F>
		BitFlags(const F &f)
		{
			*this = f;
		}
	};
*/
	typedef Lexi::BitFlags<256> BitFlags;
	
	const char *		GetString(const char *name, const char *def = "") const { return GetString(GetField(name), def); }
	int					GetInteger(const char *name, int def = 0) const { return GetInteger(GetField(name), def); }
	float				GetFloat(const char *name, float def = 0.0f) const { return GetFloat(GetField(name), def); }
	template<typename Enum>
	Enum				GetEnum(const char *name, Enum def) const { return (Enum)GetEnum(GetField(name), GetEnumNameResolver<Enum>(), def); }
	template<typename Enum>
	Enum				GetEnum(const char *name, char **table, Enum def = (Enum)0) const { return (Enum)GetEnum(GetField(name), table, def); }
	int					GetEnum(const char *name, char **table, int def = 0) const { return GetEnum(GetField(name), table, def); }
	Flags				GetFlags(const char *name, char **table, Flags def = 0) const{ return GetFlags(GetField(name), table, def); }
	template<typename Enum>
	Flags				GetFlags(const char *name, Flags def = 0) const{ return GetFlags(GetField(name), (char **)GetEnumNameResolver<Enum>().GetNames(), def); }
	BitFlags			GetBitFlags(const char *name, char **table, const BitFlags &def = BitFlags()) const{ return GetBitFlags(GetField(name), table, def); }
	template<typename Enum>
	BitFlags			GetBitFlags(const char *name, const BitFlags &def = BitFlags()) const{ return GetBitFlags(GetField(name), GetEnumNameResolver<Enum>(), def); }
	VirtualID			GetVirtualID(const char *name, Hash zone = 0) const { return GetVirtualID(GetField(name), zone); }
	
	const char *		GetIndexedField(const char *name, int n) const;
	Lexi::String		GetIndexedSection(const char *name, int n) const;
	
	const char *		GetIndexedString(const char *name, int n, const char *def = "") const { return GetString(GetSectionIndexedField(name, n), def); }
	int					GetIndexedInteger(const char *name, int n, int def = 0) const { return GetInteger(GetSectionIndexedField(name, n), def); }
	float				GetIndexedFloat(const char *name, int n, float def = 0.0f) const { return GetFloat(GetSectionIndexedField(name, n), def); }
	template<typename Enum>
	Enum				GetIndexedEnum(const char *name, int n, char **table, Enum def = (Enum)0) const{ return (Enum)GetEnum(GetSectionIndexedField(name, n), table, def); }
	int					GetIndexedEnum(const char *name, int n, char **table, int def = 0) const{ return GetEnum(GetSectionIndexedField(name, n), table, def); }
	Flags				GetIndexedFlags(const char *name, int n, char **table, Flags def = 0) const{ return GetFlags(GetSectionIndexedField(name, n), table, def); }
	BitFlags			GetIndexedBitFlags(const char *name, int n, char **table, const BitFlags &def = BitFlags()) const{ return GetBitFlags(GetSectionIndexedField(name, n), table, def); }
	template<typename Enum>
	BitFlags			GetIndexedBitFlags(const char *name, int n, const BitFlags &def = BitFlags()) const{ return GetBitFlags<Enum>(GetSectionIndexedField(name, n), def); }
	VirtualID			GetIndexedVirtualID(const char *name, int n, Hash zone = 0) const { return GetVirtualID(GetSectionIndexedField(name, n), zone); }
		
	size_t				NumFields(const char *name = "") const;
	size_t				NumSections(const char *name = "") const;
	
	bool				DoesFieldExist(const char *name) const;
	bool				DoesSectionExist(const char *name) const;
	
	bool				SetCurrentRoot(const char *root);
	bool				SetCurrentRootIndexed(const char *name, int n);
	void				ResetCurrentRoot();
	
	//	Lexi::String support
	const char *		GetString(const Lexi::String &name, const char *def = "") const { return GetString(name.c_str(), def); }
	int					GetInteger(const Lexi::String &name, int def = 0) const { return GetInteger(name.c_str(), def); }
	float				GetFloat(const Lexi::String &name, float def = 0.0f) const { return GetFloat(name.c_str(), def); }
	template<typename Enum>
	Enum				GetEnum(const Lexi::String &name, char **table, Enum def = (Enum)0) const { return GetEnum<Enum>(name.c_str(), table, def); }
	int					GetEnum(const Lexi::String &name, char **table, int def = 0) const { return GetEnum(name.c_str(), table, def); }
	Flags				GetFlags(const Lexi::String &name, char **table, Flags def = 0) const { return GetFlags(name.c_str(), table, def); }
	BitFlags			GetBitFlags(const Lexi::String &name, char **table, const BitFlags &def = BitFlags()) const { return GetBitFlags(name.c_str(), table, def); }
	template<typename Enum>
	BitFlags			GetBitFlags(const Lexi::String &name, const BitFlags &def = BitFlags()) const { return GetBitFlags<Enum>(name.c_str(), def); }
	VirtualID			GetVirtualID(const Lexi::String &name, Hash zone = 0) const { return GetVirtualID(name.c_str(), zone); }
	
	const char *		GetIndexedField(const Lexi::String &name, int n) const { return GetIndexedField(name.c_str(), n); }
	Lexi::String		GetIndexedSection(const Lexi::String &name, int n) const { return GetIndexedSection(name.c_str(), n); }
	
	const char *		GetIndexedString(const Lexi::String &name, int n, const char *def = "") const { return GetIndexedString(name.c_str(), n, def); }
	int					GetIndexedInteger(const Lexi::String &name, int n, int def = 0) const { return GetIndexedInteger(name.c_str(), n, def); }
	float				GetIndexedFloat(const Lexi::String &name, int n, float def = 0.0f) const { return GetIndexedFloat(name.c_str(), n, def); }
	template<typename Enum>
	Enum				GetIndexedEnum(const Lexi::String &name, int n, char **table, Enum def = (Enum)0) const { return GetIndexedEnum<Enum>(name.c_str(), n, table, def); }
	int					GetIndexedEnum(const Lexi::String &name, int n, char **table, int def = 0) const { return GetIndexedEnum(name.c_str(), n, table, def); }
	Flags				GetIndexedFlags(const Lexi::String &name, int n, char **table, Flags def = 0) const { return GetIndexedFlags(name.c_str(), n, table, def); }
	BitFlags			GetIndexedBitFlags(const Lexi::String &name, int n, char **table, const BitFlags &def = BitFlags()) const { return GetIndexedBitFlags(name.c_str(), n, table, def); }
	template<typename Enum>
	BitFlags			GetIndexedBitFlags(const Lexi::String &name, int n, const BitFlags &def = BitFlags()) const { return GetIndexedBitFlags<Enum>(name.c_str(), n, def); }
	VirtualID			GetIndexedVirtualID(const Lexi::String &name, int n, Hash zone = 0) const { return GetIndexedVirtualID(name.c_str(), n, zone); }

	size_t				NumFields(const Lexi::String &name) const { return NumFields(name.c_str()); }
	size_t				NumSections(const Lexi::String &name) const { return NumSections(name.c_str()); }
	
	bool				DoesFieldExist(const Lexi::String &name) const { return DoesFieldExist(name.c_str()); }
	bool				DoesSectionExist(const Lexi::String &name) const { return DoesSectionExist(name.c_str()); }

	bool				SetCurrentRoot(const Lexi::String &name) { return SetCurrentRoot(name.c_str()); }
	bool				SetCurrentRootIndexed(const Lexi::String &name, int n) { return SetCurrentRootIndexed(name.c_str(), n); }
	
private:
	
	class Field
	{
	public:
							Field(const char *name, const char *value) : m_Name(name), m_Value(value) { }
		
		const char *		GetName() const { return m_Name; }
		const char *		GetValue() const { return m_Value; }
		
	private:
		const char *		m_Name;
		const char *		m_Value;
	};
	
	class Section
	{
	public:
							Section(const char *name) : m_Name(name), m_Parent(NULL) { }
							~Section();
		
		Section *			GetParent() { return m_Parent; }
		const Section *		GetParent() const { return m_Parent; }
		void				SetParent(Section *parent) { m_Parent = parent; }
		const char *		GetName() const { return m_Name; }
		
		Section *			CreateSection(const char *sectionName);
		const Section *		GetSection(const char *sectionName) const;
		
		void				CreateField(const char *name, const char *value);
		const Field *		GetField(const char *name) const;
		
		size_t				NumSections() const { return m_Sections.size(); }
		size_t				NumFields() const { return m_Fields.size(); }
		
		const Section *		GetIndexedSection(int n) const;
		const Field *		GetIndexedField(int n) const;
		
	private:
		typedef std::vector<Section *>		Sections;
		typedef std::vector<Field>			Fields;
		
		static int SectionComparator(const Section *a, const Section *b);
		static int FieldComparator(const Field &a, const Field &b);
		
		static bool SectionFinder(const Section *section, const char *sectionName);
		static bool FieldFinder(const Field &section, const char *sectionName);

		const char *		m_Name;
		Section *			m_Parent;
		Sections			m_Sections;
		Fields				m_Fields;
	};


	const Section *		GetSection(const char *name) const;
	const Field *		GetField(const char *name) const;
	const Field *		GetSectionIndexedField(const char *name, int n) const;
	
	const char *		GetString(const Field *pField, const char *def = "") const;
	int					GetInteger(const Field *pField, int def = 0) const;
	float				GetFloat(const Field *pField, float def = 0.0f) const;
	int					GetEnum(const Field *pField, const EnumNameResolverBase &resolver, int def) const;
	int					GetEnum(const Field *pField, char **table, int def = 0) const;
	Flags				GetFlags(const Field *pField, char **table, int def = 0) const;
	BitFlags			GetBitFlags(const Field *pField, char **table, const BitFlags &def = BitFlags()) const;
	BitFlags			GetBitFlags(const Field *pField, const EnumNameResolverBase &nameResolver, const BitFlags &def = BitFlags()) const;
	VirtualID			GetVirtualID(const Field *pField, Hash zone = 0) const;

	Section				m_Root;
	const Section *		m_CurrentRoot;
	std::vector<const Section *>	m_RootHistory;

public:
	class RootResetObject
	{
	public:
		RootResetObject(Parser *parser) : m_pParser(parser) {}
		RootResetObject(Parser &parser) : m_pParser(&parser) {}
		RootResetObject(RootResetObject &old) : m_pParser(old.m_pParser) { old.m_pParser = NULL; }
		~RootResetObject() { if (m_pParser) m_pParser->ResetCurrentRoot(); }
		
	private:
		Parser *			m_pParser;
	};

//	typedef ManagedPtr<ParserRootReset>	Root;
/*	typedef ParserRootReset		Root;
	Root				ChangeRoot(const char *name)
	{
		SetCurrentRoot(name);
		return ParserRootReset(this);
	}
	
	Root				ChangeRootIndexed(const char *name, int n) 
	{
		SetCurrentRootIndexed(name, n);
		return ParserRootReset(this);
	}
*/
};


#define PARSER_SET_ROOT(parser, name) \
	Lexi::Parser::RootResetObject	reset##Parser##Root(parser); \
	parser.SetCurrentRoot(name);

#define PARSER_SET_ROOT_INDEXED(parser, name, n) \
	Lexi::Parser::RootResetObject	reset##Parser##Root(parser); \
	parser.SetCurrentRootIndexed(name, n);


#endif
