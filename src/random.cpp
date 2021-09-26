/* ************************************************************************
*   File: random.c                                      Part of CircleMUD *
*  Usage: pseudo-random number generator                                  *
************************************************************************ */

/*
 * I am bothered by the non-portablility of 'rand' and 'random' -- rand
 * is ANSI C, but on some systems such as Suns, rand has seriously tragic
 * spectral properties (the low bit alternates between 0 and 1!).  random
 * is better but isn't supported by all systems.  So, in my quest for Ultimate
 * CircleMUD Portability, I decided to include this code for a simple but
 * relatively effective random number generator.  It's not the best RNG code
 * around, but I like it because it's very short and simple, and for our
 * purposes it's "random enough".
 *               --Jeremy Elson  2/23/95
 *
 * Now that we're using GNU's autoconf, I've coded Circle to always use
 * random(), and automatically link in this object file if random() isn't
 * supported on the target system.  -JE 2/3/96
 */

/***************************************************************************/

/*
 *
 * This program is public domain and was written by William S. England
 * (Oct 1988).  It is based on an article by:
 *
 * Stephen K. Park and Keith W. Miller. RANDOM NUMBER GENERATORS:
 * GOOD ONES ARE HARD TO FIND. Communications of the ACM,
 * New York, NY.,October 1988 p.1192

 The following is a portable c program for generating random numbers.
 The modulus and multipilier have been extensively tested and should
 not be changed except by someone who is a professional Lehmer generator
 writer.  THIS GENERATOR REPRESENTS THE MINIMUM STANDARD AGAINST WHICH
 OTHER GENERATORS SHOULD BE JUDGED. ("Quote from the referenced article's
 authors. WSE" )
*/


#include <stdlib.h>

#if defined(__MWERKS__)
#define srandom srand
#define random rand
#endif

void circle_srandom(unsigned int initial_seed);
unsigned int circle_random(void);

/*
** F(z)	= (az)%m
**	= az-m(az/m)
**
** F(z)  = G(z)+mT(z)
** G(z)  = a(z%q)- r(z/q)
** T(z)  = (z/q) - (az/m)
**
** F(z)  = a(z%q)- rz/q+ m((z/q) - a(z/m))
** 	 = a(z%q)- rz/q+ m(z/q) - az
*/

#if 0
static unsigned int seed;

void circle_srandom(unsigned int initial_seed)
{
#if RAND_MAX > 32767
	srandom(initial_seed);
#else
	seed = initial_seed; 
#endif
}


#define	m	(unsigned int)2147483647
#define	q	(unsigned int)127773

#define	a	(unsigned int)16807
#define	r	(unsigned int)2836


unsigned int circle_random(void)
{
#if RAND_MAX > 32767
	return random();
#else
	unsigned int hi   = seed / q;
	unsigned int lo   = seed % q;

	seed = a * lo - r * hi;
    if (seed <= 0)	seed += m;

    return seed;
#endif
}
#else


class MersenneTwister
{
public:
	MersenneTwister();
	
	void Seed(unsigned long seed);
	unsigned long GetRandom();
	
private:
	void Generate();
	
	static const int NumTwists = 624;
	
	unsigned long	m_Twister[NumTwists];
	int				m_Index;
};


MersenneTwister::MersenneTwister()
:	m_Index(0)
{}


void MersenneTwister::Seed(unsigned long seed)
{
	m_Index = 0;
	m_Twister[0] = seed;
	for (int i = 1; i < NumTwists; ++i)
	{
		m_Twister[i] = 1812433253 * (m_Twister[i - 1] ^ (m_Twister[i - 1] >> 30)) + i;
		m_Twister[i] &= 0xFFFFFFFF;
	}
}


unsigned long MersenneTwister::GetRandom()
{
	if (m_Index == 0)
	{
		Generate();
	}
	
	unsigned long n = m_Twister[m_Index];
	n = n ^ (n >> 11);
	n = n ^ ((n << 7) & 2636928640UL);
	n = n ^ ((n << 15) & 4022730752UL);
	n = n ^ (n >> 18);
	
	m_Index = (m_Index + 1) % NumTwists;
	
	return n;
}


void MersenneTwister::Generate()
{
	for (int i = 0; i < NumTwists; ++i)
	{
		unsigned long n = (m_Twister[i] & 0x80000000) + (m_Twister[(i + 1) % NumTwists] & 0x7FFFFFFF);
		m_Twister[i] = m_Twister[(i + 397) % NumTwists] ^ (n >> 1);
		if (n & 1)
		{
			m_Twister[i] = m_Twister[i] ^ 2567483615UL;
		}
	}
}


static MersenneTwister gTwister;


void circle_srandom(unsigned int initial_seed)
{
	gTwister.Seed(initial_seed);
}


unsigned int circle_random()
{
	return gTwister.GetRandom();
}

#endif
