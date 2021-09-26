#include <stdarg.h>



#include "descriptors.h"
#include "characters.h"
#include "utils.h"
#include "comm.h"
#include "buffer.h"


void mudlogf(int type, int level, bool file, const char *format, ...) {
	va_list args;
	static char mudlog_buf[MAX_STRING_LENGTH];

	va_start(args, format);
	vsnprintf(mudlog_buf, sizeof(mudlog_buf), format, args);
	va_end(args);

	if (file)
	{
		time_t ct = time(0);
		fprintf(logfile, "%-19.19s :: %s\n", asctime(localtime(&ct)), mudlog_buf);
		fflush(logfile);
	}
	
	FOREACH(DescriptorList, descriptor_list, iter)
	{
		DescriptorData *i = *iter;
		if (i->GetState()->IsPlaying() && !PLR_FLAGGED(i->m_Character, PLR_WRITING)) {
			int tp = ((PRF_FLAGGED(i->m_Character, PRF_LOG1) ? 1 : 0) +
					  (PRF_FLAGGED(i->m_Character, PRF_LOG2) ? 2 : 0));

			if ((i->m_Character->GetLevel() >= level) && (tp >= type))
				i->m_Character->Send("`g[ %s ]`n\n", mudlog_buf);
		}
	}
}


void log(const char *format, ...)
{
	va_list args;
	time_t ct = time(0);

	fprintf(logfile, "%-19.19s :: ", asctime(localtime(&ct)));

	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);

	fprintf(logfile, "\n");
	fflush(logfile);
}
