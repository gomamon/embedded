#include "main.h"
#include "clock.h"


void mode_clock(int sw, int inflag, struct clock_data *clock_info, struct outbuf* buf_out){
    struct tm *tm;
    time_t raw_t = time(NULL); //get raw time
    tm = localtime(&raw_t);


    if(inflag ==0){
        switch(clock_info->clock_mode){
            case CLOCK_SHOW:
                buf_out->led = 128;
                break;
            case CLOCK_MODI:
                if(buf_out->led == 32)buf_out->led = 16;
                else if(buf_out->led == 16) buf_out->led = 32;
                break;                
        }
    }
    else{
        switch(sw){
            case SW1:  // chage board time(led 3/4) or save;
                switch (clock_info->clock_mode) {
                    case CLOCK_SHOW:
                        clock_info->clock_mode=CLOCK_MODI;
                        buf_out->led = 16;
                        break;
                    case CLOCK_MODI:
                        clock_info->clock_mode=CLOCK_SHOW;
                        buf_out->led = 128;
                        break;
                }
                break;
            case SW2: // reset board time;
                clock_info->clock_mode=CLOCK_MODI;
                buf_out->led = 16;
                clock_info->clock_h = 0;
                clock_info->clock_h = 0;
                break;
            case SW3: // hour++;
                clock_info->clock_mode=CLOCK_MODI;
                buf_out->led = 16;
                clock_info->clock_h = (clock_info->clock_h + 1)%25;
                break;
            case SW4: // minute ++;
                clock_info->clock_mode=CLOCK_MODI;
                buf_out->led = 16;
                clock_info->clock_m = (clock_info->clock_m + 1)%61;
                break;
        
        }
        
    }
    //save formatting string for time to buffer.
    sprintf(buf_out->fnd,"%02d%02d",(tm->tm_hour+clock_info->clock_h)%25,(tm->tm_min+clock_info->clock_m)%61);
    return;
}

