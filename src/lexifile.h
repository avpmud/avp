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
[*] File : lexifile.h                                                     [*]
[*] Usage: Lexi::BufferedFile is a file wrapper class that reads in an    [*]
[*]        entire file into memory for faster parsing.                    [*]
\***************************************************************************/

#ifndef __LEXIFILE_H__
#define __LEXIFILE_H__

#include <fstream>

#include "lexistring.h"
#include "bitflags.h"

struct VirtualID;

namespace Lexi
{

class InputFile : public std::ifstream
{
public:
	explicit InputFile(const char* s, std::ios_base::openmode mode = std::ios_base::in) :
		std::ifstream(s, mode), m_Name(s), m_Line(0) {}
	void				open(const char* s, std::ios_base::openmode mode = std::ios_base::in)
		{ std::ifstream::open(s, mode); m_Name = s;}
	void				close()	{ std::ifstream::close(); m_Name.clear(); }

	InputFile &			getline(char_type* s, std::streamsize n) { ++m_Line; std::ifstream::getline(s, n); return *this; }
	Lexi::String		getstring();

	const char *		GetFilename() const { return m_Name.c_str(); }
	int					GetLineNumber() const { return m_Line; }
private:
	String	m_Name;
	int		m_Line;
};


class OutputParserFile : public std::ofstream
{
public:
	explicit			OutputParserFile(const char *s, std::ios_base::openmode mode = std::ios_base::out) :
		std::ofstream(s, mode), m_Name(s), m_Indentation(0) {}
	
	void				open(const char* s, std::ios_base::openmode mode = std::ios_base::out)
		{ std::ofstream::open(s, mode); m_Name = s; m_Indentation = 0; m_PendingSectionNames.clear(); }
	
	void				close()	{ std::ofstream::close(); m_Name.clear(); }
	
	void				WriteLine(const char *line);
	
	void				BeginParser(const char *header = NULL);
	void				BeginParser(const Lexi::String &str) { BeginParser(str.c_str()); }
	void				EndParser();
	
	void				BeginSection(const char *tag);
	void				BeginSection(const Lexi::String &str) { BeginSection(str.c_str()); }
	void				EndSection();
	
//	void				WriteString(const char *tag, const char *str);
	void				WriteString(const char *tag, const char *str, const char *def = ""); // { if (str_cmp(str, def)) WriteString(tag, str); }
	void				WriteString(const char *tag, const Lexi::String &str, const char *def = "") { WriteString(tag, str.c_str(), def); }
//	void				WriteLongString(const char *tag, const char *str);
	void				WriteLongString(const char *tag, const char *str, const char *def = ""); // { if (str_cmp(str, def)) WriteLongString(tag, str); }
	void				WriteLongString(const char *tag, const Lexi::String &str, const char *def = "") { WriteLongString(tag, str.c_str(), def); }
	void				WritePossiblyLongString(const char *tag, const Lexi::String &str);
	void				WriteInteger(const char *tag, int val);
	inline void			WriteInteger(const char *tag, int val, int def) { if (val != def) WriteInteger(tag, val); }
	void				WriteFloat(const char *tag, float val);
	inline void			WriteFloat(const char *tag, float val, float def) { if (val != def) WriteFloat(tag, val); }
	void				WriteEnum(const char *tag, int val, char **table);
	void				WriteEnum(const char *tag, int val, char **table, int def) { if (val != def) WriteEnum(tag, val, table); }
	void				WriteEnum(const char *tag, int val, const EnumNameResolverBase &resolver);
	template<typename Enum>
	void				WriteEnum(const char *tag, Enum val) { WriteEnum(tag, val, GetEnumNameResolver<Enum>()); }
	template<typename Enum>
	void				WriteEnum(const char *tag, Enum val, Enum def) { if (val != def) WriteEnum(tag, val, GetEnumNameResolver<Enum>()); }
	void				WriteFlags(const char *tag, int val, char **table) { if (val != 0) WriteFlagsAlways(tag, val, table); }
	template<typename Enum>
	void				WriteFlags(const char *tag, Enum val) { if (val != 0) WriteFlagsAlways(tag, val, (char **)GetEnumNameResolver<Enum>().GetNames()); }
	void				WriteFlagsAlways(const char *tag, int val, char **table);
	template<size_t N, typename Enum>
	void				WriteFlags(const char *tag, const BitFlags<N, Enum> &bf, char **table) { if (bf.any()) WriteFlagsAlways(tag, bf.Wrap(), table); }
	template<size_t N, typename Enum>
	void				WriteFlagsAlways(const char *tag, const BitFlags<N, Enum> &bf, char **table) { WriteFlagsAlways(tag, bf.Wrap(), table); }
	void				WriteFlagsAlways(const char *tag, BitFlagsWrapper::Ptr wrapper, char **table);

	template<size_t N, typename Enum>
	void				WriteFlags(const char *tag, const BitFlags<N, Enum> &bf) { if (bf.any()) WriteFlagsAlways(tag, bf.Wrap()); }
	template<size_t N, typename Enum>
	void				WriteFlagsAlways(const char *tag, const BitFlags<N, Enum> &bf) { WriteFlagsAlways(tag, bf.Wrap()); }
	void				WriteFlagsAlways(const char *tag, BitFlagsWrapper::Ptr wrapper);
	
	void				WriteDate(const char *tag, time_t date);
	void				WriteVirtualID(const char *tag, const VirtualID &vid, Hash zone = 0);

	const char *		GetFilename() const { return m_Name.c_str(); }
	
	class SectionWrapper
	{
	public:
		SectionWrapper(OutputParserFile &file, Lexi::String section)
		:	m_File(file)
		,	m_Section(section)
		{
			m_File.BeginSection(m_Section);
		}
		
		~SectionWrapper()
		{
			m_File.EndSection();
		}
		
		operator bool() 
		{
			return true;
		}
		
	private:
		OutputParserFile &	m_File;
		Lexi::String		m_Section;
	};

#define PARSER_SECTION(F, N)	Lexi::OutputParserFile::SectionWrapper F##SectionWrapper(F, N)
	
private:
	void				WriteTag(const char *tag, bool postTabs = true);
	
	String				m_Name;
	int					m_Indentation;
	Lexi::StringList	m_PendingSectionNames;
};


class BufferedFile
{
public:
						BufferedFile(const char *name = NULL);
						~BufferedFile();
						
	void				open(const char *s);
	void				close();
	
	bool				good() { return m_pCursor && *m_pCursor; }
	bool				fail() { return m_Name.empty(); }
	bool				bad() { return !m_pCursor; }
	bool				eof() { return m_pCursor && *m_pCursor == 0; }
	
	char *				getline();
	char *				getstring();
	
	char *				getall() { return m_pBuffer; }
	
	const char *		peek() { return m_pCursor; }

	const char *		GetFilename() const { return m_Name.c_str(); }
	int					GetLineNumber() const { return m_Line; }
	
	static void 		PrepareString(char *dst, const char *src, bool bHandleTildes = true);
	
private:
	char *				m_pBuffer;
	char *				m_pCursor;
	
	String				m_Name;
	int					m_Line;
};


}	//	namespace Lexi

#endif
