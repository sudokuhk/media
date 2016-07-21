/*************************************************************************
  > File Name: test.cpp
  > Author: sudoku.huang
  > Mail: sudoku.huang@gmail.com 
  > Created Time: Thursday, July 14, 2016 PM03:13:18 CST
 ************************************************************************/
//g++ test.cpp -I./inc -L./lib -lx264 -pthread -ldl
//g++ encode.cpp -I./inc -L./lib -lx264 -pthread -ldl -o encode

#include <stdio.h>
#include <cassert>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

extern "C"
{
#include "x264/x264.h"
};

void printb(const uint8_t * pb, int len)
{
    for (int i = 0; i < len; i++)
        printf("%02x ", pb[i]);
    printf("\n");
}

int init_param(x264_param_t * x264_param, int width, int height, int fps, int bitrate)
{
    x264_param_default(x264_param);
    x264_param_default_preset(x264_param, "medium", NULL );
	x264_param_default_preset(x264_param, "veryfast", "zerolatency"); 
	
	//x264_param_default_preset(x264_param_, "veryfast", NULL); 
	//x264_param_default_preset(x264_param_,"superfast","zerolatency");
	// CPU flags 
    //x264_param_->i_threads  = X264_THREADS_AUTO; 
	x264_param->i_threads      = X264_SYNC_LOOKAHEAD_AUTO; 

	// Video Properties 
	x264_param->i_csp          = X264_CSP_I420;
	x264_param->i_width        = width; 
	x264_param->i_height       = height; 
	//x264_param_->i_frame_total = 0;  

	// Bitstream parameters
	//x264_param_->i_bframe  = 5;
	//x264_param_->b_open_gop  = 0;
	//x264_param_->i_bframe_pyramid = 0;
	//x264_param_->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
	//x264_param_->i_keyint_min = 0;
	//x264_param_->i_keyint_max = fps*10;

	// Rate control parameters
	//x264_param_->rc.i_qp_min = 0;
	//x264_param_->rc.i_qp_max = 20;
	//x264_param_->rc.i_qp_constant = 0;
	//x264_param_->rc.i_bitrate = 1024 * 100;
	//x264_param_->rc.i_rc_method = X264_RC_CRF;
	//x264_param_->rc.f_rf_constant  = 22.0+float(10-5);

	// muxing parameters
	x264_param->b_annexb           = 1;
	x264_param->b_repeat_headers   = 1;
	//x264_param_->i_fps_den  = 1; //* 帧率分母
	//x264_param_->i_fps_num  = 10;//* 帧率分子
	//x264_param_->i_timebase_den = x264_param_->i_fps_num;
	//x264_param_->i_timebase_num = x264_param_->i_fps_den;
	//x264_param_->b_vfr_input = 0;
	
    
	// OBS
	x264_param->i_fps_num      = fps;
	x264_param->i_keyint_max   = fps * 2;
	//x264_param_->b_deterministic = false;
	//x264_param_->b_vfr_input = 0;
	//x264_param_->i_fps_num =fps;//数值由客户端传下来，一般是15fps
	//x264_param_->i_timebase_num = 1;
	//x264_param_->i_timebase_den = 1000;

	//x264_param_->rc.i_qp_min = 16;
	//x264_param_->rc.i_qp_max = 20;
	//x264_param_->rc.i_qp_constant = 16;
	x264_param->rc.i_rc_method    = X264_RC_CRF;
	x264_param->rc.f_rf_constant  = (float)bitrate;

	x264_param_apply_profile(x264_param, "main");
}

int init_head(x264_t * x264_handle, char * header, int hlen)
{
    x264_nal_t * nal_;
    int nal_size = 0;
    
    x264_encoder_headers(x264_handle, &nal_, &nal_size);
    
    for (int i = 0; i < nal_size; i++) {
        x264_nal_t & nal = nal_[i];
        switch (nal.i_type) {
            case NAL_SPS:
                printb(nal.p_payload, nal.i_payload);
                break;
            case NAL_PPS:
                printb(nal.p_payload, nal.i_payload);
                break;
        }
    }
}

int main(int argc, char * argv[])
{
    int fps     = 15;
    int bitRate = 24;
    int width   = 640;
    int height  = 480;
    int yuvsize = (width * height * 3) >> 1;
    uint8_t * yuv = (uint8_t *)malloc(yuvsize);
    
    x264_picture_t *_picIn;
	x264_picture_t *_picOut;
    x264_t * x264_handle_        = NULL;
    x264_param_t * x264_param_   = (x264_param_t *)malloc(sizeof(x264_param_t));
    
    init_param(x264_param_, width, height, fps, bitRate);  
    
	_picIn = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	_picOut = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	
	if(x264_picture_alloc(_picIn, X264_CSP_I420, width, height) < 0 ) {
	    printf("x264_picture_alloc failed!\n");
	    exit(0);
	}
	
	_picIn->img.i_csp   = X264_CSP_I420;  
	_picIn->img.i_plane = 3; 

	x264_handle_ = x264_encoder_open(x264_param_);
	if(!x264_handle_) {
		printf("x264_encoder_open failed!\n");
		exit(0);
	}
	init_head(x264_handle_, NULL, 0);
	
	if (argc < 2) {
	    printf("please input yuv file name!\n");
	    exit(0);
	}
	const char * file = argv[1];
	printf("encode yuv file:%s\n", file);
	
	struct stat sb;
	if (stat(file, &sb) == -1) {
	    printf("stat file:%s failed!\n", file);
	    exit(0);
	}
	
	if ((sb.st_mode & S_IFMT) != S_IFREG || sb.st_size <  yuvsize) {
	    printf("file:%s invalid, size:%ld, regfile:%d!\n", file, sb.st_size, (sb.st_mode & S_IFMT) == S_IFREG);
	    exit(0);
	}
	
	int rfd = open(file, O_RDONLY);
	if (rfd < 0) {
	    printf("open file:%s failed!\n", file);
	    exit(0);
	}
	
	int off = 0, rdn = 0;
	if ((rdn = read(rfd, yuv, yuvsize - off)) != yuvsize) {
        printf("read no enough data. readn:%d, yuvsize:%d\n", rdn, yuvsize);
	} 
	close(rfd);
	
	int planesize = width * height;
	memcpy(_picIn->img.plane[0], yuv, planesize);
	memcpy(_picIn->img.plane[1], yuv + planesize, planesize >> 2);
	memcpy(_picIn->img.plane[2], yuv + planesize + (planesize >> 2), planesize >> 2);
	
	x264_nal_t * nal;
	int nalcnt = 0;
	
	int encodesize = x264_encoder_encode(x264_handle_, &nal, &nalcnt, _picIn, _picOut);
	if (encodesize <= 0) {
	    printf("encode failed. size:%d\n", encodesize);
	    exit(0);
	}
	
	const char * outfile = "h264_640x480.bin";
	int wfd = open(outfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
	if (wfd < 0) {
	    printf("open outfile:%s failed!\n", outfile);
	    exit(0);
	}
	write(wfd, nal->p_payload, encodesize);
	close(wfd);
	
	x264_encoder_close(x264_handle_);
	x264_picture_clean(_picIn);
	free(_picIn);
	free(_picOut);
	free(yuv);
	free(x264_param_);
	
	return 0;
}