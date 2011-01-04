#ifndef __NAL_H__
#define __NAL_H__

#define NAL_BUF_SIZE  65536  // maximum NAL unit size
#define RING_BUF_SIZE  8192  // input ring buffer size, MUST be a power of two!

typedef struct _nal_unit {
  int NumBytesInNALunit;
  int forbidden_zero_bit;
  int nal_ref_idc;
  int nal_unit_type;
  unsigned char *last_rbsp_byte;
} nal_unit;
 typedef struct _slice_header {
  int first_mb_in_slice;
} slice_header;
 
static int get_next_nal_unit(nal_unit *nalu); 
static int get_unsigned_exp_golomb();
static void decode_slice_header(slice_header *sh);
static int input_open(char *filename);
static void input_read(unsigned char *dest, int size);
static int input_get_bits(int bit_count);
int va_FoolGetFrame(char *filename, int maxframe, char *frame_buf); 
#endif /*__NAL_H__*/
