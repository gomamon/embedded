
#define TEXT_ENG_MODE 0
#define TEXT_NUM_MODE 1

struct text_editor_data {
	int cnt;
    int mode;
    unsigned char text[9];    
    int idx;
};

