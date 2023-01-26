/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 Help.c                                 **/
/**                                                                        **/
/** This file contains the messages printed when the -help command line    **/
/** option is used                                                         **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

char *HelpText[] =
{
  "  -help [-he]                - Print this help page",
  "  -verbose [-vb] <flags>     - Select debugging messages [1]",
  "                                 0 - Silent      1 - Startup messages",
  "                                 2 - VDP         4 - Memory mapper",
  "                                 8 - Disk/Tape  16 - PCB operations",
  "A  -adam / -cv                - Select emulation model [-adam]",
  "C  -adam / -cv                - Select emulation model [-cv]",
  "                               adam - Coleco ADAM",
  "                               cv   - ColecoVision",
  "  -sgm <mode>                - Select Super Game Module Mode [0]",
  "                                 0 - no SGM support   1 - SGM supported",
  "  -os7 <file>                - Select ColecoVision ROM image [OS7.rom]",
  "  -eos <file>                - Select EOS ROM image [EOS.rom]",
  "  -wp <file>                 - Select SmartWriter ROM image [WP.rom]",
  "  -exprom <file>             - Select Expansion ROM image [EXP.rom]",
  "  -ram <value>               - Select number of 64K expansion RAM pages [1]",
  "  -diska, -diskb, -diskc,    - Select disk/tape images to use [none]",
  "  -diskd, -tapea, -tapeb,",
  "  -tapec, -taped [-da...-td] <filename>",
  "  -diskspeed [-ds] <value>   - Set time it takes to read one block [100ms]",
  "  -tapespeed [-ts] <value>   - Set time it takes to read one block [100ms]",
#ifdef IDEHD
  "  -idehda [-idea] <file>,    - Select IDE hard drive image(s) to use",      
  "  -idehdb [-ideb] <file>       [hd_ide1.img, hd_ide2.img]",
#endif
  "  -snap [-sn] <filename>     - Load snapshot [none]",
  "  -autosnap [-asn] <mode>    - Select snapshot mode [0]",
  "                               0 - Do not automatically load/save snapshots",
  "                               1 - Automatically load/save snapshots",
  "  -cheat <code>              - Activate cheat [none]",
#ifdef MSDOS
  "  -printer [-pr] <filename>  - Select printer log file [PRN]",
  "  -lpt <filename>            - Select parallel port log file [PRN]",
#else
  "  -printer [-pr] <filename>  - Select printer log file [stdout]",
  "  -lpt <filename>            - Select parallel port log file [stdout]",
#endif
  "  -printertype [-pt] <value> - Select printer type [1]",
  "                               0 - ADAM Printer compatible",
  "                               1 - Generic/Text only",
  "                               2 - IBM Graphics compatible",
  "                               3 - Qume SPRINT 11 compatible",
  "  -cpuspeed [-cs] <speed>    - Set Z80 CPU speed [100%]",
  "  -ifreq [-if] <frequency>   - Set interrupt frequency [50Hz]",
  "  -sync [-sy] <mode>         - Select sync mode [1]",
  "                               0 - Do not sync emulation",
  "                               1 - Sync emulation on every VDP interrupt",
  "  -expansion [-ex] <mode>    - Select expansion module emulation [0]",
  "                               0 - None",
  "                               1 - Roller controller with mouse",
  "                               2 - Roller controller with analogue joystick",
  "                               3 - Driving module with analogue joystick",
  "                               4 - Driving module with mouse",
  "                               5 - Speed roller on both ports with mouse",
  "                               6 - Speed roller on port 1 with mouse",
  "                               7 - Speed roller on port 2 with mouse",
  "  -joystick [-js] <mode>     - Select joystick mode [1]",
  "                               0 - No joystick support  1 - Joystick support",
  "                               2 - Joystick at port 2   3 - Joystick at port 1",
  "  -sensitivity [-se] <value> - Select mouse/joystick sensitivity [200]",
  "                               1 - Maximum   1000 - Minimum",
  "  -swapbuttons [-sb] <flags> - Swap/Do not swap buttons [0]",
  "                               1 - Swap joystick buttons",
  "                               2 - Swap keyboard buttons",
  "                               4 - Swap mouse buttons",
  "  -keypad [-kp] <mode>       - Select numeric keypad operation [0]",
  "                               0 - Normal   1 - Reversed",
  "  -calibrate [-ca] <mode>    - Force/Do not force joystick calibration [0]",
  "                               0 - Do not force joystick calibration",
  "                               1 - Force joystick calibration",
  "  -keys [-ke] <string>       - Alter key mappings",
  "  -uperiod [-up] <value>     - Set max. nr. of interrupts per screen update [3]",
  "  -sprite <mode>             - Select sprite emulation method [0]",
  "                               0 - Show all sprites",
  "                               1 - Don't show more than 4 sprites per row",
  "  -palette [-pal] <value>    - Select palette [0]",
  "                               0 - Original   1 - V9938   2 - Original B&W",
  "                               3 - V9938 B&W",
  "  -video [-vi] <mode>        - Select video mode [0]",
#if defined(MSDOS)
  "                               0 - 320x200    1 - 256x192",
  "                               2 - 256x240",
#elif defined(LINUX_SVGA)
  "                               0 - 320x200    1 - 320x240",
#else
  "                               0 - 256x212    1 - 512x212    2 - 512x424",
#if defined(SDL)
  "                               3 - 1280x720   4 - 1920x1080",
  "  -composite [-comp] <value> - Video filter mode [0]",
  "                               0 - no filter  1 - ntsc composite",
#endif
#endif
#if defined(MSDOS) || defined(LINUX_SVGA)
  "  -overscan [-os] <mode>     - Emulate/Do not emulate overscan colour [1]",
  "                               0 - Do not emulate overscan colour",
  "                               1 - Emulate overscan colour",
#endif
#ifdef TEXT80
  "  -tdos <mode>               - Select TDOS video [0]",
  "                               0 - 40 column  1 - 80 column",
#endif
#ifdef LINUX_SVGA
  "  -chipset <value>           - Select video chipset [1]",
  "                               0 - VGA        1 - Autodetect",
#endif
  "  -soundtrack [-st] <file>   - Select file for sound logging [NULL]",
#ifdef SOUND
  "  -sound [-so] <mode>        - Select sound mode [255]",
#ifdef MSDOS
  "                               0 - No sound      1 - PC Speaker",
  "                               2 - Adlib         3 - Adlib+SoundBlaster",
  "                               4 - SoundBlaster  5 - GUS",
  "                               6 - SB AWE32",
#endif
#ifdef UNIX
  "                               0 - No sound   1 - /dev/dsp",
#endif
  "                               255 - Detect",
#ifdef MSDOS
  "  -reverb [-re] <level>      - Select reverb send level [7]",
  "                               (SoundBlaster/SB AWE32 only)",
  "                               0 - Minimum  100 - Maximum",
  "  -chorus [-ch] <level>      - Select chorus send level [0]",
  "                               (SB AWE32 only)",
  "                               0 - Minimum  100 - Maximum",
  "  -stereo <level>            - Select stereo panning level [0]",
  "                               (SoundBlaster/SB AWE32 only)",
  "                               0 - Mono     100 - Maximum",
#endif
#ifdef MSDOS
  "  -soundquality [-sq] <value>- Select sound quality [3]",
  "                               (SoundBlaster only)",
#else
  "  -soundquality [-sq] <value>- Select sound quality [3]",
#endif
  "                               1 - Lowest     5 - Highest",
#ifdef MSDOS
  "  -speakerchannels [-sc] <channel list>",
  "                             - Select PC Speaker channel list [4,3,2,1]",
  "                               (PC Speaker only)",
#endif
  "  -volume [-vo] <volume>     - Select initial volume [10]",
  "                               0 - Silent    15 - Maximum",
#endif /* SOUND */
#ifdef UNIX_X
 #ifdef MITSHM
  "  -shm <mode>                - Use/Don't use MITSHM extensions for X [1]",
  "                               0 - Don't use SHM   1 - Use SHM",
 #endif
  "  -savecpu <mode>            - Save/Don't save CPU when inactive [1]",
  "                               0 - Don't save CPU   1 - Save CPU",
#endif
#ifdef DEBUG
  "  -trap [-tr] <address>      - Trap execution when PC reaches address [-1]",
#endif
  NULL
};

