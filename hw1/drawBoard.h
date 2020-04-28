
/*define for map updated*/
#include <time.h>

#define BOARD_UPDATED 1   
#define BOARD_UPDATED_NOT 0

#define CURSOR_ON 1
#define CURSOR_OFF 0


struct draw_board_data{
	unsigned char board_map[10][7];
    int cursor_y;
    int cursor_x;
    int cursor_on;
    int cnt;
};


void mode_draw_board(int sw, int inflag, struct draw_board_data *draw_board_info, struct outbuf* buf_out);
void init_draw_board_info(struct draw_board_data *draw_board_info);
unsigned char get_dot_line_num(unsigned char line[7]);