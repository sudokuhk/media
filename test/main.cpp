/*************************************************************************
    > File Name: main.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Monday, July 18, 2016 PM02:02:52 CST
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "video_encode.h"
#include "video_decode.h"
#include "video_scale.h"

void writefile(const char * filename, const uint8_t * pdata, int dsize);

CVideoDecoder * decoder;
CVideoEncoder * encoder;
CVideoScale   * scaler;

char scale_filename[100];

class Callback 
    : public IVideoEncodeCallback
    , public IVideoDecodeCallback
    , public IVideoScaleCallback
{
public:    
    virtual void onVideoEncodeCallback(bool issuc, uint8_t * data, 
        int len, bool iframe, void * arg)
    {
        printf("encode:suc:%d, len:%d, iframe:%d!\n", issuc, len, iframe);
        writefile("encode.bin", data, len);

        decoder->decode(data, len, arg);
    }

    virtual void onVideoDecodeCallback(int width, int height, 
        uint8_t * pdata, int size, void * arg)
    {
        printf("decode, width:%d, height:%d, size:%d!\n",
            width, height, size);
        writefile("deocde.bin", pdata, size);
    }

    virtual void onVideoScaleCallback(bool issuc, uint8_t * out, 
        int outlen, void * arg)
    {
        printf("scale done, suc:%d, outlen:%d\n", issuc, outlen);
        writefile(scale_filename, out, outlen);
        encoder->encode(out, outlen, arg);
    }
};

bool getYUV(uint8_t * yuvbuf, int yuvsize, const char * yuvfile)
{
    const char * file = yuvfile;
    if (file == NULL) {
        return false;
    }
    
    struct stat sb;
	if (stat(file, &sb) == -1) {
	    printf("stat file:%s failed!\n", file);
	    return false;
	}
	
	if ((sb.st_mode & S_IFMT) != S_IFREG || sb.st_size <  yuvsize) {
	    printf("file:%s invalid, size:%ld, regfile:%d!\n", 
            file, sb.st_size, (sb.st_mode & S_IFMT) == S_IFREG);
	    return false;
	}
	
	int rfd = open(file, O_RDONLY);
	if (rfd < 0) {
	    printf("open file:%s failed!\n", file);
	    return false;
	}
	
	int off = 0, rdn = 0;
	if ((rdn = read(rfd, yuvbuf, yuvsize - off)) != yuvsize) {
        printf("read no enough data. readn:%d, yuvsize:%d\n", rdn, yuvsize);
        return false;
	} 
    printf("read data from %s, len:%d!\n", file, rdn);
	close(rfd);
    return true;
}

void writefile(const char * filename, const uint8_t * pdata, int dsize)
{
    const char * outfile = filename;
	int wfd = open(outfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
	if (wfd < 0) {
	    printf("open outfile:%s failed!\n", outfile);
	    exit(0);
	}
	write(wfd, pdata, dsize);
	close(wfd);
}

int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("less arguments.\n");
        exit(0);
    }
    
    Callback cb;
    int width = 640, height = 480, fps = 15, bitrate = 24;
    int yuvsize = width * height * 3 / 2;
    int dw = 480, dh = 360;
    //int dw = 480, dh = 320;
    //int dw = 480, dh = 400;

    snprintf(scale_filename, 100, "scale_%dx%d.bin", dw, dh);

    encoder = new CVideoEncoder();
    decoder = new CVideoDecoder();
    scaler  = new CVideoScale();

    encoder->setCallback(&cb);
    decoder->setCallback(&cb);
    scaler->setCallback(&cb);

    scaler->open(width, height, AV_PIX_FMT_YUV420P, dw, dh, AV_PIX_FMT_YUV420P);
    encoder->open(dw, dh, fps, bitrate);
    decoder->open(AV_CODEC_ID_H264);

    uint8_t * yuv = (uint8_t *)malloc(yuvsize);
    if (!getYUV(yuv, yuvsize, argv[1])) {
        printf("get yuv data failed!\n");
        exit(0);
    }

    scaler->scale(yuv, yuvsize, NULL);
    //encoder->encode(yuv, yuvsize);

    free(yuv);
    delete encoder;
    delete decoder;
    delete scaler;
    
    return 0;
}
