#--------------------------------------------------------------------------
#  Copyright (C) 2018 Fuzhou Rockchip Electronics Co. Ltd. All rights reserved.

#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of The Linux Foundation nor
#      the names of its contributors may be used to endorse or promote
#      products derived from this software without specific prior written
#      permission.

#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#--------------------------------------------------------------------------

ifeq (1,$(strip $(shell expr $(BOARD_DEFAULT_CAMERA_HAL_VERSION) \>= 3.0)))
LOCAL_PATH:= $(call my-dir)
include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

AALSRC = AAL/Camera3HAL.cpp \
         AAL/Camera3Request.cpp \
         AAL/CameraStream.cpp \
         AAL/ICameraHw.cpp \
         AAL/RequestThread.cpp \
         AAL/ResultProcessor.cpp

V4L2SRC = common/v4l2dev/v4l2devicebase.cpp \
          common/v4l2dev/v4l2subdevice.cpp \
          common/v4l2dev/v4l2videonode.cpp

PLATFORMDATASRC = common/platformdata/CameraMetadataHelper.cpp \
                  common/platformdata/CameraProfiles.cpp \
                  common/platformdata/ChromeCameraProfiles.cpp \
                  common/platformdata/Metadata.cpp \
                  common/platformdata/PlatformData.cpp \
                  common/platformdata/IPSLConfParser.cpp

MEDIACONTROLLERSRC = common/mediacontroller/MediaController.cpp \
                     common/mediacontroller/MediaEntity.cpp

IMAGEPROCESSSRC = common/imageProcess/ColorConverter.cpp \
                  common/imageProcess/ImageScalerCore.cpp

COMMONSRC = common/SysCall.cpp \
            common/Camera3V4l2Format.cpp \
            common/CameraWindow.cpp \
            common/LogHelper.cpp \
            common/LogHelperAndroid.cpp \
            common/EnumPrinthelper.cpp \
            common/FlashLight.cpp \
            common/MessageThread.cpp \
            common/PerformanceTraces.cpp \
            common/PollerThread.cpp \
            common/Utils.cpp \
            common/CommonBuffer.cpp \
            common/camera_buffer_manager_gralloc_impl.cpp \
            common/IaAtrace.cpp \
            common/GFXFormatLinuxGeneric.cpp

JPEGSRC = common/jpeg/ExifCreater.cpp \
          common/jpeg/EXIFMaker.cpp \
          common/jpeg/EXIFMetaData.cpp \
          common/jpeg/ImgEncoderCore.cpp \
          common/jpeg/ImgEncoder.cpp \
          common/jpeg/JpegMakerCore.cpp \
          common/jpeg/ImgHWEncoder.cpp \
          common/jpeg/JpegMaker.cpp \
          common/jpeg/jpeg_compressor.cpp

GCSSSRC = common/gcss/graph_query_manager.cpp \
          common/gcss/gcss_item.cpp \
          common/gcss/gcss_utils.cpp \
          common/gcss/GCSSParser.cpp \
          common/gcss/gcss_formats.cpp

include $(LOCAL_PATH)/psl/rkisp1/Android.mk

LOCAL_SRC_FILES := \
    $(AALSRC) \
    $(V4L2SRC) \
    $(PLATFORMDATASRC) \
    $(MEDIACONTROLLERSRC) \
    $(IMAGEPROCESSSRC) \
    $(COMMONSRC) \
    $(JPEGSRC) \
    $(PSLSRC) \
    $(GCSSSRC) \
    Camera3HALModule.cpp

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 26)))
LOCAL_HEADER_LIBRARIES += \
       libhardware_headers \
       libbinder_headers \
       gl_headers \
       libutils_headers
else
LOCAL_C_INCLUDES += \
    hardware/libhardware/include \
    hardware/libhardware/modules/gralloc \
    system/core/include \
    system/core/include/utils \
    frameworks/av/include \
    hardware/libhardware/include
endif
ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 29)))
LOCAL_C_INCLUDES += \
    system/core/liblog/include
endif
LOCAL_C_INCLUDES += \
    hardware/libhardware/include \
    system/media/camera/include \
    system/core/libsync \
    system/core/liblog/include \
    kernel/include/uapi \
    hardware/rockchip/librkvpu \
    hardware/rockchip/jpeghw \
    hardware/rockchip/librga \
    external/libchrome \
    $(LOCAL_PATH)/include/arc

ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)), mali-tDVx)
LOCAL_C_INCLUDES += \
       hardware/rockchip/libgralloc/bifrost
endif

ifneq (,$(filter mali-t860 mali-t760, $(TARGET_BOARD_PLATFORM_GPU)))
LOCAL_C_INCLUDES += \
       hardware/rockchip/libgralloc/midgard
endif

ifneq (,$(filter mali400 mali450, $(TARGET_BOARD_PLATFORM_GPU)))
LOCAL_C_INCLUDES += \
       hardware/rockchip/libgralloc/utgard
endif

#cpphacks
CPPHACKS = \
    -DPAGESIZE=4096 \
    -DCAMERA_HAL_DEBUG \
    -DDUMP_IMAGE \
    -DRKCAMERA_REDEFINE_LOG \
    -DRK_DRM_GRALLOC=1 \
    -DRK_HW_JPEG_ENCODE \

# rk3368 gralloc module from other platforms
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3368)
LOCAL_CFLAGS += -DTARGET_RK3368
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3126c)
LOCAL_CFLAGS += -DTARGET_RK312X
endif

ifeq ($(strip $(Have3AControlLoop)), true)
CPPHACKS += \
    -DHAVE_3A_CONTROL_LOOP
endif
LOCAL_CFLAGS += -Wno-error
LOCAL_CPPFLAGS := $(CPPHACKS)

# Enable large file support.
LOCAL_CPPFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

#Namespace Declaration
LOCAL_CPPFLAGS += -DNAMESPACE_DECLARATION=namespace\ android\ {\namespace\ camera2
LOCAL_CPPFLAGS += -DNAMESPACE_DECLARATION_END=}
LOCAL_CPPFLAGS += -DUSING_DECLARED_NAMESPACE=using\ namespace\ android::camera2

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 26)))
LOCAL_CPPFLAGS += \
    -DUSING_METADATA_NAMESPACE=using\ ::android::hardware::camera::common::V1_0::helper::CameraMetadata
else
LOCAL_CPPFLAGS += -DUSING_METADATA_NAMESPACE=
endif

ALLINCLUDES = \
              $(PSLCPPFLAGS) \
              -I$(LOCAL_PATH) \
              -I$(LOCAL_PATH)/common \
              -I$(LOCAL_PATH)/common/platformdata \
              -I$(LOCAL_PATH)/common/platformdata/gc \
              -I$(LOCAL_PATH)/common/3a \
              -I$(LOCAL_PATH)/common/mediacontroller \
              -I$(LOCAL_PATH)/common/v4l2dev \
              -I$(LOCAL_PATH)/AAL \
              -I$(LOCAL_PATH)/common/imageProcess \
              -I$(LOCAL_PATH)/common/jpeg \
              -I$(LOCAL_PATH)/common/gcss \
              -I$(LOCAL_PATH)/common/3awrapper

LOCAL_CPPFLAGS += $(ALLINCLUDES)
# LOCAL_CPPFLAGS += -v

LOCAL_STATIC_LIBRARIES := \
    libyuv_static
ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 26)))
LOCAL_STATIC_LIBRARIES += android.hardware.camera.common@1.0-helper
endif
LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    libutils \
    libjpeg \
    libchrome \
    libexpat \
    libdl \
    libsync \
    libui \
    librkisp \
    libhardware \
    libjpeghwenc \
    libvpu \
    libcamera_metadata \
    librga
ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 26)))
LOCAL_SHARED_LIBRARIES += \
    libnativewindow \
    liblog
else
LOCAL_SHARED_LIBRARIES += \
    libcamera_client
endif

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 28)))
LOCAL_SHARED_LIBRARIES += \
    libsync_vendor
endif

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 26)))
    LOCAL_CFLAGS += -DANDROID_VERSION_ABOVE_8_X
endif

LOCAL_LDFLAGS := -Wl,-z,defs
ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 26)))
    LOCAL_PROPRIETARY_MODULE := true
endif
ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 29)))
    LOCAL_CFLAGS += -DANDROID_VERSION_ABOVE_10_X
    LOCAL_CPPFLAGS += -std=c++1y
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE:= camera.$(TARGET_BOARD_HARDWARE)
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
endif
