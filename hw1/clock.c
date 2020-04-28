#include "main.h"
#include "clock.h"


void mode_clock(int sw, int inflag, struct clock_data *clock_info, struct outbuf* buf_out){
    struct tm *tm;
    time_t raw_t = time(NULL); //get raw time
    tm = localtime(&raw_t);


    if(inflag !=0){
        switch(sw){
            case KEY_VOL:
                clock_info->clock_mode = CLOCK_SHOW;
                break;
            case SW1:  // chage board time(led 3/4) or save;
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
            case SW2: // reset to board time;
                if(clock_info->clock_mode==CLOCK_MODI){
                    clock_info->unsaved_h = tm->tm_hour;
                    clock_info->unsaved_m = tm->tm_min ;
                }
                break;
            case SW3: // hour++;
                if(clock_info->clock_mode==CLOCK_MODI){
                    clock_info->unsaved_h = (clock_info->unsaved_h + 1)%24;
                }
                break;
            case SW4: // minute ++;
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

     switch(clock_info->clock_mode){
        case CLOCK_SHOW:
            sprintf(buf_out->fnd,"%02d%02d",((tm->tm_hour+clock_info->clock_h)+(tm->tm_min+clock_info->clock_m)/60)%24,(tm->tm_min+clock_info->clock_m)%60);
            buf_out->led = 128;
            break;
        case CLOCK_MODI:
            sprintf(buf_out->fnd,"%02d%02d",clock_info->unsaved_h,clock_info->unsaved_m);
            if(tm->tm_sec%2 == 0)buf_out->led = 16;
            else buf_out->led = 32;
            break;                
    }

    //save formatting string for time to buffer.
    
    /* dot_mat & text_led set empty*/
    memset(buf_out->dot_matrix,0x00,MAX_DOT_MATRIX);
    memset(buf_out->text_lcd, 0, MAX_TEXT_LCD);

    return;
}

void init_clock_info(struct clock_data *clock_info){
    clock_info->clock_h =0;
	clock_info->clock_m =0;
	clock_info->clock_mode = 0;
    clock_info->unsaved_h =0;
	clock_info->unsaved_m =0;
}
