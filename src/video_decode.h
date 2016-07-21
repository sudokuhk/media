#ifndef __VIDEO_DECODE_H__
#define __VIDEO_DECODE_H__

#include <stdint.h>

extern "C"
{
#include "libavcodec/avcodec.h"
};

class IVideoDecodeCallback
{
public:
    virtual ~IVideoDecodeCallback() {}
    virtual void onVideoDecodeCallback(int width, int height, 
        uint8_t * pdata, int size, void * arg) = 0;
};

class CVideoDecoder
{
public:
    CVideoDecoder();
    ~CVideoDecoder();

    void setCallback(IVideoDecodeCallback * cb);
    bool open(AVCodecID codec_id_);
    void close();
    int  decode(uint8_t * pdata, int size, void * arg);

    bool isOpen() { return m_bOpen; }
private:
    void cvt2yuv(int width, int height, AVFrame * frame, uint8_t * yuv);
    
private:
    IVideoDecodeCallback * m_pCb;
    
    AVCodecID           m_tCodecId ;
    AVPacket *          m_pAVPacket;
    AVFrame *           m_pAVFrame;
    AVCodec *           m_pAVCodec;
    AVCodecContext *    m_pAVContext;

    uint8_t *           m_pYUV;
    
    bool                m_bOpen;
};

#endif  //__VIDEO_DECODE_H__
