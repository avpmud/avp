#ifndef __ENUM_H__
#define __ENUM_H__


#include <algorithm>


class EnumNameResolverBase
{
public:
	virtual ~EnumNameResolverBase() {}

	const char *		GetName(unsigned int i) const;
	int					GetEnumByName(const char *n) const;
	int					GetEnumByNameAbbrev(const char *n) const;

	virtual const char **GetNames() const = 0;
	virtual unsigned int	GetNameCount() const = 0;
};


template <typename Enum, int EnumCount>
class EnumNameResolver : public EnumNameResolverBase
{
public:
	struct Pair 
	{
		Enum			e;
		const char *	n;
	};
	
	EnumNameResolver(Pair *pairs)
	{
		std::fill(m_Names, m_Names + EnumCount, "!UNUSED!");
		
		for (int i = 0; i < EnumCount; ++i)
		{
			Pair &p = pairs[i];
			
			if (!p.n)	break;
			
			if ((unsigned int)p.e < EnumCount)
			{
				m_Names[p.e] = p.n;
			}
		}
		m_Names[EnumCount] = "\n";	//	Legacy
	}
	
	virtual const char **GetNames() const { return (const char **)m_Names; }
	virtual unsigned int	GetNameCount() const { return EnumCount; }

private:
	const char *		m_Names[EnumCount + 1];
};

template<typename Enum>
const EnumNameResolverBase &GetEnumNameResolver();

template<typename Enum>
const char **GetEnumNames()				{ return GetEnumNameResolver<Enum>().GetNames(); }

template<typename Enum>
const char *GetEnumName(Enum e)			{ return GetEnumNameResolver<Enum>().GetName(e); }

template<typename Enum>
const char *GetEnumName(unsigned int e) { return GetEnumNameResolver<Enum>().GetName(e); }

template<typename Enum>
int GetEnumByName(const char *n)		{ return GetEnumNameResolver<Enum>().GetEnumByName(n); }

template<typename Enum>
int GetEnumByNameAbbrev(const char *n)	{ return GetEnumNameResolver<Enum>().GetEnumByNameAbbrev(n); }


#define ENUM_NAME_TABLE(EnumType, EnumCount) \
	typedef EnumNameResolver<EnumType, EnumCount>		EnumType##Resolver; \
	extern EnumType##Resolver::Pair	EnumType##NamePairs[EnumCount]; \
	EnumType##Resolver				EnumType##Names(EnumType##NamePairs); \
	template<> \
	const EnumNameResolverBase &	GetEnumNameResolver<EnumType>() { return EnumType##Names; } \
	EnumType##Resolver::Pair		EnumType##NamePairs[EnumCount] =


#define ENUM_NAME_TABLE_NS(NS, EnumType, EnumCount) \
	typedef EnumNameResolver<NS::EnumType, EnumCount>	EnumType##Resolver; \
	extern EnumType##Resolver::Pair	EnumType##NamePairs[EnumCount]; \
	EnumType##Resolver				EnumType##Names(EnumType##NamePairs); \
	template<> \
	const EnumNameResolverBase &	GetEnumNameResolver<NS::EnumType>() { return EnumType##Names; } \
	EnumType##Resolver::Pair		EnumType##NamePairs[EnumCount] =


#endif	//	__ENUM_H__
