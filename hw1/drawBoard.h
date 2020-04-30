
#include <time.h>

/* indicates whether data has been updated */
#define BOARD_UPDATED 1   
#define BOARD_UPDATED_NOT 0

/* indicates whether cursor showing mode */
#define CURSOR_ON 1
#define CURSOR_OFF 0

/* data struct for draw board data */
struct draw_board_data{
	unsigned char board_map[10][7]; //information about board (1 draw, 0 not draw)
    int cursor_y;   //y coordinate of cursor
    int cursor_x;   //x coordinate of cursor
    int cursor_on;  //cursor showing mode
    int cnt;        //counter
};


void mode_draw_board(int sw, int inflag, struct draw_board_data *draw_board_info, struct outbuf* buf_out);
void init_draw_board_info(struct draw_board_data *draw_board_info);
unsigned char get_dot_line_num(unsigned char line[7]);