/*************************************************************************
*   File: files.c++                  Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for file handling                                 *
*************************************************************************/



#include "types.h"
#include "utils.h"
#include "buffer.h"
#include "internal.defs.h"
#include "files.h"

// External variables
extern FILE * logfile;



/* read and allocate space for a '~'-terminated string from a given file */
Lexi::String fread_lexistring(FILE * fl, char *error, const char *filename) {
	BUFFER(buf, MAX_STRING_LENGTH * 4);
	BUFFER(tmp, MAX_STRING_LENGTH);
	char *point;
	int done = 0, length = 0, templength = 0;

	do {
		if (!fgets(tmp, 512, fl)) {
			log("SYSERR: fread_lexistring: format error at or near %s", error);
			tar_restore_file(filename);
			exit(1);
		}
/* If there is a '~', not followed by '~~', end the string; else put an "\n" over the '\n'. */
		point = strchr(tmp, '~');
		while (point)
		{
			if (*(point + 1) == '~')
				point = strchr(point + 2, '~');
			else
				break;
		}
		
		if (point) {
			*point = '\0';
			done = 1;
		}
		else if ((point = strchr(tmp, '\n')))
		{
			*(point++) = '\n';
			*point = '\0';
		}

		templength = strlen(tmp);

		if (length + templength >= ((MAX_STRING_LENGTH * 4) - 1)) {
			log("SYSERR: fread_lexistring: string too large (db.c)");
			log(error);
			tar_restore_file(filename);
			exit(1);
		} else {
			strcat(buf + length, tmp);
			length += templength;
		}
	} while (!done);

	char *p = strchr(buf, '~');
	if (p)
	{
		char *dst = p;
		
		char c;
		do
		{
			c = *p++;
			if (c == '~')	//	if we hit a ~, skip it, copy next character
				c = *p++;
			*dst++ = c;
		} while (c);
	}
		
	return buf;
}




Lexi::String fread_action(FILE * fl, int nr)
{
	BUFFER(buf, MAX_STRING_LENGTH);

	fgets(buf, MAX_STRING_LENGTH, fl);
	if (feof(fl)) {
		log("SYSERR: fread_action - unexpected EOF near action #%d", nr);
		exit(1);
	}
	
	if (*buf == '#')	*buf = '\0';
	else				*(buf + strlen(buf) - 1) = '\0';

	return buf;
}


Flags asciiflag_conv(const char *flag) {
	Flags flags = 0;
	bool is_number = true;
	const char *p;

	for (p = flag; *p; p++) {
		if (islower(*p))
			flags |= 1 << (*p - 'a');
		else if (isupper(*p))
			flags |= 1 << (26 + (*p - 'A'));

		if (!isdigit(*p))
			is_number = false;
	}

	if (is_number)
		flags = atol(flag);

	return flags;
}


void tar_restore_file(const char *filename) {
	BUFFER(archive, MAX_INPUT_LENGTH);
	BUFFER(mangled, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_INPUT_LENGTH);
	int i = 0;
	bool wrote = false;

	mudlogf(NRM, 0, TRUE, "CORRUPT: %s", filename);

#if !defined(_WIN32)
	// Get the initial mangled filename
	sprintf(mangled, "%s.mangled%i", filename, i);

	// Increment i to get a new ???.zon.mangled"i" file name

	FILE *file;
	for (i = 0; (file = fopen(mangled, "r")); i++) {
		fclose(file);
		sprintf(mangled, "%s.mangled%i", filename, i);
	}

	mudlogf(NRM, 0, TRUE,  "CORRUPT: Swapping to %s", mangled);

	rename(filename, mangled);
	
	for (i = 0; !wrote && (i <= 3); i++) {
		switch (i) {
			case 0:
			case 1:
			case 2:
				sprintf(archive, "./backup/backup_incr_%d.tar.gz", i + 1);
				break;
			case 3:
				sprintf(archive, "./backup/backup_full.tar.gz");
				break;
		}
		sprintf(buf, "pushd ..; tar -zxvf %s %s%s; popd", archive, "lib/", filename);
		mudlogf(CMP, 0, TRUE, buf);
		if (!popen(buf, "w"))		mudlog("CORRUPT: Error on port open!", CMP, 0, TRUE);
		if (fopen(filename, "r"))	wrote = true;
	}
#endif
	
	exit(1);
}
