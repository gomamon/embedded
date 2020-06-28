#include <jni.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "android/log.h"

#define LOG_TAG "MyTag"
#define LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

/*struct for dealer data*/
struct gamer{
	int status;
	int cards[12];
	int point;
	int idx;
	int money;
};

jint JNICALL Java_com_example_blackjack_MainActivity_driverOpen(JNIEnv * env, jobject this){
	/* open device driver file and get file description */
	int fd=open("/dev/blackjack", O_RDWR);
	if (fd == -1) {
		LOGV("OPEN S!");
	}

	return fd;
}

jint JNICALL Java_com_example_blackjack_MainActivity_driverClose(JNIEnv *env, jobject this, jint fd){
	/* close file */
	return close(fd);
}

jintArray JNICALL Java_com_example_blackjack_MainActivity_getDealerData(JNIEnv *env, jobject this, jint fd){
	/* get dealer's data */
	struct gamer dealer;
	int val, i;
	jintArray arr =  (*env)->NewIntArray(env,20);	//create jintArray for jni
	int buff[3] = {0,};
	jint *data = (*env)->GetIntArrayElements(env,arr, NULL); //change data type from array to jint*

	val = write(fd, buff, 2);	//write -> sleep by kernel until some invent in kernel
	read(fd, &(dealer), sizeof(struct gamer));	//read data

	/* move dealer's data to data array */
	data[0] = dealer.point;
	data[1] = dealer.idx;
	data[2] = dealer.money;
	data[3] = dealer.status;
	for(i=0; i<data[1];i++){
		data[i+4] = dealer.cards[i];	//get all cards
	}
	(*env)->ReleaseIntArrayElements(env, arr, data, 0);	//change data type from jint* to array
	return arr;
}

