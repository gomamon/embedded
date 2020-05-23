
obj-m	:= dev_driver.o

KDIR	:=  /work/achroimx_kernel
PWD		:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
push:
	adb push dev_driver.ko /data/local/tmp
clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers




# obj-m   := dev_driver.o
# MOD    := $(subst .o,.ko,$(obj-m))

# KDIR   :=  /work/achroimx_kernel
# PWD      := $(shell pwd)

# ADB         := adb
# ADB_DIR      := /data/local/tmp
# ADB_INSMOD   := "insmod $(ADB_DIR)/$(MOD)"
# ADB_RMMOD   := "rmmod $(ADB_DIR)/$(MOD)"

# default:
#    $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules 

# push: default
#    $(ADB) push $(MOD) $(ADB_DIR)
#    $(ADB) shell $(ADB_RMMOD)
#    $(ADB) shell $(ADB_INSMOD)

# clean:
#    rm -rf *.o
#    rm -rf *.ko
#    rm -rf *.mod.c
#    rm -rf *.order
#    rm -rf *.symvers
#    rm -rf .*.cmd
#    rm -rf .tmp_versions