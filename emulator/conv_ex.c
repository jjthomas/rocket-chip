// See LICENSE for license details.

#include <stdlib.h>
#include "util.h"

int printf(const char* fmt, ...);
void* memcpy(void* dest, const void* src, size_t len);
void* memset(void* dest, int byte, size_t len);

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

#define H 10
#define W 1024
#define PIPE_LATENCY 2
// window dimension is 3
#define PAD 2
// should correspond to amount of BRAM space
#define STRIP_SIZE 1024

short input[H * W];
short output[H * W];

void call_acc(short *input_burst, short *output_burst, int num_tokens) {
  int unused_result;
  asm volatile ("fence");
  CUSTOMX_R_R_R(1, unused_result, input_burst, num_tokens * 2 /* num_tokens in terms of 16-bit tokens */, 1)
  asm volatile ("fence");
}

void config_acc() {
  int unused_result;
  int throughput = 16; // cycles per 64-bit word
  asm volatile ("fence");
  CUSTOMX_R_R_R(1, unused_result, throughput, unused_result, 0)
  asm volatile ("fence");
}

int conv_fastest(short *in, short *out) {
  config_acc();
  call_acc(in, out, H * W);
  return 0;
}

inline int min(int x, int y) {
  return x < y ? x : y;
}

int conv_faster(short *in, short *out) {
  config_acc();

  short edge_buf1[STRIP_SIZE + PAD];
  short edge_buf2[PIPE_LATENCY + PAD];
  for (int i = 0; i < W; i += STRIP_SIZE) {
    int cur_strip_size = min(STRIP_SIZE, W - i); // need to pass this value in to the accelerator as <num cols for strip>
    for (int j = -PAD; j < H; j++) {
      if (j == -PAD) {
        memset(edge_buf1, 0, sizeof(short) * (cur_strip_size + PAD)); // top padding
      }
      if (j < 0) {
        call_acc(edge_buf1, 0, STRIP_SIZE + PAD);
      } else {
        int offset = j * W + i;
        if (i == 0) {
          memset(edge_buf1, 0, sizeof(short) * PAD);
        } else {
          memcpy(edge_buf1, input + offset - PAD, sizeof(short) * PAD);
        }
        call_acc(edge_buf1, edge_buf2, PAD);
        // assert(cur_strip_size >= PIPE_LATENCY); // important to check this condition for the right edge ...
        // ... if it doesn't hold maybe do the computation in software?
        call_acc(input + offset, edge_buf2 + PAD, PIPE_LATENCY);
        if (j > 0) {
          memcpy(output + (j - 1) * W + i + cur_strip_size - PIPE_LATENCY, edge_buf2, sizeof(short) * PIPE_LATENCY);
        }
        call_acc(input + offset + PIPE_LATENCY, output + offset, cur_strip_size - PIPE_LATENCY);
        if (j == H - 1) {
          call_acc(edge_buf2 /* garbage */, output + offset + cur_strip_size - PIPE_LATENCY, PIPE_LATENCY);
        }
      }
    }
  }
  return 0;
}

int conv_slow(short *in, short *out)
{
  int sum = 0;
  for (int i = PAD; i < H; i++) {
    for (int j = PAD; j < W; j++) {
      int o0 = i * W + j;
      int o1 = (i - 1) * W + j;
      int o2 = (i - 2) * W + j;
      // would normally be out[o0] = ..., but we don't support output now
      sum += in[o0] >> 1 + in[o0 - 1] >> 2 + in[o0 - 2] >> 3 + in[o1] >> 4 +
        in[o1 - 1] >> 5 + in[o1 - 2] >> 6 + in[o2] >> 7 + in[o2 - 1] >> 8 + in[o2 - 2] >> 9;
    }
  }
  return sum;
}

//--------------------------------------------------------------------------
// Main

// must be at least 64 for cache line alignment
#define DATA_SIZE 100

int main( int argc, char* argv[] )
{
  for (int i = 0; i < H * W; i++) {
    input[i] = 0;
  }

#ifdef PREALLOCATE
  // preallocate everything in the caches
  // count_chars_slow(str, 1);
#endif

  // Do the vvadd
  setStats(1);
  int result = conv_fastest(input, output);
  setStats(0);

  printf("result: %d\n", result);
  return 0;
}
