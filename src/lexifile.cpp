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
[*] File : lexifile.cp                                                    [*]
[*] Usage: Lexi::BufferedFile is a file wrapper class that reads in an    [*]
[*]        entire file into memory for faster parsing.                    [*]
\***************************************************************************/

#include "types.h"
#include "lexifile.h"
#include "internal.defs.h"
#include "buffer.h"
#include "utils.h"
#include "virtualid.h"
#include "zones.h"

Lexi::BufferedFile::BufferedFile(const char *name) :
	m_pBuffer(NULL),
	m_pCursor(NULL),
	m_Line(0)
{
	if (name)	open(name);
}


Lexi::BufferedFile::~BufferedFile()
{
	delete [] m_pBuffer;
}


void Lexi::BufferedFile::open(const char *name)
{
	delete [] m_pBuffer;	//	In case it's not empty, like reading in a second file
	m_pBuffer = NULL;
	m_pCursor = NULL;
	
	m_Name = name;
	m_Line = 0;
	
	std::ifstream	file(name /*, std::ios::binary*/);
	
	if (file.fail())
	{
		m_Name.clear();
		return;
	}
	
	file.seekg(0, std::ios::end);
	int length = file.tellg();
	file.seekg(0, std::ios::beg);
	
	m_pBuffer = new char[length + 1];
	length = file.read(m_pBuffer, length).gcount();
	m_pBuffer[length] = 0;
	
	m_pCursor = m_pBuffer;
}


void Lexi::BufferedFile::close()
{
	delete [] m_pBuffer;	//	In case it's not empty, like reading in a second file
	m_pBuffer = NULL;
	m_pCursor = NULL;
	
	m_Name.clear();
	m_Line = 0;
}


char * Lexi::BufferedFile::getline()
{
	char *result = m_pCursor;
	
	char *p = result;
	char c = *p;
	
	while (c && c != '\n')
	{
		c = *(++p);
	}
	
	if (*p == '\n')
	{
		*p++ = '\0';	// Write 0 to \n
	}
	m_pCursor = p;	//	That's our cursor!
	++m_Line;
	
	return result;
}


char * Lexi::BufferedFile::getstring()
{
	char *result = m_pCursor;
	
	char *p = result;
	char *dst = result;
	
	char c = *p++;
	if (c)
	{
		do
		{
			if (c == '~')
			{
				//	Single ~ exits
				//	Double ~~ becomes single ~
				if (*p != '~')
					break;
				
				++p;
			}
			else if (c == '\n')
			{
				++m_Line;
			}

			*dst++ = c;
			
			c = *p++;
		}
		while (c);
		
		*dst = '\0';
		
		if (c == '~')
		{
			c = *p++;
			while (c && c != '\n')	c = *p++;
			if (c == '\n')
			{
				++m_Line;
			}
		}
		else 
		{
			--p;	//	Drop back 1... c was the terminator, p is 1 past that!
		}
	}
	
	m_pCursor = p;
	
	return result;
}


void Lexi::BufferedFile::PrepareString(char *dst, const char *src, bool bHandleTildes)
{
	char c;
	do
	{
		c = *src++;
		
		//	Do not write out '\r'
		while (c == '\r')
			c = *src++;
		
		//	Copy the character
		*dst++ = c;
		
		//	Duplicate '~'
		if (bHandleTildes && c == '~')	*dst++ = '~';
	} while (c);
}

	
Lexi::String Lexi::InputFile::getstring() {
	const int MAX_GETSTRING_LENGTH = MAX_STRING_LENGTH * 4;
	BUFFER(buf, MAX_GETSTRING_LENGTH);
	BUFFER(line, MAX_STRING_LENGTH);
	char *terminator;
	size_t	length = 0, lineLength = 0;
	bool	bSkipToEnd = false, bDone = false;

	while (good() && !bDone)
	{
		getline(line, MAX_STRING_LENGTH - 3);
		
/*		if (!good())
		{
			log("SYSERR: Lexi::InputFile::getstring(): format error at %s:%d", filename(), linenumber());
			tar_restore_file(filename);
			exit(1);
		}
*/		
		terminator = strchr(line, '~');
		
		if (terminator)
		{
			*terminator = '\0';
			bDone = true;
		}
		else
		{
			strcat(line, "\n");
		}
		
		size_t	lineLength = strlen(line);
		
		if ( (length + lineLength) >= (size_t)MAX_GETSTRING_LENGTH )
		{
			log("SYSERR: Lexi::InputFile::getstring(): string too long, truncating... %s:%d", GetFilename(), GetLineNumber());			
			
			int	remainingLength = MAX_GETSTRING_LENGTH - length;
			
			if (!bDone)
				remainingLength -= 3;
			
			strncpy(buf + length, line, remainingLength);
			if (!bDone)	//	If this line lacked a terminator, add a linefeed and skip to the end.
			{
				buf[MAX_GETSTRING_LENGTH - 3] = '\0';
				strcat(buf + length, "\n");
				bSkipToEnd = true;
			}
			else
				buf[MAX_GETSTRING_LENGTH - 1] = '\0';
			
			break;
		}
		
		strcat(buf + length, line);
		length += lineLength;
		buf[length] = '\0';
	}
	
	if (bSkipToEnd)
	{
		while (good())
		{
			getline(line, MAX_STRING_LENGTH);
			
			if (fail())
			{
				log("SYSERR: Lexi::InputFile::getstring(): file failure, expecting ~-terminated string... %s:%d", GetFilename(), GetLineNumber());
//				tar_restore_file(filename);
//				exit(1);
			}
			
			if (strchr(line, '~'))
				break;
		}
	}
	
	return buf;
}


void Lexi::OutputParserFile::WriteLine(const char *line)
{
	if (!line || !*line)
		return;
	
	*this << line << std::endl;
}


void Lexi::OutputParserFile::BeginParser(const char *header)
{
	WriteLine(header);
}


void Lexi::OutputParserFile::EndParser()
{
	WriteLine("BREAK");
	m_PendingSectionNames.clear();
	m_Indentation = 0;
}


void Lexi::OutputParserFile::BeginSection(const char *tag)
{
	m_PendingSectionNames.push_back(tag);
}


void Lexi::OutputParserFile::EndSection()
{
	if (m_PendingSectionNames.size())
		m_PendingSectionNames.pop_back();
	else
		--m_Indentation;
}


void Lexi::OutputParserFile::WriteString(const char *tag, const char *str, const char *def)
{
//	if (!str || !*str)
	if (!str || !str_cmp(str, def))
		return;
	
	WriteTag(tag);
	*this << str << std::endl;
}


void Lexi::OutputParserFile::WriteLongString(const char *tag, const char *str, const char *def)
{
	BUFFER(buf, MAX_BUFFER_LENGTH);
	
//	if (!str || !*str)
	if (!str || !str_cmp(str, def))
		return;
	
	Lexi::BufferedFile::PrepareString(buf, str);
	
	WriteTag(tag, false);
	*this << "$" << std::endl << buf << "~" << std::endl;
}


void Lexi::OutputParserFile::WritePossiblyLongString(const char *tag, const Lexi::String &str)
{
	if (str.empty())
		return;
	
	if (   str.find('\n') != Lexi::String::npos
		|| str.find('\t') != Lexi::String::npos)
		WriteLongString("Value", str.c_str());
	else
		WriteString("Value", str.c_str());
}

void Lexi::OutputParserFile::WriteInteger(const char *tag, int val)
{
	WriteTag(tag);
	*this << val << std::endl;
}


void Lexi::OutputParserFile::WriteFloat(const char *tag, float val)
{
	WriteTag(tag);
	*this << val << std::endl;
}


void Lexi::OutputParserFile::WriteEnum(const char *tag, int val, char **table)
{
	WriteTag(tag);
	*this << findtype(val, table) << std::endl;
}


void Lexi::OutputParserFile::WriteEnum(const char *tag, int val, const EnumNameResolverBase &resolver )
{
	WriteTag(tag);
	*this << resolver.GetName(val) << std::endl;
}


void Lexi::OutputParserFile::WriteFlagsAlways(const char *tag, int val, char **table)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	sprintbit(val, table, buf);
	
	WriteTag(tag);
	*this << buf << std::endl;
}


void Lexi::OutputParserFile::WriteFlagsAlways(const char *tag, BitFlagsWrapper::Ptr wrapper, char **table)
{
	WriteTag(tag);
	*this << wrapper->print(table).c_str() << std::endl;
}


void Lexi::OutputParserFile::WriteFlagsAlways(const char *tag, BitFlagsWrapper::Ptr wrapper)
{
	WriteTag(tag);
	*this << wrapper->print().c_str() << std::endl;
}


void Lexi::OutputParserFile::WriteDate(const char *tag, time_t date)
{
	if (date == 0)
		return;
	
	WriteTag(tag);

	*this << date << "\t\t" << Lexi::CreateDateString(date).c_str() << std::endl;
}


void Lexi::OutputParserFile::WriteVirtualID(const char *tag, const VirtualID &vid, Hash zone)
{
	if (!vid.IsValid())
		return;
	
	if (zone && zone != vid.zone)
	{
		ZoneData *z = VIDSystem::GetZoneFromAlias(vid.zone);
		if (z && z->GetHash() == zone)
			zone = vid.zone;	//	Treat it as relative
	}
	
	WriteTag(tag);
	*this << vid.Print(zone).c_str() << std::endl;
}


void Lexi::OutputParserFile::WriteTag(const char *tag, bool postTabs)
{
	if (!m_PendingSectionNames.empty())
	{
		Lexi::StringList::iterator iter = m_PendingSectionNames.begin();
		
		while (iter != m_PendingSectionNames.end())
		{
			for (int indents = m_Indentation; indents > 0; --indents)
				*this << "\t";
			
			*this << "[" << iter->c_str() << "]" << std::endl;
			
			++m_Indentation;
			
			iter = m_PendingSectionNames.erase(iter);
		}
	}
	
	for (int indents = m_Indentation; indents > 0; --indents)
		*this << "\t";
	
	*this << tag << ":";
	
	if (postTabs)
	{
		*this << "\t";
		if (strlen(tag) < 7)	*this << "\t";
		if (strlen(tag) < 3)	*this << "\t";
	}
}
