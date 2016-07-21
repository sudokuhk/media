#ifndef __VIDEO_ENCODE_H__
#define __VIDEO_ENCODE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "x264/x264.h"

#ifdef __cplusplus
};
#endif

class IVideoEncodeCallback
{
public:
    virtual ~IVideoEncodeCallback(){}
    virtual void onVideoEncodeCallback(bool issuc, uint8_t * data, 
        int len, bool iframe, void * arg) = 0;
};

class CVideoEncoder
{
public:
    CVideoEncoder();
    ~CVideoEncoder();

    void setCallback(IVideoEncodeCallback * callback);
    bool open(int width, int height, int fps, int bitrate);
    void close();
    int  encode(uint8_t * yuvdata, int datasize, void * arg);
    void forceIframe();

    bool isOpen() { return m_bOpen; }
private:
    void init(x264_param_t * x264_param, int width, int height, 
        int fps, int bitrate);
    void destroy();
private:
    IVideoEncodeCallback * m_pCallback;
    
    int     m_nWidth;
    int     m_nHeight;
    int     m_nFps;
    int     m_nBitRate;
    int     m_nFrameCnt;
    
    bool    m_bPicAlloc;
    bool    m_bForceIframe;
    bool    m_bOpen;

    x264_picture_t *    m_pIn;
    x264_t *            m_pHandle;
    x264_param_t *      m_pParam;
};

#endif  //__VIDEO_ENCODER_H__
