#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* current clock mode */
#define CLOCK_SHOW 0
#define CLOCK_MODI 1 


/*struct for clock mode*/
struct clock_data {
	int clock_mode;	// current clock mode (CLOCK_SHOW or CLOCK_MODI)
	int clock_h; 	// diffrence of hours between system time and modified time
    int clock_m;	// diffrence of minutes between system time and modified time
	int unsaved_h;	// not saved modifying hour in CLOCK_MODI mode
	int unsaved_m;	// not saved modifying minutes in CLOCK_MODI mode
};


void mode_clock(int sw, int inflag, struct clock_data *clock_info,struct outbuf* buf_out);
void init_clock_info(struct clock_data *clock_info);