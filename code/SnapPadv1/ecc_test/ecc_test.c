#include "ecc.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>


uint8_t buf1[512];
uint8_t buf2[512];

void prep_buffers() {
  for (int i = 0; i < 512; i++) {
    buf1[i] = buf2[i] = rand();
  }
}

void zero_buffers() {
  for (int i = 0; i < 512; i++) {
    buf1[i] = buf2[i] = 0;
  }
}

bool check_buffers() {
  for (int i = 0; i < 512; i++) {
    if (buf1[i] != buf2[i]) return false;
  }
  return true;
}

bool run_no_err_test() {
  prep_buffers();
  uint32_t ecc = ecc_generate(buf1);
  return ecc_verify(buf2, ecc);
}

bool run_1_err_test() {
  prep_buffers();
  uint32_t ecc = ecc_generate(buf1);
  int byte_idx = rand()%512;
  int bit_idx = rand()%8;
  buf2[byte_idx] ^= 0x01 << bit_idx;
  bool v = ecc_verify(buf2, ecc);
  if (v) {
    if (!check_buffers()) {
      printf("VERIFIED but not FIXED\n");
      return false;
    }
  }
  return v;
}

bool run_crc_err_test() {
  prep_buffers();
  uint32_t ecc = ecc_generate(buf1);
  int bit_idx = rand()%24;
  ecc ^= 0x1 << bit_idx;
  return ecc_verify(buf2, ecc);
}

bool run_2_err_test() {
  prep_buffers();
  uint32_t ecc = ecc_generate(buf1);
  int byte_idx = rand()%512;
  int bit_idx = rand()%8;
  int byte2_idx = rand()%512;
  int bit2_idx = rand()%8;
  while ((byte2_idx == byte_idx) && (bit_idx == bit2_idx)) {
    byte2_idx = rand()%512;
    bit2_idx = rand()%8;
  }
  buf2[byte_idx] ^= 0x01 << bit_idx;
  buf2[byte2_idx] ^= 0x01 << bit2_idx;
  bool v = ecc_verify(buf2, ecc);
  return v == false;
}

uint32_t test_value(int where, int what) {
  zero_buffers();
  buf1[where] = what;
  return ecc_generate(buf1);
}

int count_bits(uint32_t v) {
  int bits = 0;
  while (v) {
    if (v & 0x01) bits++;
    v >>= 1;
  }
  return bits;
}

#define TC 100000

void main() {
  srand(time(NULL));
  printf("ECC test start.\n");
  int passes = 0;
  for (int t=0;t<TC;t++) {
    if (run_no_err_test()) { passes++; }
  }
  printf("No errors: %d/%d passed.\n",passes,TC);
  passes = 0;
  for (int t=0;t<TC;t++) {
    if (run_1_err_test()) { passes++; }
  }
  printf("One error: %d/%d passed.\n",passes,TC);
  passes = 0;
  for (int t=0;t<TC;t++) {
    if (run_crc_err_test()) { passes++; }
  }
  printf("CRC error: %d/%d passed.\n",passes,TC);
  passes = 0;
  for (int t=0;t<TC;t++) {
    if (run_2_err_test()) { passes++; }
  }
  printf("Two errors: %d/%d passed.\n",passes,TC);
}

