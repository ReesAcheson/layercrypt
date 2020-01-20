#ifndef LC_DEBUG_INCLUDED
#define LC_DEBUG_INCLUDED


//************************* some debug routines *******************************

void ShowKey(KeyStruct_t *key);

void DisplaySeeds(uint8_t *sds, int size);

void Display1024ArrayStats(uint8_t array[1024], char *msg);

int ReasonableRands(int range, int HiLimit);

void DisplayReasonables();

#endif
