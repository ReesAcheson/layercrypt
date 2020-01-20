#ifndef LC_INIT_INCLUDED
#define LC_INIT_INCLUDED



//This version uses ffmpeg style params: the value is next param.
//Handling the long params with the short.
//One hypen with Multiple short options allowed.
void ParseCmdln3(int argc, char *argv[]);

void init(int argc, char *argv[]);

#endif
