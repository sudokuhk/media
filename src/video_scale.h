#ifndef __VIDEO_SCALE_H__
#define __VIDEO_SCALE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libswscale/swscale.h"

#ifdef __cplusplus
};
#endif

class IVideoScaleCallback
{
public:
    virtual ~IVideoScaleCallback(){}
    virtual void onVideoScaleCallback(bool issuc, uint8_t * out, 
        int outlen, void * arg) = 0;
};

class CVideoScale
{
public:
    CVideoScale();
    ~CVideoScale();

    void setCallback(IVideoScaleCallback * cb);
    
    bool open(int srcW, int srcH, enum AVPixelFormat srcFormat,
            int dstW, int dstH, enum AVPixelFormat dstFormat);
    
    void close();
    
    int  scale(uint8_t * in, int inlen, void * arg);

    bool isOpen() { return m_bOpen; }

private:
    IVideoScaleCallback * m_pCb;
    
    int             m_nSrcW;
    int             m_nSrcH;
    int             m_nDstW;
    int             m_nDstH;
    int             m_nSwsW;
    int             m_nSwsH;
    int             m_nSwsY;
    int             m_nSwsUV;
    AVPixelFormat   m_eSrcF;
    AVPixelFormat   m_eDstF;

    bool            m_bOpen;

    uint8_t *       m_pOut;
    
    SwsContext *    m_pContext;
};

#endif  //__VIDEO_SCALE_H__