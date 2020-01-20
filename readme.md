## LayerCrypt: Introduction

LayerCrypt is a symmetrical encryption program written in C that, similar to many others,  produces a repeatable set of random numbers to XOR characters to encrypt and decrypt them.  Unlike others that I know of, LayerCrypt uses multiple simple random number generators (RNGs) with short, 4-byte initialization keys.  And it uses lots of them.  I think that the approach is interesting in that the security comes, not from a single stream of numbers, but from a continually varying selection of the many number streams produced.  

The "security" of the RNGs become a secondary issue, because the unpredictably varying selection of streams removes the focus from prediction a single stream's production of numbers, to that of many.  

It is a chaotic approach.  An important aspect of a random number generator is its conformity to "randomness".  Great effort is expended to insure this randomness, and a less-random generator is considered flawed.  So when trying to guess the random stream the guesser can relegate such non-randomness as improbable in their searches.  In contrast, with the layered approach, chaos is the stream's format.  And such a format includes all possibilities.

LayerCrypt uses 256 RNG's, each with its own 4-byte key.  All keys work together so that trying to guess one 4-byte key is non-productive as each of the 8,192 bits of the 256 keys must be correct before there is any indication that a correct 4-byte key has been found.

To help understand the concept I have included three folders, each adding to the previous and containing an increasingly complex version of LayerCrypt.

### Folder Basic

Each random number generator is independent, being initialized using its own 4-byte key.  These generators are organized into what I call layers and each layer has its own instance of a RNG and a place for it to put the next number in its stream.  There are 256 layers and when a new set of randoms is requested, they all produce the next number in their streams.  The result is a 256 4-byte array of random numbers (a "page").  I refer to this array as the "Rands".  The 4-byte numbers can be accessed as 4 separate bytes.    The variable “k” is generally used to indicate which of the 4 bytes is to be used.

For each character being encoded, LayerCrypt selects a byte from this 1024 byte array of Rands to xor with the character.  Periodically the generators are called upon to generate a new number and the resultant Rands array is refreshed.  This refreshing can be made to occur at regular intervals or even randomly.  If the refresh rate is set to 4, every four calls for a random a new set is made - discarding 1020 of the 1024 numbers last generated.

The description so far is illustrated in the folder 'basic' in which nearly everything has been stripped out but the essential premise of layering.  This is to make it easy to follow.  There is no ability to name a key or create a new key and so one of the proper name has been copied into the the folder.  Running the program with "source" encrypts to "target".  Running it again with "target" as the source produces a new target the same as the original source.  There are no checks to insure correct operation, and the program is not useful.

### Folder Basic2

A problem with the basic scenario above is that every encryption done with the same key will have used an identical stream of randoms.  The folder 'basic2' addresses this concern by generating a set of seeds that are used to "desyncronize" the layer generators, advancing each by varying degrees in their number production streams.  This is done before the randoms are considered ready for use.  Those seeds are randomly generated at start-up based on a generator initialized from the processor's rdtsc and so will be different for each encoding attempt, and independent from the key.  These seeds are also stored in the encrypted file header so that the decryption can use them to set up the randoms to an identical state.

It should be noted that the seeds are never used directly.  Rather, the seeds are used to select values from the Rands which are then used instead to desyncronize the layers.

Desyncronizing the layers, or desyncing, is the what makes the layer technique useful.  During the process the number production of individual layer RNGs are advanced by varying degrees.  By the time the randoms are ready for use, through desyncing all the RNGs are in different stages of their number stream production.  They could be 100's or 10's of thousands of cycles different.  The 1st variable in DesyncLayers() is a large determinant of the final layer spread.  

Because desyncing can be done with seeds, it is easy to feed an additional desyncing operation with the characters of a password used as seeds.  This has been added to the Basic2 code, but the password is not checked.  If incorrect, the decoding will still progress producing garbage.

Additionally, after desyncing, Basic2 creates a new internal key, a sub-key, from the Rands and re-initializes all the generators with this new key.  Thus, not only is each cypher performed with a unique set of random streams, the generators are initialized using a unique, and never to be used again, 1024 byte key.

The layer technique depicted in Basic or Basic2 could be incorporated in almost any encryption scheme that uses a stream of random numbers.

### Folder WithSequencers

A third folder, "WithSequencers", contains a working program.  It is still simple and with few checks, but three more features have been added than make the layer concept more useful.  Even this version should not be considered complete or usable.  It is still merely a demonstration of the layer concept.

First, the password, though still optional, is checked during the decoding to see that it matches the original.  This is done by taking a random sample picked from the randoms and storing them in the header of the file.  They will only match if an identical password is supplied.  Note that if there is a concern about exposing a part of the Rands in the encrypted file's header, the "#define MashPasswd" can be un-commented in lc_layers.c.  This uses a sequencer to expose a smaller portion of the Rands in a mixed up fashion embedded in an array of garbage.

Second, the concept of sequencers is introduced.  Sequencers are created to redirect requests.  In this case each time the 1024 byte input buffer is filled, a new 1024 byte sequence is created.  During encoding, this sequence is used to disorder the output.  During decoding, the same sequence is used to reorder the output.

Third, the cypher can be done by using modulo 256 arithmetic to combine multiple bytes for each xor of the character.  This is done with the AddMultiBytes() function and is enabled by de-commenting #define UseMultiBytes in lc_cypher.c.



#### A Self-Critique

I think a LayerCrypt strong point is that all chosen operations must be performed serially in the correct fashion for the decipher to work.  A single incorrect bit along the way will result in a completely unreadable document.  But this is also a weakness.  Not as in vulnerability, but code maintenance.  All but cosmetic changes to the code will likely break previous encryptions.  For this reason, it would be best to decide how little needs to be included while still achieving security, and strip out the rest.

Similar to the above, most of what LayerCrypt does is arbitrary - the operations are there to create chaos.  Many of its operations could be done using differing parameters, in a different order, with increased frequency, or left out entirely.  This adds to the maintenance problem.  But there is a possibly important aspect to this: whatever is done must be done for both cipher and decipher and with just a few exceptions the program insures that this will be the case.  Significantly, this means that it would be relatively easy for anyone with a basic understanding of C coding to make their own personal encryptor by merely altering a single number or operation - a compile which would then be different from all others.

####Notes

LayerCrypt is a command line program and it is neither complete nor intended for general use.  I have posted it here to introduce the concept in the hope that someone thinks it is worth pursuing. I have no interest in profiting from it and it is being released under GNU General Public License.

Layercrypt was written for the Centos 7 (Redhat) Linux platform using gcc.  No effort has been made to accommodate other platforms.  Some of you might recognize LayerCrypt to be a rewritten version of RandCrypt which was posted to Github in 2015.  RandCrypt has been removed because it was poorly written and too complicated.  I am hoping that the replacement, LayerCrypt, better illustrates the concept and helps spark further thoughts.

I am not great C programmer.  I have used Pascal and Assembler since 1984, but had merely dabbled in C until late 2014 with the start of RandCrypt.  

2019-01-18  
Rees Acheson  
23 Bley Rd.  
Alstead, NH, 03602 USA  
rees@sover.net
