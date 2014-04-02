/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_H

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>
#include <linux/version.h>
#include <linux/videodev2.h> 
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <utils/threads.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBuffer.h>
#include <system/window.h>
#include <camera/Camera.h>
#include <hardware/camera.h>
#include <camera/CameraParameters.h>
#include "Semaphore.h"
#if defined(TARGET_RK29)
#include <linux/android_pmem.h>
#endif
#include "MessageQueue.h"
#include "../jpeghw/release/encode_release/hw_jpegenc.h"
#include "../jpeghw/release/encode_release/rk29-ipp.h"
#include "CameraHal_Module.h"
#include "common_type.h"

/* 
*NOTE: 
*       CONFIG_CAMERA_INVALIDATE_RGA is debug macro, 
*    CONFIG_CAMERA_INVALIDATE_RGA must equal to 0 in official version.     
*/
#define CONFIG_CAMERA_INVALIDATE_RGA    0


#if defined(TARGET_RK30) && (defined(TARGET_BOARD_PLATFORM_RK30XX) || (defined(TARGET_BOARD_PLATFORM_RK2928)))
#include "../libgralloc_ump/gralloc_priv.h"
#if (CONFIG_CAMERA_INVALIDATE_RGA==0)
#include <hardware/rga.h>
#endif
#elif defined(TARGET_RK30) && defined(TARGET_BOARD_PLATFORM_RK30XXB)
#include <hardware/hal_public.h>
#include <hardware/rga.h>
#elif defined(TARGET_RK29)
#include "../libgralloc/gralloc_priv.h"
#endif


#include "CameraHal_Mem.h"

extern "C" int getCallingPid();
extern "C" int cameraPixFmt2HalPixFmt(const char *fmt);
extern "C" void arm_nv12torgb565(int width, int height, char *src, short int *dst,int dstbuf_w);
extern "C" int rga_nv12torgb565(int src_width, int src_height, char *src, short int *dst, 
                                int dstbuf_width,int dst_width,int dst_height);
extern "C" int rk_camera_yuv_scale_crop_ipp(int v4l2_fmt_src, int v4l2_fmt_dst, 
	            int srcbuf, int dstbuf,int src_w, int src_h,int dst_w, int dst_h,bool rotation_180);
extern "C"  int YData_Mirror_Line(int v4l2_fmt_src, int *psrc, int *pdst, int w);
extern "C"  int UVData_Mirror_Line(int v4l2_fmt_src, int *psrc, int *pdst, int w);
extern "C"  int YuvData_Mirror_Flip(int v4l2_fmt_src, char *pdata, char *pline_tmp, int w, int h);
extern "C" int YUV420_rotate(const unsigned char* srcy, int src_stride,  unsigned char* srcuv,
                   unsigned char* dsty, int dst_stride, unsigned char* dstuv,
                   int width, int height,int rotate_angle);
extern "C"  int arm_camera_yuv420_scale_arm(int v4l2_fmt_src, int v4l2_fmt_dst, 
									char *srcbuf, char *dstbuf,int src_w, int src_h,int dst_w, int dst_h,bool mirror);
extern "C" char* getCallingProcess();

extern "C" void arm_yuyv_to_nv12(int src_w, int src_h,char *srcbuf, char *dstbuf);

extern "C" int cameraFormatConvert(int v4l2_fmt_src, int v4l2_fmt_dst, const char *android_fmt_dst, 
							char *srcbuf, char *dstbuf,int srcphy,int dstphy,int src_size,
							int src_w, int src_h, int srcbuf_w,
							int dst_w, int dst_h, int dstbuf_w,
							bool mirror);


extern rk_cam_info_t gCamInfos[CAMERAS_SUPPORT_MAX];

namespace android {

#define CONFIG_CAMERAHAL_VERSION KERNEL_VERSION(0, 4, 0x1f) 

/*  */
#define CAMERA_DISPLAY_FORMAT_YUV420P   CameraParameters::PIXEL_FORMAT_YUV420P
#define CAMERA_DISPLAY_FORMAT_YUV420SP   CameraParameters::PIXEL_FORMAT_YUV420SP
#define CAMERA_DISPLAY_FORMAT_RGB565     CameraParameters::PIXEL_FORMAT_RGB565
#define CAMERA_DISPLAY_FORMAT_NV12       "nv12"
/* 
*NOTE: 
*       CONFIG_CAMERA_DISPLAY_FORCE and CONFIG_CAMERA_DISPLAY_FORCE_FORMAT is debug macro, 
*    CONFIG_CAMERA_DISPLAY_FORCE must equal to 0 in official version.     
*/
#define CONFIG_CAMERA_DISPLAY_FORCE     1
#define CONFIG_CAMERA_DISPLAY_FORCE_FORMAT CAMERA_DISPLAY_FORMAT_RGB565

#define CONFIG_CAMERA_SINGLE_SENSOR_FORCE_BACK_FOR_CTS   0
#define CONFIG_CAMERA_FRAME_DV_PROC_STAT    0
#define CONFIG_CAMERA_FRONT_MIRROR_MDATACB  1
#define CONFIG_CAMERA_FRONT_MIRROR_MDATACB_ALL  0
#define CONFIG_CAMERA_FRONT_MIRROR_MDATACB_APK  "<com.skype.raider>,<com.yahoo.mobile.client.andro>"

#define CONFIG_CAMERA_PREVIEW_BUF_CNT 4
#define CONFIG_CAMERA_DISPLAY_BUF_CNT		4
#define CONFIG_CAMERA_VIDEO_BUF_CNT 4
#define CONFIG_CAMERA_VIDEOENC_BUF_CNT		4

#define CONFIG_CAMERA_UVC_INVAL_FRAMECNT    5
#define CONFIG_CAMERA_ORIENTATION_SKYPE     0
#define CONFIG_CAMERA_FRONT_ORIENTATION_SKYPE     0
#define CONFIG_CAMERA_BACK_ORIENTATION_SKYPE      0

#define CONFIG_CAMERA_FRONT_PREVIEW_FPS_MIN    3000        // 3fps
#define CONFIG_CAMERA_FRONT_PREVIEW_FPS_MAX    40000        //30fps
#define CONFIG_CAMERA_BACK_PREVIEW_FPS_MIN     3000        
#define CONFIG_CAMERA_BACK_PREVIEW_FPS_MAX     40000

#define CAMERAHAL_VERSION_PROPERTY_KEY       "sys_graphic.cam_hal.ver"
#define CAMERADRIVER_VERSION_PROPERTY_KEY    "sys_graphic.cam_driver.ver"
#define CAMERA_PMEM_NAME                     "/dev/pmem_cam"
#define CAMERA_DRIVER_SUPPORT_FORMAT_MAX   32

#define RAW_BUFFER_SIZE_8M         (( mCamDriverPreviewFmt == V4L2_PIX_FMT_RGB565) ? 0xF40000:0xB70000)
#define RAW_BUFFER_SIZE_5M         (( mCamDriverPreviewFmt == V4L2_PIX_FMT_RGB565) ? 0x9A0000:0x740000)
#define RAW_BUFFER_SIZE_3M          (( mCamDriverPreviewFmt == V4L2_PIX_FMT_RGB565) ?0x600000 :0x480000)
#define RAW_BUFFER_SIZE_2M          (( mCamDriverPreviewFmt == V4L2_PIX_FMT_RGB565) ?0x3A0000 :0x2c0000)
#define RAW_BUFFER_SIZE_1M          (( mCamDriverPreviewFmt == V4L2_PIX_FMT_RGB565)? 0x180000 :0x1d0000)
#define RAW_BUFFER_SIZE_0M3         (( mCamDriverPreviewFmt == V4L2_PIX_FMT_RGB565)?0x150000 :0x100000)

#define JPEG_BUFFER_SIZE_8M          0x700000
#define JPEG_BUFFER_SIZE_5M          0x400000
#define JPEG_BUFFER_SIZE_3M          0x300000
#define JPEG_BUFFER_SIZE_2M          0x300000
#define JPEG_BUFFER_SIZE_1M          0x200000
#define JPEG_BUFFER_SIZE_0M3         0x100000

#define OPTIMIZE_MEMORY_USE
#define VIDEO_ENC_BUFFER            0x151800  


#define V4L2_BUFFER_MAX             32
#define V4L2_BUFFER_MMAP_MAX        16
#define PAGE_ALIGN(x)   (((x) + 0xFFF) & (~0xFFF)) // Set as multiple of 4K

#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                             GRALLOC_USAGE_HW_RENDER | \
                             GRALLOC_USAGE_SW_WRITE_OFTEN | \
                             GRALLOC_USAGE_SW_READ_OFTEN /*| \
                             GRALLOC_USAGE_SW_WRITE_MASK| \
                             GRALLOC_USAGE_SW_READ_RARELY*/ 
#define CAMERA_IPP_NAME                  "/dev/rk29-ipp"
#ifdef ALOGD
#define LOGD      ALOGD
#endif
#ifdef ALOGV
#define LOGV      ALOGV
#endif
#ifdef ALOGE
#define LOGE      ALOGE
#endif
#ifdef ALOGI
#define LOGI      ALOGI
#endif

#if defined(TARGET_BOARD_PLATFORM_RK30XX) || defined(TARGET_RK29) || defined(TARGET_BOARD_PLATFORM_RK2928)                         
    #define NATIVE_HANDLE_TYPE             private_handle_t
    #define PRIVATE_HANDLE_GET_W(hd)       (hd->width)    
    #define PRIVATE_HANDLE_GET_H(hd)       (hd->height)    
#elif defined(TARGET_BOARD_PLATFORM_RK30XXB)               
    #define NATIVE_HANDLE_TYPE             IMG_native_handle_t
    #define PRIVATE_HANDLE_GET_W(hd)       (hd->iWidth)    
    #define PRIVATE_HANDLE_GET_H(hd)       (hd->iHeight)    
#endif

#define RK_VIDEOBUF_CODE_CHK(rk_code)		((rk_code&(('R'<<24)|('K'<<16)))==(('R'<<24)|('K'<<16)))



class FrameProvider
{
public:
   virtual int returnFrame(int index,int cmd)=0;
   virtual ~FrameProvider(){};
   FrameProvider(){};
   FramInfo_s mPreviewFrameInfos[CONFIG_CAMERA_PREVIEW_BUF_CNT];
};

typedef struct rk_buffer_info {
    Mutex* lock;
    int phy_addr;
    int vir_addr;
    int buf_state;
} rk_buffer_info_t;

class BufferProvider{
public:
    int createBuffer(int count,int perbufsize,buffer_type_enum buftype);
    int freeBuffer();
    virtual int setBufferStatus(int bufindex,int status,int cmd=0);
    virtual int getOneAvailableBuffer(int *buf_phy,int *buf_vir);
    int getBufferStatus(int bufindex);
    int getBufCount();
    int getBufPhyAddr(int bufindex);
    int getBufVirAddr(int bufindex);
    int flushBuffer(int bufindex);
    BufferProvider(MemManagerBase* memManager):mCamBuffer(memManager),mBufInfo(NULL){}
    virtual ~BufferProvider(){mCamBuffer = NULL;mBufInfo = NULL;}
protected:
    rk_buffer_info_t* mBufInfo;
    int mBufCount;
    buffer_type_enum mBufType;
    MemManagerBase* mCamBuffer;
};

//preview buffer ����
class PreviewBufferProvider:public BufferProvider
{
public:

    enum PREVIEWBUFSTATUS {
        CMD_PREVIEWBUF_DISPING = 0x01,
        CMD_PREVIEWBUF_VIDEO_ENCING = 0x02,
        CMD_PREVIEWBUF_SNAPSHOT_ENCING = 0x04,
        CMD_PREVIEWBUF_DATACB = 0x08,
        CMD_PREVIEWBUF_WRITING = 0x10,
    };
    
#define CAMERA_PREVIEWBUF_ALLOW_DISPLAY(a) ((a&CMD_PREVIEWBUF_WRITING)==0x00)
#define CAMERA_PREVIEWBUF_ALLOW_ENC(a) ((a&CMD_PREVIEWBUF_WRITING)==0x00)
#define CAMERA_PREVIEWBUF_ALLOW_ENC_PICTURE(a) ((a&CMD_PREVIEWBUF_WRITING)==0x00)
#define CAMERA_PREVIEWBUF_ALLOW_DATA_CB(a)  ((a&CMD_PREVIEWBUF_WRITING)==0x00)
#define CAMERA_PREVIEWBUF_ALLOW_WRITE(a)   ((a&(CMD_PREVIEWBUF_DISPING|CMD_PREVIEWBUF_VIDEO_ENCING|CMD_PREVIEWBUF_SNAPSHOT_ENCING|CMD_PREVIEWBUF_DATACB|CMD_PREVIEWBUF_WRITING))==0x00)

    virtual int setBufferStatus(int bufindex,int status,int cmd);
    
    PreviewBufferProvider(MemManagerBase* memManager):BufferProvider(memManager){}
    ~PreviewBufferProvider(){}
};


class DisplayAdapter;
class AppMsgNotifier;
//diplay buffer ��display adapter�����й�����

/*************************
CameraAdapter ����������ͨ�ţ���Ϊ֡���ݵ��ṩ�ߣ�Ϊdisplay��msgcallback�ṩ���ݡ�
***************************/
class CameraAdapter:public FrameProvider
{
public:
    CameraAdapter(int cameraId);
    virtual ~CameraAdapter();

    DisplayAdapter* getDisplayAdapterRef(){return mRefDisplayAdapter;}
    void setDisplayAdapterRef(DisplayAdapter& refDisplayAdap);
    void setEventNotifierRef(AppMsgNotifier& refEventNotify);
    void setPreviewBufProvider(BufferProvider* bufprovider);
    CameraParameters & getParameters();
    virtual int getCurPreviewState(int *drv_w,int *drv_h);
    int getCameraFd();
   int initialize();
    
    virtual status_t startPreview(int preview_w,int preview_h,int w, int h, int fmt,bool is_capture);
    virtual status_t stopPreview();
   // virtual int initialize() = 0;
    virtual int returnFrame(int index,int cmd);
    virtual int setParameters(const CameraParameters &params_set);
    virtual void initDefaultParameters(int camFd);
    virtual status_t autoFocus();

    virtual void dump();
protected:
    //talk to driver
    virtual int cameraCreate(int cameraId);
    virtual int cameraDestroy();
    
    virtual int cameraSetSize(int w, int h, int fmt, bool is_capture); 
    virtual int cameraStream(bool on);
    virtual int cameraStart();
    virtual int cameraStop();
    //dqbuf
    virtual int getFrame(FramInfo_s** frame); 
    virtual int cameraAutoFocus(bool auto_trig_only);

    //qbuf
 //   virtual status_t fillThisBuffer();

    //define  the frame info ,such as w, h ,fmt ,dealflag(preview callback ? display ? video enc ? picture?)
    virtual void reprocessFrame(FramInfo_s* frame);
    virtual int adapterReturnFrame(int index,int cmd);

private:
    //���߳����ڼ̳�����?
    class CameraPreviewThread :public Thread
    {
        //deque ��֡�������Ҫ�ַ���DisplayAdapter�༰EventNotifier�ࡣ
        CameraAdapter* mPreivewCameraAdapter;
    public:
        CameraPreviewThread(CameraAdapter* adapter)
            : Thread(false), mPreivewCameraAdapter(adapter) { }

        virtual bool threadLoop() {
            mPreivewCameraAdapter->previewThread();

            return false;
        }
    };

    class AutoFocusThread : public Thread {
        CameraAdapter* mFocusCameraAdapter;
    public:
        AutoFocusThread(CameraAdapter* hw)
            : Thread(false), mFocusCameraAdapter(hw) { }

        virtual bool threadLoop() {
            mFocusCameraAdapter->autofocusThread();

            return false;
        }
    };

    void autofocusThread();
    sp<AutoFocusThread>  mAutoFocusThread;



protected:    
    virtual void previewThread();
protected:
    DisplayAdapter* mRefDisplayAdapter;
    AppMsgNotifier* mRefEventNotifier;
    sp<CameraPreviewThread> mCameraPreviewThread;
    int mPreviewRunning;
	int mPictureRunning;
    BufferProvider* mPreviewBufProvider;
    int mCamDrvWidth;
    int mCamDrvHeight;
    int mCamPreviewH ;
    int mCamPreviewW ;

    unsigned int mCamDriverSupportFmt[CAMERA_DRIVER_SUPPORT_FORMAT_MAX];
    enum v4l2_memory mCamDriverV4l2MemType;

    unsigned int mCamDriverPreviewFmt;
    struct v4l2_capability mCamDriverCapability;

    int mPreviewErrorFrameCount;
    int mPreviewFrameIndex;

    Mutex mCamDriverStreamLock;
    bool mCamDriverStream;

    bool camera_device_error;

    CameraParameters mParameters;
    unsigned int CameraHal_SupportFmt[6];

    char *mCamDriverV4l2Buffer[V4L2_BUFFER_MAX];
    unsigned int mCamDriverV4l2BufferLen;

    int mCamFd;
    int mCamId;

    Mutex mAutoFocusLock;
    Condition mAutoFocusCond;
    bool mExitAutoFocusThread; 
};

//soc camera adapter
class CameraSOCAdapter: public CameraAdapter
{
public:
    CameraSOCAdapter(int cameraId);
    virtual ~CameraSOCAdapter();

    /*********************
    talk to driver 
    **********************/
    //parameters
    virtual int setParameters(const CameraParameters &params_set);
    virtual void initDefaultParameters();
    virtual int cameraAutoFocus(bool auto_trig_only);
    
private:    
    int cameraFramerateQuery(unsigned int format, unsigned int w, unsigned int h, int *min, int *max);
    int cameraFpsInfoSet(CameraParameters &params);
    int cameraConfig(const CameraParameters &tmpparams,bool isInit);


    int mCamDriverFrmWidthMax;
    int mCamDriverFrmHeightMax;
    String8 mSupportPreviewSizeReally;
 
    struct v4l2_querymenu mWhiteBalance_menu[20];
    int mWhiteBalance_number;
    
    struct v4l2_querymenu mEffect_menu[20];
    int mEffect_number;
    
    struct v4l2_querymenu mScene_menu[20];
    int mScene_number;
    
    struct v4l2_querymenu mAntibanding_menu[20];
    int mAntibanding_number;
    
    int mZoomMin;
    int mZoomMax;
    int mZoomStep;
    
    struct v4l2_querymenu mFlashMode_menu[20];
    int mFlashMode_number;

	//TAE & TAF
	__u32 m_focus_mode;
	static const __u32 focus_fixed = 0xFFFFFFFF;
	__u32 m_focus_value;
	__s32 m_taf_roi[4];

	void GetAFParameters(const char* focusmode = NULL);
	int AndroidZoneMapping(
			const char* tag,
			__s32 pre_w,
			__s32 pre_h,
			__s32 drv_w,
			__s32 drv_h,
			bool bPre2Drv,
			__s32 *zone);

};

//usb camera adapter

class CameraUSBAdapter: public CameraAdapter
{
public:
    CameraUSBAdapter(int cameraId):CameraAdapter(cameraId){};
    virtual ~CameraUSBAdapter(){};
};
/*************************
DisplayAdapter Ϊ֡���������ߣ���CameraAdapter����֡���ݲ���ʾ
***************************/
class DisplayAdapter
{
    class DisplayThread : public Thread {
        DisplayAdapter* mDisplayAdapter;        
    public:
        DisplayThread(DisplayAdapter* disadap)
            : Thread(false), mDisplayAdapter(disadap){}

        virtual bool threadLoop() {
            mDisplayAdapter->displayThread();

            return false;
        }
    };

public:
    enum DisplayThreadCommands {
		// Comands
		CMD_DISPLAY_PAUSE,        
        CMD_DISPLAY_START,
        CMD_DISPLAY_STOP,
        CMD_DISPLAY_FRAME,
        CMD_DISPLAY_INVAL
    };

    enum DISPLAY_STATUS{
        STA_DISPLAY_RUNNING,
        STA_DISPLAY_PAUSE,
        STA_DISPLAY_STOP,
    };
	enum DisplayCommandStatus{
		CMD_DISPLAY_START_PREPARE = 1,
		CMD_DISPLAY_START_DONE,
		CMD_DISPLAY_PAUSE_PREPARE,
		CMD_DISPLAY_PAUSE_DONE,
		CMD_DISPLAY_STOP_PREPARE,
		CMD_DISPLAY_STOP_DONE,
		CMD_DISPLAY_FRAME_PREPARE,
		CMD_DISPLAY_FRAME_DONE,
	};
	
    typedef struct rk_displaybuf_info {
        Mutex* lock;
        buffer_handle_t* buffer_hnd;
        NATIVE_HANDLE_TYPE *priv_hnd;
        int phy_addr;
        int vir_addr;
        int buf_state;
        int stride;
    } rk_displaybuf_info_t;

    bool isNeedSendToDisplay();
    void notifyNewFrame(FramInfo_s* frame);
    
    int startDisplay(int width, int height);
    int stopDisplay();
    int pauseDisplay();

    int setPreviewWindow(struct preview_stream_ops* window);
    int getDisplayStatus(void);
    void setFrameProvider(FrameProvider* framePro);
    void dump();

    DisplayAdapter();
    ~DisplayAdapter();

    struct preview_stream_ops* getPreviewWindow();
private:
    int cameraDisplayBufferCreate(int width, int height, const char *fmt,int numBufs);
    int cameraDisplayBufferDestory(void);
    void displayThread();
    void setBufferState(int index,int status);
    void setDisplayState(int state);

    rk_displaybuf_info_t* mDisplayBufInfo;
    int mDislayBufNum;
    int mDisplayWidth;
    int mDisplayHeight;
    int mDisplayRuning;
    char mDisplayFormat[30];

    int mDispBufUndqueueMin;
    preview_stream_ops_t* mANativeWindow;
    FrameProvider* mFrameProvider;

    Mutex mDisplayLock;
    Condition mDisplayCond;
	int mDisplayState;

    MessageQueue    displayThreadCommandQ;
    sp<DisplayThread> mDisplayThread;
};

//takepicture used
 typedef struct picture_info{
    int num;
    int w;
    int h;
    int fmt;
	int quality ;
	int rotation;
	int thumbquality;
	int thumbwidth;
	int thumbheight;

    //gps info
	int	altitude;
	int	latitude;
	int	longtitude;
	int	timestamp;    
	char	getMethod[33];//getMethod : len <= 32

	int  focalen;
	int  isovalue;
	int  flash;
	int  whiteBalance;

 }picture_info_s;


//picture encode used
struct CamCaptureInfo_s
{
    int input_phy_addr;
    int input_vir_addr;
    int output_phy_addr;
    int output_vir_addr;
    int output_buflen;
};
/**************************************
EventNotifier   ������msg �Ļص������ջ���¼ӰʱҲ��Ϊ֡���ݵ������ߡ�
**************************************/
class AppMsgNotifier
{
private:
	enum CameraStatus{
		CMD_EVENT_PREVIEW_DATA_CB_PREPARE 	= 0x01,
		CMD_EVENT_PREVIEW_DATA_CB_DONE 		= 0x02,	
		CMD_EVENT_PREVIEW_DATA_CB_MASK 		= 0x03,	

		CMD_EVENT_VIDEO_ENCING_PREPARE		= 0x04,
		CMD_EVENT_VIDEO_ENCING_DONE			= 0x08,	
		CMD_EVENT_VIDEO_ENCING_MASK			= 0x0C,	

		CMD_EVENT_PAUSE_PREPARE				= 0x10,
		CMD_EVENT_PAUSE_DONE				= 0x20,	
		CMD_EVENT_PAUSE_MASK				= 0x30,	

		CMD_EVENT_EXIT_PREPARE				= 0x40,
		CMD_EVENT_EXIT_DONE					= 0x80,
		CMD_EVENT_EXIT_MASK					= 0xC0,	

		CMD_ENCPROCESS_SNAPSHOT_PREPARE		= 0x100,
		CMD_ENCPROCESS_SNAPSHOT_DONE		= 0x200,
		CMD_ENCPROCESS_SNAPSHOT_MASK		= 0x300,

		CMD_ENCPROCESS_PAUSE_PREPARE		= 0x400,
		CMD_ENCPROCESS_PAUSE_DONE			= 0x800,
		CMD_ENCPROCESS_PAUSE_MASK			= 0xC00,

		CMD_ENCPROCESS_EXIT_PREPARE			= 0x1000,
		CMD_ENCPROCESS_EXIT_DONE			= 0x2000,
		CMD_ENCPROCESS_EXIT_MASK			= 0x3000,

		STA_RECORD_RUNNING				= 0x4000,
		STA_RECEIVE_PIC_FRAME				= 0x8000,
	};
	
    //����preview data cb��video enc
    class CameraAppMsgThread :public Thread
    {
    public:
        enum EVENT_THREAD_CMD{
            CMD_EVENT_PREVIEW_DATA_CB,
            CMD_EVENT_VIDEO_ENCING,
            CMD_EVENT_PAUSE,
            CMD_EVENT_EXIT
        };
	protected:
		AppMsgNotifier* mAppMsgNotifier;
	public:
		CameraAppMsgThread(AppMsgNotifier* hw)
			: Thread(false),mAppMsgNotifier(hw) { }
	
		virtual bool threadLoop() {
			mAppMsgNotifier->eventThread();
	
			return false;
		}
    };

    //����picture
	class EncProcessThread : public Thread {
	public:
	    enum ENC_THREAD_CMD{
            CMD_ENCPROCESS_SNAPSHOT,
            CMD_ENCPROCESS_PAUSE,
            CMD_ENCPROCESS_EXIT,
	    };
	protected:
		AppMsgNotifier* mAppMsgNotifier;
	public:
		EncProcessThread(AppMsgNotifier* hw)
			: Thread(false),mAppMsgNotifier(hw) { }
	
		virtual bool threadLoop() {
			mAppMsgNotifier->encProcessThread();
	
			return false;
		}
	};

	friend class EncProcessThread;
public:
    AppMsgNotifier();
    ~AppMsgNotifier();

    void setPictureRawBufProvider(BufferProvider* bufprovider);
    void setPictureJpegBufProvider(BufferProvider* bufprovider);
    
    int takePicture(picture_info_s picinfo);
    int flushPicture();
    
    void setVideoBufProvider(BufferProvider* bufprovider);
    int startRecording(int w,int h);
    int stopRecording();
    void releaseRecordingFrame(const void *opaque);
    int msgEnabled(int32_t msg_type);
    void notifyCbMsg(int msg,int ret);
    
    bool isNeedSendToVideo();
    bool isNeedSendToPicture();
    bool isNeedSendToDataCB();
    bool isNeedToDisableCaptureMsg();
    void notifyNewPicFrame(FramInfo_s* frame);
    void notifyNewPreviewCbFrame(FramInfo_s* frame);
    void notifyNewVideoFrame(FramInfo_s* frame);
    int enableMsgType(int32_t msgtype);
    int disableMsgType(int32_t msgtype);
    void setCallbacks(camera_notify_callback notify_cb,
            camera_data_callback data_cb,
            camera_data_timestamp_callback data_cb_timestamp,
            camera_request_memory get_memory,
            void *user);
    void setFrameProvider(FrameProvider * framepro);
    int  setPreviewDataCbRes(int w,int h, const char *fmt);
    
    void stopReceiveFrame();
    void dump();
private:

   void encProcessThread();
   void eventThread();

	int captureEncProcessPicture(FramInfo_s* frame);
    int processPreviewDataCb(FramInfo_s* frame);
    int processVideoCb(FramInfo_s* frame);
    
    int copyAndSendRawImage(void *raw_image, int size);
    int copyAndSendCompressedImage(void *compressed_image, int size);
    int Jpegfillexifinfo(RkExifInfo *exifInfo,picture_info_s &params);
    int Jpegfillgpsinfo(RkGPSInfo *gpsInfo,picture_info_s &params);
    
    int32_t mMsgTypeEnabled;
    
    picture_info_s mPictureInfo;
   // bool mReceivePictureFrame;
    int mEncPictureNum;
    Mutex mPictureLock;

    Mutex mRecordingLock;
//    bool mRecordingRunning;
    int mRecordW;
    int mRecordH;

    Mutex mDataCbLock;


    sp<CameraAppMsgThread> mCameraAppMsgThread;
    sp<EncProcessThread> mEncProcessThread;
    BufferProvider* mRawBufferProvider;
    BufferProvider* mJpegBufferProvider;
    BufferProvider* mVideoBufferProvider;
    FrameProvider* mFrameProvider;

    camera_notify_callback mNotifyCb;
    camera_data_callback mDataCb;    
    camera_data_timestamp_callback mDataCbTimestamp;
    camera_request_memory mRequestMemory;
    void  *mCallbackCookie;

    MessageQueue encProcessThreadCommandQ;
    MessageQueue eventThreadCommandQ;

    camera_memory_t* mVideoBufs[CONFIG_CAMERA_VIDEO_BUF_CNT];

    char mPreviewDataFmt[30];
    int mPreviewDataW;
    int mPreviewDataH;
	int mRunningState;
    
};


/***********************
CameraHal�ฺ����cameraservice��ϵ��ʵ��
cameraserviceҪ��ʵ�ֵĽӿڡ�����ֻ���𹫹���Դ�����룬�Լ�����ķַ���
***********************/
class CameraHal
{
public:  
/*--------------------Interface Methods---------------------------------*/
    /** Set the ANativeWindow to which preview frames are sent */
    int setPreviewWindow(struct preview_stream_ops *window);

    /** Set the notification and data callbacks */
    void setCallbacks(camera_notify_callback notify_cb,
            camera_data_callback data_cb,
            camera_data_timestamp_callback data_cb_timestamp,
            camera_request_memory get_memory,
            void *user);

    /**
     * The following three functions all take a msg_type, which is a bitmask of
     * the messages defined in include/ui/Camera.h
     */

    /**
     * Enable a message, or set of messages.
     */
    void enableMsgType(int32_t msg_type);

    /**
     * Disable a message, or a set of messages.
     *
     * Once received a call to disableMsgType(CAMERA_MSG_VIDEO_FRAME), camera
     * HAL should not rely on its client to call releaseRecordingFrame() to
     * release video recording frames sent out by the cameral HAL before and
     * after the disableMsgType(CAMERA_MSG_VIDEO_FRAME) call. Camera HAL
     * clients must not modify/access any video recording frame after calling
     * disableMsgType(CAMERA_MSG_VIDEO_FRAME).
     */
    void disableMsgType(int32_t msg_type);

    /**
     * Query whether a message, or a set of messages, is enabled.  Note that
     * this is operates as an AND, if any of the messages queried are off, this
     * will return false.
     */
    int msgTypeEnabled(int32_t msg_type);

    /**
     * Start preview mode.
     */
    int startPreview();

    /**
     * Stop a previously started preview.
     */
    void stopPreview();

    /**
     * Returns true if preview is enabled.
     */
    int previewEnabled();

    /**
     * Request the camera HAL to store meta data or real YUV data in the video
     * buffers sent out via CAMERA_MSG_VIDEO_FRAME for a recording session. If
     * it is not called, the default camera HAL behavior is to store real YUV
     * data in the video buffers.
     *
     * This method should be called before startRecording() in order to be
     * effective.
     *
     * If meta data is stored in the video buffers, it is up to the receiver of
     * the video buffers to interpret the contents and to find the actual frame
     * data with the help of the meta data in the buffer. How this is done is
     * outside of the scope of this method.
     *
     * Some camera HALs may not support storing meta data in the video buffers,
     * but all camera HALs should support storing real YUV data in the video
     * buffers. If the camera HAL does not support storing the meta data in the
     * video buffers when it is requested to do do, INVALID_OPERATION must be
     * returned. It is very useful for the camera HAL to pass meta data rather
     * than the actual frame data directly to the video encoder, since the
     * amount of the uncompressed frame data can be very large if video size is
     * large.
     *
     * @param enable if true to instruct the camera HAL to store
     *        meta data in the video buffers; false to instruct
     *        the camera HAL to store real YUV data in the video
     *        buffers.
     *
     * @return OK on success.
     */
    int storeMetaDataInBuffers(int enable);

    /**
     * Start record mode. When a record image is available, a
     * CAMERA_MSG_VIDEO_FRAME message is sent with the corresponding
     * frame. Every record frame must be released by a camera HAL client via
     * releaseRecordingFrame() before the client calls
     * disableMsgType(CAMERA_MSG_VIDEO_FRAME). After the client calls
     * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
     * responsibility to manage the life-cycle of the video recording frames,
     * and the client must not modify/access any video recording frames.
     */
    int startRecording();

    /**
     * Stop a previously started recording.
     */
    void stopRecording();

    /**
     * Returns true if recording is enabled.
     */
    int recordingEnabled();

    /**
     * Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
     *
     * It is camera HAL client's responsibility to release video recording
     * frames sent out by the camera HAL before the camera HAL receives a call
     * to disableMsgType(CAMERA_MSG_VIDEO_FRAME). After it receives the call to
     * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
     * responsibility to manage the life-cycle of the video recording frames.
     */
    void releaseRecordingFrame(const void *opaque);

    /**
     * Start auto focus, the notification callback routine is called with
     * CAMERA_MSG_FOCUS once when focusing is complete. autoFocus() will be
     * called again if another auto focus is needed.
     */
    int autoFocus();

    /**
     * Cancels auto-focus function. If the auto-focus is still in progress,
     * this function will cancel it. Whether the auto-focus is in progress or
     * not, this function will return the focus position to the default.  If
     * the camera does not support auto-focus, this is a no-op.
     */
    int cancelAutoFocus();

    /**
     * Take a picture.
     */
    int takePicture();

    /**
     * Cancel a picture that was started with takePicture. Calling this method
     * when no picture is being taken is a no-op.
     */
    int cancelPicture();

    /**
     * Set the camera parameters. This returns BAD_VALUE if any parameter is
     * invalid or not supported.
     */
    int setParameters(const char *parms);
    int setParameters(const CameraParameters &params_set);
	int setParametersUnlock(const CameraParameters &params_set);

    /** Retrieve the camera parameters.  The buffer returned by the camera HAL
        must be returned back to it with put_parameters, if put_parameters
        is not NULL.
     */
    char* getParameters();

    /** The camera HAL uses its own memory to pass us the parameters when we
        call get_parameters.  Use this function to return the memory back to
        the camera HAL, if put_parameters is not NULL.  If put_parameters
        is NULL, then you have to use free() to release the memory.
    */
    void putParameters(char *);

    /**
     * Send command to camera driver.
     */
    int sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);

    /**
     * Release the hardware resources owned by this object.  Note that this is
     * *not* done in the destructor.
     */
    void release();

    /**
     * Dump state of the camera hardware
     */
    int dump(int fd);
    void dump_mem(__u16 addr_s, __u16 len, __u8* content);
/*--------------------Internal Member functions - Public---------------------------------*/
    /** Constructor of CameraHal */
    CameraHal(int cameraId);

    // Destructor of CameraHal
    virtual ~CameraHal();    

    int getCameraFd();
	bool isNeedToDisableCaptureMsg();

public:
    CameraAdapter* mCameraAdapter;
    DisplayAdapter* mDisplayAdapter;
    AppMsgNotifier*   mEventNotifier;
    MemManagerBase* mMemoryManager;
    BufferProvider* mPreviewBuf;
    BufferProvider* mVideoBuf;
    BufferProvider* mRawBuf;
    BufferProvider* mJpegBuf;
    MemManagerBase* mCamMemManager;


private:
    int selectPreferedDrvSize(int *width,int * height,bool is_capture);
    int fillPicturInfo(picture_info_s& picinfo);
	void setCamStatus(int status, int type);
	int checkCamStatus(int cmd);
    enum CommandThreadCommands { 
		// Comands
        CMD_PREVIEW_START = 1,
        CMD_PREVIEW_STOP,
        CMD_PREVIEW_CAPTURE_CANCEL,
        CMD_CONTINUOS_PICTURE,
        
        CMD_AF_START,
        CMD_AF_CANCEL,

        CMD_SET_PREVIEW_WINDOW,
        CMD_SET_PARAMETERS,
        CMD_EXIT,

    };

    enum CommandThreadStatus{
       CMD_STATUS_RUN ,
       CMD_STATUS_STOP ,

    };
	
	enum CommandStatus{
		CMD_PREVIEW_START_PREPARE 			= 0x01,
		CMD_PREVIEW_START_DONE				= 0x02,	
		CMD_PREVIEW_START_MASK				= 0x11,		
		
		CMD_PREVIEW_STOP_PREPARE			= 0x04,
		CMD_PREVIEW_STOP_DONE				= 0x08,
		CMD_PREVIEW_STOP_MASK				= 0x0C,
		
		CMD_CONTINUOS_PICTURE_PREPARE		= 0x10,
		CMD_CONTINUOS_PICTURE_DONE			= 0x20,
		CMD_CONTINUOS_PICTURE_MASK			= 0x30,		
		
		CMD_PREVIEW_CAPTURE_CANCEL_PREPARE 	= 0x40,
		CMD_PREVIEW_CAPTURE_CANCEL_DONE 	= 0x80,
		CMD_PREVIEW_CAPTURE_CANCEL_MASK 	= 0xC0,		
		
		CMD_AF_START_PREPARE				= 0x100,
		CMD_AF_START_DONE					= 0x200,
		CMD_AF_START_MASK					= 0x300,		
		
		CMD_AF_CANCEL_PREPARE				= 0x400,
		CMD_AF_CANCEL_DONE					= 0x800,
		CMD_AF_CANCEL_MASK					= 0xC00,		
		
		CMD_SET_PREVIEW_WINDOW_PREPARE		= 0x1000,
		CMD_SET_PREVIEW_WINDOW_DONE			= 0x2000,
		CMD_SET_PREVIEW_WINDOW_MASK			= 0x3000,		
		
		CMD_SET_PARAMETERS_PREPARE			= 0x4000,
		CMD_SET_PARAMETERS_DONE				= 0x8000,
		CMD_SET_PARAMETERS_MASK				= 0xC000,		
		
		CMD_EXIT_PREPARE					= 0x10000,
		CMD_EXIT_DONE						= 0x20000,
		CMD_EXIT_MASK						= 0x30000,

		STA_RECORD_RUNNING					= 0x40000,
		STA_PREVIEW_CMD_RECEIVED			= 0x80000,
		
		STA_DISPLAY_RUNNING					= 0x100000,
        STA_DISPLAY_PAUSE					= 0x200000,
        STA_DISPLAY_STOP					= 0x400000,	
		STA_DISPLAY_MASK					= 0x700000,	       
	};

	class CommandThread : public Thread {
        CameraHal* mHardware;
    public:
        CommandThread(CameraHal* hw)
            : Thread(false), mHardware(hw) { }

        virtual bool threadLoop() {
            mHardware->commandThread();

            return false;
        }
    };

   friend class CommandThread;
   void commandThread();
   sp<CommandThread> mCommandThread;
   MessageQueue commandThreadCommandQ;
   int mCommandRunning;

   void updateParameters(CameraParameters & tmpPara);
   CameraParameters mParameters;


   mutable Mutex mLock;        // API lock -- all public methods
  // bool mRecordRunning;
  // bool mPreviewCmdReceived;
   int mCamFd;
   int mCameraStatus;
};

}

#endif
//CameraHardwareInterface �нӿ�ʵ��˵��
/*
enableMsgType  
���õ�EventNotifier��enableMsgType��
EventNotifier�и���msg��enable״̬�����Ƿ���Ҫ����CameraAdapter��������frame
*/

/*
startPreview
1��CameraHal������buffer��
2��CameraAdapter��set fmt��stream on
3��DisplayAdapter��enabledisplay

�������ļ�ZSL����������APP���ط�startpreview(�������һ����stopPreview����)����ʱ
��ֱ��enabledisplay��
*/

/*startRecording
*/

/*
autoFocus
1��CameraHal ���յ�focus��֪ͨCameraAdapter
2��CameraAdapter ֪ͨ EventNotifier
3��EventNotifier �ص�(���߳��лص�)CameraAdapter��focus����ʵ��
*/

/*
takePicture
1���Ƿ���Ҫrestartpreview��
2������?��Ҫ��ͣdisplay ?
3��֪ͨEventNotifier��Ҫ����frame��Ϊpicture
4�������(�����������)ͣ��EventNotifier��picture����Ϣ����
*/

/*
setParameters

����zoom�Ĵ�������camerahal��ʵ��zoom���ܣ�
previewTread ��deque��frame��frame info�м���zoomֵ��
event thread��display thread�յ�frame����Ҫ��zoom����������
*/

/**********************************
���ڲ�ֵ
preview����capture����sizeʱ�������enum�������ֱ��ʻ�Ҫ����ѡȡ
�ʺϵ�sensor�ֱ��ʣ�����event ��display��������ֵ������
************************************/
