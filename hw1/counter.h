/* struct data for counter */
struct counter_data {
	int number; //number
    int system; //numerial system
};

/* numerial system */
#define CNT_BIN 1
#define CNT_DEC 2
#define CNT_OCT 3
#define CNT_QUA 4


void mode_counter(int sw, int inflag, struct counter_data *counter_info, struct outbuf* buf_out);
void init_counter_info(struct counter_data *counter_info);