#include "main.h"
#include "clock.h"


void mode_clock(int sw, int buf_in, int** buf_data1, struct databuf** buf_data2){
    struct tm *tm;
    time_t raw_t = time(NULL); //get raw time
    tm = localtime(&raw_t);
    printf("sw: %d, buf_in:%d\n", buf_in);
    switch(sw){
        case SW1: //chage board time(led 3/4) or save;
            if(buf_in == 0){
                if(**buf_data1 == 128){ //show time
                    sprintf((*buf_data2)->d_buf,"%02d%02d",tm->tm_hour,tm->tm_min);
                    printf("time! %s", (*buf_data2)->d_buf);
                    (*buf_data2)->d_nread = strlen((*buf_data2)->d_buf);
                }
                else if( **buf_data1 == 32 ||  **buf_data1 == 16){//modify
                    if(**buf_data1 == 32)**buf_data1 = 16;
                    else if(**buf_data1 == 16) **buf_data1 = 32;
                }
            }
            else
            {
                if(**buf_data1 == 128) **buf_data1 = 32;
                else if( **buf_data1 == 32 ||  **buf_data1 == 16) **buf_data1 == 128;
            }
            break;
        case SW2: // reset board time;
        case SW3: // h++
        case SW4:
        break;
    }
}

