#include <string.h>
#include <stdio.h>
#include "va_getframe.h"

static unsigned char nal_buf[NAL_BUF_SIZE];
static unsigned char ring_buf[RING_BUF_SIZE];
static int input_remain = 0;
static int ring_pos = 0;
static int nal_pos;
static int nal_bit;
extern FILE *input_fd;
static int frame_no = 0, cur_frame_no = 0;

#define SLICE_NUM 4 
#define RING_MOD  ((RING_BUF_SIZE)-1)
#define HALF_RING ((RING_BUF_SIZE)/2)

#define gnn_advance() do { \
	ring_pos = (ring_pos+1)&RING_MOD; \
	--input_remain; \
	if(ring_pos==0) input_read(&ring_buf[HALF_RING],HALF_RING); \
	if(ring_pos==HALF_RING) input_read(&ring_buf[0],HALF_RING); \
} while(0)

#define gnn_add_segment(end) do { \
	int size = end-segment_start; \
	if(size>0) { \
		memcpy(&nal_buf[nalu_size],&ring_buf[segment_start],size); \
		nalu_size += size; \
	} \
	segment_start = end&RING_MOD; \
} while(0)

static int input_get_bits(int bit_count) 
{
	int res = 0;
	register unsigned int x = 
		(nal_buf[nal_pos]<<24)|
		(nal_buf[nal_pos+1]<<16)|
		(nal_buf[nal_pos+2]<<8)|
		nal_buf[nal_pos+3];
	res = (x>>(32-bit_count-nal_bit))&((1<<bit_count)-1);
	nal_bit += bit_count;
	nal_pos += nal_bit>>3;
	nal_bit &= 7;
	return res;
}

int input_get_one_bit() 
{
	int res = (nal_buf[nal_pos]>>(7-nal_bit))&1;
	if(++nal_bit>7) 
	{
		++nal_pos;
		nal_bit = 0;
	}
	return res;
}

int get_unsigned_exp_golomb() 
{
	int exp;
	for(exp = 0; !input_get_one_bit(); ++exp);
	if(exp) return (1<<exp)-1+input_get_bits(exp);
	else return 0;
}

void decode_slice_header(slice_header *sh ) 
{
	memset((void*)sh,0,sizeof(slice_header));
	sh->first_mb_in_slice = get_unsigned_exp_golomb(); 
}

static void input_read(unsigned char *dest, int size) 
{
	int count = fread(dest,1,size,input_fd);
	input_remain += count;
}

static int get_next_nal_unit(nal_unit *nalu) 
{
    int i,segment_start;
    int nalu_size = 0;
    int NumBytesInRbsp = 0;

	// search for the next NALU start
	// here is the sync that the start of the NALU is 0x00000001
	for(;;) 
	{
		if(input_remain<= 4) 
		{
            //clip restart
			memset(ring_buf,0,sizeof(char)*RING_BUF_SIZE);
			memset(nal_buf,0,sizeof(char)*NAL_BUF_SIZE);

			fseek(input_fd,0,SEEK_SET);
			input_remain = 0;
			input_read(ring_buf,RING_BUF_SIZE);
			ring_pos = 0;
			return 1;
		}
		if((!ring_buf[ring_pos]) &&
				(!ring_buf[(ring_pos+1)&RING_MOD]) &&
				(!ring_buf[(ring_pos+2)&RING_MOD]) &&
				( ring_buf[(ring_pos+3)&RING_MOD]==1))
			break;
		gnn_advance();
	}
	for(i=0;i<4;++i) gnn_advance();

	// add bytes to the NALU until the end is found
	segment_start = ring_pos;
	while(input_remain) 
	{
		if((!ring_buf[ring_pos]) &&
				(!ring_buf[(ring_pos+1)&RING_MOD]) &&
				(!ring_buf[(ring_pos+2)&RING_MOD]))
			break;
		ring_pos = (ring_pos+1)&RING_MOD;
		--input_remain;
		if(ring_pos==0) 
		{
			gnn_add_segment(RING_BUF_SIZE);
			input_read(&ring_buf[HALF_RING],HALF_RING);
		}
		if(ring_pos==HALF_RING)
		{
			gnn_add_segment(HALF_RING);
			input_read(&ring_buf[0],HALF_RING);
		}
	}

	gnn_add_segment(ring_pos);
	if(!nalu_size) 
		fclose(input_fd);

	// read the NAL unit
	nal_pos = 0; nal_bit = 0;
	nalu->forbidden_zero_bit = input_get_bits(1);
	nalu->nal_ref_idc = input_get_bits(2);
	nalu->nal_unit_type = input_get_bits(5);
	nalu->last_rbsp_byte = &nal_buf[nalu_size-1];
	nalu->NumBytesInNALunit = nalu_size; 
	return 1;
}

int va_FoolGetFrame(char *frame_buf) 
{
	int i = 0, frame_pos = 0;
	static slice_header sh; 
	static nal_unit nalu;

    //save the current frame number
	cur_frame_no = frame_no;
    //read the clip , here is the first frame, let the clip go on frame by frame 
	if(!frame_no)
		input_read(ring_buf,RING_BUF_SIZE);

	while(get_next_nal_unit(&nalu))
	{
		if(nalu.nal_unit_type == 1 || nalu.nal_unit_type == 5) 
		{
			decode_slice_header(&sh);
			if(0 == sh.first_mb_in_slice)
			{
				++frame_no;
				frame_pos = 0;
			}
			if(frame_no>(cur_frame_no+1))
				break;
			memcpy(frame_buf+frame_pos, nal_buf+1, sizeof(char)*(nalu.NumBytesInNALunit-1));
			frame_pos += nalu.NumBytesInNALunit;
		}
	}
    //close the env_value clip ;if some other area use this clip need to open it again
	fclose(input_fd);
	return 1; 
}
