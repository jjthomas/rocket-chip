// See LICENSE for license details.

#include "util.h"

int printf(const char* fmt, ...);

#define STR1(x) #x
#define STR(x) STR1(x)
#define EXTRACT(a, size, offset) (((~(~0 << size) << offset) & a) >> offset)

#define CUSTOMX_OPCODE(x) CUSTOM_##x
#define CUSTOM_0 0b0001011
#define CUSTOM_1 0b0101011
#define CUSTOM_2 0b1011011
#define CUSTOM_3 0b1111011

#define CUSTOMX(X, rd, rs1, rs2, funct) \
  CUSTOMX_OPCODE(X)                   | \
  (rd                   << (7))       | \
  (0x7                  << (7+5))     | \
  (rs1                  << (7+5+3))   | \
  (rs2                  << (7+5+3+5)) | \
  (EXTRACT(funct, 7, 0) << (7+5+3+5+5))

#define CUSTOMX_R_R_R(X, rd, rs1, rs2, funct)           \
  asm ("mv a4, %[_rs1]\n\t"                             \
       "mv a5, %[_rs2]\n\t"                             \
       ".word "STR(CUSTOMX(X, 15, 14, 15, funct))"\n\t" \
       "mv %[_rd], a5"                                  \
       : [_rd] "=r" (rd)                                \
       : [_rs1] "r" (rs1), [_rs2] "r" (rs2)             \
       : "a4", "a5");

int count_chars_slow(char *str, char needle)
{
  int i = 0;
  int count = 0;
  while (str[i] != 0) {
    if (str[i] == needle) {
      count++;
    }
    i++;
  }
  return count;
}

int count_chars_fast(char *str, char needle)
{
  int result;
  asm volatile ("fence");
  CUSTOMX_R_R_R(2, result, str, needle, 0)
  // asm volatile ("custom2 %[result], %[addr], %[needle], 0" : : [result]"r"(result), [addr]"r"(str), [needle]"r"(needle));
  asm volatile ("fence");
  return result;
}

//--------------------------------------------------------------------------
// Main

// must be at least 64 for cache line alignment
#define DATA_SIZE 100

int main( int argc, char* argv[] )
{
  char raw_str[DATA_SIZE];
  char *str = raw_str;
  for (int i = 0; i < DATA_SIZE - 1; i++) {
    str[i] = 1;
  }
  str[DATA_SIZE - 1] = 0;
  // intptr_t ip = ((intptr_t)(str + 63) >> 6) << 6;
  // printf("expected result: %d\n", DATA_SIZE - 1 - (ip - (intptr_t)str));
  // str = (char *)ip;
  // char *str = "hello";

#ifdef PREALLOCATE
  // preallocate everything in the caches
  // count_chars_slow(str, 1);
#endif

  // Do the vvadd
  setStats(1);
  int result = count_chars_fast(str, 1);
  setStats(0);

  printf("addr: %p\n", str);
  printf("result: %d\n", result);
  return 0;
}
