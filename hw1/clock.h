#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define CLOCK_SHOW 0
#define CLOCK_MODI 1 

struct clock_data {
	int clock_mode;
	int clock_h; 
    int clock_m;
};


void mode_clock(int sw, int inflag, struct clock_data *clock_info,struct outbuf* buf_out);
