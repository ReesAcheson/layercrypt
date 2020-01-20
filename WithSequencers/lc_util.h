#ifndef LC_UTIL_INCLUDED
#define LC_UTIL_INCLUDED

void abortprog(char *s1, char *s2);

void abortprogI(char *s1, int n);

//Looks for word in choices table and returns 1 based word position (1st word, 2nd word, etc)
//Choices is a char array that is space delimited (extra spaces ignored).
//Does not alter either string.
//Returns zero if not found in choices
//example: strWordPos(s, "help usage debug")
size_t strWordPos(const char *word, char *choices);

// Copy src to string dst of size siz.  At most siz-1 characters will be copied.
// Returns strlen(dst); if length src >= siz, truncation occurred.
size_t _strlcpy(char *dst, const char *src, size_t siz);	//siz=sizeof()

inline int isPowerOfTwo(uint32_t x);


#endif
