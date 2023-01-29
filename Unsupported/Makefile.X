#### Makefile for ADAMEm Unix/X
# Following #defines can be used:
# UNIX_X		Compile Unix/X version
# UNIX                  Compile for a UNIX machine
# MITSHM		Use MIT SHM extensions
# DEBUG			Compile with builtin debugger
# LSB_FIRST		Compile for a low-endian machine
# X86_ASM		Use inline 80x86 assembly
# HAVE_CLOCK		clock() is available on this machine
# JOYSTICK		Use Linux joystick driver
# SOUND			Use USS(/Lite) sound driver
# ZLIB                  Compile ZLIB version

CC	= gcc	# C compiler used
LD	= gcc	# Linker used

CFLAGS	= -Wall -O2 -fomit-frame-pointer \
	  -DLSB_FIRST -DUNIX_X -DUNIX -DMITSHM -DHAVE_CLOCK -DSOUND -DZLIB

OBJECTS = ADAMEm.o Coleco.o Z80.o \
          DSP.o Sound.o X.o Z80Debug.o Bitmap.o Unix.o

all:    adamem cvem keys adamem.snd z80dasm snapedit

clean:
	rm -f *.o *~

adamem: $(OBJECTS)
	$(LD) -s -Wl,-Map,adamem.map -L/usr/X11R6/lib \
              -o adamem $(OBJECTS) -lXext -lX11 -lz

cvem: adamem
	rm -f cvem
	ln -s adamem cvem

snapedit.o: snapedit.c
snapedit: snapedit.o
	$(LD) -s -o snapedit snapedit.o -lz

keys.o: keys.c
keys: keys.o
	$(LD) -s -o keys keys.o

adamem.snd: makedata
	makedata

makedata.o: makedata.c
makedata: makedata.o
	$(LD) -s -o makedata makedata.o -lz

z80dasm.0: z80dasm.c
z80dasm: z80dasm.o
	$(LD) -s -o z80dasm z80dasm.o

ADAMEm.o:   ADAMEm.c Coleco.h Z80.h Help.h Z80IO.h Sound.h X.h
Coleco.o:   Coleco.c Coleco.h Z80.h Z80IO.h Sound.h
Bitmap.o:   Bitmap.c Bitmap.h
DSP.o:      DSP.c Mixer.h dev_dsp.h Coleco.h Sound.h
Sound.o:    Sound.c Coleco.h Sound.h
Z80.o:      Z80.c Z80.h Z80CDx86.h Z80IO.h Z80DAA.h
Z80Debug.o: Z80Debug.c Z80.h Z80IO.h z80dasm.h
X.o:        X.c X.h Unix.h Common.h Coleco.h
Unix.o:     Unix.c Unix.h Coleco.h

