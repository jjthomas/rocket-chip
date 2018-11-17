// parameter to the hardware memory system is size of token (in this case 16, the size of short)

#define PIPE_LATENCY 2
// in terms of shorts
#define BURST_SIZE 64
// window dimension is 3
#define PAD 2
// some multiple of the burst size, - PAD
// should correspond to amount of BRAM space
#define STRIP_SIZE (BURST_SIZE - PAD)

void call_acc(void *input_burst, void *output_burst, int num_tokens); // output_burst set to 0 discards output
// should probably also have a call_acc_nonblocking call

short input[H * W];
short output_raw[PIPE_LATENCY + H * W]; // a few garbage outputs at the start
short *output = output_raw - PIPE_LATENCY;
short edge_buf1[BURST_SIZE];
short edge_buf2[BURST_SIZE];
for (int i = 0; i < W; i += STRIP_SIZE) {
  for (int j = -PAD; j < H; j++) {
    if (j == -PAD) {
      memset(edge_buf1, 0, BURST_SIZE); // top padding
    }
    for (int k = 0; k < STRIP_SIZE; k += BURST_SIZE) {
      int offset = j * W + i + k;
      if (j < 0) {
        call_acc(edge_buf1, 0, BURST_SIZE);
      } else if (j >= 0 && k == 0 && i == 0) { // left edge
        memset(edge_buf1, 0, PAD);
        memcpy(edge_buf1 + PAD, input + offset, STRIP_SIZE);
        call_acc(edge_buf1, edge_buf2, BURST_SIZE);
        memcpy(output + offset - PIPE_LATENCY, edge_buf2, PIPE_LATENCY);
        memcpy(output + offset, edge_buf2 + PIPE_LATENCY + PAD, BURST_SIZE - PIPE_LATENCY - PAD);
      } else {
        call_acc(input + offset, output + offset - PIPE_LATENCY, min(W - i, BURST_SIZE);
      }
      if (k + BURST_SIZE >= STRIP_SIZE) {
        call_acc(input + offset /* garbage */, output + offset, PIPE_LATENCY);
      }
    }
  }
}
