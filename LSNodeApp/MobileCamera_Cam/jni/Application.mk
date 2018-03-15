 APP_ABI := armeabi armeabi-v7a
 APP_CFLAGS += -DPLATFORM_ARMV4I -D_SXDEF_CPU_80X86=0 -D_SXDEF_LITTLE_ENDIAN=1


# APP_ABI := x86
# APP_CFLAGS += -D_M_IX86 -D_SXDEF_CPU_80X86=1 -D_SXDEF_LITTLE_ENDIAN=1


ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
APP_CFLAGS += -DPLATFORM_ARMV7
endif


###############################
# ÊÇ·ñÎª51»¥Öú°²×¿¹Ò»ú¶Ë±àÒë  #
###############################
  APP_CFLAGS += -DFOR_WL_YKZ
# APP_CFLAGS += -DFOR_51HZ_GUAJI
# APP_CFLAGS += -DFOR_MAYI_GUAJI


APP_CFLAGS += -DANDROID_NDK

APP_CFLAGS += -Wno-error=format-security

APP_STL := gnustl_static
APP_PLATFORM := android-9
