## Folder WithSequencers

The WithSequencers folder contains the final LayerCrypt version in the demo.

This version adds:
 
  1) Password checking;

  2) Sequencing the output to disrupt the order;

  3) Using multi-byte randoms with modulo 256 arithmetic to encrypt the character.

The version in this folder is still just a demo of the concept, but it now usefully encrypts and decrypts data.


### The BootSeeds
  The BootSeeds are an array of numbers, unique to each time the program encrypts.  They are needed to create a unique set of number streams from the generators. Their values do not matter, only that they are likely unique from past encryptions.  Randomness is immaterial, but a random generator is a convenient way to get them.

  The BootSeeds are used to desync the layer RNGs so that they are at a unique combination of desyncronization.  The streams produced will also be unique.  During decryption, these seeds are passed to the program to produce an 
identical stream of numbers.  The seeds are never used directly, but are exchanged with numbers selected from 
randoms in the layers.  The seeds merely point to where to find the numbers to use.

  The BootSeeds will be visible and so it is probably best if they were not obtained from the layer generators, as these are related to the key.  Instead, I have used a separate generator, isolated from the others by initializing it with the processor rdtsc.  However, using a system RNG such as /dev/urandom would be just as good.

  Note that if there is a concern about exposing a part of the Rands in the encrypted file's header during password checking, the "#define MashPasswd" can be un-commented in lc_layers.c.  This uses a sequencer (see below) to expose a smaller portion of the Rands embedded in a mixed up fashion into an array of garbage.


### Sequencers
Generating Sequences:
  A sequencer first fills a sequence array with sequential numbers, zero to size (i.e. 0,1,2,3,4...).  The sequencer then uses the current state of the layer randoms to build a random sequence array by swapping members of the array.  All of the original numbers still exist but in a mixed order. The sequence is then used to redirect a request in some way:

  GenerateSequence8(sequence, size, 0, 1);
  for (i=0; i<size; i++)
    disarry[sequence[i]] = inarry[i];
//And then, to get it back in proper order:
  for (i=0; i<size; i++)
    outarry[i] = disarry[sequence[i]];

  Although sequencers are quite useful, the WithSequencers version of LayerCrypt only uses sequencers twice: 
- Once when preparing the hash for password checking; 
- And once during the output of encryption, or input of decrypting.

  The swapping is based on the current state of the layer randoms. For example, position 0 of the sequence array is swapped with the position pointed to by the random byte held in layer 0. Position 1 with the position layer 1 contains,
and so on. A single pass makes for a well scrambled sequence as many of the values will have been swapped multiple times. One of the parameters to the sequencers is the number of mix-ups to perform. The function enters a loop of mix-ups to
additionally scramble the order. A mix of zero returns an unscrambled, naturally ordered array of numbers. Sequence creation is surprisingly fast and continually making lots of new sequences has very little speed penalty.

  The layers make it fast and easy to build sequences.



### The Password
If supplied, Password text is used to desync the generators.
After this the uint8_t array Pass8 is filled with randoms created with the desynced generators.  
If encrypting, Pass8 is copied to options.Pass8 so that it is written to the encrypted file's header.
If decrypting, Pass8 is compared with options.Pass8 obtained from the encrypted file's header. 
The bytes will match if the passwords were identical (including if no password is given). 

Concerns about exposing a random sample of the randoms in the header:
  If  #define MashPasswd is defined, the Pass8 contents are obfuscated:
  - Fill Pass8 with independent randoms (meaningless garbage).  
  - Create a sequence.  
  - Using Sequence, overwrite some of the garbage just created in Pass8 with numbers from the layers.
Those overwritten numbers become a hash of the states of the RNGs at this point.  During decrypting we will use that same sequence to pick out the valid numbers to check that the RNG states are identical.  If not, then password incorrect, or program error.



### FileSize
The file size of the original file is added to a 64-bit number gathered from the Rands.  It is then stored in the options header of the encoded file (options.FileSize).  Upon decoding, that same random number is subtracted from the options.FileSize which then becomes the original FileSize number.  FileSize is not used in this program, but it may be desirable to pad the encoded file to a rounded length to hide it's true length.  Passing the FileSize in this way allows for truncating the padded file to the original length.


