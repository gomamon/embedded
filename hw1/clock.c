#include "main.h"
#include "clock.h"


void mode_clock(int sw, int inflag, struct clock_data *clock_info, struct outbuf* buf_out){
    
    /* variable for system time */
    struct tm *tm;
    time_t raw_t = time(NULL);
    tm = localtime(&raw_t);

    /* When key or switch pushed */
    if(inflag !=0){
        switch(sw){
            case KEY_VOL:   
                /*Initailize devices' output and clock mode*/
                memset(buf_out->dot_matrix,0x00,MAX_DOT_MATRIX);
                memset(buf_out->text_lcd, 0, MAX_TEXT_LCD);
                clock_info->clock_mode = CLOCK_SHOW;
                break;
            case SW1:  
                /*changes clock mode*/
                switch (clock_info->clock_mode) {
                    case CLOCK_SHOW:
                        clock_info->clock_mode=CLOCK_MODI;
                        clock_info->unsaved_h = ((tm->tm_hour+clock_info->clock_h)+(tm->tm_min+clock_info->clock_m)/60)%24;
                        clock_info->unsaved_m = (tm->tm_min+clock_info->clock_m)%60 ;
                        break;
                    case CLOCK_MODI:
                        clock_info->clock_mode=CLOCK_SHOW;
                        clock_info->clock_h = clock_info->unsaved_h - tm->tm_hour;
                        clock_info->clock_m = clock_info->unsaved_m - tm->tm_min;
                        break;
                }
                break;
            case SW2: 
                /* reset to board time */
                if(clock_info->clock_mode==CLOCK_MODI){
                    clock_info->unsaved_h = tm->tm_hour;
                    clock_info->unsaved_m = tm->tm_min ;
                }
                break;
            case SW3: 
                /* In CLOCK_MODI mode, hour++ */
                if(clock_info->clock_mode==CLOCK_MODI){
                    clock_info->unsaved_h = (clock_info->unsaved_h + 1)%24;
                }
                break;
            case SW4:
                /* In CLOCK_MODI mode, min++ */
                if(clock_info->clock_mode==CLOCK_MODI){
                    clock_info->unsaved_m = (clock_info->unsaved_m + 1);
                    if(clock_info->unsaved_m){
                        clock_info->unsaved_h = (clock_info->unsaved_h + (clock_info->unsaved_m/60))%24;
                        (clock_info->unsaved_m)%=60;                    
                    }
                }
                break;
        }
    }

    /* Set devices' output(fnd, led) buffer to formatted data */
    switch(clock_info->clock_mode){
        case CLOCK_SHOW:
            /* set current_time(tm_hour, tm_min)+saved_time_diffrence(clock_h, clock_m) to fnd output in CLOCK_SHOW mode*/
            sprintf(buf_out->fnd,"%02d%02d",((tm->tm_hour+clock_info->clock_h)+(tm->tm_min+clock_info->clock_m)/60)%24,(tm->tm_min+clock_info->clock_m)%60);
            /* set led output in CLOCK_SHOW mode*/
            buf_out->led = 128;
            break;
        case CLOCK_MODI:
            /* set unsaved time to output in CLOCK_MODI mode*/
            sprintf(buf_out->fnd,"%02d%02d",clock_info->unsaved_h,clock_info->unsaved_m);
            /* set led output blink in CLOCK_MODI mode */
            if(tm->tm_sec%2 == 0)buf_out->led = 16; // When system sec is even, set led 4 on
            else buf_out->led = 32;                 // When system sec is odd, set led 3 on
            break;                
    }

    return;
}

void init_clock_info(struct clock_data *clock_info){
    /* Initialize clock data */
    clock_info->clock_h =0;
	clock_info->clock_m =0;
	clock_info->clock_mode = 0;
    clock_info->unsaved_h =0;
	clock_info->unsaved_m =0;
}
