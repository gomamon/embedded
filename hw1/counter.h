struct counter_data {
	int number;
    int system;
};


#define CNT_BIN 1
#define CNT_DEC 2
#define CNT_OCT 3
#define CNT_QUA 4


void mode_counter(int sw, int inflag, struct counter_data *counter_info, int* buf_data1, struct databuf* buf_data2);
