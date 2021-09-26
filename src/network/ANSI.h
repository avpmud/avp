#ifndef __ANSI_H__
#define __ANSI_H__

#include "lexistring.h"

namespace ANSI
{
	enum ScreenMode
	{
		ScreenMode_Default	= 3,
		ScreenMode_Large	= 4
	};

	Lexi::String	PositionCursor(int x, int y);
	Lexi::String	SetScreenMode(ScreenMode);
	
	enum ShadeType
	{
		eShadeNone			= 0,
		eShadeLight,
		eShadeMedium,
		eShadeDark,
		eShadeFull
	};
	
	enum
	{
		eBoxRight			= 0x01,
		eBoxLeft			= 0x02,
		eBoxHorizontal		= 0x03,
		eBoxHorizontalDouble= 0x04,
		
		eBoxDown			= 0x08,
		eBoxUp				= 0x10,
		eBoxVertical		= 0x18,
		eBoxVerticalDouble	= 0x20
	};
	
	enum BlockType
	{
		eBlockNone			= 0,
		eBlockL,
		eBlockR,
		eBlockT,
		eBlockB,
		eBlock
	};
	
	char GetShadeSymbol(ShadeType shade);
	char GetBoxSymbol(int shape);
	char GetBlockSymbol(BlockType block);
}

#endif