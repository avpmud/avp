#include "enum.h"
#include "utils.h"

const char *EnumNameResolverBase::GetName(unsigned int i) const
{
	return (i < GetNameCount())	? GetNames()[i] : "UNDEFINED";	
}


int EnumNameResolverBase::GetEnumByName(const char *n) const
{
	const char **table = GetNames();
	int top = GetNameCount();
	
	for (int i = 0; i < top; ++i)
		if (!str_cmp(n, table[i]))
			return i;
	
	return -1;
}


int EnumNameResolverBase::GetEnumByNameAbbrev(const char *n) const
{
	size_t length = strlen(n);
	
	//	Avoid "" to match the first available string
	if (length == 0)	length = 1;

	const char **table = GetNames();
	int top = GetNameCount();
	
	for (int i = 0; i < top; ++i)
		if (!strn_cmp(n, table[i], length))
			return i;
	
	return -1;
}
