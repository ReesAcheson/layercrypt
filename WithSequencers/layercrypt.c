/*
Copyright 2019 Rees H. Acheson
               23 Bley Rd.
               Alstead, NH 03602, USA
               rees@sover.net

    layercrypt is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    layercrypt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with layercrypt.  If not, see <http://www.gnu.org/licenses/>.

This program is for the demonstration of the layering concept only.  It is not meant
to be a useful rendition of that concept.

Written and compiled using Linux Centos 7 with gcc.  
It has not been tested under anything else and no effort has been made to accommodate.

Replaces RandCrypt which was posted to Github in 2015.  Randcrypt was too 
complicated and poorly written.  This attempt breaks down the demo into three stages:

  Basic:  Merely demonstrates multiple RNGs in layers.  Not useful.
  Basic2: Simple, but adds key creation, layer desyncing and unique RNG progressions.
  WithSequencers: A working demo program.

*/

#include "lc_vars.h"
#include "lc_util.h"
#include "lc_debug.h"
#include "lc_layers.h"
#include "lc_files.h"
#include "lc_init.h"
#include "lc_crypt.h"


/*
 ************************** The BootSeeds **************************
  The BootSeeds are needed to create a unique set of number streams from the RNG generators.
Their value does not matter, only that the are likely unique from past encryptions.  
Randomness is immaterial, but a random generator is a convenient way to get them.
  These seeds will be visible and so it is probably best if they were not obtained from
the layer generators, as these are related to the key.  Instead, I have used a separate 
generator, isolated from the others by initializing it with the processor rdtsc.  However,
using a system RNG such as /dev/urandom would be just as good.
 *******************************************************************


 ************************** The Password ****************************
If supplied, Password text is used to desyncronize the generators.
After this, the uint8_t array pass8 is filled with randoms created with the 
newly desynced generators.  
If encrypting, Pass8 is copied to options.Pass8 so that it is written to the encrypted file's header.
If decrypting, Pass8 is compareed with options.Pass8 obtained from the encrypted file's header. 
  The bytes will match if the passwords were identical (including if optional password is omitted). 
 ********************************************************************


 ************************** FileSize ********************************
The file size of the original file is added to a 64-bit number gathered from the rands.
It is then stored in the options header of the encoded file (options.FileSize).
Upon decoding, that same random number is subrtacted from the options.FileSize
which then becomes the original FileSize number.
FileSize is not used in this program, but it may be desirable to pad the encoded file
to a rounded length to hide it's true length.  Passing the FileSize in this
way allows for truncating the padded file to the original length.
*/



//Fill the options struct with name, version and BootSeeds.
//The BootSeeds are selected using a separate random generator that has been initialized
//using the rdtsc.  Therefore the seeds are unique to each encrypted file.
//There is no connection to the layers of randoms, or to the key used to make them.
//op->Pass8 gets filled in later, after processing the optional password.
void FillOpHeader(options_t *op)
{ int i;
  _strlcpy(op->ProgName, ProgramName, sizeof(op->ProgName));
  _strlcpy(op->Ver, VersionProg, sizeof(op->Ver));
  for (i=0; i<BootSeedSize; i++)
    op->BootSeeds[i] = (uint8_t)ARandom(1);	//Don't use layers for these randoms.
}	//FillOpHeader



//****************************** main ********************************

int main(int argc, char *argv[])
{
  init(argc, argv);
  if (OpMode == mode_none)
    abortprog("Nothing to do.","");

  if (OpMode == mode_newkey) {
    if(FileCreateKey(TheKey, KeyFn))
      printf("Key Created: %s\n", KeyFn);
    else
      abortprog("Unable to create keyfile:", KeyFn);
    return EXIT_SUCCESS;
  }

  if (!FileLoadKey(TheKey, KeyFn))
    abortprog("Unable to load the key:", KeyFn);

  if (!OpenFiles(&src, &dest))
    abortprog("Unable to open files", "");

  InitRandLayers(TheKey);

  switch (OpMode) {
    case mode_encrypt: FillOpHeader(&options);		//except for Pass8
                       FileSize = FileGetSize(src);
                       //printf("FileSize: %i\n", (uint32_t)FileSize);
                       SetupRandoms();
                       options.FileSize = FileSize + Rand8Bytes();
                       //printf("options.fs: %" PRIx64 "\n", options.FileSize);
                       if (!FileWriteOptions(&options, dest))
                         abortprog("Writing options failed.", "");
                       EncryptFile(src, dest); 
                       break;
    case mode_decrypt: if (FileReadOptions(&options, src)) {
                         SetupRandoms();
                         FileSize = options.FileSize - Rand8Bytes();
                         //printf("FileSize: %i\n", (uint32_t)FileSize);
                         DecryptFile(src, dest); 
                       }
                         else abortprog("unable to retrieve options from", SrcFn);
                       break;
  }	//switch
  if (debug == dg_RandsCheck)
    DisplayReasonables();
  return EXIT_SUCCESS;
}
