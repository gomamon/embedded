LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:=blackjack
LOCAL_PRELINK_MODULE:=false
LOCAL_SRC_FILES:=driver.c
LOCAL_LDLIBS := -llog
#LOCAL_LDLIB := -L$(SYSROOT)/usr/lib -llog
#LOCAL_LDFLAGS += $(TARGET_OUT_INTERMEDIATE_LIBRARIES)/liblog.so

include $(BUILD_SHARED_LIBRARY)