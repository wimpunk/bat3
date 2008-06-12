#include <stdio.h>
#include "bat3.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

/*
  Read out the data written by bat3save and print it.
 */
FILE *f = 0;
bat3Info b[256];

#define SAMPLES 1

// normalize battery voltage (~4 -> 4)
float nV(float f) {
  return f;
}

// normalize battery current (~800000 -> 4)
float nI(int i) {
  return (float)i / 200000;
}

// normalize temperature (~90 -> 4.5)
float nT(float f) {
  return f/20;
}

int main() {
  FILE *f = fopen("bat.dat","r");
  int i = 0,j;
  float V,I,F;

  if (!f) {
    perror("fopen:");
    return 1;
  }
  printf("#,V,I,F\n");
  while (!feof(f)) {
    if (fread(&b,sizeof(bat3Info),SAMPLES,f) == SAMPLES) {
      V = I = F = 0;
      for (j=0;j<SAMPLES;j++) {
	V += b[j].battV;
	I += b[j].battI;
	F += b[j].tempF;
      }
      V /= SAMPLES;
      I /= SAMPLES;
      F /= SAMPLES;
      if (i++ > 0) {
	printf("%d,%.4f,%.0f,%.2f\n",i,V,I,F);
      }
    }
  }
  fclose(f);
  return 0;
}
