
#include "lc_vars.h"


/*
static __inline__ unsigned long long rdtsc(void)
{ unsigned long long int x;
 __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
 return x;
}
*/

//Processor Time Stamp Counter
//Returns the number of cycles since processor reset. 
uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

//************************ LAYERS ********************************

//New rands for a single layer, loop times
//Used when refreshing the randoms, and for decyncing
void AdvanceLayer(uint8_t lay, uint16_t loop)
{ int i;
  //load the gen with this layer's data
  local__setstate_r(LayerGenState[lay]->State, &LayerGenState[lay]->RandData);
  for (i=0; i<loop; i++) {
    local__random_r(&LayerGenState[lay]->RandData, &Rands[lay].rnd4);
  }    
}

//New rands for all layers (except LayersMax).
void Next4Rands() 
{ int i;				
  for (i=0; i<LayersMax; i++) 
    AdvanceLayer(i, 1);
  if (debug ==  dg_RandsCheck)
    ReasonableRands(ReasonableRange, ReasonableMax);
}

//Get new LayerPick & BytePick values
//BytePick is normalized to 0 to 3
inline void NewPicks()
{ LayerPick = Rands[LayerPick].byte[BytePick];
  BytePick = Rands[LayerPick].byte[BytePick] &3;	//0 thru 3
  LayerPick = Rands[LayerPick].byte[BytePick];
}

//Inc the RandsAccessCount and generate new rands if count 
//  is greater than RandRefreshRate.
inline void RefreshRands()
{ RandsAccessCount++;
  if (RandsAccessCount > RandRefreshRate) {	//Time for new rands?  Rate is #define in lc_vars.h
    Next4Rands();
    RandsAccessCount = 0;
  }	//if
}

//return byte at layer "lay", and byte position "k"
//k = 0 thru 3 (k is normalized here.)
//Also see Rand1024Byte[], which accesses the 1024 bytes directly, not by layers, or RandByte()
//Mmost selections from the randoms are made through this function, or RandByte() which calls it.
//This function also refreshes the randoms during a byte selection.
uint8_t LayerRandByte(uint8_t lay, uint8_t k)
{ k = k & 3;
  RefreshRands();
  return Rands[lay].byte[k];
}

//Select a number from a layer and byte using the randoms for the selection
//If zero, then it tries again. 
//Because zeros should be avoided when encrypting using xor, this function should be used for that.
uint8_t RandByte()
{ uint8_t b;
  NewPicks();
  b = LayerRandByte(LayerPick, BytePick);
  if (!b) 		//If zero, pick another
    b = RandByte();
  return b;
}


/*
//Add 4 BandBytes together into a uint8_t so that they wrap.
uint8_t Bytes4()
{ uint8_t b = 0;
  int i;
  for (i=0; i<4; i++) {
    NewPicks();
    b = Rands[LayerPick].byte[BytePick];
  } 
  RefreshRands();
}
*/


//Will not return zero
uint16_t Rand2Bytes()
{ uint16_t w;
  NewPicks();
  RefreshRands();
  w = Rands[LayerPick].rnd2[BytePick&1];
  if (!w) 		//If zero, pick another
    w = Rand2Bytes();
  return w;
}

//Will not return zero
uint32_t Rand4Bytes()
{ uint32_t w;
  NewPicks();
  RefreshRands();
  w = Rands[LayerPick].rnd4;
  if (!w) 		//If zero, pick another
    w = Rand4Bytes();
  return w;
}

uint64_t Rand8Bytes()
{ NewPicks();
  RefreshRands();
  return Rands8[RandByte() & 0x7f]; 
}


//Initialize each of the random generator buffers.  (Excluding extra at LayersMax)
//Call to restart the generators.
void InitRandLayers(KeyStruct_t *key) 
{ int i;
  uint32_t n;			
  //inspect the seeds of 1st 3 layers to see if key non-zero
  if (key->keys[0] == 0 && key->keys[1] == 0 && key->keys[2] == 0)
     abortprog("It appears that the key has not been loaded or created.", "");  
  for (i=0; i<LayersMax; i++) 	{	//don't do the extra general purpose generator 
    n = key->keys[i];	//the key value for this layer.
    //initstate must be passed zeroed ram 1st time.
    memset(LayerGenState[i], 0, sizeof(RNG_state_t));
    if (local__initstate_r(n, LayerGenState[i]->State, RandStateSize, &LayerGenState[i]->RandData) != 0)
       abortprog("Error in initstate_r", "");
  }
  //Make the 1st set of rands available, but not the extra one
  Next4Rands();
}

//Initialize the general purpose RNG (layer 256).
//Call once before 1st use of ARandom.
void InitARandom()
{ local__initstate_r(rdtsc(), LayerGenState[LayersMax]->State, RandStateSize, &LayerGenState[LayersMax]->RandData);
}

//A general purpose random number producer.
//ARandom gets initialized in init(). It is independent of layer operations.
//Use for generating randoms not connected with the layers and the key - they
//  will be different each time the program is run.
//jmp is for advancing into production
uint32_t ARandom(uint16_t jmp)
{ uint32_t n;
  int i;
  if (!jmp) jmp++;	//zero would not return a random
  local__setstate_r(LayerGenState[LayersMax]->State, &LayerGenState[LayersMax]->RandData);
  for (i=0; i<jmp; i++)
    local__random_r(&LayerGenState[LayersMax]->RandData, &n);
  return n;
}

//Copy the current set of Rands to the key part of KeyStruct_t.
//Use to make a sub-key from Rands.
//Key is not installed.
void KeyFromRands(KeyStruct_t *key)
{ memcpy(key->keys, Rands, sizeof(key->keys)); }


//************************* Decync ********************************

//In a loop, get next layer randoms and then desynchronize layers.
//Scale is the degree of desyncing, with each scale increment a doubling of decyncing.
//Scale 0-7 (will be normalized)
void DesyncLayers(uint8_t loops, uint16_t scale)
{ int i, j;
  scale = (scale&7)+1;	        //normalize to 1-8
  scale = (1 << scale)-1;	//1, 3, 7, 15, 31, 63, 127, 255
  for (j=0; j<loops; j++) {
    Next4Rands();
    for (i=0; i<LayersMax; i++)
      AdvanceLayer(i, RandByte()*scale);	//advance layer i, by RandByte*scale interations
  }    
}

//DesyncWithSeeds uses a seeds array to perform desyncronizing of layer generators.
//Desyncs all layers.
//Advances each layer generator's production by an amount selected from rands by a seed
// #define ScaleLimit 0x3f			//a PowerOf2-1,  Limits the scale of disruption
#define ScaleLimit 0x0f
void DesyncWithSeeds(uint8_t SeedArray[], uint8_t SeedsUsed, uint8_t loops)
{ int i, j, k=0;
  uint8_t seed, scale=1;
  if (isPowerOfTwo(SeedsUsed)) {
    SeedsUsed--;	//make zero based
    for (j=0; j<loops; j++)
      for (i=0; i<LayersMax; i++) {
        k = LayerRandByte(i, k) & 3;
        seed = SeedArray[i & SeedsUsed];
        scale = RandByte() & ScaleLimit;
        AdvanceLayer(i, LayerRandByte(seed, k)*scale);
      }	//for i
  }	//if
  else {
    for (j=0; j<loops; j++)
      for (i=0; i<LayersMax; i++) {
        k = LayerRandByte(i, k) & 3;
        seed = SeedArray[i % SeedsUsed];
        scale = RandByte() & ScaleLimit;
        AdvanceLayer(i, LayerRandByte(seed, k)*scale);
      }	//for i
  }
}

//Fill the ExSeed array with the randoms that the OrigSeeds point to.
//k is the byte of the layer to use (0-3), and is normalized here.
//ExSeeds and OrigSeeds can be the same address.
void ExchangeSeedsForRands(uint8_t *ExSeeds, uint8_t *OrigSeeds, uint8_t size, uint8_t k)
{ int i;
  k = k &3;
  for (i=0; i<size; i++)
    ExSeeds[i] = LayerRandByte(OrigSeeds[i], k);
}

//Fill array with numbers from the layers
void FillArrayWithRands(uint8_t *array, int size)
{ int i;
  for (i=0; i<size; i++) {
    NewPicks();
    array[i] = Rands[LayerPick].byte[BytePick];
  }
}


 #define MashPasswd
//If defined, obfuscate the Pass8 hash by using only part of it. An observer would not know
//which bytes were being used.


//If a password is used, step through each char and use them to further decync the layers.
//Create hash of RNG states (Pass8). 
//If encrypting store Pass8 in options.Pass8;
//If decryp compare with options.Pass8.  Should match.
int ProcessPassword(char *pass)
{ int i, n;
  uint8_t Pass8[PasswdSize], sequ[256];
  FillArrayWithRands(Pass8, PasswdSize);	//borrow Pass8 for a moment
  MixPw2Array8(Pass8, PasswdSize, 1);		//disrupt the order
  if (*pass) {			//replace some bytes of Pass8 with Password chars in sequ order
    GenerateSequence8(sequ, PasswdSize, RandByte(), 2);
    for (i=0; pass[i]; i++)
      Pass8[sequ[i]] = (uint8_t)pass[i];
    puts("Using a password to additionally decync layers.");
  }

  Next4Rands();
  ExchangeSeedsForRands(Pass8, Pass8, PasswdSize, BytePick);
  DesyncWithSeeds(Pass8, PasswdSize, RandByte());

#ifdef MashPasswd
//Fill Pass8 with indepenent randoms (garbage).  
//Create a sequence.  
//Using Sequence, overwrite some of the garbage just created in Pass8 with numbers from the layers.
//Those overwritten numbers become a hash of the states of the RNGs at this point.
//During decrypting we will use that same sequence to pick out the valid numbers
// to check that the RNG states are identical.  If not, then password incorrect, or progtam error.
  for (i=0; i<PasswdSize; i++)	//Fill array with independent randoms
    Pass8[i] = ARandom(1);
  GenerateSequence8(sequ, PasswdSize, RandByte(), 2);

  n = (RandByte() & 31) +24;	//Select the number of bytes to use (24 to 56).
  for (i=0; i<n; i++)		//Replace some of the array with randoms from the layers
    Pass8[sequ[i]] = RandByte();//  using a sequencer.
  if (OpMode == mode_encrypt) {
    memcpy(&options.Pass8, &Pass8, PasswdSize);	//copy Pass8 to options for inclusion in the header
    return 1;
  }
  else  
    for (i=0; i<n; i++)	//check to see if passowd and layers ok.
      if (Pass8[sequ[i]] != options.Pass8[sequ[i]])	//The sequenced positions should match, other not
        return 0;
#else
//Fill Pass8 with randoms from the layers.  Pass8 becomes a hash of the states of
//the RNGs at this point.  We will use Pass8 during decrypting to check that the 
//states are identical.
  FillArrayWithRands(Pass8, PasswdSize);
  if (OpMode == mode_encrypt)
    memcpy(&options.Pass8, &Pass8, PasswdSize);	//copy Pass8 to options for inclusion in the header
  else
    for (i=0; i<PasswdSize; i++)
      if (options.Pass8[i] != Pass8[i])
        return 0;
#endif

  return 1;
}



//Cycle the generators to unevenly advance their number progressions.
void SetupRandoms()
{ uint8_t seeds[BootSeedSize];
  KeyStruct_t *SubKey;
  int i;

//If encrypting, we don't want to exchange the the original BootSeeds because they will
//be written to the file. Instead, work on a copy.
//Replace the seeds with randoms based on the seeds
  ExchangeSeedsForRands(seeds, options.BootSeeds, BootSeedSize, 0);	
  DesyncWithSeeds(seeds, BootSeedSize, seeds[0]);

  if (!ProcessPassword(password))	//Desync using password, and setup Pass8 hash
    abortprog("Password error", "");

//Up until now the generators have been running under the initialization of the original key.
//Now we will make a new key (a sub-key) and reinitialize the generators with it.
  SubKey = malloc(sizeof(KeyStruct_t));
  KeyFromRands(SubKey);		//make a sub-key
  InitRandLayers(SubKey);	//Restart the generators with the new sub-key
  free(SubKey);

  DesyncLayers(RandByte()&1, RandByte());
//Increasing the above &1 greatly increases the desyncing, and also slows the setup.

//  MixLayerGens();	//Uncomment to mixup the RNGs

//The generators have been restarted using a key that is unique to each encrypted file,
// and they are now each in different stages of their number progression.
//We are ready to begin using the generators for encryption.
  if (verbose)
    puts("Finished setting up.");
}
