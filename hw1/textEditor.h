
#define TEXT_ENG_MODE 0
#define TEXT_NUM_MODE 1

struct text_editor_data {
	int cnt;
    int mode;
    unsigned char text[MAX_TEXT_LCD];    
    int idx;
};

void mode_text_editor(int sw, int inflag, struct text_editor_data *text_editor_info, struct outbuf* buf_out);
int get_switch_num(int code);