#ifndef LC_LAYERS_INCLUDED
#define LC_LAYERS_INCLUDED

//************************ LAYERS ********************************

//New rands for 1 layer loop times
//Used when refreshing the randoms, and for decyncing
void AdvanceLayer(uint8_t lay, uint16_t loop);

//New rands for all layers (except LayersMax).
void Next4Rands() ;

//return byte at layer "lay", and byte position "k"
//k = 0 thru 3 (k is normalized here.)
//Also see Rand1024Byte[], which accesses the 1024 bytes directly, not by layers, or RandByte()
//Almost all selection from the randoms are made through this function, or RandByte() which calls it.
//This function also refreshes the randoms during a byte selection.
uint8_t LayerRandByte(uint8_t lay, uint8_t k);

//Select a number from a layer and a byte using the randoms for the selection
//If zero, then try again. 
//Because zeros should be avoided when encrypting using xor, this function should be used for that.
uint8_t RandByte();


/*
//Add 4 BandBytes together into a uint8_t so that they wrap.
//Calls RandByte() 4 times
uint8_t Bytes4();
*/

//Return a random two-byte word by calling RandBte twice
uint16_t Rand2Bytes();

//Will not return zero
uint32_t Rand4Bytes();

uint64_t Rand8Bytes();

//Initialize each of the random generator buffers.  (Excluding extra at LayersMax)
//Call to restart the generators.
void InitRandLayers(KeyStruct_t *key) ;

//Initialize the general purpose RNG, layer 256
//Call once before 1st use of ARandom.
void InitARandom();

//A general purpose random number producer.
//ARandom gets initialized in init(). It is independent of layer operations.
//Use for generating randoms not connected with the layers and the key.
//jmp is for advancing into production
uint32_t ARandom(int jmp);

//Copy the current set of Rands to the key part of KeyStruct_t
//to make a sub-key from Rands.
//Key is not installed.
void KeyFromRands(KeyStruct_t *key);

//************************* Decync ********************************

//In a loop, Get next layer randoms and then desync layers.
//Scale is the degree of desyncing with each scale increment a doubling of decyncing.
//Scale 0-7 (will be normalized)
void DesyncLayers(uint8_t loops, uint8_t scale);

//DesyncWithSeeds uses a seeds array to perform desyncing.
//Desyncs all layers.
//Advances each layer by an amount selected from rands by a seed
//In this function k is normalized to 0-3.
void DesyncWithSeeds(uint8_t SeedArray[], uint8_t SeedsUsed, uint8_t loops);


//Exchange the bytes in BootSeeds array with the randoms that the they point to.
void ExchangeSeeds(options_t *opt);


//If a password is used, step through each char and use them to further decync the layers
void ProcessPassword(char *pass);

//for debugging
void DisplayOptSeeds();

//Cycle the generators to unevenly advance their progressions.
void SetupRandoms();

#endif
