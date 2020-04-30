#include "main.h"
#include "drawBoard.h"

void mode_draw_board(int sw, int inflag, struct draw_board_data *draw_board_info, struct outbuf *buf_out) {
    /* 
        MODE 4 : Draw Board Mode
        * Counting the input button in this mode 
        * Draw dot by cursor to dot matrix devices
        * 
        * <parameter>
        * sw : input switch code
        * inflag : check exist input
        * draw_board_info : save the draw information
        * buf_out: output buffer to control devices   
    */
    
    int i, j;
    int update = BOARD_UPDATED_NOT;
    int cur_curx = draw_board_info->cursor_x;
    int cur_cury = draw_board_info->cursor_y;
    unsigned char line[7];

    /* time for cursor blinking */
    struct tm *tm;
    time_t raw_t = time(NULL); //get raw time
    tm = localtime(&raw_t);

    if (inflag) {
        update = BOARD_UPDATED;
        switch (sw) {
            case KEY_VOL:  // enter draw board mode
            case SW1:  // reset image
                /* reset data and reset draw board data*/
                buf_out->led = 0;
                buf_out->text_lcd[0] = 0;
                init_draw_board_info(draw_board_info);
                break;
            case SW2:   // UP
                /* move cursor y to up and increase count*/
                (cur_cury)--;
                draw_board_info->cursor_y = (cur_cury < 0) ? 0 : cur_cury;
                draw_board_info->cnt++;
                break;
            case SW3:   // cursor ON/OFF
                /* change cursor mode and increase count */
                if(draw_board_info->cursor_on == CURSOR_OFF)
                    draw_board_info->cursor_on = CURSOR_ON;
                else draw_board_info->cursor_on = CURSOR_OFF;
                draw_board_info->cnt++;
                break;
            case SW4:   //LEFT
                /* change cursor x to left and increase count */
                cur_curx--;
                draw_board_info->cursor_x = (cur_curx < 0) ? 0 : cur_curx;
                draw_board_info->cnt++;
                break;
            case SW5:   // SELECT
                /* draw dot by coordiate of cursor and increase count*/
                (draw_board_info->board_map)[cur_cury][cur_curx] = 1;
                draw_board_info->cnt++;
                break;
            case SW6:
                /* change cursor x to right and incrase count */
                cur_curx++;
                draw_board_info->cursor_x = (cur_curx > 6) ? 6 : cur_curx;
                draw_board_info->cnt++;
                break;
            case SW7:   //clear
                /* clear map and increase count*/
                for (i = 0; i < 10; i++){
                    for(j=0; j< 7; j++){
                        (draw_board_info->board_map)[i][j] = 0;
                    }
                }
                draw_board_info->cnt++;
                break;
            case SW8:   //DOWN
                /* change cursor to down and increase count */
                (cur_cury)++;
                draw_board_info->cursor_y = (cur_cury > 9) ? 9 : cur_cury;
                draw_board_info->cnt++;
                break;
            case SW9: //reverse
                /* reverse dot in board ( 0->1 , 1->0 ) and increase count */
                for (i = 0; i < 10; i++){
                    for(j=0; j< 7; j++){
                        (draw_board_info->board_map)[i][j] = ((draw_board_info->board_map)[i][j] == 1) ? 0 : 1;
                    }
                }
                draw_board_info->cnt++;
                break;
        }
    }

    /* count : save formatted count data to output buffer for output fnd device */
    (draw_board_info->cnt)%=10000;
    sprintf(buf_out->fnd,"%04d",draw_board_info->cnt);

    /* board : save formatted board map data to output buffer for output dot matrix device*/
    if (update == BOARD_UPDATED) {
        for (i = 0; i < 10; i++) {
            buf_out->dot_matrix[i] = get_dot_line_num((draw_board_info->board_map)[i]);
        }
    }

    /* cursor : add draw cursor to output buffer for output dot matrix device */
    if(draw_board_info->cursor_on == CURSOR_ON){
        int tmp_linenum;
        if(tm->tm_sec%2 == 0){
            // add dot in cursor coordinate
            tmp_linenum = get_dot_line_num(draw_board_info->board_map[draw_board_info->cursor_y]);
            tmp_linenum = tmp_linenum | (1<<6-draw_board_info->cursor_x);
            (buf_out->dot_matrix)[draw_board_info->cursor_y] =  tmp_linenum;
        }else{
            // remove dot in cursor coordinate
            tmp_linenum = get_dot_line_num(draw_board_info->board_map[draw_board_info->cursor_y]);
            tmp_linenum = tmp_linenum & 0x7f - (1<<6-draw_board_info->cursor_x);
            (buf_out->dot_matrix)[draw_board_info->cursor_y] =  tmp_linenum;
        }
    }


}


unsigned char get_dot_line_num(unsigned char line[7]) {
    /* compute formatted number from dot map line to control dot matrix device */
    int i = 0;
    int num = 0;
    for (i = 0; i < 7; i++) {
        num = num << 1;
        num += line[i];
    }
    return num;
}


void init_draw_board_info(struct draw_board_data *draw_board_info) {
    /* initializing draw_board_info struct data*/
    int i,j;
    for (i = 0; i < 10; i++)
        for(j =0; j<7 ;j++) (draw_board_info->board_map)[i][j] = 0x00;
    draw_board_info->cursor_x = 0;
    draw_board_info->cursor_y = 0;
    draw_board_info->cnt = 0;
    draw_board_info->cursor_on = CURSOR_ON;
}