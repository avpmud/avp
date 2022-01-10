/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2005             [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on code from Copper 2                                           [*]
[-]-----------------------------------------------------------------------[-]
[*] File : weather.cp                                                     [*]
[*] Usage: Weather Engine                                                 [*]
\***************************************************************************/

#include "weather.h"
#include "descriptors.h"
#include "zones.h"
#include "characters.h"
#include "rooms.h"
#include "utils.h"
#include "comm.h"
#include "db.h"

extern struct TimeInfoData time_info;

void weather_and_time(int mode);
void another_hour(int mode);


void weather_and_time(int mode) {
	another_hour(mode);
	if (mode) WeatherSystem::UpdateAllSystems();
}

					/*    J   F   M   A   M   J   J   A   S   O   N   D */
static int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void another_hour(int mode) {
	++time_info.hours;
	++time_info.total_hours;

	if (time_info.hours >= 24)
	{
		time_info.hours = 0;
		++time_info.day;
		++time_info.weekday;

		if (time_info.weekday >= 7)
			time_info.weekday = 0;
		
		if (time_info.day >= days_in_month[time_info.month])
		{
			time_info.day = 0;
			++time_info.month;

			if (time_info.month >= 12)
			{
				time_info.month = 0;
				++time_info.year;
			}
		}
	}
}


Weather::Climate::Season::Season() :
	m_Wind(Wind::Calm),
	m_WindDirection(NORTH),
	m_WindVaries(true),
	m_Precipitation(Precipitation::Average),
	m_Temperature(Temperature::Mild)
{
}


Weather::Climate::Climate() :
	m_SeasonalPattern(Seasons::FourEqual),
	m_Energy(0)
{
}

int Weather::Climate::GetCurrentSeason() {
	int season = 0;
	
	switch (m_SeasonalPattern)
	{
		case Seasons::One:				season = 0;									break;
		case Seasons::TwoEqual:		season = time_info.month / 6;				break;
		case Seasons::TwoFirstLong:	season = (time_info.month < 8) ? 0 : 1;		break;
		case Seasons::TwoSecondLong:	season = (time_info.month < 4) ? 0 : 1;		break;
		case Seasons::Three:			season = (time_info.month < 4) ? 0 : (time_info.month < 8) ? 1 : 2;	break;
		case Seasons::FourEqual:		season = time_info.month / 3;				break;
		case Seasons::FourEvenLong:	season = (time_info.month < 2) ? 0 : (time_info.month < 6) ? 1 :
													 (time_info.month < 8) ? 2 : 3;		break;
		case Seasons::FourOddLong:		season = (time_info.month < 4) ? 0 : (time_info.month < 6) ? 1 :
													 (time_info.month < 10) ? 2 : 3;	break;
		default:
//			log("SYSERR: Zone %d (%s): Bad season (%d) in Zone::GetSeason.",
//					number, name, climate.pattern);
			season = 0;
			break;
	}
	
	return season;
}





WeatherSystem::WeatherSystemList WeatherSystem::ms_WeatherSystems;

WeatherSystem::WeatherSystem() :
	m_Energy(0),
	m_Temperature(0),
	m_Humidity(0),
	m_WindSpeed(0),
	m_WindDirection(0),
	m_PrecipitationRate(0),
	m_PrecipitationDepth(0),
	m_PrecipitationChange(0),
	m_Pressure(0),
	m_PressureChange(0),
	m_Light(0)
{
	ms_WeatherSystems.push_back(this);
}


WeatherSystem::~WeatherSystem()
{
	ms_WeatherSystems.remove(this);
}


void WeatherSystem::UpdateAllSystems()
{
	FOREACH(WeatherSystemList, ms_WeatherSystems, iter)
		(*iter)->Update();
}


void WeatherSystem::Register(ZoneData *z)
{
	m_Zones.push_back(z);
}


void WeatherSystem::Unregister(ZoneData *z)
{
	m_Zones.remove(z);
}


enum
{
	PrecipitationStart = 1060,
	PrecipitationStop = 970
};


void WeatherSystem::Start()
{
	const int winds[7] = {2,12,30,40,50,80,100};
	const int precip[9] = {0,0,0,1,3,8,15,23,45};
	const int humid[9] = {4,10,20,30,40,50,60,75,100};
	const int temps[11] = {-20,-10,0,2,7,17,23,29,37,55,95};

	/* get the season */
	int s = m_Climate.GetCurrentSeason();

	/* These are pretty standard start values */
	m_Pressure = 980;
	m_Energy = 10000;
	m_PrecipitationDepth = 0;

	/* These use the default conditions above */
	m_WindSpeed = winds[m_Climate.m_Seasons[s].m_Wind];
	m_WindDirection = m_Climate.m_Seasons[s].m_WindDirection;
	m_PrecipitationRate = precip[m_Climate.m_Seasons[s].m_Precipitation];
	m_Temperature = temps[m_Climate.m_Seasons[s].m_Temperature];
	m_Humidity = humid[m_Climate.m_Seasons[s].m_Precipitation];

	/* Set ambient light */
	CalculateLight();
}

void WeatherSystem::Update()
{
	int		magic,
			oldWind, oldTemp, oldPrecip;
	
	oldTemp = m_Temperature;
	oldPrecip = m_PrecipitationRate;
	oldWind = m_WindSpeed;
	
	int seasonNum = m_Climate.GetCurrentSeason();		//	Find season
	
	Weather::Climate::Season &	season = m_Climate.m_Seasons[seasonNum];
	
	m_Flags.clear(Controlled);	//	Clear controlled weather bit
	
	m_Energy = RANGE(m_Climate.m_Energy + m_Energy, 3000, 50000);
	
	switch (season.m_Wind)
	{
		default:
		case Weather::Wind::Calm:		m_WindSpeed += (m_WindSpeed > 25) ? -5 : Number(-2, 1);	break;
		case Weather::Wind::Breezy:		m_WindSpeed += (m_WindSpeed > 40) ? -5 : Number(-2, 2);	break;
		case Weather::Wind::Unsettled:	m_WindSpeed += (m_WindSpeed < 5) ? 5 :
				(m_WindSpeed > 60) ? -5 : Number(-6, 6);		break;
		case Weather::Wind::Windy:		m_WindSpeed += (m_WindSpeed < 15) ? 5 :
				(m_WindSpeed > 80) ? -5 : Number(-6, 6);		break;
		case Weather::Wind::Chinook:		m_WindSpeed += (m_WindSpeed < 25) ? 5 :
				(m_WindSpeed > 110) ? -5 : Number(-15, 15);	break;
		case Weather::Wind::Violent:		m_WindSpeed += (m_WindSpeed < 40) ? 5 : Number(-8, 8);	break;
		case Weather::Wind::Hurricane:	m_WindSpeed = 100;			break;
	}
	
	m_Energy += m_WindSpeed;	//	Add or subtract?
	
	if (m_Energy < 0)				m_Energy = 0;
	else if (m_Energy > 20000)		m_WindSpeed += Number(-10, -1);
	
	m_WindSpeed = MAX(0, m_WindSpeed);
	
	if (!season.m_WindVaries)				m_WindDirection = season.m_WindDirection;
	else if (dice(2, 15) * 1000 < m_Energy)	m_WindDirection = Number(0, 3);
	
	switch (season.m_Temperature)
	{
		default:
		case Weather::Temperature::Arctic:
			m_Temperature += (m_Temperature > -20) ? -4 : Number(-3, 3);	break;
		case Weather::Temperature::SubFreezing:
			m_Temperature += (m_Temperature < -30) ? 2 :
							 (m_Temperature > -5 ? -3 : Number(-3, 3));		break;
		case Weather::Temperature::Freezing:
			m_Temperature += (m_Temperature < -10) ? 2 :
							 (m_Temperature > 0 ? -2 : Number(-2, 2));		break;
		case Weather::Temperature::Cold:
			m_Temperature += (m_Temperature < -5) ? 1 :
							 (m_Temperature > 8 ? -2 : Number(-2, 2));		break;
		case Weather::Temperature::Cool:
			m_Temperature += (m_Temperature < -3) ? 2 :
							 (m_Temperature > 14 ? -2 : Number(-3, 3));		break;
		case Weather::Temperature::Mild:
			m_Temperature += (m_Temperature < 7) ? 2 :
							 (m_Temperature > 26 ? -2 : Number(-2, 2));		break;
		case Weather::Temperature::Warm:
			m_Temperature += (m_Temperature < 19) ? 2 :
							 (m_Temperature > 33 ? -2 : Number(-3, 3));		break;
		case Weather::Temperature::Hot:
			m_Temperature += (m_Temperature < 24) ? 3 :
							 (m_Temperature > 46 ? -2 : Number(-3, 3));		break;
		case Weather::Temperature::Blustery:
			m_Temperature += (m_Temperature < 34) ? 3 :
							 (m_Temperature > 53 ? -2 : Number(-5, 5));		break;
		case Weather::Temperature::Heatstroke:
			m_Temperature += (m_Temperature < 44) ? 5 :
							 (m_Temperature > 60 ? -5 : Number(-3, 3));		break;
		case Weather::Temperature::Boiling:
			m_Temperature += (m_Temperature < 80) ? 5 :
							 (m_Temperature > 120 ? -5 : Number(-6, 6));	break;
	}
	
	if (m_Flags.test(SunVisible))						m_Temperature += 1;
	else if (!m_Climate.m_Flags.test(Weather::NoSun))	m_Temperature -= 1;
	
	switch (season.m_Precipitation)
	{
		default:
		case Weather::Precipitation::None:
			if (m_PrecipitationRate > 0)	m_PrecipitationRate /= 2;
			m_Humidity = 0;
			break;
		case Weather::Precipitation::Arid:
			m_Humidity += (m_Humidity > 30) ? -3 : Number(-3, 2);
			if (oldPrecip > 20)	m_PrecipitationRate -= 8;
			break;
		case Weather::Precipitation::Dry:
			m_Humidity += (m_Humidity > 50) ? -3 : Number(-4, 3);
			if (oldPrecip > 35)	m_PrecipitationRate -= 6;
			break;
		case Weather::Precipitation::Low:
			m_Humidity += (m_Humidity < 13) ? 3 : (m_Humidity > 91) ? -2 : Number(-5, 4);
			if (oldPrecip > 45)	m_PrecipitationRate -= 10;
			break;
		case Weather::Precipitation::Average:
			m_Humidity += (m_Humidity < 30) ? 3 : (m_Humidity > 80) ? -2 : Number(-9, 9);
			if (oldPrecip > 55)	m_PrecipitationRate -= 10;
			break;
		case Weather::Precipitation::High:
			m_Humidity += (m_Humidity < 40) ? 3 : (m_Humidity > 90) ? -2 : Number(-8, 8);
			if (oldPrecip > 65)	m_PrecipitationRate -= 10;
			break;
		case Weather::Precipitation::Stormy:
			m_Humidity += (m_Humidity < 50) ? 4 : Number(-6, 6);
			if (oldPrecip > 80)	m_PrecipitationRate -= 10;
			break;
		case Weather::Precipitation::Torrent:
			m_Humidity += (m_Humidity < 60) ? 4 : Number(-6, 9);
			if (oldPrecip > 100)	m_PrecipitationRate -= 15;
			break;
		case Weather::Precipitation::Constant:
			m_Humidity = 100;
			if (m_PrecipitationRate == 0)	m_PrecipitationRate = Number(5, 12);
			break;
	}
	
	m_Humidity = RANGE(m_Humidity, 0, 100);
	
	m_PressureChange = RANGE(m_PressureChange + Number(-3, 3), -8, 8);
	m_Pressure = RANGE(m_Pressure + m_PressureChange, 960, 1040);
	
	m_Energy += m_PressureChange;
	
	/* The numbers that follow are truly magic since  */
	/* they have little bearing on reality and are an */
	/* attempt to create a mix of precipitation which */
	/* will seem reasonable for a specified climate   */
	/* without any complicated formulae that could    */
	/* cause a performance hit. To get more specific  */
	/* or exacting would certainly not be "Diku..."   */
	
	//	< 970 = stop raining
	//	970-1060 = keep raining/not raining
	//	> 1060 = start raining

	//	ORIGINAL FORMULA FOR REFERENCE
	magic = ((1240 - m_Pressure) * m_Humidity / 16) +
			m_Temperature + (oldPrecip * 2) + ((m_Energy - 10000)/100);
	
	if (oldPrecip == 0)
	{
		if (magic > PrecipitationStart)
		{
			m_PrecipitationRate += 1;
			
			if (m_Temperature > 0)				Message("It begins to rain.\n");
			else								Message("It starts to snow.\n");
		}
		else if (oldWind == 0 && m_WindSpeed != 0)	Message("The wind begins to blow.\n");
		else if ((m_WindSpeed - oldWind) > 10)	Message("The wind picks up some.\n");
		else if ((m_WindSpeed - oldWind) < -10)	Message("The wind calms down a bit.\n");
		else if (m_WindSpeed > 60)
		{
			if (m_Temperature > 50)				Message("A violent scorching wind blows hard in the face of any poor travellers in the area.\n");
			else if (m_Temperature > 21)		Message("A hot wind gusts wildly through the area.\n");
			else if (m_Temperature > 0)			Message("A fierce wind cuts the air like a razor-sharp knife.\n");
			else if (m_Temperature > -10)		Message("A freezing gale blasts through the area.\n");
			else								Message("An icy wind drains the warmth from all in sight.\n");
		}
		else if (m_WindSpeed > 25)
		{
			if (m_Temperature > 50)				Message("A hot, dry breeze blows languidly around.\n");
			else if (m_Temperature > 22)		Message("A warm pocket of air is rolling through here.\n");
			else if (m_Temperature > 10)		Message("It's breezy.\n");
			else if (m_Temperature > 2)			Message("A cool breeze wafts by.\n");
			else if (m_Temperature > -5)		Message("A slight wind blows a chill into living tissue.\n");
			else if (m_Temperature > -15)		Message("A freezing wind blows gently but firmly against you.\n");
			else								Message("The wind isn't very strong here, but the cold makes it quite noticeable.\n");
		}
		else if (m_Temperature > 52)			Message("It's hotter than anyone could imagine.\n");
		else if (m_Temperature > 37)			Message("It's really, really hot here.  A slight breeze would really improve things.\n");
		else if (m_Temperature > 25)			Message("It's hot out here.\n");
		else if (m_Temperature > 19)			Message("It's nice and warm out.\n");
		else if (m_Temperature > 9)				Message("It's mild out today.\n");
		else if (m_Temperature > 1)				Message("It's cool out here.\n");
		else if (m_Temperature > -5)			Message("It's a bit nippy here.\n");
		else if (m_Temperature > -20)			Message("It's cold!\n");
		else if (m_Temperature > -25)			Message("It's really cold!.\n");
		else									Message("Better get inside - this is too cold for you!\n");
	}
	else if (magic < PrecipitationStop)
	{
		m_PrecipitationRate = 0;
		if (oldTemp > 0)						Message("The rain stops.\n");
		else									Message("It stops snowing.\n");
	}
	else
	{
		//	Still precipitating, update the rain
		//	Check rain->snow or snow->rain changes
		
		m_PrecipitationChange += (m_Energy > 10000) ? Number(-3, 4) : Number(-4, 2);
		m_PrecipitationChange = RANGE(m_PrecipitationChange, -20, 20);
		m_PrecipitationRate = RANGE(m_PrecipitationRate + m_PrecipitationChange, 1, 100);
		
		m_Energy -= m_PrecipitationRate * 3 - abs(m_PrecipitationChange);
		
		if (oldTemp > 0 && m_Temperature <= 0)	Message("The rain turns to snow.\n");
		else if (oldTemp <= 0 && m_Temperature > 0)		Message("The snow turns to a cold rain.\n");
		else if (m_PrecipitationChange > 5)
		{
			if (m_Temperature > 0)				Message("It rains a bit harder.\n");
			else								Message("The snow is coming down faster now.\n");
		}
		else if (m_PrecipitationChange < -5)
		{
			if (m_Temperature > 0)				Message("The rain is falling less heavily now.\n");
			else								Message("The snow has let up a little.\n");
		}
		else if (m_Temperature > 0)
		{
			//	Above freezing
			if (m_PrecipitationRate > 80)
			{
				if (m_WindSpeed > 80)			Message("There's a hurricane out here!\n");
				else if (m_WindSpeed > 40)		Message("The wind and rain are nearly too much to handle.\n");
				else							Message("It's raining really hard right now.\n");
			}
			else if (m_PrecipitationRate > 50)
			{
				if (m_WindSpeed > 60)			Message("What a rainstorm!\n");
				else if (m_WindSpeed > 30)		Message("The wind is lashing this wild rain straight into your face.\n");
				else							Message("It's raining pretty hard.\n");
			}
			else if (m_PrecipitationRate > 30)
			{
				if (m_WindSpeed > 50)			Message("A respectable rain is being thrashed about by a vicious wind.\n");
				else if (m_WindSpeed > 25)		Message("It's rainy and windy, but altogether not too uncomfortable.\n");
				else							Message("It's raining.\n");
			}
			else if (m_PrecipitationRate > 10)
			{
				if (m_WindSpeed > 50)			Message("The light rain here is nearly unnoticeable compared to the horrendous wind.\n");
				else if (m_WindSpeed > 24)		Message("A light rain is being driven fiercely by the wind.\n");
				else							Message("It's raining lightly.\n");
			}
			else if (m_WindSpeed > 55)			Message("A few drops of rain are falling amidst a fierce windstorm.\n");
			else if (m_WindSpeed > 30)			Message("The wind and a bit of rain hint at the possibility of a storm.\n");
			else								Message("A light drizzle is falling.\n");
		}
		else
		{	//	Below 0
			if (m_PrecipitationRate > 70)			//	snow
			{
				if (m_WindSpeed > 50)			Message("This must be the worst blizzard ever.\n");
				else if (m_WindSpeed > 25)		Message("There's a blizzard out here, making it quite difficult to see.\n");
				else							Message("It's snowing very hard.\n");
			}
			else if (m_PrecipitationRate > 40)	//	snow
			{
				if (m_WindSpeed > 60)			Message("The heavily falling snow is being whipped up to a frenzy by a ferocious wind.\n");
				else if (m_WindSpeed > 35)		Message("A heavy snow is being blown randomly about by a brisk wind.\n");
				else if (m_WindSpeed > 18)		Message("Drifts in the snow are being formed by the wind.\n");
				else							Message("The snow's coming down pretty fast now.\n");
			}
			else if (m_PrecipitationRate > 19)
			{
				if (m_WindSpeed > 70)			Message("The snow wouldn't be too bad, except for the awful wind blowing it everywhere.\n");
				else if (m_WindSpeed > 45)		Message("There's a minor blizzard, more wind than snow.\n");
				else if (m_WindSpeed > 12)		Message("Snow is being blown about by a stiff breeze.\n");
				else							Message("It is snowing.\n");
			}
			else if (m_WindSpeed > 60)			Message("A light snow is being tossed about by a fierce wind.\n");
			else if (m_WindSpeed > 42)			Message("A lightly falling snow is being driven by a strong wind.\n");
			else if (m_WindSpeed > 18)			Message("A light snow is falling admist an unsettled wind.\n");
			else								Message("It is lightly snowing.\n");
		}
	}
	
	//	Handle celestial objects
	if (!m_Climate.m_Flags.test(Weather::NoSun))
	{
		bool bSkyObscured = m_Humidity > 90 || m_PrecipitationRate > 80;
		bool bWasVisible = m_Flags.test(SunVisible);
		
		if (time_info.hours < 6 /* Before 7 AM */ || time_info.hours >= 19 /* 8 PM or later */ || bSkyObscured)
		{
			m_Flags.clear(SunVisible);
			
			if (time_info.hours == 5 /* 6 AM */)
			{
				if (!bSkyObscured)				Message("The sun rises in the east.\n");
			}
			else if (time_info.hours == 19)
			{
				if (bWasVisible)				Message("The night has begun.\n");
			}
			else if (bWasVisible /* && bSkyObscured*/)
			{
				if (m_PrecipitationRate > 80)	Message("The rain is so heavy, you can no longer see the sun.\n");
				else if (m_Humidity > 90)		Message("The clouds darken, obscuring the sun.\n");
			}
		}
		else
		{
			m_Flags.set(SunVisible);			

			if (time_info.hours == 6 /* 7 AM */)Message("The day has begun.\n");
			else if (time_info.hours == 18 /* 7 PM */)	Message("The sun slowly disappears in the west.\n");
			else if (!bWasVisible)				Message("Some sunlight finally makes it's way through the dark sky.\n");
		}
	}
	
	if (!m_Climate.m_Flags.test(Weather::NoMoon))
	{
		bool bSkyObscured = m_Humidity > 80 || m_PrecipitationRate > 70;
		bool bWasVisible = m_Flags.test(MoonVisible);
		
		if (time_info.hours > 5 || time_info.hours < 19
			|| time_info.day < 3 || time_info.day > 31
			|| bSkyObscured)
		{
			m_Flags.clear(MoonVisible);
		}
		else
		{
			m_Flags.set(MoonVisible);
			
			if (!m_Flags.test(SunVisible) && !bWasVisible)
			{
				if (time_info.day == 17)		Message("The full moon floods the area with light.\n");
				else							Message("The moon casts a little bit of light on the ground.\n");
			}
		}
	}
	
	CalculateLight();
}


void WeatherSystem::CalculateLight()
{
	int		lightSum = 0,
			temp1, temp2;
	
	if (m_Flags.test(SunVisible))		//	Sun is only up from 7am to 7:59pm
	{
		temp1 = time_info.hours;
		if (temp1 > 11)		temp1 = 24 - temp1;	//	if after noon; time = 23-time.  0 to 11, 12 to 1
		temp1 -= 5;								//	time reduced by 5... -5 to 6, 7 to -4 (0 at 6am, 8pm)
		if (temp1 > 0)
		{
			lightSum += temp1 * 10;
		}
	}
	
	if (m_Flags.test(MoonVisible))
	{
		temp1 = abs(time_info.hours - 12) - 7;	//	5 to 1, 1 to 4, (0 to 4, 16 to 23)
		if (temp1 > 0)
		{
			temp2 = 17 - abs((int)time_info.day - 17) - 3;	//	1 to 7 (4 to 17 to 30)
			if (temp2 > 0)
				lightSum += (temp1 * temp2) / 2;	//	
		}
	}
	
	m_Light = RANGE(lightSum - m_PrecipitationRate, 0, 100); 
}


void WeatherSystem::Message(const char *msg)
{
	FOREACH(DescriptorList, descriptor_list, iter)
	{
		DescriptorData *i = *iter;
		if (i->GetState()->IsPlaying() && i->m_Character && AWAKE(i->m_Character) && OUTSIDE(i->m_Character) && 
				i->m_Character->InRoom()->GetZone() &&
				Lexi::IsInContainer(m_Zones, i->m_Character->InRoom()->GetZone()))
			i->Write(msg);
	}
}


int Weather::num_seasons[Weather::Seasons::NumPatterns] = {
	1,
	2, 2, 2,
	3,
	4, 4, 4
};


ENUM_NAME_TABLE_NS(Weather::Seasons, Pattern, Weather::Seasons::NumPatterns)
{
	{ Weather::Seasons::One,			"One"			},
	{ Weather::Seasons::TwoEqual,		"TwoEqual"		},
	{ Weather::Seasons::TwoFirstLong,	"TwoFirstLong"	},
	{ Weather::Seasons::TwoSecondLong,	"TwoSecondLong"	},
	{ Weather::Seasons::Three,			"Three"			},
	{ Weather::Seasons::FourEqual,		"FourEqual"		},
	{ Weather::Seasons::FourEvenLong,	"FourEvenLong"	},
	{ Weather::Seasons::FourOddLong,	"FourOddLong"	}
};

ENUM_NAME_TABLE_NS(Weather::Wind, WindType, Weather::Wind::NumTypes)
{
	{ Weather::Wind::Calm,				"Calm"			},
	{ Weather::Wind::Breezy,			"Breezy"		},
	{ Weather::Wind::Unsettled,			"Unsettled"		},
	{ Weather::Wind::Windy,				"Windy"			},
	{ Weather::Wind::Chinook,			"Chinook"		},
	{ Weather::Wind::Violent,			"Violent"		},
	{ Weather::Wind::Hurricane,			"Hurricane"		}
};


ENUM_NAME_TABLE_NS(Weather::Precipitation, PrecipitationType, Weather::Precipitation::NumTypes)
{
	{ Weather::Precipitation::None,		"None"			},
	{ Weather::Precipitation::Arid,		"Arid"			},
	{ Weather::Precipitation::Dry,		"Dry"			},
	{ Weather::Precipitation::Low,		"Low"			},
	{ Weather::Precipitation::Average,	"Average"		},
	{ Weather::Precipitation::High,		"High"			},
	{ Weather::Precipitation::Stormy,	"Stormy"		},
	{ Weather::Precipitation::Torrent,	"Torrent"		},
	{ Weather::Precipitation::Constant,	"Constant"		}
};

ENUM_NAME_TABLE_NS(Weather::Temperature, TemperatureType, Weather::Temperature::NumTypes)
{
	{ Weather::Temperature::Arctic,		"Arctic"		},
	{ Weather::Temperature::SubFreezing,"Subfreezing"	},
	{ Weather::Temperature::Freezing,	"Freezing"		},
	{ Weather::Temperature::Cold,		"Cold"			},
	{ Weather::Temperature::Cool,		"Cool"			},
	{ Weather::Temperature::Mild,		"Mild"			},
	{ Weather::Temperature::Warm,		"Warm"			},
	{ Weather::Temperature::Hot,		"Hot"			},
	{ Weather::Temperature::Blustery,	"Blustery"		},
	{ Weather::Temperature::Heatstroke,	"Heatstroke"	},
	{ Weather::Temperature::Boiling,	"Boiling"		}
};


ENUM_NAME_TABLE_NS(Weather, ClimateBits, Weather::NumClimateBits)
{
	{ Weather::NoMoon,					"NoMoon"		},
	{ Weather::NoSun,					"NoSun"			},
	{ Weather::Uncontrollable,			"Uncontrollable"},
	{ Weather::AffectsIndoors,			"AffectsIndoors"}
};



ENUM_NAME_TABLE_NS(WeatherSystem, WeatherBits, WeatherSystem::NumWeatherBits)
{
	{ WeatherSystem::MoonVisible,		"MoonVisible"	},
	{ WeatherSystem::SunVisible,		"SunVisible"	},
	{ WeatherSystem::Controlled,		"Controlled"	}
};
