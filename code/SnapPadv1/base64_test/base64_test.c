#include "base64.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void main() {
  char in[3];
  char out[4];
  while (!feof(stdin)) {
    int count = fread(in,1,3,stdin);
    while (!feof(stdin) && count<3) {
      count += fread(in,1,3-count,stdin);
    }
    encode(in,out,count);
    fwrite(out,4,1,stdout);
  }
}
