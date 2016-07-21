/*************************************************************************
  > File Name: test.cpp
  > Author: sudoku.huang
  > Mail: sudoku.huang@gmail.com 
  > Created Time: Thursday, July 14, 2016 PM03:13:18 CST
 ************************************************************************/
//g++ test.cpp -I./inc -L./lib -lx264 -pthread -ldl
//g++ decode.cpp -I./inc -L./lib -pthread -lavcodec -lx264 -lavutil  -lz -o decode -ldl
//g++ decode.cpp -I./inc -L./lib -pthread -lavcodec -lavutil -lz -o decode -ldl -lrt

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
#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
};

void printb(const uint8_t * pb, int len)
{
    for (int i = 0; i < len; i++)
        printf("%02x ", pb[i]);
    printf("\n");
}

int main(int argc, char * argv[])
{
    avcodec_register_all();
    
    AVCodecID codec_id_ = AV_CODEC_ID_H264;
    AVPacket * packet_  = av_packet_alloc();
    AVFrame * frame_    = av_frame_alloc();
    AVCodec * codec_    = avcodec_find_decoder(codec_id_);
    AVCodecContext * ctx_ = avcodec_alloc_context3(codec_);
    
    avcodec_open2(ctx_, codec_, NULL);
    
    uint8_t * h264 = NULL;
    if (argc < 2) {
        printf("please input h264 file!\n");
        exit(0);
    }
    const char * file = argv[1];
    
    struct stat sb;
	if (stat(file, &sb) == -1) {
	    printf("stat file:%s failed!\n", file);
	    exit(0);
	}
	
	if ((sb.st_mode & S_IFMT) != S_IFREG) {
	    printf("file:%s invalid, size:%ld, regfile:%d!\n", 
	        file, sb.st_size, (sb.st_mode & S_IFMT) == S_IFREG);
	    exit(0);
	}
	h264 = (uint8_t *)malloc(sb.st_size);
	
	int rfd = open(file, O_RDONLY);
	if (rfd < 0) {
	    printf("open file:%s failed!\n", file);
	    exit(0);
	}
	
	int rdn = 0;
    if ((rdn = read(rfd, h264, sb.st_size)) != sb.st_size) {
        printf("read failed, not read all data into buffer!, read:%d, file:%ld\n",
            rdn, sb.st_size);
        exit(0);
    }
    close(rfd);
    
    int gotpic = 0;
    packet_->size = sb.st_size;
    packet_->data = h264;
    int decodelen = avcodec_decode_video2(ctx_, frame_, &gotpic, packet_);
    if (decodelen < 0) {
        printf("decode failed, ret:%d\n", decodelen);
        exit(0);
    }
    
    int width = ctx_->width;
    int height = ctx_->height;
    printf("width:%d, height:%d, linesize:%d!\n",
        width, height, frame_->linesize[0]);
    
    int yuvsize = (width * height * 3) >> 1;
    int planesize = (width * height);
    
    printf("decodelen:%d, yuvsize:%d\n", decodelen, yuvsize);
    
    uint8_t * yuv = (uint8_t *)malloc(yuvsize);
    
    int vheight = height >> 1;
    int vwidth  = width >> 1;
    int vplanesize   = planesize >> 2;
    
    int cpylen, off;
    uint8_t * woff, * roff;
    
    for (int h = 0; h < vheight; h++) {
        
        off     = h;
        woff    = yuv + width * off;
        roff    = frame_->data[0] + frame_->linesize[0] * off;
        cpylen  = width;
        memcpy(woff, roff, cpylen);
        
        off     = h + vheight;
        woff    = yuv + width * off;
        roff    = frame_->data[0] + frame_->linesize[0] * off;
        cpylen  = width;
        memcpy(woff, roff, cpylen);
        
        off     = h;
        woff    = yuv + planesize + vwidth * off;
        roff    = frame_->data[1] + frame_->linesize[1] * off;
        cpylen  = vwidth;
        memcpy(woff, roff, cpylen);
        
        off     = h;
        woff    = yuv + planesize + vplanesize + vwidth * off;
        roff    = frame_->data[2] + frame_->linesize[2] * off;
        cpylen  = vwidth;
        memcpy(woff, roff, cpylen);
    }
    
    const char * yuvfile = "yuv.bin";
	int wfd = open(yuvfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
	if (wfd < 0) {
	    printf("open outfile:%s failed!\n", yuvfile);
	    exit(0);
	}
	write(wfd, yuv, yuvsize);
	close(wfd);
	
	return 0;
}
