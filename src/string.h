
#ifndef __STRING_H__
#define __STRING_H__

#include "types.h"
#include "utils.h"

class StringContainer;
class String;

class StringContainer {
private:
	friend class String;
	
					StringContainer(void) : count(0), size(0), string(NULL) { };
					StringContainer(const char *source) : count(1), size(strlen(source) + 1),
							string(NULL) {
						string = new char[this->size];
						strcpy(this->string, source);
					}
					StringContainer(const StringContainer &source) : count(1), size(strlen(source.string) + 1),
							string(NULL) {
						string = new char[this->size];
						strcpy(this->string, source.string);
					}
					StringContainer(SInt32 source) : count(1), size(source),
							string(new char[source]) { };
					~StringContainer(void) { delete [] this->string; };
					
	char *			string;
	SInt32			size;
	SInt32			count;
};


class String {
public:
					String(void) : str(strEmpty) {
						this->strEmpty->count++;
					}
					String(const char *source) : str(new StringContainer(source)) { }
					String(const String & source) : str(source.str) {
						this->str->count++;
					}
					String(SInt32 size) : str(new StringContainer(size)) {
						*this->str->string = '\0';
					}
					~String(void) {
						if (str->count == 1)	delete str;
						else					str->count--;
					};
					
					operator const char *() const { return this->str->string; };
					
	SInt32			Length(void) const;
	SInt32			Size(void) const;		//	Not necessary?
	void			Clear(void);
	void			Allocate(SInt32 size);	//	Not necessary?
	const char *	Chars(void) const;
	char *			Dup(void) const;		//	Not necessary?
	
	String &		TrimLeft(void);
	String &		TrimRight(void);
	String &		Strip(char ch);
	
	bool			IsNumber(void) const;
	bool			IsAbbrev(const char *arg) const;
	
	
private:
	static StringContainer *strEmpty;
	StringContainer *str;
};

#endif

