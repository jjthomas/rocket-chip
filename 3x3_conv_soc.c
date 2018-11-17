// parameter to the hardware memory system is size of token (in this case 16, the size of short)

#define PIPE_LATENCY 2
// window dimension is 3
#define PAD 2
// should correspond to amount of BRAM space
#define STRIP_SIZE 100

void call_acc(void *input_burst, void *output_burst, int num_tokens); // output_burst set to 0 discards output
// should probably also have a call_acc_nonblocking call

short input[H * W];
short output[H * W];
short edge_buf1[STRIP_SIZE + PAD];
short edge_buf2[PIPE_LATENCY + PAD];
for (int i = 0; i < W; i += STRIP_SIZE) {
  int cur_strip_size = min(STRIP_SIZE, W - i); // need to pass this value in to the accelerator as <num cols for strip>
  for (int j = -PAD; j < H; j++) {
    if (j == -PAD) {
      memset(edge_buf1, 0, cur_strip_size + PAD); // top padding
    }
    int offset = j * W + i;
    if (j < 0) {
      call_acc(edge_buf1, 0, STRIP_SIZE + PAD);
    } else {
      if (i == 0) {
        memset(edge_buf1, 0, PAD);
      } else {
        memcpy(edge_buf1, input + offset - PAD, PAD);
      }
      call_acc(edge_buf1, edge_buf2, PAD);
      assert(cur_strip_size >= PIPE_LATENCY); // important to check this condition for the right edge ...
      // ... if it doesn't hold maybe do the computation in software?
      call_acc(input + offset, edge_buf2 + PAD, PIPE_LATENCY);
      if (j > 0) {
        memcpy(output + (j - 1) * W + i + cur_strip_size - PIPE_LATENCY, edge_buf2, PIPE_LATENCY);
      }
      call_acc(input + offset + PIPE_LATENCY, output + offset, cur_strip_size - PIPE_LATENCY);
    }
    if (j == H - 1) {
      call_acc(edge_buf2 /* garbage */, output + offset + cur_strip_size - PIPE_LATENCY, PIPE_LATENCY);
    }
  }
}
