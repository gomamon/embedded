#include "main.h"
#include "counter.h"
void mode_counter(int sw, int inflag, struct counter_data *counter_info, struct outbuf* buf_out){
    int i, tmp;
    printf("cnt info : %d, %d\n", counter_info->system, counter_info->number);
    if(inflag == 1){
        //switch updated;
        switch(sw){
            case KEY_VOL:
                //dot_mat & text_led set empty
                memset(buf_out->dot_matrix,0x00,MAX_DOT_MATRIX);
                memset(buf_out->text_lcd, 0, MAX_TEXT_LCD);
                //init
                init_counter_info(counter_info);
                break;
            case SW1:
                //change numberic system
                if(counter_info->system == 2)  counter_info->system = 10;
                else if(counter_info->system == 8) counter_info->system = 4;
                else counter_info->system -= 2;
                break;
            case SW2:
                counter_info->number +=  (counter_info->system)* (counter_info->system);
                break;
            case SW3:
                counter_info->number += (counter_info->system);
                break;
            case SW4:
                counter_info->number += 1;
        }
    }
    switch(counter_info->system){
        case 2:
            i=0;
            counter_info->number = (counter_info->number)%16;
            tmp = (counter_info->number);
            sprintf(buf_out->fnd, "0000");
            while(tmp>0){
                (buf_out->fnd)[3-i] = '0'+tmp%2;
                tmp = tmp/2;
                i++;
            }
            buf_out->led = 1 << 7;
            break;
        case 10:
            counter_info->number = (counter_info->number)%1000;
            sprintf(buf_out->fnd, "%04d",(counter_info->number));
            buf_out->led = 1 << 6;
            break;
        case 8:
            counter_info->number = (counter_info->number)%4096;
            sprintf(buf_out->fnd,"%04o",(counter_info->number));
            buf_out->led = 1 <<5;
            break;
        case 4:
            i=0;
            counter_info->number = (counter_info->number)%256;
            tmp = (counter_info->number);
            sprintf(buf_out->fnd, "0000");
            while(tmp>0){
                (buf_out->fnd)[3-i] = '0'+tmp%4;
                tmp = tmp/4;
                i++;
            }
            buf_out->led = 1 << 4;
            break;
        
    }



    return;
}

void init_counter_info(struct counter_data *counter_info){
	counter_info->number = 0;
	counter_info->system = 10;
}