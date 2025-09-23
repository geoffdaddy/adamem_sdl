#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include "fujinet_communications.h"
#include "fujinet_slip.h"

typedef struct _FN_COMMUNICATIONS
{
    pthread_t id;
    int *adamem_socket;
    int adamem_port;
    int *fujinet_socket;
    bool is_connected;
    bool ready_to_receive;
    bool thread_stopped;
    bool *shutdown;
} FN_COMMUNICATIONS;

pthread_mutex_t queue_lock;

unsigned char fujinet_sequence_number = 0x00;
unsigned char escaped_data[MAX_MESSAGE_SIZE];

int adamem_socket = 0;
int fujinet_socket = 0;
pthread_t socket_creation_thread_id;
pthread_t dead_message_thread_id;
bool shutdown_communications = false;

MESSAGE_QUEUE message_queue[MAX_MESSAGE_QUEUE];

unsigned char last_raw_message[50][MAX_MESSAGE_SIZE];
int last_raw_message_size[50];

FN_COMMUNICATIONS fn_communications;

static unsigned long lock_count = 0;
static unsigned long unlock_count = 0;

void message_lock(char *where)
{
    lock_count++;
    printf("lock   %s (  lock_count:%ld)\n", where, lock_count);
    pthread_mutex_lock(&queue_lock);
}

void message_unlock(char *where)
{
    unlock_count++;
    printf("unlock %s (unlock_count %ld)\n", where, lock_count);
    pthread_mutex_unlock(&queue_lock);
}

void start_communications_thread(void)
{
    memset(&message_queue, 0, sizeof(message_queue));

    shutdown_communications = false;
    adamem_socket = 0;
    fujinet_socket = 0;

    fn_communications.is_connected = false;
    fn_communications.ready_to_receive = false;

    fn_communications.adamem_socket = &adamem_socket;
    fn_communications.adamem_port = 1985;
    fn_communications.fujinet_socket = &fujinet_socket;
    fn_communications.shutdown = &shutdown_communications;

    printf("Creating Sockets thread\n");
    pthread_create(&socket_creation_thread_id, NULL, &create_socket_thread, &fn_communications);

    printf("Creating communications thread\n");
    pthread_create(&fn_communications.id, NULL, communications_thread, &fn_communications);

    printf("Creating dead message thread\n");
    pthread_create(&dead_message_thread_id, NULL, dead_message_thread, NULL);
}

void stop_communications_thread(void)
{
    shutdown_communications = 1;
}

bool communications_are_active()
{
    return (bool)(fn_communications.is_connected && fn_communications.ready_to_receive);
}

void *dead_message_thread(void *p)
{
    time_t now;

    while (true)
    {
        time(&now);
        for (int queue_position = 0; queue_position < MAX_MESSAGE_QUEUE; queue_position++)
        {
            if ((message_queue[queue_position].message_size != 0) &&
                (now - message_queue[queue_position].time_added) >= MESSAGE_EXPIRY_PERIOD)
            {
                printf("*********************** message %02x purged size:%d\n", message_queue[queue_position].message[SEND_SEQUENCE_NUMBER], message_queue[queue_position].message_size);
                message_queue[queue_position].message_size = 0;
            }
        }
        sleep(1);
    }
} 

void *create_socket_thread(void *p)
{
    int opt = 1;
    char address_str[INET6_ADDRSTRLEN];
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    FN_COMMUNICATIONS *fn_communications = (FN_COMMUNICATIONS *)p;

    printf("Creating Adamem socket...\n");
    while (!shutdown_communications)
    {
        // create the socket
        if ((adamem_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            continue;
        }

        // Forcefully attaching socket to the port
        if (setsockopt(adamem_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
        {
            printf("Could not force reuse of address for spoofed applewin\n");
            exit(EXIT_FAILURE);
        }

        // accept a connection from anywhere on Spoofed_AppleWin_port
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(fn_communications->adamem_port);

        // When a socket has both an IP address and a port number
        // it is said to be 'bound to a port', or 'bound to an address'.
        // A bound socket can receive data because it has a complete address.
        // The process of allocating a port number to a socket is
        // called 'binding'.
        if (bind(*fn_communications->adamem_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            continue;
        }
        else
        {
            break;
        }
    } // while not bound

    if (!shutdown_communications)
    {
        // listen() marks the socket referred to by spoofed_applewin as a passive
        // socket, that is, as a socket that will be used to accept
        // incoming connection requests using accept(2).

        if (listen(*fn_communications->adamem_socket, 3) < 0)
        {
            printf("Adamem Listen failed\n");
            exit(-1);
        }

        if ((fujinet_socket = accept(*fn_communications->adamem_socket, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            printf("Accept fujinet connection failed\n");
            exit(-2);
        }

        // convert the received ip address to a string
        inet_ntop(AF_INET, &(address.sin_addr), address_str, INET_ADDRSTRLEN);

        printf("\n*** Received connection from Fujinet (%s:%d) ***\n\n", address_str, address.sin_port);
        fn_communications->is_connected = true;
    }

    printf("Exiting Socket Creation thread\n");
    return NULL;
}

void *communications_thread(void *p)
{
    FN_COMMUNICATIONS *fnc = (FN_COMMUNICATIONS *)p;
    unsigned char slip_message[MAX_MESSAGE_SIZE];
    struct timeval timeout;
    ssize_t size;

    fnc->thread_stopped = 0;

    printf("Communications Thread Waiting for sockets\n");

    while ((fnc->fujinet_socket == 0) || (!fnc->is_connected))
        sleep(1);

    fnc->ready_to_receive = true;
    printf("Sockets available\n");

    // timeout if we wait more than a second.
    // this is so the thread knows to stop.
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(*fnc->fujinet_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        printf("setsockopt failed\n");
    }

    while (!*fnc->shutdown)
    {
        if( (size = receive_via_slip(slip_message, sizeof(slip_message)) )) 
        {
            // process
            process_received_message(slip_message, size);
        } else
        {
            if (size == 0)
            {
                printf("size == 0\n");
                *fnc->shutdown = 1;
                break;
            }
            else
            {
                if (errno != EAGAIN)
                {
                    printf("bytes_read: %d   errno: %d\n", size, errno);
                }
            }
        }
    }
    fnc->ready_to_receive = false;
    close(*fnc->fujinet_socket);
    *fnc->fujinet_socket = 0;

    *fnc->shutdown = 1; // ask all forward threads to stop
    fnc->thread_stopped = 1;

    printf("Communications Thread Exiting...\n");
    return NULL;
}




void process_received_message(unsigned char *raw_message, int size)
{
    int queue_position;
    time_t now;

    time(&now);

    for (queue_position = 0; queue_position < MAX_MESSAGE_QUEUE; queue_position++)
    {
        
        if (message_queue[queue_position].message_size != 0)
        {
            if (message_queue[queue_position].message[SEND_SEQUENCE_NUMBER] == raw_message[RESPONSE_SEQUENCE_NUMBER])
            {
                message_lock("process_received_message");
                memcpy(message_queue[queue_position].response, raw_message, size);
                message_queue[queue_position].response_size = size;
                message_queue[queue_position].processed = false;
                message_queue[queue_position].time_added = now;
                message_queue[queue_position].response_received = true;
                message_unlock("process_received_message");

                switch (message_queue[queue_position].message[SEND_CMD])
                {
                    case SLIP_CMD_INIT_REQUEST:
                        process_init_response(&message_queue[queue_position]);
                        break;
                    case SLIP_CMD_RESET_REQUEST:
                        process_reset_response(&message_queue[queue_position]);
                        break;
                    default:
                        break;
                }
            }
        }
        
    } 
   
}

void process_init_response(MESSAGE_QUEUE *queue_item)
{
    printf("init response\n");
    if (queue_item->auto_process)
    {
        printf("init auto-processed\n");
        set_message_processed(queue_item);
    }
}

void process_reset_response(MESSAGE_QUEUE *queue_item)
{
    printf("reset response\n");
    if (queue_item->auto_process)
    {
        printf("reset auto-processed\n");
        set_message_processed(queue_item);
    }
}

bool add_to_message_queue(unsigned char *raw_message, int size, void **queue)
{
    int queue_position;
    int available_position = -1;
    unsigned char sequence_number = raw_message[SEND_SEQUENCE_NUMBER];
    time_t now;
    bool status;
    time(&now);

    message_lock("add_to_message_queue");

    // confirm sequence isn't already in queue
    for (queue_position = 0; queue_position < MAX_MESSAGE_QUEUE; queue_position++)
    {
        // if there is a really old message in the queue that hasn't been
        // processed, purge it
        // or if it's been processed
        if ((message_queue[queue_position].message_size !=0) && (message_queue[queue_position].processed))
        {
            printf("processed %02x - removing from queue\n", message_queue[queue_position].message[SEND_SEQUENCE_NUMBER]);
            message_queue[queue_position].message_size = 0;
        }

        // if we find an available position, remember it
        if ((message_queue[queue_position].message_size == 0) &&
            (available_position == -1))
            available_position = queue_position;

        // if the sequence number is already in the queue, ignore it
        if ((message_queue[queue_position].message_size !=0) &&
            (message_queue[queue_position].message[SEND_SEQUENCE_NUMBER] == sequence_number))
        {
            available_position = -1;
        }
    }

    if (available_position != -1)
    {
        printf(">>>>>>>>>>>>>>>>>>>queue %02x\n", raw_message[SEND_SEQUENCE_NUMBER]);

        memcpy(message_queue[available_position].message, raw_message, size);
        message_queue[available_position].message_size = size;
        message_queue[available_position].time_added = now;
        message_queue[available_position].auto_process = (queue == NULL);
        message_queue[available_position].processed = false;
        message_queue[available_position].response_received = false;
        

        if (queue != NULL)
            *queue = &message_queue[available_position];

        status = true;
    } else
    {
        printf("Queue full or same sequence number reused\n");
        status = false;
    }

    message_unlock("add_to_message_queue");

    return status;
}

int sequence_number(bool next)
{
    if (next)
        fujinet_sequence_number++;

    return fujinet_sequence_number;
}

bool send_via_slip(unsigned char *raw_data, int in_size, void **queue)
{
    int out_size;

    if (! add_to_message_queue(raw_data, in_size, queue))
        return false;

    //printf("->[%d]fujinet: ", in_size);
    //for (int i = 0; i < in_size; i++)
    //    printf("%02x ", raw_data[i]);
    //printf("\n");

    create_slip_message(raw_data, in_size, escaped_data, &out_size);

    printf("slip out: ");
    for (int i = 0; i < out_size; i++)
        printf("%02x ", escaped_data[i]);
    printf("\n");

    if (send(fujinet_socket, escaped_data, out_size, 0) < 0)
    {
        printf("failed.\n");
        return false;
    } 
    return true;
    
}

ssize_t receive_via_slip(unsigned char *raw_data, int buffersize)
{
static unsigned char from_last_message[MAX_MESSAGE_SIZE*2];
static unsigned char from_last_message_size;
    int success = false;
    int received_size = 0;
    ssize_t size=0;
    ssize_t end_of_message=0;

    // need to put add 'from_last_message' into escaped_data
    // so it has a full message

    received_size = recv(fujinet_socket, escaped_data, sizeof(escaped_data), 0);
    if (received_size <= 0)
    {
        size = received_size;
    } else
    {
        if (received_size != -1)
        {
            get_raw_data(escaped_data, received_size, raw_data, &size, &end_of_message);

            if (received_size > end_of_message+1)
            {
                from_last_message_size = received_size - end_of_message+1;
                memcpy(from_last_message, escaped_data[end_of_message+1], from_last_message_size);
            }
        }
    } // received_size > 0


    if (size > 0)
    {
        printf("<-[%d]fujinet: ", size);
        for (int i = 0; i < size; i++)
            printf("%02x ", raw_data[i]);
        printf("\n");
    } 

    return size;
}

void create_slip_message(unsigned char *data_in, int data_in_size, unsigned char *data_out, int *data_out_size)
{
    data_out[0] = FRAME_END;
    create_escaped_data(data_in, data_in_size, &data_out[1], data_out_size);
    (*data_out_size)++; // include the initial FRAME_END
    data_out[(*data_out_size)++] = FRAME_END;
}

void create_escaped_data(unsigned char *data_in, int data_in_size, unsigned char *data_out, int *data_out_size)
{    
    *data_out_size = 0;
    for(int i=0; i<data_in_size; i++)
    {
        if (data_in[i] == FRAME_END)
        {
            data_out[(*data_out_size)++] = ESCAPE_TERMINATOR_IN_DATA[0];
            data_out[(*data_out_size)++] = ESCAPE_TERMINATOR_IN_DATA[1];
        } else
        {
            if (data_in[i] == FRAME_ESCAPE)
            {
                data_out[(*data_out_size)++] = ESCAPE_ESCAPE_IN_DATA[0];
                data_out[(*data_out_size)++] = ESCAPE_ESCAPE_IN_DATA[1];
            } else
                data_out[(*data_out_size)++] = data_in[i];
        }
    }
    
}

void get_raw_data(unsigned char *data_in, int data_in_size, unsigned char *data_out, int *data_out_size, int *end_of_message)
{
    bool skip_next_byte = false;
    bool start_found = false;

    *data_out_size = 0;
    *end_of_message = -1; 

    for(int i=0; i<data_in_size; i++)
    {
        // if we've started the message, but there is a FRAME_END -- 
        // then we've reached the end of the message, regardless if
        // there is still more data
        if (start_found)
        {
            if (data_in[i] == FRAME_END)
            {
                printf("end of message %d\n", i);
                *end_of_message = i;
                break;
            }
        }

        if (! start_found)
        {
            if (data_in[i] == FRAME_END)
            {
                printf("start of message %d\n", i);
                start_found = true;
                continue;
            }
        }

        if (! start_found)
            continue;

        if (skip_next_byte)
        {
            skip_next_byte = false;
            continue;
        }
        
        if (i < (data_in_size-1))
        {
            if ((data_in[i] == ESCAPE_TERMINATOR_IN_DATA[0]) && (data_in[i+1] == ESCAPE_TERMINATOR_IN_DATA[1]))
            {
                data_out[*data_out_size] = FRAME_END;
                (*data_out_size)++;
                skip_next_byte = true;
                continue;
            }
            else
            {
                if ((data_in[i] == ESCAPE_ESCAPE_IN_DATA[0]) && (data_in[i + 1] == ESCAPE_ESCAPE_IN_DATA[1]))
                {
                    data_out[*data_out_size] = FRAME_ESCAPE;
                    (*data_out_size)++;
                    skip_next_byte = true;
                    continue;
                }
            }
        }
        data_out[*data_out_size] = data_in[i];
        (*data_out_size)++;
    }
}
