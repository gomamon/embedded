#include "main.h"
#include "drawBoard.h"

unsigned char get_dot_line_num(unsigned char line[7]) {
    int i = 0;
    int num = 0;
    for (i = 0; i < 7; i++) {
        num = num << 1;
        num += line[i];
    }
    return num;
}


void mode_draw_board(int sw, int inflag, struct draw_board_data *draw_board_info, struct outbuf *buf_out) {
    int i, j;
    int update = BOARD_UPDATED_NOT;
    int cur_curx = draw_board_info->cursor_x;
    int cur_cury = draw_board_info->cursor_y;
    unsigned char line[7];

    struct tm *tm;
    time_t raw_t = time(NULL); //get raw time
    tm = localtime(&raw_t);

    if (inflag) {
        update = BOARD_UPDATED;
        switch (sw) {
            case KEY_VOL:
            case SW1:  // reset image
                init_draw_board_info(draw_board_info);
                break;
            case SW2:   // UP
                (cur_cury)--;
                draw_board_info->cursor_y = (cur_cury < 0) ? 0 : cur_cury;
                draw_board_info->cnt++;
                break;
            case SW3:   // cursor ON/OFF
                if(draw_board_info->cursor_on == CURSOR_OFF)
                    draw_board_info->cursor_on = CURSOR_ON;
                else draw_board_info->cursor_on = CURSOR_OFF;
                draw_board_info->cnt++;
                break;
            case SW4:   //LEFT
                cur_curx--;
                draw_board_info->cursor_x = (cur_curx < 0) ? 0 : cur_curx;
                draw_board_info->cnt++;
                break;
            case SW5:   // SELECT
                (draw_board_info->board_map)[cur_cury][cur_curx] = 1;
                draw_board_info->cnt++;
                break;
            case SW6:
                cur_curx++;
                draw_board_info->cursor_x = (cur_curx > 6) ? 6 : cur_curx;
                draw_board_info->cnt++;
                break;
            case SW7:
                for (i = 0; i < 10; i++){
                    for(j=0; j< 7; j++){
                        (draw_board_info->board_map)[i][j] = 0;
                    }
                }
                draw_board_info->cnt++;
                break;
            case SW8:
                (cur_cury)++;
                draw_board_info->cursor_y = (cur_cury > 9) ? 9 : cur_cury;
                draw_board_info->cnt++;
                break;
            case SW9:
                for (i = 0; i < 10; i++){
                    for(j=0; j< 7; j++){
                        (draw_board_info->board_map)[i][j] = ((draw_board_info->board_map)[i][j] == 1) ? 0 : 1;
                    }
                }
                draw_board_info->cnt++;
                break;
        }
    }

    /* count */
    (draw_board_info->cnt)%=10000;
    sprintf(buf_out->fnd,"%04d",draw_board_info->cnt);

    /* board */
    if (update == BOARD_UPDATED) {
        for (i = 0; i < 10; i++) {
            buf_out->dot_matrix[i] = get_dot_line_num((draw_board_info->board_map)[i]);
        }
    }

    /* cursor */
    if(draw_board_info->cursor_on == CURSOR_ON){
        int tmp_linenum;
        if(tm->tm_sec%2 == 0){
            /* set cursor to map*/
            tmp_linenum = get_dot_line_num(draw_board_info->board_map[draw_board_info->cursor_y]);
            tmp_linenum = tmp_linenum | (1<<6-draw_board_info->cursor_x);
            (buf_out->dot_matrix)[draw_board_info->cursor_y] =  tmp_linenum;
        }else{
            tmp_linenum = get_dot_line_num(draw_board_info->board_map[draw_board_info->cursor_y]);
            tmp_linenum = tmp_linenum & 0x7f - (1<<6-draw_board_info->cursor_x);
            (buf_out->dot_matrix)[draw_board_info->cursor_y] =  tmp_linenum;
        }
    }

}

void init_draw_board_info(struct draw_board_data *draw_board_info) {
    int i,j;
    for (i = 0; i < 10; i++)
        for(j =0; j<7 ;j++) (draw_board_info->board_map)[i][j] = 0x00;
    draw_board_info->cursor_x = 0;
    draw_board_info->cursor_y = 0;
    draw_board_info->cnt = 0;
    draw_board_info->cursor_on = CURSOR_ON;
}