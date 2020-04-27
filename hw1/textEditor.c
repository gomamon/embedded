#include "main.h"
#include "textEditor.h"

unsigned char dot_A[10] = {0x1c, 0x36, 0x63, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x63};
unsigned char dot_1[10] = {0x0c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3f, 0x3f};

unsigned char alphamap[9][3] = {{'.','Q','Z'},{'A','B','C'},{'D','E','F'},{'G','H','I'},{'J','K','L'},{'M','N','O'},{'P','R','S'},{'T','U','V'},{'W','X','Y'}};

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

void mode_text_editor(int sw, int inflag, struct text_editor_data *text_editor_info, struct outbuf* buf_out){
    int curmode = text_editor_info->mode;
    int curidx = text_editor_info->idx;
    int curcnt = text_editor_info->cnt;
    int swnum;
    if(inflag == 1){
        switch(sw){
            case (SW2|SW3): // text LCD clear
                memset(text_editor_info->text , 0, 9);
                text_editor_info->idx = 0;
                text_editor_info->cnt = 0;
                break;
            case (SW5|SW6): //change mode (ENG <-> NUM)
                text_editor_info->mode = (curmode+1)%2;
                break;
            case (SW8|SW9): //space
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
            case SW9:
                if((swnum = get_switch_num(sw))==-1)  break;
                text_editor_info->cnt = (curcnt+1)%10000;
                if(curmode == TEXT_NUM_MODE){
                    printf("NUMODE%c\n",'0'+swnum);
                    text_editor_info->text[curidx] = '0'+swnum;
                    text_editor_info->idx += 1;
                }else{
                    char bef_char = '0';
                    if(curidx >0) bef_char = text_editor_info->text[curidx-1];
                    if(curidx==0 ||  (bef_char != alphamap[swnum-1][0] && bef_char != alphamap[swnum-1][1] )){
                        text_editor_info->text[curidx] = alphamap[swnum-1][0];
                        text_editor_info->idx += 1;
                    }else{
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

    if(text_editor_info->idx > 8){
        int i;
        for(i=1; i<=8; i++){
            text_editor_info->text[i-1] = text_editor_info->text[i];
        }
        text_editor_info->idx = 8;
    }

    switch(text_editor_info->mode){
        case TEXT_NUM_MODE:
            strncpy(buf_out->dot_matrix,dot_1,10);
            break;
        case TEXT_ENG_MODE:
            strncpy(buf_out->dot_matrix, dot_A,10);
            break;
    }

    /* save text edit data to buf_out form */
   
    sprintf(buf_out->fnd,"%04d",text_editor_info->cnt);
    sprintf(buf_out->text_lcd,"%s",text_editor_info->text);
    printf("TEST: %s\n", buf_out->text_lcd);
    buf_out->led = 0;

    return;
}