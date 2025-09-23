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

#include "fujinet.h"
#include "fujinet_communications.h"
#include "fujinet_slip.h"

typedef struct _SMARTPORT_DEVICE
{
    int AdamNetDeviceID;
    char SmartPortDeviceString[21];
    int SmartPortDeviceID;

} SMARTPORT_DEVICE;


SMARTPORT_DEVICE SmartPortTranslationTable[] =
    {
        {0x09, "FUJINET_DISK_0", 0x01},
        {0x0A, "FUJINET_DISK_1", 0x02},
        {0x0B, "FUJINET_DISK_2", 0x03},
        {0x0C, "FUJINET_DISK_3", 0x04},
        {0xFF, "CPM", 0x05},
        {0x0F, "FN_CLOCK", 0x06},
        {0x0F, "NETWORK", 0x07},
        {0x02, "PRINTER", 0x08},
        {0x0F, "MODEM", 0x09},
        {0x00, "", 0x00}
    };





unsigned char raw_data[MAX_MESSAGE_SIZE];

int wait_for_response(void **pqueue_item)
{
    MESSAGE_QUEUE *queue_item = (MESSAGE_QUEUE *)pqueue_item;
    int status = 0;
    //time_t now;

    //printf("wait for response %02x - %02x\n", queue_item->message[SEND_CMD], queue_item->message[SEND_SEQUENCE_NUMBER]);
    //printf("message_size %d\n", queue_item->message_size);
    //printf("response_received %d\n", queue_item->response_received);
    //printf("queue_item->processed %d\n", queue_item->processed);
    while (true)
    {
        //time(&now);
        //printf("timeout %ld\n", now - queue_item->time_added);
        // possible race condition
        if (queue_item->message_size == 0)
        {
            status = 0;
            break;
        }
        if (queue_item->response_received)
        {
            status= 1;
            break;
        }
        if (queue_item->processed)
        {
            status= 2;
            break;
        }

        usleep(1);
    }

    return status;
}

unsigned char *get_status_response(void *pqueue_item)
{
    MESSAGE_QUEUE *queue_item = (MESSAGE_QUEUE *)pqueue_item;
    return &queue_item->response[RESPONSE_STATUS];
}

unsigned char *get_data_response(void *pqueue_item)
{
    MESSAGE_QUEUE *queue_item = (MESSAGE_QUEUE *)pqueue_item;
    return &queue_item->response[RESPONSE_DATA];
}

void set_message_processed(void *pqueue_item)
{
    MESSAGE_QUEUE *queue_item = (MESSAGE_QUEUE *)pqueue_item;

    printf("Setting %d as processed\n", queue_item->message[SEND_SEQUENCE_NUMBER]);
    queue_item->processed = true;
    queue_item->message_size = 0;
}


char get_apple_device_id(int adam_id)
{
    int i = 0;
    while( SmartPortTranslationTable[i].AdamNetDeviceID != 0)
    {
        if (SmartPortTranslationTable[i].AdamNetDeviceID == adam_id)
            return SmartPortTranslationTable[i].SmartPortDeviceID;
        i++;
    }
    return 0;
}

bool fujinet_reset(int adam_device_id, void **response)
{
    bool success = false;
    int data_size = 0;
 
    printf("fujinet_reset\n");
    if (communications_are_active())
    {
        raw_data[data_size++] = sequence_number(true);
        raw_data[data_size++] = SLIP_CMD_RESET_REQUEST;
        raw_data[data_size++] = get_apple_device_id(adam_device_id);

        success = send_via_slip(raw_data, data_size, response);

        if (success)
        {
            //success = get_fujinet_response(raw_data, &data_size);
            //if (success)
            //{
            //    success = validate_fujinet_response(raw_data, data_size);
            //}
        }
        return success;
    } else
        return false;
}

bool fujinet_clock(FUJI_TIME *ft)
{
    void *msg_queue;
    bool success = false;
    int data_size = 0;
    int wait_result;
    unsigned char *data;
    unsigned char *status;
    

    if (communications_are_active())
    {
        raw_data[data_size++] = sequence_number(true);
        raw_data[data_size++] = SLIP_CMD_STATUS_REQUEST;
        raw_data[data_size++] = 6; // smartport
        raw_data[data_size++] = 'T';

        success = send_via_slip(raw_data, data_size, &msg_queue);
        if (success)
        {
            wait_result = wait_for_response(msg_queue);
            if (wait_result == 1)
            {
                status = get_status_response(msg_queue);
                if (*status != STATUS_OK)
                {
                    printf("clock status: %02x\n", *status);
                    return false;
                }

                data = get_data_response(msg_queue);
                printf("Filling fujitime\n");
                memcpy(ft, data, sizeof(FUJI_TIME));
                success = true;

                set_message_processed(msg_queue);
            }
        }
    }

    return success;
}

bool fujinet_init(int adam_device_id, void **response)
{
    bool success = false;
    int data_size = 0;
    int smartport_id;

    smartport_id = get_apple_device_id(adam_device_id);
    printf("fujinet_init(%d) -> smartport: %d\n", adam_device_id, smartport_id);

    if (smartport_id == 0)
        return false;

    if (communications_are_active())
    {
        raw_data[data_size++] = sequence_number(true);
        raw_data[data_size++] = SLIP_CMD_INIT_REQUEST;
        raw_data[data_size++] = get_apple_device_id(adam_device_id);

        success = send_via_slip(raw_data, data_size, response);

        if (success)
        {
            // success = get_fujinet_response(raw_data, &data_size);
            // if (success)
            //{
            //     success = validate_fujinet_response(raw_data, data_size);
            // }
        }
    }
    return success;
}



bool validate_fujinet_response(unsigned char *raw_data, int size)
{
    if (raw_data[SLIP_RESPONSE_SEQUENCE] == sequence_number(false) && raw_data[SLIP_RESPONSE_STATUS] == 0)
        return true;
    return false;
}