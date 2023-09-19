# ADAMEM SDL Port using SDL 2.0 library

This is a port of Marcel de Kogel's excellent ADAMEm emulator for the Coleco ADAM computer. 
Marcel's code was laid out with good hooks for creating new drivers for various different host hardware. 
I (Geoff Oltmans) back in 2006 wanted ADAMEm to work with Windows XP, which would no longer work with 
the DOS version after a few patches and updates to Windows were installed. To future proof things a bit,
looked at the various options available at the time for graphics and sound libraries that would be 
fairly portable from the various systems at the time. SDL seemed to fit that bill. This distribution of
ADAMEm is basically the same as what you can get from Marcel's website today, but with a few additions:
Powermate IDE emulation, borrowed from BlueMSX, Super Game Module support - my own creation, composite video
filter via Blarggs ntsc library for more accurate video emulation. I also came up with my own sound emulation
routines since the original ADAMEm sources relied upon sampled sounds from real hardware. A lot of further work
done by others in the interim led to better understanding of how the SN76489AN chip generates sound, so I used
information for those various sources and datasheets to come up with something that loads the sound stream directly.

I have moved a lot of the original driver code under the Unsupported directory. That's not to say that you couldn't
use these, but I think at this point the folks that care about say a build for MS-DOS using Gravis Ultrasound is 
vanishingly small, so I don't want to spend cycles on that. Or if someone wanted to use SVGALib for Linux instead of SDL.
If you do, knock yourself out.

The SDL driver has been successfully used with Windows 10/11, Linux (Ubuntu 20.04 and greater), and Mac OS.

The Linux target can be built with 'make sdl' as long as the libsdl2 libraries are installed.

The Windows target is built using the Visual Studio solution.

Mac OS support via Xcode project (still cleaning these up, stay tuned).
