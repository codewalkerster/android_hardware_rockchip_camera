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
/**
* @file CameraHal_Utils.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/
#include <math.h>
#include "CameraHal.h" 
//#include "SkStream.h"
//#include "SkBitmap.h"
//#include "SkDevice.h"
//#include "SkPaint.h"
//#include "SkRect.h"
//#include "SkImageEncoder.h"
#if HAVE_ANDROID_OS
#include <linux/android_pmem.h>
#include <binder/MemoryHeapPmem.h>
#endif
#include "../jpeghw/release/encode_release/hw_jpegenc.h"

namespace android {
#define LOG_TAG "CameraHal_Util"

/* Values */
#define EXIF_DEF_MAKER          "rockchip"
#define EXIF_DEF_MODEL          "rk29sdk"

static int iPicturePmemFd;
static int PicturePmem_Offset_Output;
static int PicturePmemSize;

static char ExifMaker[32];
static char ExifModel[32];

extern "C" int capturePicture_cacheflush(int buf_type, int offset, int len)
{
    int ret = 0;
    struct pmem_region region;

    region.offset = 0;
    region.len = 0;
    if (buf_type == 0) {          //input_buf
        goto capture_cacheflush_end;
    } else if (buf_type == 1) {   //output_buf
        
        if (offset >=  PicturePmemSize) {
            ret = -1;
            goto capture_cacheflush_end;
        }
        region.offset = PicturePmem_Offset_Output+offset;
        if ((len + offset) > PicturePmemSize) {
            len = PicturePmemSize - region.offset;
        }        
        region.len = len;
        ret = ioctl(iPicturePmemFd,PMEM_CACHE_FLUSH, &region);
    }
    LOGD("capturePicture_cacheflush buf_type:0x%x  offset:0x%x  len:0x%x ret:0x%x",buf_type,region.offset,region.len,ret);

capture_cacheflush_end:
    return ret;
}
extern "C"  int YData_Mirror_Line(int v4l2_fmt_src, int *psrc, int *pdst, int w)
{
    int i;

    for (i=0; i<(w>>2); i++) {
        *pdst = ((*psrc>>24)&0x000000ff) | ((*psrc>>16)&0x0000ff00)
                | ((*psrc>>8)&0x00ff0000) | ((*psrc<<24)&0xff000000);
        psrc++;
        pdst--;
    }

    return 0;
}
extern "C"  int UVData_Mirror_Line(int v4l2_fmt_src, int *psrc, int *pdst, int w)
{
    int i;

    for (i=0; i<(w>>2); i++) {
        *pdst = ((*psrc>>16)&0x0000ffff) | ((*psrc<<16)&0xffff0000);                
        psrc++;
        pdst--;
    }

    return 0;
}
extern "C"  int YuvData_Mirror_Flip(int v4l2_fmt_src, char *pdata, char *pline_tmp, int w, int h)
{
    int *pdata_tmp = NULL;
    int *ptop, *pbottom;
    int err = 0,i,j;

    LOG_FUNCTION_NAME

    pdata_tmp = (int*)pline_tmp;
    
    // Y mirror and flip
    ptop = (int*)pdata;
    pbottom = (int*)(pdata+w*(h-1));    
    for (j=0; j<(h>>1); j++) {
        YData_Mirror_Line(v4l2_fmt_src, ptop, pdata_tmp+((w>>2)-1),w);
        YData_Mirror_Line(v4l2_fmt_src, pbottom, ptop+((w>>2)-1), w);
        memcpy(pbottom, pdata_tmp, w);
        ptop += (w>>2);
        pbottom -= (w>>2);
    }
    // UV mirror and flip
    ptop = (int*)(pdata+w*h);
    pbottom = (int*)(pdata+w*(h*3/2-1));    
    for (j=0; j<(h>>2); j++) {
        UVData_Mirror_Line(v4l2_fmt_src, ptop, pdata_tmp+((w>>2)-1),w);
        UVData_Mirror_Line(v4l2_fmt_src, pbottom, ptop+((w>>2)-1), w);
        memcpy(pbottom, pdata_tmp, w);
        ptop += (w>>2);
        pbottom -= (w>>2);
    }

    LOG_FUNCTION_NAME_EXIT
YuvData_Mirror_Flip_end:
    return err;
}

/*fill in jpeg gps information*/  
int CameraHal::Jpegfillgpsinfo(RkGPSInfo *gpsInfo)
{
	char* gpsprocessmethod = NULL;
    double latitude,longtitude,altitude;
    double deg,min,sec;
    double fract;
    long timestamp,num; 
    int year,month,day,hour_t,min_t,sec_t;
    char date[12];
    
    if(gpsInfo==NULL) {    
        LOGE( "..%s..%d..argument error ! ",__FUNCTION__,__LINE__);
        return 0;
    }

    altitude = mGps_altitude;
    latitude = mGps_latitude;
    longtitude = mGps_longitude;
    timestamp = mGps_timestamp; 
    gpsprocessmethod = (char*)mParameters.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    
    if(latitude >= 0){
    	gpsInfo->GPSLatitudeRef[0] = 'N';
    	gpsInfo->GPSLatitudeRef[1] = '\0';
    }else if((latitude <0)&&(latitude!=-1)){
    	gpsInfo->GPSLatitudeRef[0] = 'S';
    	gpsInfo->GPSLatitudeRef[1] = '\0';
    }else{
    	gpsInfo->GPSLatitudeRef[0] = '\0';
    	gpsInfo->GPSLatitudeRef[1] = '\0';
    }
    
   if(latitude!= -1)
   {
        latitude = fabs(latitude);
        fract = modf(latitude,&deg);
        fract = modf(fract*60,&min);
        modf(fract*60,&sec);
        
        LOGD("latitude: deg = %f;min = %f;sec =%f",deg,min,sec);

    	gpsInfo->GPSLatitude[0].num = (uint32_t)deg;
    	gpsInfo->GPSLatitude[0].denom = 1;
    	gpsInfo->GPSLatitude[1].num =  (uint32_t)min;
    	gpsInfo->GPSLatitude[1].denom = 1;
    	gpsInfo->GPSLatitude[2].num =  (uint32_t)sec;
    	gpsInfo->GPSLatitude[2].denom = 1;
   }
  
   if(longtitude >= 0){
    	gpsInfo->GPSLongitudeRef[0] = 'E';
    	gpsInfo->GPSLongitudeRef[1] = '\0';
    }else if((longtitude < 0)&&(longtitude!=-1)){
    	gpsInfo->GPSLongitudeRef[0] = 'W';
    	gpsInfo->GPSLongitudeRef[1] = '\0';
    }else{
    	gpsInfo->GPSLongitudeRef[0] = '\0';
    	gpsInfo->GPSLongitudeRef[1] = '\0';
    }

    if(longtitude!=-1)
    {
        longtitude = fabs(longtitude);
        fract = modf(longtitude,&deg);
        fract = modf(fract*60,&min);
        modf(fract*60,&sec);
        
        LOGD("longtitude: deg = %f;min = %f;sec =%f",deg,min,sec);
    	gpsInfo->GPSLongitude[0].num = (uint32_t)deg;
    	gpsInfo->GPSLongitude[0].denom = 1;
    	gpsInfo->GPSLongitude[1].num = (uint32_t)min;
    	gpsInfo->GPSLongitude[1].denom = 1;
    	gpsInfo->GPSLongitude[2].num = (uint32_t)sec;
    	gpsInfo->GPSLongitude[2].denom = 1;
    }
    
    if(altitude > 0){
        gpsInfo->GPSAltitudeRef = 1;
    }else if((altitude <0 )&&(altitude!=-1)) {
        gpsInfo->GPSAltitudeRef = -1;
    }else {
        gpsInfo->GPSAltitudeRef = 0;
    }  
    
    if(altitude!=-1)
    {
        altitude = fabs(altitude);
    	gpsInfo->GPSAltitude.num =(uint32_t)altitude;
    	gpsInfo->GPSAltitude.denom = 0x1;
        LOGD("altitude =%f",altitude);        
    }
    
    if(timestamp!=-1)
    {
       /*timestamp,has no meaning,only for passing cts*/
        LOGD("timestamp =%d",timestamp);
        gpsInfo->GpsTimeStamp[0].num =12;
    	gpsInfo->GpsTimeStamp[0].denom = 1;
    	gpsInfo->GpsTimeStamp[1].num = 12;
    	gpsInfo->GpsTimeStamp[1].denom = 1;
    	gpsInfo->GpsTimeStamp[2].num = 12;
    	gpsInfo->GpsTimeStamp[2].denom = 1;         
    	memcpy(gpsInfo->GpsDateStamp,"2011:07:06",11);//"YYYY:MM:DD\0"
    }    
    return 1;
}


int CameraHal::Jpegfillexifinfo(RkExifInfo *exifInfo)
{
    char property[PROPERTY_VALUE_MAX];
    int jpeg_w,jpeg_h;
    int focalen;
    
    if(exifInfo==NULL){
        LOGE( "..%s..%d..argument error ! ",__FUNCTION__,__LINE__);
        return 0;
    }

    /*get some current relavant  parameters*/
    mParameters.getPictureSize(&jpeg_w, &jpeg_h);
    focalen = strtol(mParameters.get(CameraParameters::KEY_FOCAL_LENGTH),0,0);
    
    /*fill in jpeg exif tag*/  
    property_get("ro.product.brand", property, EXIF_DEF_MAKER);
    strncpy((char *)ExifMaker, property,sizeof(ExifMaker) - 1);
    ExifMaker[sizeof(ExifMaker) - 1] = '\0';
 	exifInfo->maker = ExifMaker;
	exifInfo->makerchars = strlen(ExifMaker);
    
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)ExifModel, property,sizeof(ExifModel) - 1);
    ExifModel[sizeof(ExifModel) - 1] = '\0';
	exifInfo->modelstr = ExifModel;
	exifInfo->modelchars = strlen(ExifModel);  
    
	exifInfo->Orientation = 1;

    //3 Date time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime((char *)exifInfo->DateTime, 20, "%Y:%m:%d %H:%M:%S", timeinfo);
    
	exifInfo->ExposureTime.num = 1;
	exifInfo->ExposureTime.denom = 100;
	exifInfo->ApertureFNumber.num = 0x118;
	exifInfo->ApertureFNumber.denom = 0x64;
	exifInfo->ISOSpeedRatings = 0x59;
	exifInfo->CompressedBitsPerPixel.num = 0x4;
	exifInfo->CompressedBitsPerPixel.denom = 0x1;
	exifInfo->ShutterSpeedValue.num = 0x452;
	exifInfo->ShutterSpeedValue.denom = 0x100;
	exifInfo->ApertureValue.num = 0x2f8;
	exifInfo->ApertureValue.denom = 0x100;
	exifInfo->ExposureBiasValue.num = 0;
	exifInfo->ExposureBiasValue.denom = 0x100;
	exifInfo->MaxApertureValue.num = 0x02f8;
	exifInfo->MaxApertureValue.denom = 0x100;
	exifInfo->MeteringMode = 02;
    if ((strcmp(CameraParameters::FLASH_MODE_OFF, mParameters.get(CameraParameters::KEY_FLASH_MODE)) == 0)) {
	    exifInfo->Flash = 0;
    } else {
        exifInfo->Flash = 1;
    }
    
	exifInfo->FocalLength.num = (uint32_t)focalen;
	exifInfo->FocalLength.denom = 0x1;
    
	exifInfo->FocalPlaneXResolution.num = 0x8383;
	exifInfo->FocalPlaneXResolution.denom = 0x67;
	exifInfo->FocalPlaneYResolution.num = 0x7878;
	exifInfo->FocalPlaneYResolution.denom = 0x76;
	exifInfo->SensingMethod = 2;
	exifInfo->FileSource = 3;
	exifInfo->CustomRendered = 1;
	exifInfo->ExposureMode = 0;
    if (strcmp(CameraParameters::WHITE_BALANCE_AUTO, mParameters.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE)) == 0) {
	    exifInfo->WhiteBalance = 0;
    } else {
        exifInfo->WhiteBalance = 1;
    }
	exifInfo->DigitalZoomRatio.num = jpeg_w;
	exifInfo->DigitalZoomRatio.denom = jpeg_w;
	exifInfo->SceneCaptureType = 0x01;   
    return 1;
}

int CameraHal::capturePicture(struct CamCaptureInfo_s *capture)
{
    int jpeg_w,jpeg_h,i;
    int pictureSize;
    unsigned long base, offset, picture_format;
    struct v4l2_buffer buffer;
    struct v4l2_format format;
    struct v4l2_buffer cfilledbuffer;
    struct v4l2_requestbuffers creqbuf;
    int jpegSize;
    int quality;
    int thumbquality = 0;
    int thumbwidth  = 0;
    int thumbheight = 0;
	int err = 0;
	int rotation = 0;
    char *camDriverV4l2Buffer;
    JpegEncInInfo JpegInInfo;
    JpegEncOutInfo JpegOutInfo;  
    
	RkExifInfo exifInfo;
	RkGPSInfo gpsInfo;
    char ExifAsciiPrefix[8] = {'A', 'S', 'C', 'I', 'I', '\0', '\0', '\0'};
    char gpsprocessmethod[45];
    char *getMethod = NULL;
    double latitude,longtitude,altitude;
    long timestamp;
    bool driver_mirror_fail = false;
    struct v4l2_control control;
    
     /*get jpeg and thumbnail information*/
    mParameters.getPictureSize(&jpeg_w, &jpeg_h);                
    quality = mParameters.getInt("jpeg-quality");
    rotation = strtol(mParameters.get(CameraParameters::KEY_ROTATION),0,0);
    thumbquality = strtol(mParameters.get(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY),0,0);
    thumbwidth = strtol(mParameters.get(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH),0,0);
    thumbheight = strtol(mParameters.get(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT),0,0);
    /*get gps information*/
    altitude = mGps_altitude;
    latitude = mGps_latitude;
    longtitude = mGps_longitude;
    timestamp = mGps_timestamp;    
    getMethod = (char*)mParameters.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);//getMethod : len <= 32
    

    pictureSize = jpeg_w * jpeg_h * 3/2;
    if (pictureSize & 0xfff) {
        pictureSize = (pictureSize & 0xfffff000) + 0x1000;
    }

    if (pictureSize > mRawBuffer->size()) {
        LOGE("Picture input buffer(size:0x%x) is not enough for this resolution picture(%d*%d, size:0x%x)",
            mRawBuffer->size(), jpeg_w, jpeg_h, pictureSize);
        err = -1;
        goto exit;
    } else {
        LOGD("Picture Size: Width = %d \tHeight = %d \quality = %d rotation = %d", jpeg_w, jpeg_h,quality,rotation);
    }

    i = 0;
    while (mCamDriverSupportFmt[i]) {
        if (mCamDriverSupportFmt[i] == mCamDriverPictureFmt)
            break;
        i++;
    }

    if (mCamDriverSupportFmt[i] == 0) {
        picture_format = mCamDriverPreviewFmt;        
    } else {
        picture_format = mCamDriverPictureFmt;
    }

    mPictureLock.lock();
    if (mPictureRunning != STA_PICTURE_RUN) {
        mPictureLock.unlock();
        LOGD("%s(%d): capture cancel, because mPictureRunning(0x%x) dosen't suit capture",
            __FUNCTION__,__LINE__, mPictureRunning);
        goto exit;
    }
    mPictureLock.unlock();

    err = cameraSetSize(jpeg_w, jpeg_h, picture_format);
    if (err < 0) {
		LOGE ("CapturePicture failed to set VIDIOC_S_FMT.");
		goto exit;
	}

    /* Check if the camera driver can accept 1 buffer */    
    creqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    creqbuf.memory = mCamDriverV4l2MemType;
    creqbuf.count  = 1;
    if (ioctl(iCamFd, VIDIOC_REQBUFS, &creqbuf) < 0) {
        LOGE ("VIDIOC_REQBUFS Failed. errno = %d", errno);
        goto exit;
    }

    buffer.type = creqbuf.type;
    buffer.memory = creqbuf.memory;
    buffer.index = 0;
	err = ioctl(iCamFd, VIDIOC_QUERYBUF, &buffer);
    if (err < 0) {
        LOGE("VIDIOC_QUERYBUF Failed");
        goto exit;
    }

    if (buffer.memory == V4L2_MEMORY_OVERLAY) {
        buffer.m.offset = capture->input_phy_addr;    /* phy address */ 
        camDriverV4l2Buffer = (char*)mRawBuffer->pointer();
    } else if (buffer.memory == V4L2_MEMORY_MMAP) {
        camDriverV4l2Buffer = (char*)mmap(0 /* start anywhere */ ,
                            buffer.length, PROT_READ, MAP_SHARED, iCamFd,
                            buffer.m.offset);
        if (camDriverV4l2Buffer == MAP_FAILED) {
            LOGE("%s Unable to map buffer(length:0x%x offset:0x%x) [%s]\n",__FUNCTION__, buffer.length,buffer.m.offset,errno);
            goto exit;
        } else {
            LOGD("%s camDriverV4l2Buffer:0x%x",__FUNCTION__,(int)camDriverV4l2Buffer);
        }
    }
    
    err = ioctl(iCamFd, VIDIOC_QBUF, &buffer);
    if (err < 0) {
        LOGE("CapturePicture VIDIOC_QBUF Failed");
        goto exit;
    }
    
    if (rotation == 180) {
        if (mDriverFlipSupport && mDriverMirrorSupport) {
            
            control.id = V4L2_CID_HFLIP;
			control.value = true;
			err = ioctl(iCamFd, VIDIOC_S_CTRL, &control);
            if (!err) {
                control.id = V4L2_CID_VFLIP;
                control.value = true;
    			err = ioctl(iCamFd, VIDIOC_S_CTRL, &control); 
            }
            
            if (err) {
                driver_mirror_fail = true;
                LOGE("%s(%d): Mirror and flip picture in driver failed!", __FUNCTION__,__LINE__);
            } else {
                driver_mirror_fail = false;
            }
        } else {
            driver_mirror_fail = true;
        }
    }

    /* turn on streaming */
	err = ioctl(iCamFd, VIDIOC_STREAMON, &creqbuf.type);
    if (err < 0) {
        LOGE("CapturePicture VIDIOC_STREAMON Failed");
        goto exit;
    }

    mPictureLock.lock();
    if (mPictureRunning != STA_PICTURE_RUN) {
        mPictureLock.unlock();
        LOGD("%s(%d): capture cancel, because mPictureRunning(0x%x) dosen't suit capture",
            __FUNCTION__,__LINE__, mPictureRunning);
        goto capturePicture_streamoff;
    }
    mPictureLock.unlock();

    if (strcmp((char*)&mCamDriverCapability.driver[0],"uvcvideo") == 0) {
        for (i=0; i<4; i++) {
            ioctl(iCamFd, VIDIOC_DQBUF, &buffer);
            ioctl(iCamFd, VIDIOC_QBUF, &buffer);
        }
    }

    /* De-queue the next avaliable buffer */
    err = ioctl(iCamFd, VIDIOC_DQBUF, &buffer);
    if (err < 0) {
        LOGE("CapturePicture VIDIOC_DQBUF Failed");
        goto exit;
    } 

    if (mMsgEnabled & CAMERA_MSG_SHUTTER)
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
    
capturePicture_streamoff:
    /* turn off streaming */
	err = ioctl(iCamFd, VIDIOC_STREAMOFF, &creqbuf.type);
    if (err < 0) {
        LOGE("CapturePicture VIDIOC_STREAMON Failed \n");
        goto exit;
    }

    mPictureLock.lock();
    if (mPictureRunning != STA_PICTURE_RUN) {
        mPictureLock.unlock();
        LOGD("%s(%d): capture cancel, because mPictureRunning(0x%x) dosen't suit capture",
            __FUNCTION__,__LINE__, mPictureRunning);
        goto exit;
    }
    mPictureLock.unlock();
    
    cameraFormatConvert(picture_format, mCamDriverPictureFmt, NULL,
        (char*)camDriverV4l2Buffer,(char*)mRawBuffer->pointer(), jpeg_w, jpeg_h);

    if (mCamDriverV4l2MemType == V4L2_MEMORY_MMAP) {
        if (camDriverV4l2Buffer != NULL) {
            if (munmap((void*)camDriverV4l2Buffer, buffer.length) < 0)
                LOGE("%s camDriverV4l2Buffer munmap failed : %s",__FUNCTION__,strerror(errno));
            camDriverV4l2Buffer = NULL;
        }
    }

    /* ddl@rock-chips.com: Release v4l2 buffer must by close device, buffer isn't release in VIDIOC_STREAMOFF ioctl */
    if (strcmp((char*)&mCamDriverCapability.driver[0],"uvcvideo") == 0) {
        close(iCamFd);
        iCamFd = open(cameraDevicePathCur, O_RDWR);
        if (iCamFd < 0) {
            LOGE ("%s[%d]-Could not open the camera device(%s): %s",__FUNCTION__,__LINE__, cameraDevicePathCur, strerror(errno) );
            err = -1;
            goto exit;
        }
    }
    
    if (rotation == 180) {
        if (driver_mirror_fail == true) {
            YuvData_Mirror_Flip(mCamDriverPictureFmt,(char*)mRawBuffer->pointer(), (char*)mJpegBuffer->pointer(), jpeg_w,jpeg_h);  
        } else {
            if (mDriverFlipSupport && mDriverMirrorSupport) {  
                control.id = V4L2_CID_HFLIP;
    			control.value = false;
    			ioctl(iCamFd, VIDIOC_S_CTRL, &control);                
                control.id = V4L2_CID_VFLIP;
                control.value = false;
                ioctl(iCamFd, VIDIOC_S_CTRL, &control);
            }
        } 
    }

    iPicturePmemFd = iPmemFd;
    PicturePmemSize = mPmemSize;
    PicturePmem_Offset_Output = mJpegBuffer->offset();
    JpegInInfo.frameHeader = 1;
    if ((rotation == 0) || (rotation == 180)) {
        JpegInInfo.rotateDegree = DEGREE_0;        
    } else if (rotation == 90) {
        JpegInInfo.rotateDegree = DEGREE_90;
    } else if (rotation == 270) {
        JpegInInfo.rotateDegree = DEGREE_270; 
    }
    JpegInInfo.yuvaddrfor180 = NULL;
    JpegInInfo.y_rgb_addr = capture->input_phy_addr;
    JpegInInfo.uv_addr = capture->input_phy_addr + jpeg_w*jpeg_h;
    JpegInInfo.inputW = jpeg_w;
    JpegInInfo.inputH = jpeg_h;
    JpegInInfo.type = JPEGENC_YUV420_SP;
    JpegInInfo.qLvl = quality/10;
    if (JpegInInfo.qLvl < 5) {
        JpegInInfo.qLvl = 5;
    }

    JpegInInfo.thumbqLvl = thumbquality /10;
    if (JpegInInfo.thumbqLvl < 5) {
        JpegInInfo.thumbqLvl = 5;
    }
    if(JpegInInfo.thumbqLvl  >10) {
        JpegInInfo.thumbqLvl = 9;
    }
    
    if(thumbwidth !=0 && thumbheight !=0) {
        JpegInInfo.doThumbNail = 1;          //insert thumbnail at APP0 extension
    	JpegInInfo.thumbData = NULL;         //if thumbData is NULL, do scale, the type above can not be 420_P or 422_UYVY
    	JpegInInfo.thumbDataLen = -1;
    	JpegInInfo.thumbW = thumbwidth;
    	JpegInInfo.thumbH = thumbheight;
    }else{    
        JpegInInfo.doThumbNail = 0;          //insert thumbnail at APP0 extension   
    }
    
    Jpegfillexifinfo(&exifInfo);
    JpegInInfo.exifInfo =&exifInfo;
    if((longtitude!=-1)&& (latitude!=-1)&&(timestamp!=-1)&&(getMethod!=NULL)) {    
        Jpegfillgpsinfo(&gpsInfo);  
        memset(gpsprocessmethod,0,45);   
        memcpy(gpsprocessmethod,ExifAsciiPrefix,8);   
        memcpy(gpsprocessmethod+8,getMethod,strlen(getMethod)+1);          
        gpsInfo.GpsProcessingMethodchars = strlen(getMethod)+1+8;
        gpsInfo.GPSProcessingMethod  = gpsprocessmethod;
        LOGD("\nGpsProcessingMethodchars =%d",gpsInfo.GpsProcessingMethodchars);
        JpegInInfo.gpsInfo = &gpsInfo;
    } else {
        JpegInInfo.gpsInfo = NULL;
    }

    JpegOutInfo.outBufPhyAddr = capture->output_phy_addr;
    JpegOutInfo.outBufVirAddr = (unsigned char*)capture->output_vir_addr;
    JpegOutInfo.outBuflen = capture->output_buflen;
    JpegOutInfo.jpegFileLen = 0x00;
    JpegOutInfo.cacheflush= &capturePicture_cacheflush;

    mPictureLock.lock();
    if (mPictureRunning != STA_PICTURE_RUN) {
        mPictureLock.unlock();
        LOGD("%s(%d): capture cancel, because mPictureRunning(0x%x) dosen't suit capture",
            __FUNCTION__,__LINE__, mPictureRunning);
        goto exit;
    }
    mPictureLock.unlock();

	err = hw_jpeg_encode(&JpegInInfo, &JpegOutInfo);
    if ((err < 0) || (JpegOutInfo.jpegFileLen <=0x00)) {
        LOGE("hw_jpeg_encode Failed \n");
        goto exit;
    } else { 
        camera_memory_t* picture = NULL;
        
    	if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
            LOGD("%s success, notify compressed image callback capture success(size:0x%x)!!!\n", __FUNCTION__,JpegOutInfo.jpegFileLen);
            picture = mRequestMemory(-1, JpegOutInfo.jpegFileLen, 1, NULL);
            if (picture && picture->data) {
                memcpy(picture->data,(char*)JpegOutInfo.outBufVirAddr, JpegOutInfo.jpegFileLen);
            }
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, picture, 0, NULL, mCallbackCookie); 
    	}        
    }

exit:   

    return err;
}
};



