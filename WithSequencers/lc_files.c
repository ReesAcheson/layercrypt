
#include "lc_vars.h"


//Return 1 if exists
//Mode values of accesss(const char *path,int mode)
//  00 Existence only
//  02 Write permission
//  04 Read permission
//  06 Read and write permission
int fExists(char *fn, int AccessMode)
{ if( (access( fn, AccessMode )) == 0 )
    return 1;
  else
    return 0; 		//Not exists
}


//Create a new key file. Does not use layers and so generators need not be started.
int FileCreateKey(KeyStruct_t *key, char *fn)
{ FILE *f;
  int bytes, i;
  if (!key) abortprog("TheKey has not been allocated RAM.","");
  for (i=0; i<LayersMax; i++) //Fill the keys with randoms from the independant generator
    key->keys[i] = ARandom(1);
  f = fopen(fn, "w");
  if (!f)
    abortprog("Unable to create key file", fn);
  _strlcpy(key->ProgName, ProgramName, sizeof(key->ProgName));
  _strlcpy(key->Ver, VersionProg, sizeof(TheKey->Ver));
  bytes = fwrite(key, 1, sizeof(KeyStruct_t), f);
  printf("%i bytes wrttten to %s\n", bytes, fn);
  fclose(f);
  if (bytes != sizeof(KeyStruct_t))
    abortprog("ERROR writing key to file", "");
  if (debug == dg_ShowKey)
    ShowKey(key);
  return 1;
}	//FileCreateKey


//Load the key but do not install it.
int FileLoadKey(KeyStruct_t *key, char *fn)
{ FILE *f;
  int bytes;
  f = fopen(fn, "r");
  if (!f) return 0;
  bytes = fread(key, 1, sizeof(KeyStruct_t), f);
  fclose(f);
  if (debug == dg_ShowKey)
    ShowKey(key);
  if (strncmp(key->ProgName, ProgramName, strlen(ProgramName)) != 0)
    return 0;
  if (bytes != sizeof(KeyStruct_t))
    return 0;
  return 1;
}	//FileLoadKey

//Write the options to the open file
int FileWriteOptions(options_t *op, FILE *f)
{ int bytes; 
  if (!f) return 0;
  bytes = fwrite(op, 1, sizeof(options), f);
  if (bytes != sizeof(options)) return 0;
  return 1;
}

//Read the options from the open file
int FileReadOptions(options_t *op, FILE *f)
{ int bytes;
  if (!f) return 0;
  bytes = fread(op, 1, sizeof(options), f);
  if (bytes != sizeof(options)) return 0;
  if (strncmp(op->ProgName, ProgramName, strlen(ProgramName)) != 0)
    return 0;
  return 1;
}

//Open both the source and target files.
int OpenFiles(FILE **src, FILE **dest)
{ if (*SrcFn==0 || !*DestFn)
    return 0;
  if (!OverwriteTarget && fExists(DestFn, 0))
    abortprog("Target file exists.  Please erase", DestFn);
  *src = fopen(SrcFn, "r");
  if (!*src)
    abortprog("Unable to open source:", SrcFn);
  *dest = fopen(DestFn, "w");
  if (!*dest) {
    fclose(*src);
    abortprog("Unable to create target:", DestFn);
  }
  return 1;
}

//Get size in bytes of previously opened file f.
//Leaves file open and rewound to beginning.
uint32_t FileGetSize(FILE *f)
{ uint32_t Size;
  if (!f) return 0;
  fseek(f, 0, SEEK_END);
  Size = ftell(f);
  rewind(f);
  return Size;
}
