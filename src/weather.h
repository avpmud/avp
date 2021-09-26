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
[*] File : weather.h                                                      [*]
[*] Usage: Weather Engine                                                 [*]
\***************************************************************************/

#ifndef __WEATHER_H__
#define __WEATHER_H__

#include "types.h"

#include "bitflags.h"
#include <list>


namespace Weather
{
	namespace Seasons
	{
		enum Pattern
		{
			One				= 0,
			TwoEqual,
			TwoFirstLong,
			TwoSecondLong,
			Three,
			FourEqual,
			FourEvenLong,
			FourOddLong,
			NumPatterns
		};
	}	//	namespace Seasons

	const int MaxSeasons = 4;

	namespace Wind
	{
		enum WindType
		{
			Calm			= 0,
			Breezy,
			Unsettled,
			Windy,
			Chinook,
			Violent,
			Hurricane,
			NumTypes
		};
	}	//	namespace Wind

	namespace Precipitation
	{
		enum PrecipitationType
		{
			None			= 0,
			Arid,
			Dry,
			Low,
			Average,
			High,
			Stormy,
			Torrent,
			Constant,
			NumTypes
		};
	}	//	namespace Precipitation

	namespace Temperature
	{
		enum TemperatureType
		{
			Arctic			= 0,
			SubFreezing,
			Freezing,
			Cold,
			Cool,
			Mild,
			Warm,
			Hot,
			Blustery,
			Heatstroke,
			Boiling,
			NumTypes
		};
	}	//	namespace Temperature
		
	enum ClimateBits		//	Flags
	{
		NoMoon,
		NoSun,
		Uncontrollable,
		AffectsIndoors,
		NumClimateBits
	};

	class Climate
	{
	public:
							Climate();
		
		class Season
		{
		public:
								Season();
			Wind::WindType			m_Wind;
			char				m_WindDirection;
			bool				m_WindVaries;
			Precipitation::PrecipitationType	m_Precipitation;
			Temperature::TemperatureType	m_Temperature;
		};
		
		Seasons::Pattern	m_SeasonalPattern;
		Season				m_Seasons[4];
		Lexi::BitFlags<NumClimateBits, ClimateBits>	m_Flags;
		int					m_Energy;
		
		int					GetCurrentSeason();
	};
	
	extern int num_seasons[Seasons::NumPatterns];
}	//	namespace Weather


class WeatherSystem
{
public:
						WeatherSystem();
						~WeatherSystem();
	
	void				Start();
	void				Update();
	
	void				Register(ZoneData *zone);
	void				Unregister(ZoneData *zone);
	
	enum WeatherBits
	{
		MoonVisible,
		SunVisible,
		Controlled,
		NumWeatherBits
	};
	
	Weather::Climate	m_Climate;
	
	std::list<ZoneData *>	m_Zones;
	
	int					m_Energy;
	int					m_Temperature,
						m_Humidity;
	int					m_WindSpeed,
						m_WindDirection;
	int					m_PrecipitationRate,
						m_PrecipitationDepth,
						m_PrecipitationChange;
	
	int					m_Pressure,
						m_PressureChange;
	
	Lexi::BitFlags<NumWeatherBits, WeatherBits>		m_Flags;
	int					m_Light;
	
private:
						WeatherSystem(const WeatherSystem &);
	WeatherSystem &		operator=(const WeatherSystem &);
	
	void				Message(const char *msg);
	void				CalculateLight();
	
	typedef std::list<WeatherSystem *> WeatherSystemList;
	static WeatherSystemList ms_WeatherSystems;

public:
	static void			UpdateAllSystems();
};


#endif
