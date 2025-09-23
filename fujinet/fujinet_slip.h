#include <stdbool.h>

#define FRAME_END  (unsigned char) '\xC0'
#define FRAME_ESCAPE (unsigned char) '\xDB'
#define ESCAPE_TERMINATOR_IN_DATA "\xDB\xDC"
#define ESCAPE_ESCAPE_IN_DATA "\xDB\xDD"
#define MAX_MESSAGE_SIZE 1024

#define SLIP_CMD_STATUS_REQUEST     (unsigned char) 0
#define SLIP_CMD_READBLOCK_REQUEST 1
#define SLIP_CMD_WRITE_BLOCK_REQUEST 2
#define SLIP_CMD_FORMAT_REQUEST 3
#define SLIP_CMD_CONTROL_REQUEST 4
#define SLIP_CMD_INIT_REQUEST 5
#define SLIP_CMD_OPEN_REQUEST 6
#define SLIP_CMD_CLOSE_REQUEST 7
#define SLIP_CMD_READ_REQUEST 8
#define SLIP_CMD_WRITE_REQUEST 9
#define SLIP_CMD_RESET_REQUEST 0xA

#define SLIP_RESPONSE_SEQUENCE 0
#define SLIP_RESPONSE_STATUS   1

#define SEND_SEQUENCE_NUMBER (0)
#define SEND_CMD (1)
#define SEND_SMARTPORT_NUMBER (2)
#define SEND_BYTE_COUNT (3)
#define SEND_ADDRESS (5)
#define SEND_DATA (8)

#define RESPONSE_SEQUENCE_NUMBER (0)
#define RESPONSE_STATUS (1)
#define RESPONSE_DATA (2)

#define STATUS_OK (0)

#define MAX_MESSAGE_QUEUE (30)

#define MESSAGE_EXPIRY_PERIOD (10)

typedef struct _MESSAGE_QUEUE
{
    unsigned char message[MAX_MESSAGE_SIZE];
    int message_size;
    unsigned char response[MAX_MESSAGE_SIZE];
    int response_size;
    bool response_received;
    time_t time_added;
    bool auto_process;
    bool processed;
} MESSAGE_QUEUE;

void *create_socket_thread(void *p);
void *communications_thread(void *p);
void *dead_message_thread(void *p);
int sequence_number(bool next);
void create_slip_message(unsigned char *data_in, int data_in_size, unsigned char *data_out, int *data_out_size);
void create_escaped_data(unsigned char *data_in, int data_in_size, unsigned char *data_out, int *data_out_size);
void get_raw_data(unsigned char *data_in, int data_in_size, unsigned char *data_out, int *data_out_size, int *end_of_message);
bool send_via_slip(unsigned char *raw_data, int in_size, void **queue);
ssize_t receive_via_slip(unsigned char *raw_data, int buffersize);
bool communications_are_active();
void process_received_message(unsigned char *raw_message, int size);
void process_init_response(MESSAGE_QUEUE *queue_item);
void process_reset_response(MESSAGE_QUEUE *queue_item);
void message_lock(char *where);
void message_unlock(char *where);


