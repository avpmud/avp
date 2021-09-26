
#include "string.h"




SInt32 String::Length(void) const {
	return strlen(this->str->string);
}


SInt32 String::Size(void) const {
	return this->str->size;
}


const char *String::Chars(void) const {
	return this->str->string; 
}


void String::Clear(void) {
	if (this->str->count > 1) {
		this->str->count--;
		this->str = new StringContainer("");
	} else *this->str->string = '\0';
}


String & String::TrimLeft(void) {	//	A sort of Skip-spaces
	char	*dest, *source;
	if( *this->str->string ) {
		if( this->str->count > 1 ) {
			this->str->count--;
			this->str = new StringContainer(*this->str);
		}
		
		source = dest = this->str->string;
		
		while(isspace(*source))	source++;
		while (*dest)			*dest++ = *source++;	// Replace with strcpy(dest, src) ?
	}
	return *this;
}


String & String::TrimRight(void) {
	char	*str;
	if( this->str->count > 1 ) {
		this->str->count--;
		this->str = new StringContainer(*this->str);
	}

	str = this->str->string;
	while(*str && *str != '\n' && *str != '\r' )
		str++; 
	*str = '\0';
	return *this;
}


String & String::Strip(char ch) {
	char	*dest, *source;
	
	if (*this->str->string) {
		if (this->str->count > 1) {
			this->str->count--;
			this->str = new StringContainer(*this->str);
		}
		
		source = dest = this->str->string;
		
		while (*source) {
			if (*source != ch)
				*dest++ = *source;
			*source++;
		}
		*dest = '\0';
	}
	return *this;
}


bool String::IsNumber(void) const {
	char *	source = this->str->string;
	
	if (!*source)						return false;
	if (*source == '-')					source++;
	while (*source && isdigit(*source))	source++;
	if (!source || isspace(*source))	return true;
	else								return false;
}


bool String::IsAbbrev(const char *arg) const {
	char *	source;
	if (!arg || !*arg)
		return false;
	for (source = this->str->string; *source && *arg; source++, arg++)
		if (tolower(*source) != tolower(*arg))
			return false;
	
	if (!*source)	return true;
	else			return false;
}
