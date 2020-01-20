#include "lc_vars.h"

void abortprog(char *s1, char *s2)
{ printf("%s ", s1);
  if (*s2)
    printf("%s", s2);
  printf("\n");
  if (src)
    fclose(src);
  if (dest)
  fclose(dest);
  exit(EXIT_FAILURE);
}

void abortprogI(char *s1, int n)
{ printf("%s %i\n", s1, n);
  if (src)
    fclose(src);
  if (dest)
  fclose(dest);
  exit(EXIT_FAILURE);
}

//Looks for word in choices table and returns 1 based word position (1st word, 2nd word, etc)
//Choices is a char array that is space delimited (extra spaces ignored).
//Does not alter either string.
//Returns zero if word not found in choices
//example: strWordPos(s, "help usage debug")
size_t strWordPos(const char *word, char *choices)
{ size_t len;	//length of 'word'
  size_t WordCount = 0;
  char *p = choices;
  if (word == NULL || choices == NULL) return 0;
  if (*word == 0 || *choices == 0) return 0;
  len = strlen(word);
  while (*p) {
    WordCount++;
    if (strncmp(word, p, len) == 0)
      return WordCount;
    for (; *p && *p>' '; p++);	//skip to next space
    for (; *p && *p<=' '; p++);	//skip to next non-space
  }	//while
  return 0;
}

// Copy src to string dst of size siz.  At most siz-1 characters will be copied.
// Returns strlen(dst); if length src >= siz, truncation occurred.
size_t _strlcpy(char *dst, const char *src, size_t siz)	//siz=sizeof()
{ int i;
  for (i=0; i<siz-1 && src[i]; i++)
    dst[i] = src[i];
  dst[i] = 0;
  return i; 
}


//Returns 1 if x is a power-of-two
inline int isPowerOfTwo(uint32_t x)
{ return ((x != 0) && !(x & (x - 1))); }


