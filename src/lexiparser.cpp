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
[*] File : lexiparser.cp                                                  [*]
[*] Usage: Lexi::Parser parses Lexi's special file format for easy        [*]
[*]        reading by client code.  It relies on the Lexi::File.          [*]
[*]        The Lexi file format is heavily derived from 
\***************************************************************************/

#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "lexiparser.h"

int str_cmp_numeric(const char *arg1, const char *arg2);


#define PARSER_BUFFER_SIZE	8192

static int sectionCount = 0;
static int fieldCount = 0;


namespace Lexi {

void Parser::Parse(Lexi::BufferedFile & file)
{
	if (file.bad())
		return;
	
	std::list<Section *>	sectionStack;
	Section *				pSection = &m_Root;
	
	while (!file.eof())
	{
		char *line = file.getline();
		if (!str_cmp(line, "BREAK"))
			break;
		
		int lineDepth = 0;
		
		//	Skip non-tab spaces
		while (*line == ' ')
			++line;
		
		//	Count and skip tabs
		while (*line == '\t')
		{
			++lineDepth;
			++line;
		}
		
		if (lineDepth > sectionStack.size())
		{
			//	log
			log("Warning: depth of field doesn't match (%d) actual depth (%d)... %s:%d",
				lineDepth, sectionStack.size(), file.GetFilename(), file.GetLineNumber());
			continue;
		}
		
		while (lineDepth < sectionStack.size())
		{
			pSection = sectionStack.back();
			sectionStack.pop_back();
		}
		
		//	Is it the start of a new section?
		char *closingBracket = strchr(line, ']');
		if (*line == '[' && closingBracket)
		{
			char *sectionName = line + 1;
			*closingBracket = '\0';
			
			if (!*sectionName)	//	Empty section name = bad
				continue;
			
			sectionStack.push_back(pSection);
			
			char *subsectionSeparator = strchr(sectionName, '/');
			if (subsectionSeparator)
			{
				if (!subsectionSeparator[1])
				{
					sectionStack.pop_back();
					continue;	//	Section/ not allowed, must be Section/Subsection	
				}
					
				*subsectionSeparator = '\0';
				//	Dont push it to the stack, because it won't match the depth!
				pSection = pSection->CreateSection(sectionName);
				sectionName = subsectionSeparator + 1;
			}
			
			pSection = pSection->CreateSection(sectionName);
			
			continue;
		}
		
		//	It's a field (potentially)
		char *	fieldname = line;
		
		line = strchr(line, ':');
		
		if (!line)
		{
			log("Warning: invalid line '%s'... %s:%d", line, file.GetFilename(), file.GetLineNumber());
			continue;
		}
		
		*line++ = '\0';
		
		char c = *line;
		if (c == '$')
		{
			pSection->CreateField(fieldname, file.getstring());
		}
		else if (c == '\t')
		{
			while (*line == '\t')
				++line;
			pSection->CreateField(fieldname, line);
		}
		else
		{
			log("Warning: invalid line '%s'... %s:%d", line, file.GetFilename(), file.GetLineNumber());
			continue;
		}
	}
}


const char *Parser::GetString(const Field *pField, const char *def) const
{
	return pField ? pField->GetValue() : def;
}


int Parser::GetInteger(const Field *pField, int def) const
{
	return pField ? atoi(pField->GetValue()) : def;
}


float Parser::GetFloat(const Field *pField, float def) const
{
	return pField ? atof(pField->GetValue()) : def;
}


int Parser::GetEnum(const Field *pField, char **table, int def) const
{
	if (pField)
	{
		//	Look it up!
		int index = search_block(pField->GetValue(), table, true);
		if (index != -1)
			return index;
	}
	
	return def;
}


int Parser::GetEnum(const Field *pField, const EnumNameResolverBase &resolver, int def) const
{
	if (pField)
	{
		//	Look it up!
		int index = resolver.GetEnumByName(pField->GetValue());
		if (index != -1)
			return index;
	}
	
	return def;
}


Flags Parser::GetFlags(const Field *pField, char **table, int def) const
{
	if (!pField)	return def;
	
	BUFFER(bit, PARSER_BUFFER_SIZE);
	Flags	result = 0;
	const char *bits = pField->GetValue();
	
	skip_spaces(bits);
	
	while (*bits)
	{
		bits = any_one_arg(bits, bit);
		
		int index = search_block(bit, table, true);
		if (index != -1)
			result |= (1 << index);
	}
	
	return result;
}


Parser::BitFlags Parser::GetBitFlags(const Field *pField, char **table, const BitFlags &def) const
{
	if (!pField)	return def;	
	
	BUFFER(bit, PARSER_BUFFER_SIZE);
	BitFlags	result;
	const char *bits = pField->GetValue();
	
	skip_spaces(bits);
	
	while (*bits)
	{
		bits = any_one_arg(bits, bit);
		
		int index = search_block(bit, table, true);
		if (index != -1)
			result.set(index);
	}

	return result;
}


Parser::BitFlags Parser::GetBitFlags(const Field *pField, const EnumNameResolverBase &nameResolver, const BitFlags &def) const
{
	if (!pField)	return def;	
	
	BUFFER(bit, PARSER_BUFFER_SIZE);
	BitFlags	result;
	const char *bits = pField->GetValue();
	
	skip_spaces(bits);
	
	while (*bits)
	{
		bits = any_one_arg(bits, bit);
		
		int index = search_block(bit, nameResolver.GetNames(), true);
		if (index != -1)
			result.set(index);
	}

	return result;
}


VirtualID Parser::GetVirtualID(const Field *pField, Hash zone) const
{
	if (!pField || !*pField->GetValue())	return VirtualID();
	
	return VirtualID(pField->GetValue(), zone);
}


size_t Parser::NumFields(const char *name) const
{
	const Section *pSection = GetSection(name);
	
	return pSection ? pSection->NumFields() : 0;
}

size_t Parser::NumSections(const char *name) const
{
	const Section *pSection = GetSection(name);
	
	return pSection ? pSection->NumSections() : 0;
}

bool Parser::DoesFieldExist(const char *name) const
{
	return GetField(name) != NULL;
}

bool Parser::DoesSectionExist(const char *name) const
{
	return GetSection(name) != NULL;
}

const char * Parser::GetIndexedField(const char *name, int n) const
{
	const Field *pField = GetSectionIndexedField(name, n);

	return pField ? pField->GetName() : NULL;
}


Lexi::String Parser::GetIndexedSection(const char *name, int n) const
{
	const Section *pSection = GetSection(name);
	
	if (pSection)	pSection = pSection->GetIndexedSection(n);
	
	if (!pSection)	return "";

	Lexi::String	out = pSection->GetName();
	while ( (pSection = pSection->GetParent())
		&& *pSection->GetName())
	{
		out = "/" + out;
		out = pSection->GetName() + out;
	}
	return out;
}


bool Parser::SetCurrentRoot(const char *root)
{
	m_RootHistory.push_back(m_CurrentRoot);
	m_CurrentRoot = GetSection(root);
	return m_CurrentRoot != NULL;
}


bool Parser::SetCurrentRootIndexed(const char *root, int n)
{
	m_RootHistory.push_back(m_CurrentRoot);
	const Section *section = GetSection(root);
	m_CurrentRoot = section ? section->GetIndexedSection(n) : NULL;
	return m_CurrentRoot != NULL;
}

void Parser::ResetCurrentRoot()
{
	if (m_RootHistory.empty()) { return; }
	
	m_CurrentRoot = m_RootHistory.back();
	m_RootHistory.pop_back();
}


const Parser::Section *Parser::GetSection(const char *name) const
{
	BUFFER(buf, PARSER_BUFFER_SIZE);
	const Section *	pSection = m_CurrentRoot;
	char *p = NULL;
	
	strcpy(buf, name);
	
	while (pSection && (p = strsep(&buf, "/")) && *p)
	{
		pSection = pSection->GetSection(p);
	}
	
	return pSection;
}


const Parser::Field *Parser::GetField(const char *name) const
{
	const Section *	pSection = m_CurrentRoot;
	const char *p = name;
	const char *delimeter;
	
	if (!pSection)
		return NULL;
	
	skip_spaces(p);
	
	do
	{
		delimeter = strchr(p, '/');
		
		if (delimeter)
		{
			Lexi::String	sectionName(p, delimeter - p);
			p = delimeter + 1;
			
			pSection = pSection->GetSection(sectionName.c_str());
			
			if (!pSection)
				return NULL;
		}
		else
		{
			return pSection->GetField(p);
		}
	} while (delimeter != NULL);
	
	return NULL;
}


const Parser::Field * Parser::GetSectionIndexedField(const char *name, int n) const
{
	const Section *pSection = GetSection(name);
	
	if (pSection)
	{
		return pSection->GetIndexedField(n);
	}
	
	return NULL;
}


Parser::Section::~Section()
{
	Sections::iterator section = m_Sections.begin(),
					   sectionsEnd = m_Sections.end();
	
	for (; section != sectionsEnd; ++section)
		delete *section;
}


bool Parser::Section::SectionFinder(const Section *section, const char *name)
{
	int result = str_cmp /*_numeric*/(section->GetName(), name);
	return result == 0;
}

bool Parser::Section::FieldFinder(const Field &field, const char *name)
{
	int result = str_cmp /*_numeric*/(field.GetName(), name);
	return result == 0;
}


Parser::Section *Parser::Section::CreateSection(const char *sectionName)
{
	Section *pSection = const_cast<Section *>(GetSection(sectionName));
	
	if (!pSection)
	{
		pSection = new Section(sectionName);
		pSection->SetParent(this);
		
//		Sections::iterator iter = std::lower_bound(m_Sections.begin(), m_Sections.end(), sectionName, SectionFinder);
		Sections::iterator iter = std::lower_bound(m_Sections.begin(), m_Sections.end(), pSection, SectionComparator);
		
		m_Sections.insert(iter, pSection);
	}
	
	return pSection;
}


const Parser::Section *Parser::Section::GetSection(const char *sectionName) const
{
//	Sections::const_iterator iter = std::lower_bound(m_Sections.begin(), m_Sections.end(),
//			sectionName, SectionFinder);
	Sections::const_iterator iter = Lexi::FindIf(m_Sections,
		std::bind2nd(std::ptr_fun(SectionFinder), sectionName));
	
	if (iter != m_Sections.end() && !str_cmp((*iter)->GetName(), sectionName))
		return *iter;
	
	return NULL;
}


void Parser::Section::CreateField(const char *name, const char *value)
{
//	Fields::iterator iter = std::lower_bound(m_Fields.begin(), m_Fields.end(), name, FieldFinder);
	Field	newField(name, value);
	Fields::iterator iter = std::lower_bound(m_Fields.begin(), m_Fields.end(), newField, FieldComparator);
	
	m_Fields.insert(iter, newField);
}


const Parser::Field *Parser::Section::GetField(const char *name) const
{
//	Fields::const_iterator iter = std::lower_bound(m_Fields.begin(), m_Fields.end(),
//			name, FieldFinder);
//	Fields::const_iterator iter = Lexi::FindIf(m_Fields,
//			std::bind2nd(std::ptr_fun(FieldFinder), name));
	
//	if (iter != m_Fields.end() && !str_cmp(iter->GetName(), name))
//		return &*iter;
	FOREACH_CONST(Fields, m_Fields, iter)
		if (!str_cmp(iter->GetName(), name))
			return &*iter;

	return NULL;
}



const Parser::Section *Parser::Section::GetIndexedSection(int n) const
{
	if (n >= 0 && n < m_Sections.size())
	{
		return m_Sections[n];
	}

	return NULL;
}

const Parser::Field *Parser::Section::GetIndexedField(int n) const
{
	if (n >= 0 && n < m_Fields.size())
	{
		return &m_Fields[n];
	}
	
	return NULL;
}


int Parser::Section::SectionComparator(const Section *a, const Section *b) { return str_cmp_numeric(a->GetName(), b->GetName()) < 0; }
int Parser::Section::FieldComparator(const Field &a, const Field &b) { return str_cmp_numeric(a.GetName(), b.GetName()) < 0; }

}	//	namespace Lexi


#if 0
void TestParser()
{
	extern char *sector_types[];
	
	Lexi::BufferedFile	file("parsertest.txt");
	Lexi::Parser		parser;
	
	parser.Parse(file);
	
	const char *	name = parser.GetString("Name", "ERROR");
	const char *	desc = parser.GetString("Desc", "ERROR");
	const char *	flags = parser.GetString("Flags", "ERROR");
	int				sector = parser.GetEnum<RoomSector>("SecType", -1);
	int				pointsSpecIdx = parser.GetInteger("POINTS/SpecIdx", -1);
	int				pointsRating = parser.GetInteger("POINTS/Rating", -1);
	int				neVnum = parser.GetInteger("EXIT northeast/ToVnum", -1);
	const char *	neMaterial = parser.GetString("EXIT northeast/Material", "ERROR");
	float			coordinatesX = parser.GetFloat("COORDINATES/X", -1.0f);
	float			coordinatesY = parser.GetFloat("COORDINATES/Y", -1.0f);
	float			coordinatesZ = parser.GetFloat("COORDINATES/Z", -1.0f);
	
	size_t			numSelling = parser.NumSections("SELLING");
	Lexi::String	thirdSellingSection = parser.GetIndexedSection("SELLING", 2);
	const char *	thirdSellingType = parser.GetString((thirdSellingSection + "/Type").c_str(), "ERROR");
	int				thirdSellingVNum = parser.GetInteger((thirdSellingSection + "/Vnum").c_str(), -1);
	
	size_t			numSkills = parser.NumFields("SKILLS");
	Lexi::String	secondSkill = parser.GetIndexedField("SKILLS", 1);
	int				secondSkillValue = parser.GetIndexedInteger("SKILLS", 1);
	
	file.close();
}
#endif
