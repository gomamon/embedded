#include "main.h"
#include "textEditor.h"

/* A, 1 data for showing in dot matrix */
unsigned char dot_A[10] = {0x1c, 0x36, 0x63, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x63};
unsigned char dot_1[10] = {0x0c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3f, 0x3f};

/* alphabet map for indicating character by switch */
unsigned char alphamap[9][3] = {{'.','Q','Z'},{'A','B','C'},{'D','E','F'},{'G','H','I'},{'J','K','L'},{'M','N','O'},{'P','R','S'},{'T','U','V'},{'W','X','Y'}};


void mode_text_editor(int sw, int inflag, struct text_editor_data *text_editor_info, struct outbuf* buf_out){
    
    /*  MODE 3 : Text editor Mode
        * For showing character by input, save formatted data to output buffer to control device  
        *  
        * <parameter>
        * sw : input switch code
        * inflag : check exist input
        * text_editor_info : save the text editor information
        * buf_out: output buffer to control devices (shared mem)   
    */

    int curmode = text_editor_info->mode;
    int curidx = text_editor_info->idx;
    int curcnt = text_editor_info->cnt;
    int swnum, i;

    if(inflag == 1){
        switch(sw){
            case KEY_VOL: // enter the text mode 
                /*initializing text editor information*/
                init_text_editor_info(text_editor_info);
                buf_out->led = 0;
                break;
            case (SW2|SW3): // text LCD clear
                /* make text clear and reset count and index*/
                for(i=0; i<MAX_TEXT_LCD ; i++)
                    (text_editor_info->text)[i] = 0 ;
                text_editor_info->idx = 0;
                text_editor_info->cnt = 0;
                break;
            case (SW5|SW6): //change mode (ENG <-> NUM)
                text_editor_info->mode = (curmode+1)%2;
                break;
            case (SW8|SW9): //space
                /* add to ' ' on current index */
                text_editor_info->text[curidx] = ' ';
                text_editor_info->idx += 1; 
                text_editor_info->cnt = (curcnt+1)%10000;
                break;
            case SW1:
            case SW2:
            case SW3:
            case SW4:
            case SW5:
            case SW6:
            case SW7:
            case SW8:
            case SW9: // If input switch SW1 ~ SW9, add character 
                if((swnum = get_switch_num(sw))==-1)  break; // get switch number from switch code
                text_editor_info->cnt = (curcnt+1)%10000; //make counter less than 10000
                
                if(curmode == TEXT_NUM_MODE){ 
                /* If TEXT_NUM_MODE, add number on index of text*/
                    text_editor_info->text[curidx] = '0'+swnum;
                    text_editor_info->idx += 1;
                }else{
                /* If TEXT_ENG_MODE, add english on index of text*/
                    char bef_char = '0';
                    if(curidx >0) bef_char = text_editor_info->text[curidx-1]; //get before character
                    if(curidx==0 ||  (bef_char != alphamap[swnum-1][0] && bef_char != alphamap[swnum-1][1] )){ 
                        /* add new character */
                        text_editor_info->text[curidx] = alphamap[swnum-1][0];
                        text_editor_info->idx += 1;
                    }else{  
                        /* change latest character if in same button*/
                        if(bef_char == alphamap[swnum-1][0])
                            text_editor_info->text[curidx-1] = alphamap[swnum-1][1];
                        else if(bef_char == alphamap[swnum-1][1])
                            text_editor_info->text[curidx-1] = alphamap[swnum-1][2];
                    }
                } 
                break;
            default:
                break;
        }
    }

    /* charater move to left move when the number of text is over 32 */
    if(text_editor_info->idx > 32){
        int i;
        for(i=1; i<=32; i++){
            text_editor_info->text[i-1] = text_editor_info->text[i];
        }
        text_editor_info->text[32] = 0;
        text_editor_info->idx = 32;
    }

    /* save to output buffer of dot matrix for showing 'A' or '1' by mode*/
    switch(text_editor_info->mode){
        case TEXT_NUM_MODE:
            strncpy(buf_out->dot_matrix,dot_1,10);
            break;
        case TEXT_ENG_MODE:
            strncpy(buf_out->dot_matrix, dot_A,10);
            break;
    }

    /* make data into format and save the data to buf_out*/
    sprintf(buf_out->fnd,"%04d",text_editor_info->cnt);
    sprintf(buf_out->text_lcd,"%s",text_editor_info->text);
    

    return;
}


int get_switch_num(int code){
    int i;
    int cmp = 1;
    for(i=0; i<9; i++){
        if((cmp & code) == cmp)
            return 9-i;
        code = code >> 1;
    }
    return -1;
}

void init_text_editor_info(struct text_editor_data *text_editor_info){
    /* initialize text editor information */
    text_editor_info->cnt = 0;
	text_editor_info->mode = TEXT_ENG_MODE;
	memset(text_editor_info->text , 0, MAX_TEXT_LCD);
	text_editor_info->idx = 0;
}