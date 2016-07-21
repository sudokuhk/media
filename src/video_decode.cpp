#include "video_decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PIC_WIDTH   (1920)
#define MAX_PIC_HEIGHT  (1080)

CVideoDecoder::CVideoDecoder()
    : m_pCb(NULL)
    , m_tCodecId(AV_CODEC_ID_NONE)
    , m_pAVPacket(NULL)
    , m_pAVFrame(NULL)
    , m_pAVCodec(NULL)
    , m_pAVContext(NULL)
    , m_pYUV(NULL)
    , m_bOpen(false)
{
}

CVideoDecoder::~CVideoDecoder()
{
    close();
}

void CVideoDecoder::setCallback(IVideoDecodeCallback * cb)
{
    m_pCb = cb;
}

bool CVideoDecoder::open(AVCodecID codec_id_)
{
    //avcodec_register_all, has static variable inside. just execute once.
    avcodec_register_all();
    
    m_tCodecId      = codec_id_;

    int yuvsize     = (MAX_PIC_WIDTH * MAX_PIC_HEIGHT) * 3 / 2;
    m_pYUV          = (uint8_t *)malloc(yuvsize);
    if (m_pYUV == NULL) {
        goto failed;
    }
    
    m_pAVPacket     = av_packet_alloc();
    if (m_pAVPacket == NULL) {
        goto failed;
    }
    
    m_pAVFrame      = av_frame_alloc();
    if (m_pAVFrame == NULL) {
        goto failed;
    }
    
    m_pAVCodec      = avcodec_find_decoder(m_tCodecId);
    if (m_pAVCodec == NULL) {
        goto failed;
    }
    
    m_pAVContext    = avcodec_alloc_context3(m_pAVCodec);
    if (m_pAVContext == NULL) {
        goto failed;
    }

    if (avcodec_open2(m_pAVContext, m_pAVCodec, NULL) < 0) {
        goto failed;
    }
    m_bOpen  = true;
    return m_bOpen;

failed:
    close();
    return false;
}

void CVideoDecoder::close()
{
    if (m_bOpen) {
        m_bOpen = false;
        avcodec_close(m_pAVContext);
    }

    if (m_pAVContext != NULL) {
        avcodec_free_context(&m_pAVContext);
        m_pAVContext = NULL;
    }

    m_pAVCodec  = NULL;
    
    if (m_pAVFrame != NULL) {
        av_frame_free(&m_pAVFrame);
        m_pAVFrame = NULL;
    }

    if (m_pAVPacket != NULL) {
        av_packet_free(&m_pAVPacket);
        m_pAVPacket = NULL;
    }

    if (m_pYUV != NULL) {
        free(m_pYUV);
        m_pYUV = NULL;
    }
    m_bOpen = false;
    m_pCb   = NULL;
}

int  CVideoDecoder::decode(uint8_t * pdata, int size, void * arg)
{
    int gotpic = 0;
    m_pAVPacket->size = size;
    m_pAVPacket->data = pdata;

    int decodelen = avcodec_decode_video2(m_pAVContext, m_pAVFrame, 
        &gotpic, m_pAVPacket);
    
    if (decodelen > 0) {
        int width       = m_pAVContext->width;
        int height      = m_pAVContext->height;
        int yuvsize     = (width * height * 3) >> 1;

        cvt2yuv(width, height, m_pAVFrame, m_pYUV);

        if (m_pCb != NULL) {
            m_pCb->onVideoDecodeCallback(width, height, m_pYUV, yuvsize, arg);
        }
    }
    return decodelen;
}

void CVideoDecoder::cvt2yuv(int width, int height, AVFrame * frame, uint8_t * yuv)
{
    int yplanesize  = width * height;
    int vheight     = height >> 1;
    int vwidth      = width >> 1;
    int uvplanesize = yplanesize >> 2;
    
    int cpylen, off;
    uint8_t * woff, * roff;
    
    for (int h = 0; h < vheight; h++) {
        
        off     = h;
        woff    = yuv + width * off;
        roff    = frame->data[0] + frame->linesize[0] * off;
        cpylen  = width;
        memcpy(woff, roff, cpylen);
        
        off     = h + vheight;
        woff    = yuv + width * off;
        roff    = frame->data[0] + frame->linesize[0] * off;
        cpylen  = width;
        memcpy(woff, roff, cpylen);
        
        off     = h;
        woff    = yuv + yplanesize + vwidth * off;
        roff    = frame->data[1] + frame->linesize[1] * off;
        cpylen  = vwidth;
        memcpy(woff, roff, cpylen);
        
        off     = h;
        woff    = yuv + yplanesize + uvplanesize + vwidth * off;
        roff    = frame->data[2] + frame->linesize[2] * off;
        cpylen  = vwidth;
        memcpy(woff, roff, cpylen);
    }
}