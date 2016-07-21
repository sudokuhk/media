#include "video_encode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CVideoEncoder::CVideoEncoder()
    : m_pCallback(NULL)
    , m_nWidth(0)
    , m_nHeight(0)
    , m_nFps(0)
    , m_nBitRate(0)
    , m_nFrameCnt(0)
    , m_bPicAlloc(false)
    , m_bForceIframe(false)
    , m_bOpen(false)
    , m_pIn(NULL)
    , m_pHandle(NULL)
    , m_pParam(NULL)
{
}

CVideoEncoder::~CVideoEncoder()
{
    destroy();
}

void CVideoEncoder::setCallback(IVideoEncodeCallback * callback)
{
    m_pCallback = callback;
}

bool CVideoEncoder::open(int width, int height, int fps, int bitrate)
{
    m_nWidth    = width;
    m_nHeight   = height;
    m_nFps      = fps;
    m_nBitRate  = bitrate;

    bool succ = false;

    m_pParam = (x264_param_t *)malloc(sizeof(x264_param_t));
    if (m_pParam == NULL) {
        goto failed;
    }

    init(m_pParam, width, height, fps, bitrate);

    m_pIn = (x264_picture_t*)malloc(sizeof(x264_picture_t));
    if (m_pIn == NULL) {
        goto failed;
    }
    
    if(x264_picture_alloc(m_pIn, X264_CSP_I420, width, height) < 0) {
	    goto failed;
	}
    m_pIn->img.i_csp    = X264_CSP_I420;  
	m_pIn->img.i_plane  = 3; 
    m_bPicAlloc         = true;

    m_pHandle = x264_encoder_open(m_pParam);
    if (m_pHandle == NULL) {
		goto failed;
	}
    succ    = true;
    m_bOpen = true;
    
    return succ;
    
failed:    
    destroy();
    return succ;
}

void CVideoEncoder::close()
{
    destroy();
}

int  CVideoEncoder::encode(uint8_t * yuvdata, int datasize, void * arg)
{
    int yplanesize  = m_nWidth * m_nHeight;
    int needsize    = (yplanesize * 3) >> 1;
    int uvplanesize = yplanesize >> 2;

    //protected, avoid overflow.
    if (datasize < needsize) {
        return -1;
    }
    
    memcpy(m_pIn->img.plane[0], yuvdata, yplanesize);
    memcpy(m_pIn->img.plane[1], yuvdata + yplanesize, uvplanesize);
    memcpy(m_pIn->img.plane[2], yuvdata + yplanesize + uvplanesize, uvplanesize);

#if 1
    m_pIn->i_type       = X264_TYPE_AUTO;
    if (m_bForceIframe) {
        m_bForceIframe = false;
        m_pIn->i_type   = X264_TYPE_IDR;
    }
    
    m_pIn->i_qpplus1    = 0;
    m_pIn->i_pts        = m_nFrameCnt ++;
    m_pIn->param        = NULL;
#endif

    int num_nals = 0;
    int encodesize = 0;
    x264_nal_t * xnals = NULL;
    x264_picture_t x264oxpic_;
    memset(&x264oxpic_, 0 , sizeof(x264_picture_t));

    if ((encodesize = x264_encoder_encode(m_pHandle, &xnals, &num_nals, 
        m_pIn, &x264oxpic_)) > 0) {

        bool isIFrame = false;
        if (IS_X264_TYPE_I(x264oxpic_.i_type)) {
            isIFrame = true;
        }

        if (m_pCallback != NULL) {
            uint8_t * pdata = xnals->p_payload;
            m_pCallback->onVideoEncodeCallback(true, pdata, encodesize, isIFrame, arg);
        }
    }
    
    return encodesize;
}

void CVideoEncoder::forceIframe()
{
    m_bForceIframe = true;
}

void CVideoEncoder::init(x264_param_t * x264_param, int width, int height, 
    int fps, int bitrate)
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

	x264_param_apply_profile(x264_param, "baseline");
}

void CVideoEncoder::destroy()
{
    if (m_pHandle != NULL) {
        x264_encoder_close(m_pHandle);
        m_pHandle = NULL;
    }

    if (m_pIn != NULL) {
        if (m_bPicAlloc) {
    	    x264_picture_clean(m_pIn);
            m_bPicAlloc = false;
        }
        free(m_pIn);
        m_pIn = NULL;
    }

    if (m_pParam != NULL) {
	    free(m_pParam);
        m_pParam = NULL;
    }
    m_bOpen     = false;
    m_pCallback = NULL;
}

