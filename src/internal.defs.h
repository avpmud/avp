#define OPT_USEC		100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC			* PASSES_PER_SEC

#define PULSE_ZONE		(10 RL_SEC)
#define PULSE_MOBILE	(1 RL_SEC / 5)
#define PULSE_POINTS	(25 RL_SEC)	//	WILL REMOVE
#define PULSE_VIOLENCE	(2 RL_SEC)	//	WILL REMOVE ?
#define PULSE_BUFFER	(5 RL_SEC)
#define PULSE_SCRIPT	(1 RL_SEC)

#define TICK_WRAP_COUNT	16
#define TICK_WRAP_COUNT_MASK (TICK_WRAP_COUNT - 1)

//	Variables for the output buffering system */
#define MAX_SOCK_BUF		(12 * 1024)		//	Size of kernel's sock buf
#define MAX_PROMPT_LENGTH	512				//	Max length of prompt
#define GARBAGE_SPACE		64				//	Space for **OVERFLOW**, telnet negotiation, etc
#define SMALL_BUFSIZE		1024			//	Static output buffer size
//	Max amount of output that can be buffered
#define LARGE_BUFSIZE		(MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define MAX_MESSAGE_LENGTH	4096				//	arbitrary -- change if needed
#define MAX_INPUT_LENGTH	1024				//	Max length per *line* of input
#define MAX_RAW_INPUT_LENGTH	1024			//	Max size of *raw* input
#define MAX_MESSAGES		60

#define SPECIAL(name)	int (name)(CharData *ch, Ptr me, const char * cmd, char *argument)

/* misc editor defines **************************************************/


#define ROAMING_COMBAT 1
//#define GROUP_COMBAT 1
