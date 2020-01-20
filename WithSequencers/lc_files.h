#ifndef LC_FILES_INCLUDED
#define LC_FILES_INCLUDED

//Return 1 if exists
//Mode values of accesss(const char *path,int mode)
//  00 Existence only
//  02 Write permission
//  04 Read permission
//  06 Read and write permission
int fExists(char *fn, int AccessMode);

int FileCreateKey(KeyStruct_t *key, char *fn);

int FileLoadKey(KeyStruct_t *key, char *fn);

//Write the options to the open file
int FileWriteOptions(options_t *op, FILE *f);

//Read the options from the open file
int FileReadOptions(options_t *op, FILE *f);

//Open both the source and target files.
int OpenFiles(FILE **src, FILE **dest);

//Get size in bytes of previously opened file f.
//Leaves file open and rewinded to beginning.
uint32_t FileGetSize(FILE *f);

#endif
