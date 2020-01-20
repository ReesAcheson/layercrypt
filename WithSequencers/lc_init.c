
#include "lc_vars.h"

//************************* init ******************************

void syntax(char *progname)
{ printf("syntax: %s [-e | -d | --newkey FeyFn] [-k KeyFn] [-o] [--password pass] Source Target\n", progname);
  puts  ("  -e Encrypt mode.");
  puts  ("  -d Decrypt mode.");
  puts  ("  -k KeyFn  Specify the filename containing the key.");
  puts  ("  --newkey keyname  Create a new key.");
  puts  ("  -o Overwrite target file if one already exists.");
  puts  ("  --password pass  Use a password to alter the encryption. Optional.");
  puts  ("      If used, must also be supplied for decrypt.");
  puts  ("  --debug [n] Enter debug mode. If n is supplied set a specific debug mode.");
  puts  ("This is just a demo to illustrate the layer concept. No attempt has");
  puts  ("  been made to provide or insure file name conformity such as file extensions.");
  exit(1);
}

//This version uses ffmpeg style params: the value is next param.
//Handling the long params with the short.
//One hypen with Multiple short options allowed.
void ParseCmdln3(int argc, char *argv[])
{ int i, j;
  int params = 0;
  char s[120], *arg;
  if (argc == 0)
    syntax(argv[0]);
  for (i=1; i<argc; i++) {
    _strlcpy(s, argv[i], sizeof(s));
    arg = NULL;		//argument to parameter
    if (i<argc-1) {
      arg = argv[i+1];
      if (arg && *arg=='-') arg=NULL;	//point to next arg only if not another option
    }
    if (s[0] == '-') {
      if (s[1] == '-')	//we have 2 hyphens.
        //skip over 2 hyphens, find option, set s[1] to short option, set [s2] to '='
        //to stop multiple option mode.  If no short equivelent, set to hi or low chars.
        //123-127, & 0-32 available. Only 7 bit allowed.
        switch (strWordPos(s+2, "help key newkey password debug verbose")) {
          case 0: abortprog(s, " Long Option not found"); break;
          case 1: syntax(argv[0]);  break;
          case 2: s[1] = 'k';  s[2] = '='; break;	//key name
          case 3: s[1] = 3;    s[2] = '='; break;	//newkey
          case 4: s[1] = 'p';  s[2] = '='; break;	//password
          case 5: s[1] = 5;    s[2] = '='; break;	//debug
          case 6: s[1] = 6;    s[2] = '='; break;	//verbose
          default: abortprog(s, " Long Option not allowed for"); break;	
        }	//switch

      for (j=1; s[j] && s[j]!='='; j++)
        switch (s[j]) {
          case 'h': syntax(argv[0]); 
                    break;
          case 'e': OpMode = mode_encrypt;
                    break;
          case 'd': OpMode = mode_decrypt;
                    break;
          case 'k': if (arg) {
                      i++;
                      _strlcpy(KeyFn, arg, sizeof(KeyFn));
                    }
                    break;
          case 'o': OverwriteTarget = 1;
                    break;
          case 'p': if (arg) {	//password
                      i++;
                      _strlcpy(password, arg, sizeof(password));
                    }
                    break;
          case 3: if (arg) {	//newkey
                      i++;
                      _strlcpy(KeyFn, arg, sizeof(KeyFn));
                    }
                    OpMode = mode_newkey;
                    break;
           case 5: debug = 1; 
                   if (arg) {	//debug number
                      i++;
                      debug = atoi(arg);
                    }
                   break;
           case 6: verbose = 1; 
                   break;
          default : abortprog ("Unrecognized option.", s);
        }	//switch

    }	//not a hyphen
    else {	// not a hyphen
      params++;
      switch (params) {
        case 1: _strlcpy(SrcFn, s, sizeof(SrcFn)); break;
        case 2: _strlcpy(DestFn, s, sizeof(DestFn)); break;
        default: printf("Parameter %i ignored\n", params);
      }	//switch
    }	//not a hyphen
  }	//for i
}	//ParseCmdln3()



void init(int argc, char *argv[])
{ int i;
  ParseCmdln3(argc, argv);
  memset(&options, 0, sizeof(options_t));		//zero the options
  TheKey = calloc(1, sizeof(KeyStruct_t));

//Allocate RAM for each random gen data 
//Accomodate the extra general purpose generator (<=)
  for (i=0; i<=LayersMax; i++)			
    LayerGenState[i] = calloc(1, sizeof(RNG_state_t)); 	
    //initstate must be passed zeroed ram 1st time (calloc)
    
//Then calloc space for generator results - the rands. 
//Note that there is none for extra gen purpose gen (256)
  Rands = calloc(LayersMax, sizeof(uint32_t));

//ptr to the rands to access as 1024 array, rather than as layers. Rand2Bytes() uses this.
  Rand1024Byte = (uint8_t*)Rands;
  Rands2 = (uint16_t*)Rands;
//Instead of Rands4[] use: Rands[layer].rnd4;		
  Rands8 = (uint64_t*)Rands;
  
//Initialize the general purpose RNG, layer 256
  InitARandom();
}	//init

