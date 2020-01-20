#include "lc_vars.h"

//************************* some debug routines *******************************

int RandsAvgSum = 0;

void ShowKey(KeyStruct_t *key)
{ int i, j=0;
  for (i=0; i<LayersMax; i++) {
    if (j>=12) {puts(""); j=0;}
    printf("%08x ", key->keys[i]);
    j++;
  }
  puts("");
}


void DisplaySeeds(uint8_t *sds, int size)
{ int i;
  puts("Seeds:");
  for (i=0; i<size; i++)
   printf("%4i", sds[i]);
  puts("");
}

//Inspect a single page of randoms.
void Display1024ArrayStats(uint8_t array[1024], char *msg)
{ int i, j, k, mean;
  uint8_t n;
  uint8_t counts[256];
  uint32_t accum = 0;
  printf("%s\n", msg);
  memset(counts, 0, sizeof(counts));
  for (i=0; i<1024; i++) {	//calc mean
      n = array[i];
      accum += n;
      counts[n]++;
    }  
  mean = accum / 1024;  
  printf("Of the 1024 bytes the mean value was: %d\n", mean);
  accum=0;
  for (i=0; i<256; i++)
    accum += counts[i];
   printf("Byte value usage (hex). Average use was %d times:\n", accum/256);  
  j = 0;
  k = 0;
  printf("  ");
  for (i=0; i<16; i++)	//print the top row legend
    printf("%3x", i);
  printf("\n0)");		//new line then print a zero
  for (i=0; i<256; i++) {	//print times used for each of the 1024 bytes
    printf("%3x", counts[i]);
    k += counts[i];
    if ((((i+1) &15) == 0) && (i < 255)) {	//time for a new line?
      j++;
      printf("\n%x)", j);	//start new line with vertical legend
    }  
  }  
  puts("");  
}


//If debug =  dg_RandsCheck then each call to Next4Rands() calls ReasonableRands(), 
//which keeps track of info about the randoms.
//When done, calling Displayreasonbles() will display that info.
//This is used to get an idea of what the layers look like when compared to one aonther.

int ReasonableRands(int range, int HiLimit)
{ int i, avg, accum = 0;
  uint8_t *counts;
  uint8_t hi = 0;
  counts = calloc(256, 1);
  ReasonablesChecked++;
  for (i=0; i<LayersMax*4; i++) {
    counts[Rand1024Byte[i]]++;
    accum = accum + Rand1024Byte[i];
  }
  for (i=0; i<256; i++)
    if (counts[i] > hi) 
      hi = counts[i];
  avg = accum / (LayersMax*4);
  RandsAvgSum +=avg;
  free(counts);
  if ((avg <(127-range)) || (avg > (127+range)) || (hi > HiLimit)) {
    Unreasonables++;
    return 0;	//Not a reasonable set.
  }  
  return 1;  
}



void DisplayReasonables()
{ printf("Reasonable Check: with range of +/-%d and repeats limit of %d\n", ReasonableRange, ReasonableMax);
  printf("  Unreasonable sets = %d; out of %d checked (%3.2f%%);  Average value = %d\n", Unreasonables, ReasonablesChecked, (((float)Unreasonables/ReasonablesChecked)*100), RandsAvgSum/ReasonablesChecked);
}


