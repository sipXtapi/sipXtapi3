# 
# Copyright (C) 2009 SIPfoundry Inc.
# Licensed by SIPfoundry under the LGPL license.
# 
# Copyright (C) 2009 SIPez LLC.
# Licensed to SIPfoundry under a Contributor Agreement.
# 
#
#//////////////////////////////////////////////////////////////////////////
#
# Author: Dan Petrie (dpetrie AT SIPez DOT com)
#
#
# Top level sipX Application.mk for Android
#
#

SIPX_LIBS := libsipXport libsipXsdp libsipXtack libsipXmedia libsipXmediaAdapter libsipXcall libsipXtapi libpcre
SIPX_CODEC_LIBS := libcodec_pcmapcmu libcodec_tones libcodec_g722
SIPX_SPEEX_LIBS := libspeex libspeexdsp
JNI_UNIT_LIBS := libsipxportjnisandbox libsipxmediajnisandbox
UNIT_TESTS := libsipxUnit sipxsandbox sipxportunit sipxsdpunit sipxtackunit mediasandbox sipxmediaunit sipxmediaadapterunit sipxcallunit sipxtapiunit

APP_MODULES := $(SIPX_LIBS) \
   $(APP_EXTRA_LIBS) \
   $(SIPX_CODEC_LIBS) \
   $(SIPX_SPEEX_LIBS) \
   $(JNI_UNIT_LIBS) \
   $(UNIT_TESTS)

APP_PROJECT_PATH := $(call my-dir)/project
#APP_BUILD_SCRIPT := $(call my-dir)/Android.mk
#APP_OPTIM        := release
APP_OPTIM        := debug
APP_CFLAGS      := -D__pingtel_on_posix__ \
                   -DANDROID \
                   -DDEFINE_S_IREAD_IWRITE \
                   -DSIPX_TMPDIR=\"/sdcard\" \
                   -DSIPX_CONFDIR=\"/etc/sipx\" \
                   -DTEST_DIR=\"/sdcard\" \
                   -include apps/sipxtapi/project/jni/sipXportLib/include/os/OsIntTypes.h \
                   -O2 -g

# Optimizations for ARM Cortex-A8, used in Motorola Droid/Milestone, HTC Nexus One, etc
# This enables real FPU instructions too.
APP_CFLAGS += -march=armv7-a -mtune=cortex-a8 -mfpu=neon
APP_CFLAGS += -D__ARM_ARCH_6__ -D__ARM_ARCH_7__ -D__ARM_ARCH_7A__
APP_CFLAGS += -ftree-vectorize -ffast-math -mvectorize-with-neon-quad -mno-apcs-stack-check

