/*
 * Copyright (c) 2008-2009 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "loadsurface_yuv.h"

static int scale_2dimage(unsigned char *src_img, int src_imgw, int src_imgh,
                  unsigned char *dst_img, int dst_imgw, int dst_imgh)
{
    int row=0, col=0;

    for (row=0; row<dst_imgh; row++) {
        for (col=0; col<dst_imgw; col++) {
            *(dst_img + row * dst_imgw + col) = *(src_img + (row * src_imgh/dst_imgh) * src_imgw + col * src_imgw/dst_imgw);
        }
    }

    return 0;
}


static int YUV_blend_with_pic(int width, int height,
                  unsigned char *Y_start, int Y_pitch,
		  unsigned char *U_start, int U_pitch,
                  unsigned char *V_start, int V_pitch,
		  int UV_interleave, int fixed_alpha)
{
    /* PIC YUV format */
    unsigned char *pic_y_old = yuvga_pic;
    unsigned char *pic_u_old = pic_y_old + 640*480;
    unsigned char *pic_v_old = pic_u_old + 640*480/4;
    unsigned char *pic_y, *pic_u, *pic_v;

    int alpha_values[] = {100,90,80,70,60,50,40,30,20,30,40,50,60,70,80,90};
    
    static int alpha_idx = 0;
    int alpha;
    int allocated = 0;
    
    int row, col;

    if (fixed_alpha == 0) {
        alpha = alpha_values[alpha_idx % 16 ];
        alpha_idx ++;
    } else
        alpha = fixed_alpha;

    //alpha = 0;
    
    pic_y = pic_y_old;
    pic_u = pic_u_old;
    pic_v = pic_v_old;
    
    if (width != 640 || height != 480) { /* need to scale the pic */
        pic_y = (unsigned char *)malloc(width * height);
        pic_u = (unsigned char *)malloc(width * height/4);
        pic_v = (unsigned char *)malloc(width * height/4);

        allocated = 1;
        
        scale_2dimage(pic_y_old, 640, 480,
                      pic_y, width, height);
        scale_2dimage(pic_u_old, 320, 240,
                      pic_u, width/2, height/2);
        scale_2dimage(pic_v_old, 320, 240,
                      pic_v, width/2, height/2);
    }

    /* begin blend */

    /* Y plane */
    for (row=0; row<height; row++) 
        for (col=0; col<width; col++) {
            unsigned char *p = Y_start + row * Y_pitch + col;
            unsigned char *q = pic_y + row * width + col;
            
            *p  = *p * (100 - alpha) / 100 + *q * alpha/100;
        }

    if (UV_interleave == 0) {
        for (row=0; row<height/2; row++) 
            for (col=0; col<width/2; col++) {
                unsigned char *p = U_start + row * U_pitch + col;
                unsigned char *q = pic_u + row * width/2 + col;
            
                *p  = *p * (100 - alpha) / 100 + *q * alpha/100;
            }
    
        for (row=0; row<height/2; row++) 
            for (col=0; col<width/2; col++) {
                unsigned char *p = V_start + row * V_pitch + col;
                unsigned char *q = pic_v + row * width/2 + col;
            
                *p  = *p * (100 - alpha) / 100 + *q * alpha/100;
            }
    }  else { /* NV12 */
        for (row=0; row<height/2; row++) 
            for (col=0; col<width/2; col++) {
                unsigned char *pU = U_start + row * U_pitch + col*2;
                unsigned char *qU = pic_u + row * width/2 + col;
                unsigned char *pV = pU + 1;
                unsigned char *qV = pic_v + row * width/2 + col;
            
                *pU  = *pU * (100 - alpha) / 100 + *qU * alpha/100;
                *pV  = *pV * (100 - alpha) / 100 + *qV * alpha/100;
            }
    }
        
    
    if (allocated) {
        free(pic_y);
        free(pic_u);
        free(pic_v);
    }
    
    return 0;
}

static int yuvgen_planar(int width, int height,
                         unsigned char *Y_start, int Y_pitch,
                         unsigned char *U_start, int U_pitch,
                         unsigned char *V_start, int V_pitch,
                         int UV_interleave, int box_width, int row_shift,
                         int field)
{
    int row, alpha;

    /* copy Y plane */
    for (row=0;row<height;row++) {
        unsigned char *Y_row = Y_start + row * Y_pitch;
        int jj, xpos, ypos;

        ypos = (row / box_width) & 0x1;

        /* fill garbage data into the other field */
        if (((field == VA_TOP_FIELD) && (row &1))
            || ((field == VA_BOTTOM_FIELD) && ((row &1)==0))) { 
            memset(Y_row, 0xff, width);
            continue;
        }
        
        for (jj=0; jj<width; jj++) {
            xpos = ((row_shift + jj) / box_width) & 0x1;
                        
            if ((xpos == 0) && (ypos == 0))
                Y_row[jj] = 0xeb;
            if ((xpos == 1) && (ypos == 1))
                Y_row[jj] = 0xeb;
                        
            if ((xpos == 1) && (ypos == 0))
                Y_row[jj] = 0x10;
            if ((xpos == 0) && (ypos == 1))
                Y_row[jj] = 0x10;
        }
    }
  
    /* copy UV data */
    for( row =0; row < height/2; row++) {
        unsigned short value = 0x80;

        /* fill garbage data into the other field */
        if (((field == VA_TOP_FIELD) && (row &1))
            || ((field == VA_BOTTOM_FIELD) && ((row &1)==0))) {
            value = 0xff;
        }

        if (UV_interleave) {
            unsigned short *UV_row = (unsigned short *)(U_start + row * U_pitch);

            memset(UV_row, value, width);
        } else {
            unsigned char *U_row = U_start + row * U_pitch;
            unsigned char *V_row = V_start + row * V_pitch;
            
            memset (U_row,value,width/2);
            memset (V_row,value,width/2);
        }
    }

    if (getenv("AUTO_NOUV"))
        return 0;

    if (getenv("AUTO_ALPHA"))
        alpha = 0;
    else
        alpha = 70;
    
    YUV_blend_with_pic(width,height,
                       Y_start, Y_pitch,
                       U_start, U_pitch,
                       V_start, V_pitch,
                       UV_interleave, alpha);
    
    return 0;
}

static int upload_surface(VADisplay va_dpy, VASurfaceID surface_id,
                          int box_width, int row_shift,
                          int field)
{
    VAImage surface_image;
    void *surface_p=NULL, *U_start,*V_start;
    VAStatus va_status;
    
    va_status = vaDeriveImage(va_dpy,surface_id,&surface_image);
    CHECK_VASTATUS(va_status,"vaDeriveImage");

    vaMapBuffer(va_dpy,surface_image.buf,&surface_p);
    assert(VA_STATUS_SUCCESS == va_status);
        
    U_start = (char *)surface_p + surface_image.offsets[1];
    V_start = (char *)surface_p + surface_image.offsets[2];

    /* assume surface is planar format */
    yuvgen_planar(surface_image.width, surface_image.height,
                  (unsigned char *)surface_p, surface_image.pitches[0],
                  (unsigned char *)U_start, surface_image.pitches[1],
                  (unsigned char *)V_start, surface_image.pitches[2],
                  (surface_image.format.fourcc==VA_FOURCC_NV12),
                  box_width, row_shift, field);
        
    vaUnmapBuffer(va_dpy,surface_image.buf);

    vaDestroyImage(va_dpy,surface_image.image_id);

    return 0;
}
