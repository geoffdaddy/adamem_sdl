default:
	@echo "To compile ADAMEm, use one of the following:"
	@echo " make svga  - Make the Linux/SVGALib version"
	@echo " make x     - Make the Unix/X version"
	@echo " make msdos - Make the MS-DOS version (DJGPP only)"
	@echo " make linux - Build both the Linux/SVGALib and the Unix/X versions"
	@echo " make sdl   - Build the SDL version"
	@echo "When compiling the SVGALib version, make sure you are logged in as root"
	@echo "Please check Makefile.MSDOS, Makefile.SVGALib, Makefile.X and Z80.h"
	@echo "before compiling ADAMEm"

svga:
	make -f Makefile.SVGALib

x:
	make -f Makefile.X

msdos:
	make -f Makefile.MSDOS

sdl:
	make -f Makefile.SDL

clean:
	rm -f adamem.map
	rm -f PARAM.SFO
	rm -f *.o

linux:
	make clean
	make x
	mv adamem adamemx
	rm -f cvemx
	ln -s adamemx cvemx
	mv keys keysx
	make clean
	make svga

