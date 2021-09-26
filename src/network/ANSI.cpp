

#include "ANSI.h"

#include <stdio.h>



#define CSI "\x1B["

namespace ANSI
{
	const char *ControlSequenceIntroducer = CSI;
}

Lexi::String ANSI::PositionCursor(int x, int y)
{
	char	buffer[32];
	
	if (x == 1 && y == 1)	strcpy(buffer, CSI "H" );
	else if (y == 1)		sprintf(buffer, CSI ";%dH", x);
	else if (x == 1)		sprintf(buffer, CSI "%d;H", y);
	else					sprintf(buffer, CSI "%d;%dH", y,x);
	
	return buffer;
}



Lexi::String ANSI::SetScreenMode(ANSI::ScreenMode mode)
{
	char	buffer[32];
	
	sprintf(buffer, CSI "=%dh", mode);
	
	return buffer;
}




namespace ANSI
{
	namespace Symbols
	{
		enum
		{
			NoShade			= 0x20,
			LightShade		= 0xB0,
			MediumShade		= 0xB1,
			DarkShade		= 0xB2,
			FullShade		= 0xDB,
			
			BoxV			= 0xB3,
			BoxH			= 0xC4,
			
			BoxV_H			= 0xC5,
			
			BoxV_L			= 0xB4,
			BoxV_R			= 0xC3,
			BoxH_U			= 0xC1,
			BoxH_D			= 0xC2,
			
			BoxD_L			= 0xBF,
			BoxU_R			= 0xC0,
			BoxU_L			= 0xD9,
			BoxD_R			= 0xDA,
			
			BoxV_LL			= 0xB5,
			BoxV_RR			= 0xC6,
			BoxH_UU			= 0xD0,
			BoxH_DD			= 0xD2,
			
			BoxVV			= 0xBA,	//	1011 1010
			BoxHH			= 0xCD,	//	1100 1101
			BoxVV_HH		= 0xCE, //	1100 1110
			
			BoxVV_H			= 0xD7,
			BoxV_HH			= 0xD8,
			
			BoxVV_L			= 0xB6,
			BoxVV_R			= 0xC7,
			BoxHH_U			= 0xCF,
			BoxHH_D			= 0xD1,
			
			BoxVV_LL		= 0xB9,
			BoxVV_RR		= 0xCC,
			BoxHH_UU		= 0xCA,
			BoxHH_DD		= 0xCB,
			
			BoxDD_L			= 0xB7,
			BoxDD_R			= 0xD6,
			BoxD_LL			= 0xB8,
			BoxD_RR			= 0xD5,
			BoxUU_L			= 0xBD,
			BoxUU_R			= 0xD3,
			BoxU_LL			= 0xBE,
			BoxU_RR			= 0xD4,
			
			BoxDD_LL		= 0xBB,
			BoxDD_RR		= 0xC9,
			BoxUU_LL		= 0xBC,
			BoxUU_RR		= 0xC8,
			
			FullBlock		= 0xDB,
			HalfBlockT		= 0xDF,
			HalfBlockB		= 0xDC,
			HalfBlockL		= 0xDD,
			HalfBlockR		= 0xDE,
			
			Square			= 0xFE,
		};
	}
}


namespace ANSI
{
	extern char BoxTable[64];
	extern char ShadeTable[5];
	extern char BlockTable[6];	
}

char ANSI::GetShadeSymbol(ANSI::ShadeType shade)
{
	return ShadeTable[shade];
}


char ANSI::GetBoxSymbol(int shape)
{
	return BoxTable[shape];
}

char ANSI::GetBlockSymbol(ANSI::BlockType block)
{
	return BlockTable[block];
}





using namespace ANSI::Symbols;

char ANSI::ShadeTable[5] =
{
	NoShade,
	LightShade,
	MediumShade,
	DarkShade,
	FullShade
};

//	Bits
//	000	Nothing
//	001	Second Half	(Right/Down)
//	010	First Half	(Left/Up)
//	011	Across
//	100 Double
//	VVVHHH

char ANSI::BoxTable[64] =
{
	/*  UD  LR */
	/* VVV HHH */
	/* 000 000 */	NoShade,/* 000 001 */	'X',	/* 000 010 */	'X',	/* 000 011 */	BoxH,
	/* 000 100 */	'X',	/* 000 101 */	'X',	/* 000 110 */	'X',	/* 000 111 */	BoxHH,

	/* 001 000 */	'X',	/* 001 001 */	BoxD_R,	/* 001 010 */	BoxD_L,	/* 001 011 */	BoxH_D,
	/* 001 100 */	'X',	/* 001 101 */	BoxD_RR,/* 001 110 */	BoxD_LL,/* 001 111 */	BoxHH_D,

	/* 010 000 */	'X',	/* 010 001 */	BoxU_R,	/* 010 010 */	BoxU_L,	/* 010 011 */	BoxH_U,
	/* 010 100 */	'X',	/* 010 101 */	BoxU_RR,/* 010 110 */	BoxU_LL,/* 010 111 */	BoxHH_U,

	/* 011 000 */	BoxV,	/* 011 001 */	BoxV_R,	/* 011 010 */	BoxV_L,	/* 011 011 */	BoxV_H,
	/* 011 100 */	'X',	/* 011 101 */	BoxV_RR,/* 011 110 */	BoxV_LL,/* 011 111 */	BoxV_HH,

	/*  UD  LR */
	/* VVV HHH */
	/* 100 000 */	'X',	/* 100 001 */	'X',	/* 100 010 */	'X',	/* 100 011 */	'X',
	/* 100 100 */	'X',	/* 100 101 */	'X',	/* 100 110 */	'X',	/* 100 111 */	'X',

	/* 101 000 */	'X',	/* 101 001 */	BoxDD_R,/* 101 010 */	BoxDD_L,/* 101 011 */	BoxH_DD,
	/* 101 100 */	'X',	/* 101 101 */	BoxDD_RR,/* 101 110 */	BoxDD_LL,/* 101 111 */	BoxHH_DD,

	/* 110 000 */	'X',	/* 110 001 */	BoxUU_R,/* 110 010 */	BoxUU_L,/* 110 011 */	BoxH_UU,
	/* 110 100 */	'X',	/* 110 101 */	BoxUU_RR,/* 110 110 */	BoxUU_LL,/* 110 111 */	BoxHH_UU,

	/* 111 000 */	BoxVV,	/* 111 001 */	BoxVV_R,/* 111 010 */	BoxVV_L,/* 111 011 */	BoxVV_H,
	/* 111 100 */	'X',	/* 111 101 */	BoxVV_RR,/* 111 110 */	BoxVV_LL,/* 111 111 */	BoxVV_HH,
};


char ANSI::BlockTable[6] =
{
	' ',
	HalfBlockT,
	HalfBlockB,
	HalfBlockL,
	HalfBlockR,
	FullBlock	
};
