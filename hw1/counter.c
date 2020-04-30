#include "main.h"
#include "counter.h"

void mode_counter(int sw, int inflag, struct counter_data *counter_info, struct outbuf* buf_out){
    /* 
        MODE 2 : Counter Mode
        * For showing count number by numeral system, control and Save formatted data to output buffer(buf_out)
        * 
        * <parameter>
        * sw : input switch code
        * inflag : check exist input
        * counter_info : save the coutner information
        * buf_out: output buffer to control devices (shared mem)   
    */
    
    int i, tmp;
    if(inflag == 1){
        //switch updated;
        switch(sw){
            case KEY_VOL: //When enter the counter mode
                //dot_mat & text_led set empty
                memset(buf_out->dot_matrix,0x00,MAX_DOT_MATRIX);
                memset(buf_out->text_lcd, 0, MAX_TEXT_LCD);
                // initializing counter information data
                init_counter_info(counter_info);
                break;
            case SW1:
                // change numberic system
                if(counter_info->system == 2)  counter_info->system = 10;
                else if(counter_info->system == 8) counter_info->system = 4;
                else counter_info->system -= 2;
                break;
            case SW2:
                // increase 3nd digit
                counter_info->number +=  (counter_info->system)* (counter_info->system);
                break;
            case SW3:
                // increase 2nd digit
                counter_info->number += (counter_info->system);
                break;
            case SW4: 
                // increase first digit
                counter_info->number += 1;
        }
    }
    
    /* save the formatting data by numeral system  */
    switch(counter_info->system){
        case 2: // binary
            i=0;
            /* make number less than 16 (4 digit binary) */
            counter_info->number = (counter_info->number)%16;
            tmp = (counter_info->number);
            sprintf(buf_out->fnd, "0000");
            /* compute and save a binary number*/
            while(tmp>0){
                (buf_out->fnd)[3-i] = '0'+tmp%2;
                tmp = tmp/2;
                i++;
            }
            /* save output buffer for first led on*/
            buf_out->led = 1 << 7;
            break;
        case 10: // decimal
            /* compute and save number to decimal (not over 999)*/
            counter_info->number = (counter_info->number)%1000;
            sprintf(buf_out->fnd, "%04d",(counter_info->number));

            /* save output buffer for second led on*/
            buf_out->led = 1 << 6;
            break;
        case 8: // octagonal
            /* compute and save number to octagonal (4 digit octagonal) */
            counter_info->number = (counter_info->number)%4096;
            sprintf(buf_out->fnd,"%04o",(counter_info->number));
            /* save output buffer for third led on*/
            buf_out->led = 1 <<5;
            break;
        case 4: //quadratic
            i=0;
             /* make number less than 256 (4 digit quadratic) */
            counter_info->number = (counter_info->number)%256;

            /* compute and save quadratic number*/
            tmp = (counter_info->number);
            sprintf(buf_out->fnd, "0000");
            while(tmp>0){
                (buf_out->fnd)[3-i] = '0'+tmp%4;
                tmp = tmp/4;
                i++;
            }
            /* save output buffer for fourth led on*/
            buf_out->led = 1 << 4;
            break;
    }

    return;
}

void init_counter_info(struct counter_data *counter_info){
    /* initializing counter information */
	counter_info->number = 0;
	counter_info->system = 10;
}