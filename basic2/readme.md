## Basic2

  This folder contains a still simple version of LayerCrypt, but one that is more useful than the one in Basic.  It will encrypt and decrypt and use an optional password. It is here to introduce three more concepts: 

  1) Desyncronizing the number streams;

  2) Using BootSeeds to make the desycronizing, and thus the encryption, unique.

  3) Sub-Keys

  This version also has the ability to create a key file.  Due to the use of seeds, encryption and decryption must now be specified using the -e or -d commandline options.

### Desync
  Desyncronizing the layers, or desyncing, is the what makes the layer technique useful.  During the process the number production of individual layer RNGs are advanced by varying degrees.  By the time the randoms are ready for use, through desyncing, all of the 256 RNGs are in different stages of their number stream production. 

### BootSeeds
  The BootSeeds are an array of random numbers, unique to each time the program encrypts.  They are used to desync the layer RNGs so that they are at a unique combination of desyncronization.  The combined randoms produced will also be unique.  During decryption, these seeds are passed to the program to produce an identical stream of numbers.
The seeds are never used directly, but are exchanged with numbers selected from randoms in the layers.  In that way nothing in the options header is used directly in the decryption - they merely point to where to find the numbers to use.

### Sub-Keys
  Not coincidentally, the structure of the "Rands" (the array of 256 4-byte numbers) is identical to the structure of the array for the 256 keys to initialize the RNGs.  After using seeds to desync, a unique set of Rands will exist.  Merely coping the Rands into a key structure will have created a new and unique sub-key.  Restarting the RNGs using this new sub-key will produce a set of number streams unrelated to that of the streams produced with the main key.  Basic2 uses only one sub-key, but two or more in succession is also feasible.

  The layer technique depicted in Basic or Basic2 could be incorporated in almost any encryption scheme that uses a stream of random numbers.
