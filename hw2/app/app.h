#define MAJOR_NUM 242

#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 0, int)
#define IOCTL_COMMAND _IOW(MAJOR_NUM, 1,char *)