/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Advanced Buffer System by George Greer, Copyright (C) 1998-99         [*]
[-]-----------------------------------------------------------------------[-]
[*] File : buffer.cp                                                      [*]
[*] Usage: Advanced Buffer System                                         [*]
\***************************************************************************/


#define _BUFFER_C_

#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "interpreter.h"
#include "comm.h"

enum {
	BUF_NUKE =		(1 << 0),	//	Use memset() to clear after use
	BUF_CHECK =		(1 << 1),	//	Report how much is used
	BUF_STAYUP =	(1 << 2),	//	Try to stay up after an overflow
	BUF_DETAIL =	(1 << 3),	//	Output detail
	BUF_VERBOSE =	(1 << 4),	//	Output extremely detailed info
};
Flags	Buffer::options = /*BUF_NUKE |*/ BUF_STAYUP;

const char * const Buffer::optionsDesc[] = {
	"NUKE", "CHECK", "STAYUP", "DETAIL", "VERBOSE", "\n"
};

//	1 = Check every pointer in every buffer allocated on every new allocation.
//	0 = Recommended, unless you are tracking memory corruption at the time.
//	NOTE: This only works under Linux.  Do not try to use with anything else!
#define BUFFER_PARANOIA		0

/*
 * 1 = Use pvalloc() to allocate pages to protect with mprotect() to make
 *	sure no one fiddles with structures they shouldn't.  Good for the
 *	BUFFER_SNPRINTF support and as a much more noticeable addition to
 *	the paranoia code just above.  It is NOT recommended to use this
 *	option along with BUFFER_MEMORY unless you have a lot of memory.
 *	This is also incompatible with Electric Fence and anything else
 *	which overrides malloc() at link time.  This is tested only on
 *	the GNU C Library v2.
 * 0 = Recommended unless you have memory overruns somewhere.
 */
#define BUFFER_PROTECT		0

namespace {

//	MAGIC_NUMBER - Any arbitrary character unlikely to show up in a string.
//		In this case, we use a control character. (0x6)
//	BUFFER_LIFE - How long can a buffer be unused before it is free()'d?
//	LEASE_LIFE - The amount of time a buffer can be used before it is
//		considered forgotten and released automatically.
const unsigned char MAGIC_NUMBER =	0x6;
const int BUFFER_LIFE =	60 RL_SEC;		//	5 minutes
const int LEASE_LIFE = 	10 RL_SEC;		//	1 minute


//	MEMORY ARRAY OPTIONS
//	--------------------
//	If you change the maximum or minimum allocations, you'll probably also want
//	to change the hash number.  See the 'bufhash.c' program for determining a
//	decent one.  Think prime numbers.
//
//	MAX_ALLOC - The largest buffer magnitude expected.
//	MIN_ALLOC - The lowest buffer magnitude ever handed out.  If 20 bytes are
//		requested, we will allocate this size buffer anyway.
//	BUFFER_HASH - The direct size->array hash mapping number. This is _not_ an
//		arbitrary number.
const int MAX_ALLOC = 	16;		//	2**16 = 65536 bytes/
const int MIN_ALLOC =	6;		//	2**6  = 64 bytes
const int BUFFER_HASH =	11;		//	Not arbitrary!
}

//	End of configurables section.
// ----------------------------------------------------------------------------


#if BUFFER_PROTECT
#include <sys/mman.h>
#include <malloc.h>
#endif

#if BUFFER_PARANOIA && !defined(__linux__)
#undef BUFFER_PARANOIA
#define BUFFER_PARANOIA 0
#endif

#if BUFFER_PARANOIA == 0
#define valid_pointer(ptr)	true
#else
#define valid_pointer(ptr)	(((unsigned long)(ptr) & 0x48000000))
#endif


const int BUFFER_MINSIZE	= (1 << MIN_ALLOC);	//	Minimum size, in bytes.
const int BUFFER_MAXSIZE	= (1 << MAX_ALLOC);	//	Maximum size, in bytes.
const int B_FREELIFE		= (BUFFER_LIFE / PULSE_BUFFER);
const int B_GIVELIFE		= (LEASE_LIFE / PULSE_BUFFER);


/* ------------------------------------------------------------------------ */


//	Global variables.
//	buffers - Head of the main buffer allocation linked list.
//	buffer_array - The minimum and maximum array ranges for 'buffers'.
namespace {
int		buffer_array[2] = {0,0};
enum {
	BUF_MAX = 0,
	BUF_MIN = 1
//	BUF_RANGE = 2
};
int		buffer_crash_overflows = 0;
}

/* External functions and variables. */
extern int circle_shutdown;
extern int circle_reboot;

/* Private functions. */
ACMD(do_buffer);
namespace {
int get_magnitude(unsigned int number);
}

//	Useful macros.
#define buffer_hash(x)		((x) % BUFFER_HASH)
#define USED(x)				((x)->who)
#define SIZE_T_HIGHBIT		(sizeof(size_t) * CHAR_BIT - 1)

/* ------------------------------------------------------------------------ */

/*
 * Private: round_to_pow2(number to round)
 * Converts a number to the next highest power of two.
 * 0000011000111010 (1594)
 *        to
 * 0000100000000000 (2048)
 */

static size_t round_to_pow2(size_t y)
{
	size_t v = y;
	
	if (/*(v & (v - 1)) == 0	//	0 or power of 2
		||*/ v & 0x80000000)	//	highest bit already set
		return v;
	
	//	Masks all bits together, shifitng lower, to create a mask of all-set for all bits
	//	below and including the highest set bit
	//	Then add 1 to push to next power of 2
	--v;	//	To make it 0, so that 1 does not become 2
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	++v;
	
	if (IS_SET(Buffer::options, BUF_VERBOSE))	log("BUF: round_to_pow2: %d -> %d", y, v);
	
	return v;
}


void Buffer::Reboot(void) {
	Init();
	if (!IS_SET(options, BUF_STAYUP) || ++buffer_crash_overflows == 30)
	{
		send_to_all("Emergency reboot.. come back in a minute or two.\n");
		log("BUF: SYSERR: Emergency reboot, buffer corrupted!");
		circle_shutdown = circle_reboot = 1;
		return;
	}
	log("BUF: SYSERR: Buffer list cleared, crash imminent!");
}

#if BUFFER_EXTREME_PARANOIA	/* only works on Linux */

#define valid_pointer(ptr)	(((unsigned long)(ptr) & 0x48000000))

//	Private: buffer_sanity_check(nothing)
//	Checks the entire buffer/memory list for overruns.
static void buffer_sanity_check(struct buf_data **check) {
	bool die = false;

	if (!check) {
		buffer_sanity_check(get_buffer_head());
		buffer_sanity_check(get_memory_head());
		return;
	}
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		struct buf_data *chk;
		if (check[magnitude] && !valid_pointer(check[magnitude]) && (die = true))
			log("BUF: SYSERR: %s/%d head corrupted.", check == get_buffer_head() ? "buffer" : "memory", magnitude);
		for (chk = check[magnitude]; chk; chk = chk->next) {
			if (!valid_pointer(chk->data) && (die = true))
				log("BUF: SYSERR: %p->%p (data) corrupted.", chk, chk->data);
			if (chk->next && !valid_pointer(chk->next) && (die = true))
				log("BUF: SYSERR: %p->%p (next) corrupted.", chk, chk->next);
			if (chk->type != BT_MALLOC && ((unsigned long)chk->var & 0xff0000) != 0)
				log("BUF: SYSERR: %p->%p is set on non-malloc memory.", chk, chk->var);
			if (chk->magic != MAGIC_NUMBER && (die = TRUE))
				log("BUF: SYSERR: %p->%d (magic) corrupted.", chk, chk->magic);
			if (chk->data[chk->req_size] != MAGIC_NUMBER && (die = true))
				log("BUF: SYSERR: %p (%s:%d) overflowed requested size (%d).", chk, chk->who, chk->line, chk->req_size);
			if (chk->data[chk->size] != MAGIC_NUMBER && (die = true))
				log("BUF: SYSERR: %p (%s:%d) overflowed real size (%d).", chk, chk->who, chk->line, chk->size);
		}
	}
	if (die)
		Buffer::Reboot();
}
#endif


//	Private: magic_check(buffer to check)
//	Makes sure a buffer hasn't been overwritten by something else.
void Buffer::Check(void) {
	if (magic != MAGIC_NUMBER) {
		char trashed[16];
		strncpy(trashed, (char *)this, 15);
		trashed[15] = '\0';
		log("BUF: SYSERR: Buffer %p was trashed! (%s)", this, trashed);
		Reboot();
	}
}


//	Private: get_magnitude(number)
//	Simple function to avoid math library, and enforce maximum allocations.
//	If it is not enforced here, we will overrun the bounds of the array.
namespace {
int get_magnitude(unsigned int y) {
	int number = buffer_hash(y) - buffer_array[BUF_MIN];

	if (number > buffer_array[BUF_MAX]) {
		log("BUF: SYSERR: Hash result %d out of range 0-%d.", number, buffer_array[BUF_MAX]);
		number = buffer_array[BUF_MAX];
	}
	
	if (IS_SET(Buffer::options, BUF_VERBOSE))
		log("BUF: get_magnitude: %d bytes -> %d", y, number);

	return number;
}
}

int Buffer::Magnitude(void) {
//	return get_magnitude(round_to_pow2(size));
	return get_magnitude(size);
}


//	Public: exit_buffers()
//	Called to clear out the buffer structures and log any BT_MALLOC's left.
void Buffer::Exit(void) {
//	log("BUF: Shutting down.");

//	log("BUF: Clearing buffer memory.");
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		for (Buffer *b = Buffer::buf[magnitude], *bn = NULL; b; b = bn) {
			bn = b->next;
			b->Clear();
			b->Remove();
			delete b;
		}
	}
//	log("BUF: Done.");
}


//	Private: find_hash(none)
//	Determine the size of the buffer array based on the hash size and the
//	allocation limits.
static int find_hash(void) {
	int	maxhash = 0,
		result;

//	log("BUF: Calculating array indices...");
	for (int i = MIN_ALLOC; i <= MAX_ALLOC; i++) {
		result = buffer_hash(1 << i);
		if (result > maxhash)	maxhash = result;
//		if (result < minhash)	minhash = result;
		if (IS_SET(Buffer::options, BUF_DETAIL | BUF_VERBOSE))
			log("BUF: %7d -> %2d", 1 << i, result);
	}
	if (IS_SET(Buffer::options, BUF_DETAIL | BUF_VERBOSE)) {
		log("BUF: ...done.");
		log("BUF: Array range is 0-%d.", maxhash);
	}
//	buffer_array[BUF_MIN] = minhash;
	buffer_array[BUF_MAX] = maxhash;
//	buffer_array[BUF_RANGE] = maxhash - minhash;
	return maxhash;
}



Buffer **Buffer::buf;

//	Public: init_buffers(none)
//	This is called from main() to get everything started.
void Buffer::Init(void) {
	static bool	initialized = false;
	
	initialized = true;

	find_hash();

	//	Allocate room for the array of pointers.  We can't use the CREATE
	//	macro here because that may use the array we're trying to make!
	Buffer::buf = (Buffer **)calloc(buffer_array[BUF_MAX] + 1, sizeof(Buffer *));

	//	Put any persistant buffers here.
	//	Ex: new_buffer(8192, BT_PERSIST);
}


//	Private: decrement_all_buffers(none)
//	Reduce the life on all buffers by 1.
void Buffer::Decrement(void)
{
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		for (Buffer *clear = Buffer::buf[magnitude]; clear; clear = clear->next) {
			if (clear->life < 0)
				log("BUF: SYSERR: %p from %s:%d has %d life.", clear, clear->who, clear->line, clear->life);
			else if (clear->life != 0)	//	We don't want to go negative.
				clear->life--;
		}
	}
}


//	Private: release_old_buffers()
//	Check for any used buffers that have no remaining life.
void Buffer::ReleaseOld(void)
{
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++)
	{
		for (Buffer *relbuf = Buffer::buf[magnitude]; relbuf; relbuf = relbuf->next)
		{
			if (!USED(relbuf))			continue;	//	Can't release, no one is using this
			if (relbuf->life > 0)		continue;	//	We only want expired buffers
			
			log("BUF: %s:%d forgot to release %d bytes in %p.", relbuf->who ? relbuf->who : "UNKNOWN", relbuf->line, relbuf->size, relbuf);
			relbuf->Clear();
		}
	}
}


//	Private: free_old_buffers(void)
//	Get rid of any old unused buffers.
void Buffer::FreeOld(void) {
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		for (Buffer *freebuf = Buffer::buf[magnitude], *next_free = NULL; freebuf; freebuf = next_free) {
			next_free = freebuf->next;
			if (freebuf->life > 0)					continue;	//	Hasn't expired yet
			if (USED(freebuf))						continue;	//	Needs to be cleared first if used

			freebuf->Remove();
			delete freebuf;
		}
	}
}


/*
 * Public: release_all_buffers()
 * Forcibly release all buffers currently allocated.  This is useful to
 * reclaim any forgotten buffers.
 * See structs.h for PULSE_BUFFER to change the release rate.
 */
void Buffer::ReleaseAll(void) {
	Buffer::Decrement();
	Buffer::ReleaseOld();
	Buffer::FreeOld();
}

/*
 * Private: remove_buffer(buffer to remove)
 * Removes a buffer from the list without freeing it.
 */
void Buffer::Remove(void) {
	Buffer **	head;
	int			magnitude;

	head = Buffer::Head();
	magnitude = Magnitude();

	for (Buffer *traverse = head[magnitude], *prev = NULL; traverse; traverse = traverse->next) {
		if (this == traverse) {
			if (traverse == head[magnitude])	head[magnitude] = traverse->next;
			else if (prev)						prev->next = traverse->next;
			else log("BUF: SYSERR: remove_buffer: Don't know what to do with %p.", this);
			return;
		}
		prev = traverse;
	}
	log("BUF: SYSERR: remove_buffer: Couldn't find the buffer %p from %s:%d.", this, who, line);
}

/*
 * Private: clear_buffer(buffer to clear)
 * This is used to declare an allocated buffer unused.
 */
void Buffer::Clear(void)
{
	Check();

	/*
	 * If the magic number we set is not there then we have a suspected
	 * buffer overflow.
	 */
	if (data[req_size] != MAGIC_NUMBER) {
		log("BUF: SYSERR: Overflow in %p from %s:%d.",
				this, who ? who : "UNKNOWN", line);
		log("BUF: SYSERR: ... ticketed for doing %d in a %d (%d) zone.",
				strlen(data) + 1, req_size, size);
		if (data[size] == MAGIC_NUMBER) {
			log("BUF: SYSERR: ... overflow did not compromise memory.");
			data[req_size - 1] = '\0';
			data[req_size] = MAGIC_NUMBER;
		} else
			Buffer::Reboot();
	} else {
		if (IS_SET(Buffer::options, BUF_CHECK)) {
			int used = (data && *data) ?
					(IS_SET(Buffer::options, BUF_NUKE) ? Used() : strlen(data)) : 0;
			//	If nothing in clearme->data return 0, else if paranoid_buffer, return
			//	the result of Buffer::Used(), otherwise just strlen() the buffer.
			log("BUF: %s:%d used %d/%d bytes in %p.",
					who, line, used, req_size, this);
		}
		if (IS_SET(Buffer::options, BUF_NUKE))	memset(data, '\0', size);
		else									*data = '\0';
		who = NULL;
		line = 0;
		req_size = size;	//	For Buffers::Exit()
		life = B_FREELIFE;
	}
}
 
 
void Buffer::Detach(const char *func, const int line_n)
{
	if (!this->who)
		log("BUF: SYSERR: Buffer::Detach: Buffer %p, requested by %s:%d, already released.", this, func, line_n);
	else {
		if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
			log("BUF: %s:%d released %d bytes in buffer (%p) from %s:%d.", func, line_n,
					this->size, this, this->who ? this->who : "UNKNOWN", this->line);
		Clear();
	}
} 

Buffer::~Buffer(void) {
	if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
		log("BUF: %p->~Buffer(): Freeing %d bytes in expired buffer.", this, size);

	if (data)	free(data);
	else		log("BUF: SYSERR: %p->~Buffer(): Hey, no data?", this);
}

/*
 * Private: new_buffer(size of buffer, type flag)
 * Finds where it should place the new buffer it's trying to malloc.
 */
Buffer::Buffer(size_t size) : magic(MAGIC_NUMBER), req_size(0),
		size(size), data(NULL), next(NULL), line(0), who(NULL) {
	Buffer **	buflist;
	int			magnitude;
	
	size = round_to_pow2(size);
	
	buflist = Buffer::Head();
	magnitude = Magnitude();
	
	life = B_FREELIFE;
	
	data = new char[size + 1];
	
	//	Emulate calloc
	if (IS_SET(Buffer::options, BUF_NUKE))	memset(data, 0, size);
	else									*data = '\0';
	data[size] = MAGIC_NUMBER;	//	Implant magic safety catch
	
	next = buflist[magnitude];
	buflist[magnitude] = this;
}


/*
 * We no longer search for buffers outside the specified power of 2 since
 * the next highest array index may actually be smaller.
 */
Buffer *Buffer::FindAvailable(size_t size)
{
	Buffer *search;
	int		magnitude, scans = 0;
	
//	size = round_to_pow2(size);
    
	magnitude = get_magnitude(size);
    
	for (search = Buffer::buf[magnitude]; search; search = search->next)
	{
		scans++;
		if (USED(search))			continue;
		if (search->size < size)	continue;
		break;
	}
	if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
		log("BUF: find_free_buffer: %d scans for %d bytes, found %p.", scans, size, search);

	return search;
}


/*
 * Public: as get_buffer(size of buffer)
 * Requests a buffer from the free pool.  If a buffer of the desired size
 * is not available, one is created.
 */
Buffer *Buffer::Acquire(size_t size, const char *varname, const char *who, int line)
{
	Buffer *	give;

#if BUFFER_EXTREME_PARANOIA
  buffer_sanity_check(NULL);
#endif

	if (size > BUFFER_MAXSIZE)
		log("BUF: %s:%d requested %d bytes for '%s'.", who, line, size, varname);

	if (size == 0) {
		log("BUF: SYSERR: %s:%d requested 0 bytes.", who, line);
		return NULL;
	}
	else if (!(give = FindAvailable(size))) {
		//	Minimum buffer size, since small ones have high overhead.
		size_t allocate_size = size;
		if (allocate_size < BUFFER_MINSIZE)
			allocate_size = BUFFER_MINSIZE;

		if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
			log("BUF: acquire_buffer: Making a new %d byte buffer for %s:%d.", allocate_size, who, line);

		/*
		 * If we don't have a valid pointer by now, we're out of memory so just
		 * return NULL and hope things don't crash too soon.
		 */
		give = new Buffer(allocate_size);
	}

	give->Check();

	give->who = who;
	give->line = line;
	give->req_size = size;

	give->life = B_GIVELIFE;

	if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
		log("BUF: %s:%d requested %d bytes for '%s', received %d in %p.", who, line, size, varname ? varname : "a buffer", give->size, give);

	/*
	 * Plant a magic number to see if someone overruns the buffer. If the first
	 * character of the buffer is not NUL then somehow our memory was
	 * overwritten...most likely by someone doing a release_buffer() and
	 * keeping a secondary pointer to the buffer.
	 */
	give->data[give->req_size] = MAGIC_NUMBER;
	if (*give->data != '\0') {
		log("BUF: SYSERR: acquire_buffer: Buffer %p is not empty as it ought to be!", give);
		*give->data = '\0';
	}

	if (give->life <= 1) {
		log("BUF: SYSERR: acquire_buffer: give->life <= 1 (Invalid lease life?!)");
		give->life = 10;
	}

	return give;
}


/*
 * Public: as 'show buffers'
 * This is really only useful to see who has lingering buffers around
 * or if you are curious.  It can't be called in the middle of a
 * command run by a player so it'll usually show the same thing.
 * You can call this with a NULL parameter to have it logged at any
 * time though.
 */
/*
 * XXX: This code works but is ugly and misleading.
 */
void show_buffers(CharData *ch, int display_type)
{
	Buffer **head = Buffer::Head();
	Buffer *disp;
	int i;

	if (display_type == 1) {
		log("BUF: --- Buffer list --- (Inaccurate if not perfect hash.)");
		
		for (int size = MIN_ALLOC; size <= MAX_ALLOC; size++) {
			int bytes, magnitude = get_magnitude(1 << size);
			for (i = 0, bytes = 0, disp = head[magnitude]; disp; disp = disp->next, i++)
				bytes += disp->size;
			log("%5d bytes (%2d): %5d items, %6d bytes, %5d overhead.", (1 << size), magnitude, i, bytes, sizeof(Buffer) * i);
		}
		return;
	}

	BUFFER(buf, MAX_STRING_LENGTH);
	
	if (ch)		ch->Send("-- ## Size  Life Type    Allocated\n");
	else		log("-- ## Size  Life Type    Allocated");
	for (int size = 0; size <= buffer_array[BUF_MAX]; size++)
	{
		for (i = 0, disp = head[size]; disp; disp = disp->next)
		{
			sprintf(buf, "%2d %2d %5d %4d %s:%d%s",
					size, ++i, disp->size, disp->life,
					disp->who ? disp->who : "unused",
					disp->line ? disp->line : 0, ch ? "\n" : "");
			if (ch)		ch->Send("%s", buf);
			else		log(buf);
		}
	}
}

char *BUFFER_FORMAT =
"buffer (add | delete) size\n"
"buffer verbose - toggle verbose mode.\n"
"buffer detailed - toggle detailed mode.\n"
"buffer paranoid - toggle between memset() or *buf = NUL.\n"
"buffer check - toggle buffer usage checking.\n"
"buffer overflow - toggle immediate reboot on overflow.\n";

ACMD(do_buffer) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	BUFFER(arg3, MAX_INPUT_LENGTH);
	long size;

	//	This looks nifty.
	half_chop(argument, arg1, argument);
	half_chop(argument, arg2, argument);
	half_chop(argument, arg3, argument);

	if (!*arg1)								size = -1;
	else if (!*arg2 || !*arg3)				size = -2;
	else if ((size = atoi(arg2)) == 0)		size = -1;

  if (size == -1)	/* Oops, error. */
    ch->Send(BUFFER_FORMAT);
  else if (size == -2) {	/* -2 means a toggle command. */
    if (is_abbrev(arg1, "verbose")) {
      Buffer::options ^= BUF_VERBOSE;
      ch->Send(IS_SET(Buffer::options, BUF_VERBOSE) ? "Verbose On.\n" : "Verbose Off.\n");
    } else if (is_abbrev(arg1, "detailed")) {
      Buffer::options ^= BUF_DETAIL;
      ch->Send(IS_SET(Buffer::options, BUF_DETAIL) ? "Detailed On.\n" : "Detailed Off.\n");
    } else if (is_abbrev(arg1, "paranoid")) {
      Buffer::options ^= BUF_NUKE;
      ch->Send(IS_SET(Buffer::options, BUF_NUKE) ? "Now paranoid.\n" : "No longer paranoid.\n");
    } else if (is_abbrev(arg1, "check")) {
      Buffer::options ^= BUF_CHECK;
      ch->Send(IS_SET(Buffer::options, BUF_CHECK) ? "Checking on.\n" : "Not checking.\n");
    } else if (is_abbrev(arg1, "overflow")) {
      Buffer::options ^= BUF_STAYUP;
      ch->Send(!IS_SET(Buffer::options, BUF_STAYUP) ? "Reboot on overflow.\n" : "Will try to keep going.\n");
    } else
      ch->Send(BUFFER_FORMAT);
  } else if (is_abbrev(arg1, "delete")) {
    Buffer *toy, **b_head = Buffer::buf;
    for (toy = b_head[get_magnitude(size)]; toy; toy = toy->next) {
      if (USED(toy))
	continue;
      if (toy->size != size)
	continue;
      toy->Remove();
      delete toy;
      break;
    }
    if (!toy)
      ch->Send("Not found.\n");
  } else if (is_abbrev(arg1, "add"))
    new Buffer(size); /* So easy. :) */
  else
    ch->Send(BUFFER_FORMAT);
}


int Buffer::Used(void) {
	int cnt = req_size - 1;
	while (cnt > 0 && data[cnt] == '\0')
		cnt--;
	return cnt;
//	int cnt;
//	for (cnt = buf->req_size - 1; cnt > 0 && buf->data[cnt] == '\0'; cnt--);
//	return cnt;
}

/* ------------------------------------------------------------------------ */
