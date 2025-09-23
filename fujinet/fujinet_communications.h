#include <stdbool.h>


void *fujinet_communications_thread(void *p);
void start_communications_thread(void);
char get_apple_device_id(int adam_id);
bool fujinet_init(int adam_device_id, void **response);
bool fujinet_reset(int adam_device_id, void **response);
bool validate_fujinet_response(unsigned char *raw_data, int size);
unsigned char *get_status_response(void *pqueue_item);
unsigned char *get_data_response(void *pqueue_item);
void set_message_processed(void *pqueue_item);
int wait_for_response(void **pqueue_item);
