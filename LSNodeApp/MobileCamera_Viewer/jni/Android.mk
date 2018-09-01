# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


LOCAL_PATH:= $(call my-dir)



# lib cpufeatures
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := cpufeatures
LOCAL_C_INCLUDES += $(LOCAL_PATH)/cpufeatures
LOCAL_SRC_FILES := cpufeatures/cpu-features.c

include $(BUILD_STATIC_LIBRARY)



# lib colorspace
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := colorspace
LOCAL_C_INCLUDES += $(LOCAL_PATH)/colorspace
LOCAL_SRC_FILES := colorspace/colorspace.cpp

include $(BUILD_STATIC_LIBRARY)



# lib g72x
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := g72x
LOCAL_C_INCLUDES += $(LOCAL_PATH)/g72x
LOCAL_SRC_FILES := g72x/g711.c  g72x/g721.c  g72x/g723_24.c  g72x/g723_40.c  g72x/g72x.c

include $(BUILD_STATIC_LIBRARY)



# lib g729a
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := g729a
LOCAL_C_INCLUDES += $(LOCAL_PATH)/g729a
LOCAL_SRC_FILES := g729a/ACELP_CA.c  g729a/BASIC_OP.c  g729a/BITS.c  g729a/COD_LD8A.c  g729a/CODER.c  g729a/COR_FUNC.c  g729a/DE_ACELP.c  g729a/DEC_GAIN.c \
			g729a/DEC_LAG3.c  g729a/DEC_LD8A.c  g729a/DSPFUNC.c  g729a/FILTER.c  g729a/GAINPRED.c  g729a/LPC.c  g729a/LPCFUNC.c  g729a/LSPDEC.c \
			g729a/LSPGETQ.c  g729a/OPER_32B.c  g729a/P_PARITY.c  g729a/PITCH_A.c  g729a/POST_PRO.c  g729a/POSTFILT.c  g729a/PRE_PROC.c  \
			g729a/PRED_LT3.c  g729a/QUA_GAIN.c  g729a/QUA_LSP.c  g729a/TAB_LD8A.c  g729a/TAMING.c  g729a/UTIL.c

include $(BUILD_STATIC_LIBRARY)



# lib ffavcodec
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := ffavcodec
LOCAL_CFLAGS  += -fPIC
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ffavcodec  $(LOCAL_PATH)/ffavcodec/libavutil  $(LOCAL_PATH)/ffavcodec/libavcodec  $(LOCAL_PATH)/ffavcodec/libavutil/arm  $(LOCAL_PATH)/ffavcodec/libavcodec/arm
LOCAL_SRC_FILES := ffavcodec/libavutil/avstring.c  ffavcodec/libavutil/error.c  ffavcodec/libavutil/integer.c  ffavcodec/libavutil/lls.c  \
		ffavcodec/libavutil/log.c  ffavcodec/libavutil/mathematics.c  ffavcodec/libavutil/mem.c  ffavcodec/libavutil/pixdesc.c  ffavcodec/libavutil/rational.c \
		ffavcodec/libavcodec/allcodecs.c  ffavcodec/libavcodec/avpacket.c  ffavcodec/libavcodec/bitstream.c  ffavcodec/libavcodec/cabac.c \
		ffavcodec/libavcodec/dsputil.c  ffavcodec/libavcodec/error_resilience.c  ffavcodec/libavcodec/eval.c  ffavcodec/libavcodec/golomb.c \
		ffavcodec/libavcodec/h263.c  ffavcodec/libavcodec/h263_parser.c  ffavcodec/libavcodec/h263dec.c  ffavcodec/libavcodec/h264.c \
		ffavcodec/libavcodec/h264_cabac.c  ffavcodec/libavcodec/h264_cavlc.c  ffavcodec/libavcodec/h264_direct.c  ffavcodec/libavcodec/h264_loopfilter.c \
		ffavcodec/libavcodec/h264_parser.c  ffavcodec/libavcodec/h264_ps.c  ffavcodec/libavcodec/h264_refs.c  ffavcodec/libavcodec/h264_sei.c \
		ffavcodec/libavcodec/h264dsp.c  ffavcodec/libavcodec/h264idct.c  ffavcodec/libavcodec/h264pred.c  ffavcodec/libavcodec/imgconvert.c \
		ffavcodec/libavcodec/intelh263dec.c  ffavcodec/libavcodec/ituh263dec.c  ffavcodec/libavcodec/jrevdct.c  ffavcodec/libavcodec/mpegvideo.c \
		ffavcodec/libavcodec/opt.c  ffavcodec/libavcodec/options.c  ffavcodec/libavcodec/parser.c  ffavcodec/libavcodec/ratecontrol.c \
		ffavcodec/libavcodec/raw.c  ffavcodec/libavcodec/simple_idct.c  ffavcodec/libavcodec/utils.c
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_SRC_FILES += ffavcodec/libavcodec/arm/dsputil_init_arm.c  ffavcodec/libavcodec/arm/dsputil_init_armv5te.c \
		ffavcodec/libavcodec/arm/mpegvideo_arm.c  ffavcodec/libavcodec/arm/mpegvideo_armv5te.c \
		ffavcodec/libavcodec/arm/h264dsp_init_arm.c  ffavcodec/libavcodec/arm/h264pred_init_arm.c \
		ffavcodec/libavcodec/arm/asm.S  ffavcodec/libavcodec/arm/dsputil_arm.S \
		ffavcodec/libavcodec/arm/jrevdct_arm.S  ffavcodec/libavcodec/arm/mpegvideo_armv5te_s.S \
		ffavcodec/libavcodec/arm/simple_idct_arm.S  ffavcodec/libavcodec/arm/simple_idct_armv5te.S
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES += ffavcodec/libavcodec/arm/dsputil_init_arm.c  ffavcodec/libavcodec/arm/dsputil_init_armv5te.c \
		ffavcodec/libavcodec/arm/mpegvideo_arm.c  ffavcodec/libavcodec/arm/mpegvideo_armv5te.c \
		ffavcodec/libavcodec/arm/h264dsp_init_arm.c  ffavcodec/libavcodec/arm/h264pred_init_arm.c \
		ffavcodec/libavcodec/arm/asm.S  ffavcodec/libavcodec/arm/dsputil_arm.S \
		ffavcodec/libavcodec/arm/jrevdct_arm.S  ffavcodec/libavcodec/arm/mpegvideo_armv5te_s.S \
		ffavcodec/libavcodec/arm/simple_idct_arm.S  ffavcodec/libavcodec/arm/simple_idct_armv5te.S
LOCAL_SRC_FILES += ffavcodec/libavcodec/arm/dsputil_init_armv6.c \
		ffavcodec/libavcodec/arm/dsputil_init_neon.c  ffavcodec/libavcodec/arm/dsputil_init_vfp.c \
		ffavcodec/libavcodec/arm/dsputil_armv6.S  ffavcodec/libavcodec/arm/dsputil_neon.S  ffavcodec/libavcodec/arm/dsputil_vfp.S \
		ffavcodec/libavcodec/arm/h264dsp_neon.S  ffavcodec/libavcodec/arm/h264idct_neon.S  ffavcodec/libavcodec/arm/h264pred_neon.S \
		ffavcodec/libavcodec/arm/int_neon.S  ffavcodec/libavcodec/arm/simple_idct_armv6.S  ffavcodec/libavcodec/arm/simple_idct_neon.S
LOCAL_ARM_NEON  := true
endif
include $(BUILD_STATIC_LIBRARY)



# lib up2p
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := up2p
LOCAL_C_INCLUDES += $(LOCAL_PATH)/up2p
LOCAL_C_INCLUDES += $(LOCAL_PATH)/up2p/core
LOCAL_C_INCLUDES += $(LOCAL_PATH)/up2p/revolver
LOCAL_C_INCLUDES += $(LOCAL_PATH)/up2p/rudp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/up2p/udt
LOCAL_SRC_FILES := up2p/core/core_packet.cpp  up2p/revolver/base_epoll_reactor.cpp  up2p/revolver/base_event_handler.cpp  up2p/revolver/base_file.cpp  up2p/revolver/base_hex_string.cpp  \
		 up2p/revolver/base_inet_addr.cpp  up2p/revolver/base_log.cpp  up2p/revolver/base_log_thread.cpp  up2p/revolver/base_nodes_load.cpp  up2p/revolver/base_select_reactor.cpp  \
		 up2p/revolver/base_sock_acceptor.cpp  up2p/revolver/base_sock_connector.cpp  up2p/revolver/base_sock_dgram.cpp  up2p/revolver/base_sock_stream.cpp  up2p/revolver/base_socket.cpp \
		 up2p/revolver/base_thread.cpp  up2p/revolver/base_timer_value.cpp  up2p/revolver/date_time.cpp  up2p/revolver/gettimeofday.cpp  up2p/revolver/timer_ring.cpp  \
		 up2p/rudp/rudp_adapter.cpp  up2p/rudp/rudp_ccc.cpp  up2p/rudp/rudp_interface.cpp  up2p/rudp/rudp_log_macro.cpp  up2p/rudp/rudp_recv_buffer.cpp  up2p/rudp/rudp_segment.cpp  \
		 up2p/rudp/rudp_send_buffer.cpp  up2p/rudp/rudp_socket.cpp  up2p/rudp/rudp_stream.cpp  \
		 up2p/udt/rudp_thread.cpp  up2p/udt/udt.cpp  up2p/udt/user_adapter.cpp  up2p/udt/user_connection.cpp
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES :=
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)



# lib shdir
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := shdir
LOCAL_C_INCLUDES += $(LOCAL_PATH)/shdir
LOCAL_C_INCLUDES += $(LOCAL_PATH)/up2p/udt
LOCAL_C_INCLUDES += $(LOCAL_PATH)/avrtp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/g72x
LOCAL_C_INCLUDES += $(LOCAL_PATH)/g729a
LOCAL_SRC_FILES := shdir/platform.cpp  shdir/CommonLib.cpp  shdir/UPnP.cpp  shdir/udp.cpp  shdir/stun.cpp  shdir/phpMd5.cpp \
			shdir/Discovery.cpp  shdir/ControlCmd.cpp  shdir/TWSocket.cpp  shdir/HttpOperate.cpp  shdir/AudioCodec.cpp  viewer_if.cpp  viewer_jni.cpp
LOCAL_STATIC_LIBRARIES := g72x g729a
LOCAL_SHARED_LIBRARIES := up2p
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)



# lib avrtp
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := avrtp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/avrtp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/cpufeatures
LOCAL_C_INCLUDES += $(LOCAL_PATH)/colorspace
LOCAL_C_INCLUDES += $(LOCAL_PATH)/g72x
LOCAL_C_INCLUDES += $(LOCAL_PATH)/g729a
LOCAL_C_INCLUDES += $(LOCAL_PATH)/x264
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ffavcodec  $(LOCAL_PATH)/ffavcodec/libavutil  $(LOCAL_PATH)/ffavcodec/libavcodec
LOCAL_C_INCLUDES += $(LOCAL_PATH)/up2p/udt
LOCAL_C_INCLUDES += $(LOCAL_PATH)/shdir
LOCAL_SRC_FILES := avrtp/wps_queue.cpp  avrtp/FakeRtpRecv.cpp  avrtp/TLVRecv.cpp  avrtp/G729Recv.cpp  avrtp/FF263Recv.cpp  avrtp/FF264Recv.cpp  avrtp_jni.cpp \
			avrtp/FakeRtpSend.cpp  avrtp/TLVSend.cpp  avrtp/G729Send.cpp  avrtp/H264Send.cpp \
			mobcam_av.cpp  mobcam_if.cpp  mobcam_main.cpp
LOCAL_CPP_FEATURES += rtti exceptions
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_LDFLAGS += $(LOCAL_PATH)/x264/libx264-armv5te.a
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_LDFLAGS += $(LOCAL_PATH)/x264/libx264-armv7a.a
endif
LOCAL_STATIC_LIBRARIES += colorspace g72x g729a ffavcodec cpufeatures
LOCAL_SHARED_LIBRARIES += up2p shdir
LOCAL_LDLIBS += -llog -ldl

include $(BUILD_SHARED_LIBRARY)



# lib serial_port
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := serial_port
LOCAL_C_INCLUDES += $(LOCAL_PATH)/serialport
LOCAL_SRC_FILES := serialport/SerialPort.c
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
