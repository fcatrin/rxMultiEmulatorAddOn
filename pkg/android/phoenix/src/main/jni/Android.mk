LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := retroarch-jni
RARCH_DIR := ../../..
LOCAL_CFLAGS += -std=gnu99 -Wall -DHAVE_LOGGER -DRARCH_DUMMY_LOG -DHAVE_ZLIB -DHAVE_MMAP -DRARCH_INTERNAL
LOCAL_LDLIBS := -llog -lz
LOCAL_SRC_FILES := apk-extract/apk-extract.c $(RARCH_DIR)/libretro-common/file/file_extract.c $(RARCH_DIR)/libretro-common/file/file_path.c $(RARCH_DIR)/file_ops.c $(RARCH_DIR)/libretro-common/string/string_list.c $(RARCH_DIR)/libretro-common/compat/compat.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(RARCH_DIR)/libretro-common/include/

include $(BUILD_SHARED_LIBRARY)

HAVE_VKEY := 0
HAVE_NEON := 1
HAVE_LOGGER := 1
GLES := 3

include $(CLEAR_VARS)
ifeq ($(TARGET_ARCH),arm)
   LOCAL_CFLAGS += -DANDROID_ARM -marm
   LOCAL_ARM_MODE := arm
endif

ifeq ($(TARGET_ARCH),x86)
   LOCAL_CFLAGS += -DANDROID_X86 -DHAVE_SSSE3
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)

ifeq ($(HAVE_NEON),1)
	LOCAL_CFLAGS += -D__ARM_NEON__
   LOCAL_SRC_FILES += $(RARCH_DIR)/audio/audio_utils_neon.S.neon
   LOCAL_SRC_FILES += $(RARCH_DIR)/audio/drivers_resampler/sinc_neon.S.neon
   LOCAL_SRC_FILES += $(RARCH_DIR)/audio/drivers_resampler/cc_resampler_neon.S.neon
endif
LOCAL_CFLAGS += -DSINC_LOWER_QUALITY 

LOCAL_CFLAGS += -DANDROID_ARM_V7
endif

ifeq ($(TARGET_ARCH),mips)
   LOCAL_CFLAGS += -DANDROID_MIPS -D__mips__ -D__MIPSEL__
endif

LOCAL_MODULE := retroarch-activity

LOCAL_SRC_FILES  +=	cheats/cheats.c
ifeq ($(HAVE_VKEY),1)
	LOCAL_SRC_FILES  +=	$(RARCH_DIR)/gfx/nanovg/nanovg.c
	LOCAL_SRC_FILES  +=	$(RARCH_DIR)/input/vkey/vkey.c
	LOCAL_SRC_FILES  +=	$(RARCH_DIR)/input/vkey/vkey_c64.c
	LOCAL_CFLAGS += -DNANOVG_GLES2_IMPLEMENTATION -DGL_GLEXT_PROTOTYPES
	LOCAL_CFLAGS += -DHAVE_VKEY
endif
LOCAL_SRC_FILES  +=	$(RARCH_DIR)/griffin/griffin.c

ifeq ($(HAVE_LOGGER), 1)
   LOCAL_CFLAGS += -DHAVE_LOGGER
   LOGGER_LDLIBS := -llog
endif

ifeq ($(GLES),3)
   GLES_LIB := -lGLESv3
   LOCAL_CFLAGS += -DHAVE_OPENGLES3
else
   GLES_LIB := -lGLESv2
endif

LOCAL_CFLAGS += -Wall -pthread -Wno-unused-function -fno-stack-protector -funroll-loops -DRARCH_MOBILE -DHAVE_GRIFFIN -DANDROID -DHAVE_DYNAMIC -DHAVE_OPENGL -DHAVE_FBO -DHAVE_OVERLAY -DHAVE_OPENGLES -DHAVE_OPENGLES2 -DGLSL_DEBUG -DHAVE_DYLIB -DHAVE_GLSL -DHAVE_MENU -DHAVE_RGUI -DHAVE_ZLIB -DHAVE_RPNG -DINLINE=inline -DLSB_FIRST -DHAVE_THREADS -D__LIBRETRO__ -DHAVE_RSOUND -DHAVE_NETPLAY -DHAVE_NETWORKING -DRARCH_INTERNAL -DHAVE_FILTERS_BUILTIN -DHAVE_GLUI -DHAVE_XMB -std=gnu99 -DHAVE_LIBRETRODB -DHAVE_STB_FONT
LOCAL_CFLAGS += -DHAVE_7ZIP -DHAVE_ZLIB_DEFLATE

ifeq ($(NDK_DEBUG),1)
LOCAL_CFLAGS += -O0 -g
else
LOCAL_CFLAGS += -O2 -DNDEBUG
endif

LOCAL_LDLIBS	:= -L$(SYSROOT)/usr/lib -landroid -lEGL $(GLES_LIB) $(LOGGER_LDLIBS) -ldl
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(RARCH_DIR)/libretro-common/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(RARCH_DIR)/

LOCAL_CFLAGS += -DHAVE_SL
LOCAL_LDLIBS += -lOpenSLES -lz

include $(BUILD_SHARED_LIBRARY)

