/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : help.cp                                                        [*]
[*] Usage: Help system code                                               [*]
\***************************************************************************/

#include "help.h"
#include "characters.h"
#include "buffer.h"
#include "utils.h"
#include "rooms.h"
#include "handler.h"
#include "lexifile.h"
#include "db.h"
#include "files.h"

class HelpManagerImplementation : public HelpManager
{
public:
	~HelpManagerImplementation();

	virtual void		Load();
	virtual void		Save();
	
	virtual Entry *		Find(const char *keyword);
	virtual void		Add(const Entry &e);

	virtual const std::vector<Entry *> &GetTable() const { return m_Entries; }

private:
	typedef std::vector<Entry *>	Vector;
	
	Vector				m_Entries;
	
	void				Clear();
};


HelpManager *HelpManager::Instance()
{
	static HelpManagerImplementation sHelpManager;
	return &sHelpManager;
}


HelpManagerImplementation::~HelpManagerImplementation()
{
	Clear();
}


void HelpManagerImplementation::Clear()
{
	FOREACH(Vector, m_Entries, help)
		delete *help;	
}



#define READ_SIZE 1024


void HelpManagerImplementation::Load()
{
	Clear();
		
	Lexi::InputFile	file(HELP_FILE);

	if (file.fail())
	{
		log("SYSERR: Unable to open file %s", file.GetFilename());
		tar_restore_file(file.GetFilename());
	}
		
	BUFFER(key, READ_SIZE+1);
	BUFFER(entry, 32384);
	BUFFER(line, READ_SIZE+1);

	/* get the keyword line */
	file.getline(key, READ_SIZE);
	while (*key != '$')
	{
		Entry *help = new Entry();
		m_Entries.push_back(help);
		
		file.getline(line, READ_SIZE);
		*entry = '\0';
		while (*line != '#')
		{
			sprintf_cat(entry, "%s\n", line);
			file.getline(line, READ_SIZE);
			
			if (file.fail())
			{
				log("SYSERR: Error reading %s, possibly too long of a line for READ_SIZE passed to getline", file.GetFilename());
//				tar_restore_file(file.GetFilename());
			}
		}
		
		help->m_MinimumLevel = 0;
		if ((*line == '#') && (*(line + 1) != 0))
			help->m_MinimumLevel = RANGE(atoi(line + 1), 0, LVL_CODER);

		/* now, add the entry to the index with each keyword on the keyword line */
		help->m_Entry = entry;
		help->m_Keywords = key;
		
		/* get next keyword line (or $) */
		file.getline(key, READ_SIZE);
	}
}


void HelpManagerImplementation::Save()
{
	BUFFER(buf, 32384);
	
	FILE *fp = fopen(HELP_FILE ".new", "w+");
	
	if (!fp) {
		log("SYSERR: Can't open help file '%s'", HELP_FILE);
		exit(1);
	}
	
	
	FOREACH(Vector, m_Entries, iter)
	{
		Entry *help = *iter;
		
		if (help->m_Keywords.empty() || help->m_Entry.empty())
			continue;
		
		/*. Remove the '\n' sequences from entry . */
		Lexi::BufferedFile::PrepareString(buf, help->m_Entry.c_str(), false);
		fprintf(fp, "%s\n"
					"%s#%d\n",
				help->m_Keywords.c_str(),
				buf,
				help->m_MinimumLevel);
	}

	fprintf(fp, "$\n");
	fclose(fp);
	
	remove(HELP_FILE);
	rename(HELP_FILE ".new", HELP_FILE);
}


HelpManager::Entry * HelpManagerImplementation::Find(const char *keyword)
{
	FOREACH(Vector, m_Entries, help)
	{
		if (isname(keyword, (*help)->m_Keywords.c_str()))
			return *help;
	}
	
	return NULL;
}


void HelpManagerImplementation::Add(const Entry &entry)
{
	m_Entries.push_back(new HelpManager::Entry(entry));
}
