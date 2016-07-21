#include "video_scale.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CVideoScale::CVideoScale()
    : m_pCb(NULL)
    , m_nSrcW(0)
    , m_nSrcH(0)
    , m_nDstW(0)
    , m_nDstH(0)
    , m_nSwsW(0)
    , m_nSwsH(0)
    , m_nSwsY(0)
    , m_nSwsUV(0)
    , m_eSrcF(AV_PIX_FMT_NONE)
    , m_eDstF(AV_PIX_FMT_NONE)
    , m_bOpen(false)
    , m_pOut(NULL)
    , m_pContext(NULL)
{
}

CVideoScale::~CVideoScale()
{
    close();
}

void CVideoScale::setCallback(IVideoScaleCallback * cb)
{
    m_pCb = cb;
}

bool CVideoScale::open(int srcW, int srcH, enum AVPixelFormat srcFormat,
            int dstW, int dstH, enum AVPixelFormat dstFormat)
{
    m_nSrcW = srcW;
    m_nSrcH = srcH;
    m_nDstW = dstW;
    m_nDstH = dstH;
    m_eSrcF = srcFormat;
    m_eDstF = dstFormat;

    float ratio = (float)dstH / dstW;
    m_nSwsW = m_nSrcW;
    m_nSwsH = m_nSrcW * ratio;
    if (m_nSwsH > m_nSrcH) {
        m_nSwsH = m_nSrcH;
        m_nSwsW = m_nSrcH / ratio;
    }

    m_nSwsW -= m_nSwsW & 0x01;
    m_nSwsH -= m_nSwsH & 0x01;

    int woff    = m_nSrcW - m_nSwsW;
    int hoff    = m_nSrcH - m_nSwsH;

    woff    -= woff & 0x03;
    hoff    -= hoff & 0x03;

    m_nSwsY     = (woff >> 1) + (m_nSrcW * (hoff >> 1));
    m_nSwsUV    = (woff >> 2) + (m_nSrcW * (hoff >> 3));

    //printf("src[%d, %d], dst[%d, %d], sws[%d, %d], y:%d, uv:%d\n",
    //    srcW, srcH, dstW, dstH, m_nSwsW, m_nSwsH, m_nSwsY, m_nSwsUV);
    
    m_pContext = sws_getContext(m_nSwsW, m_nSwsH, m_eSrcF,
        m_nDstW, m_nDstH, m_eDstF, SWS_BICUBIC, NULL, NULL, NULL);

    if (m_pContext == NULL) {
        goto failed;
    }

    m_pOut  = (uint8_t *)malloc(m_nDstW * m_nDstH * 3 / 2);
    if (m_pOut == NULL) {
        goto failed;
    }

    m_bOpen = true;
    return true;

failed:
    close();
    return false;
}

void CVideoScale::close()
{
    if (m_pContext != NULL) {
        sws_freeContext(m_pContext);
        m_pContext = NULL;
    }

    if (m_pOut != NULL) {
        free(m_pOut);
        m_pOut = NULL;
    }
    m_bOpen = false;
    m_pCb   = NULL;
}

int  CVideoScale::scale(uint8_t * in, int inlen, void * arg)
{
    if (m_pContext == NULL) {
        return -1;
    }

    int syplanesize     = m_nSrcW * m_nSrcH;
    int suvplanesize    = syplanesize >> 2;
    
    int dyplanesize     = m_nDstW * m_nDstH;
    int duvplanesize    = dyplanesize >> 2;
    
    uint8_t * sslice[4] = { 
        in + m_nSwsY, 
        in + syplanesize + m_nSwsUV, 
        in + syplanesize + suvplanesize + m_nSwsUV, 
        NULL,
    };
    
    int sstride[4]  = {
        m_nSrcW, 
        m_nSrcW >> 1, 
        m_nSrcW >> 1, 
        0,
    };

    uint8_t * dslice[4] = {
        m_pOut,
        m_pOut + dyplanesize, 
        m_pOut + dyplanesize + duvplanesize, 
        NULL,
    };
    
    int dstride[4]  = {
        m_nDstW, 
        m_nDstW >> 1, 
        m_nDstW >> 1, 
        0,
    };

    int sslicey = 0;
    int ssliceh = m_nSwsH;

    //printf("slicey = %d, sliceh = %d\n", sslicey, ssliceh);

    int dheight = sws_scale(m_pContext, sslice, sstride, sslicey, ssliceh,
                            dslice, dstride);

    if (m_pCb != NULL) {
        m_pCb->onVideoScaleCallback(dheight == m_nDstH,
            m_pOut, dyplanesize + (duvplanesize << 1), arg);
    }
    
    return dheight;
}
