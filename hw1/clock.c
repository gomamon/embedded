#include "main.h"
#include "clock.h"


void mode_clock(int sw, int inflag, struct clock_data *clock_info, int* buf_data1, struct databuf* buf_data2){
    struct tm *tm;
    time_t raw_t = time(NULL); //get raw time
    tm = localtime(&raw_t);


    if(inflag ==0){
        switch(clock_info->clock_mode){
            case CLOCK_SHOW:
                *buf_data1 = 128;
                break;
            case CLOCK_MODI:
                if(*buf_data1 == 32)*buf_data1 = 16;
                else if(*buf_data1 == 16) *buf_data1 = 32;
                break;                
        }
    }
    else{
        switch(sw){
            case SW1:  // chage board time(led 3/4) or save;
                switch (clock_info->clock_mode) {
                    case CLOCK_SHOW:
                        clock_info->clock_mode=CLOCK_MODI;
                        *buf_data1 = 16;
                        break;
                    case CLOCK_MODI:
                        clock_info->clock_mode=CLOCK_SHOW;
                        *buf_data1 = 128;
                        break;
                }
                break;
            case SW2: // reset board time;
                clock_info->clock_mode=CLOCK_MODI;
                *buf_data1 = 16;
                clock_info->clock_h = 0;
                clock_info->clock_h = 0;
                break;
            case SW3: // hour++;
                clock_info->clock_mode=CLOCK_MODI;
                *buf_data1 = 16;
                clock_info->clock_h = (clock_info->clock_h + 1)%25;
                break;
            case SW4: // minute ++;
                clock_info->clock_mode=CLOCK_MODI;
                *buf_data1 = 16;
                clock_info->clock_m = (clock_info->clock_m + 1)%61;
                break;
        
        }
        
    }
    sprintf((buf_data2)->d_buf,"%02d%02d",tm->tm_hour+clock_info->clock_h,tm->tm_min+clock_info->clock_m);
    (buf_data2)->d_nread = 4;
    return;
}

