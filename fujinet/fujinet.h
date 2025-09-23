#include <stdbool.h>

extern bool fujinet_verbose;

#define FUJINET_VERSION " FujiNet 0.1"
#define FUJINET_SD_DIR  "FUJINET_SD/"

/** Devices **************************************************/
#define FUJINET_DEV 0x0F

#define FUJINET_DEV_N1  0x09
#define FUJINET_DEV_N2  0x0A
#define FUJINET_DEV_N3  0x0B
#define FUJINET_DEV_N4  0x0C




#define FUJINET_DEV_CMD_GET_TIME            0xD2
#define FUJINET_DEV_CMD_RANDOM              0xD3
#define FUJINET_DEV_CMD_CLOSE_APP_KEY       0xDB
#define FUJINET_DEV_CMD_OPEN_APP_KEY        0xDC
#define FUJINET_DEV_CMD_READ_APP_KEY        0xDD
#define FUJINET_DEV_CMD_WRITE_APP_KEY       0xDE

/*
    Devices (15 max):
        Device 00 = Master 6801 ADAMnet controller (uses the adam_pcb as DCB)
        Device 01 = Keyboard
        Device 02 = ADAM printer
        Device 03 = Copywriter (projected)
        Device 04 = Disk drive 1
        Device 05 = Disk drive 2
        Device 06 = Disk drive 3 (third party)
        Device 07 = Disk drive 4 (third party)
        Device 08 = Tape drive 1
        Device 09 = FUJINET N1
        Device 0A = FUJINET N2
        Device 0B = FUJINET N3
        Device 0C = FUJINET N4
        Device 0D = ADAM parallel interface (never released)
        Device 0E = ADAM serial interface (never released)
        Device 0F = FUJINET DEVICE
        Device 18 = Tape drive 2 (share DCB with Tape1)
        Device 19 = Tape drive 4 (projected, may have share DCB with Tape3)
        Device 20 = Expansion RAM disk drive (third party ID, not used by Coleco)
        Device 52 = Disk
*/

/** Prototypes ***********************************************/
// fujinet.c
void DisplayDCB(char *device, int mode, int device_id, unsigned dcb);
unsigned int Z80AddrFromDCBBuffer(unsigned dcb);
void UpdateFujiDevice(int mode, int device_id, unsigned dcb);

bool fujinet_setup(void);


/** DCB Commands *********************************************/
#define DCB_CMD_STATUS 0x01
#define DCB_CMD_SOFT_RESET 0x02
#define DCB_CMD_WRITE 0x03
#define DCB_CMD_READ 0x04

    typedef struct _dcb
{
    // both the status and command byte. You write the command, the Adam does the operation, and writes back a status sometime later.
    unsigned char status;     // 1 byte

    // 16 bit memory address in Z80 space for the target operation
    char buf[2];              // 2

    // Length of I/O operation relative to the buffer.
    unsigned short len;       // 2

    // Used by block devices to denote the desired 32-bit block address.
    unsigned long block;      // 4

    // logical unit number specified.
    unsigned char unit;       // 1

    unsigned char reserved0;  // 1
    unsigned char reserved1;  // 1
    unsigned char reserved2;  // 1
    unsigned char reserved3;  // 1
    unsigned char reserved4;  // 1
    unsigned char reserved5;  // 1

    // The Adamnet node number for this device. (1 = keyboard, 2 = printer, etc.)
    unsigned char dev;        // 1

    // The maximum size of payload allowed for this device
    unsigned short max_len;   // 2

    // Device type, 0 = character, 1 = block
    unsigned char type;       // 1

    // The 8-bit status byte returned in the last byte of a status packet.
    unsigned char dev_status; // 1
} DCB;                        // 21 bytes total



/** Fujinet Commands **************************************************/
#define FUJI_GET_TIME 0xD2

#define ACK 0x80
#define NAK 0x90

#define MAX_URL (256) 
#define MAX_APP_DATA 1024
#define MAX_PATH (512)


#define FUJI_CMD_CMD    0
#define FUJI_CMD_MODE   1
#define FUJI_CMD_TRANS  3
#define FUJI_CMD_DATA   4

typedef struct
{
    unsigned char  cmd;         // 1
    unsigned short mode;        // 2
    unsigned char  trans;       // 1
    unsigned char  data[MAX_URL];
} FUJI_CMD;

typedef struct
{
    unsigned char century,  // Century
                  year,     // Year
                  month,    // Month
                  day,      // Day
                  hour,     // Hour
                  minute,   // Minute
                  second;   // Second
} FUJI_TIME;  // 7 bytes total

typedef struct
{
    unsigned char  cmd;
    unsigned short creator;
    unsigned char  app;
    unsigned char  key;
} FUJI_APP;

typedef struct
{
    unsigned char  cmd;
    unsigned short creator;
    unsigned char  app;
    unsigned char  key;
    unsigned char  data[MAX_APP_DATA];
} FUJI_APP_DATA;

/*
CMD	Fujinet Description

$D1	Device Enable Status
$D2	Get Time
$D3	Random Number
$D4	Disable Device
$D5	Enable Device
$D6	[Set Boot Mode]
$D7	[Mount All]
$D8	[Copy File]
$D9	[Enable/Disable CONFIG in D1:]
$DA	[Get Device Slot Filename]
$DB	[Close App Key]
$DC	[Open App Key]
$DD	[Read App Key]
$DE	[Write App Key]
$DF	[Set External SIO Clock]
$E0	[Get Host Prefix]
$E1	[Set Host Prefix]
$E2	[Set Filename for Device Slot]
$E4	[Set Directory Position]
$E5	[Get Directory Position]
$E6	[Unmount Host]
$E7	[New Disk]
$E8	[Get Adapter Config]
$E9	[Unmount Device Image]
$F1	[Write Device Slots]
$F2	[Read Device Slots]
$F3	[Write Host Slots]
$F4	[Read Host Slots]
$F5	[Close Directory]
$F6	[Read Directory]
$F7	[Open Directory]
$F8	[Mount Device Image]
$F9	[Mount Host]
$FA	[Get WiFi Status]
$FB	[Set SSID and Connect]
$FC	[Get Scan Result]
$FD	[Scan Networks]
$FE	[Get SSID]
$FF	[Reset FujiNet]
*/

