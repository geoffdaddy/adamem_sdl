#import "Preferences.h"
#import "Coleco.h"
#import "Z80.h"
#import "AdamemSDL.h"
#import <sys/stat.h>

@implementation Preferences

//static Preferences *sharedInstance = nil;
//
//+ (Preferences *)sharedInstance {
//    return sharedInstance ? sharedInstance : [[self alloc] init];
//}
//
//- (id)init {
//    if (sharedInstance) {		// We just have one instance of the Preferences class, return that one instead
//        [self release];
//    } else if (self = [super init]) {
//        sharedInstance = self;
//    }
//    return sharedInstance;
//}
//
//- (void)dealloc {
//    if (self != sharedInstance) [super dealloc];	// Don't free the shared instance
//}

- (IBAction)showPreferences:(id)sender {
//	if (!panel) {
        if (![NSBundle loadNibNamed:@"Preferences" owner:self])  {
            NSLog(@"Failed to load Preferences.nib");
            NSBeep();
            return;
        }
		WindowManagerSetPause(1);
		[panel setHidesOnDeactivate:NO];
		[panel setExcludedFromWindowsMenu:YES];
//		[panel setMenu:nil];
        [self updateUI];
        [panel center];
//    }
    [panel makeKeyAndOrderFront:nil];
	
}
//extern int  Verbose;                  /* Debug msgs ON/OFF                  */
//extern int  UPeriod;                  /* Number of interrupts/screen update */
//extern int  PrnType;                  /* Type of printer attached           */
//extern int  DiskSpeed;                /* Time in ms it takes to read one... */
//extern int  TapeSpeed;                /* ... block                          */
//extern char *CartName;                /* Cartridge ROM file                 */
//extern char *OS7File,*EOSFile,*WPFile;/* Main ROMs                          */
//extern char *ExpRomFile;               
//extern int  RAMPages;                 /* Number of 64K expansion RAM pages  */
//extern char *DiskName[4];             /* Disk image file names              */
//extern char *TapeName[4];             /* Tape image file names              */
//extern char *PrnName;                 /* Printer log file                   */
//extern char *LPTName;                 /* Parallel port log file             */
//extern char *SoundName;               /* Sound log file                     */
//extern int  JoyState[2];              /* Joystick status                    */
//extern int  SpinnerPosition[2];       /* Spinner positions [0..500]         */
//#define NR_PALETTES     4
//extern byte Palettes[NR_PALETTES][16*3];
//extern int  PalNum;                   /* Palette number                     */
//#define Coleco_Palette	Palettes[PalNum]
//extern int  SaveSnapshot;             /* If 1, auto-save snapshot           */
//extern char *SnapshotName;            /* Snapshot file name                 */
//#define MAX_CHEATS      16            /* Maximum number of cheat codes      */
///* supported                          */
//extern int  Cheats[16];               /* Cheats to patch into game ROM      */
//extern int  CheatCount;               /* Number of cheats                   */

- (void)updateUI {
	[iFreq selectCellWithTag:IFreq];
	[video selectCellWithTag:videomode];
	[adamColecoVision selectCellWithTag:EmuMode];
	[sprite selectCellWithTag:Support5thSprite];
	[palette selectCellWithTag:PalNum];
	[cpuSpeed setIntValue: (Z80_IPeriod*(100*1000))/3579545];
	[uPeriods setIntValue:UPeriod];
	[sync selectCellWithTag:syncemu];
    [sgmMode selectCellWithTag:sgmmode];
    [videoFilter selectCellWithTag:ntscFilter];

	
}
//- (IBAction)setInterruptFrequency:(id)sender {
//	newIfreq = [[sender selectedCell] tag];
//}
//
//- (IBAction)setScreenSize:(id)sender {
//	newVideoMode = [[sender selectedCell] tag];
//}
//
//- (IBAction)setCv:(id)sender {
//	newCv = [[sender selectedCell] tag];
//}
//
//- (IBAction)setSpriteMode:(id)sender {
//	newSprite = [[sender selectedCell] tag];
//}
//
//- (IBAction)setPaletteMode:(id)sender {
//	newPalette = [[sender selectedCell] tag];
//}
//
//- (IBAction)setCpu:(id)sender {
//	newCpuSpeed = [sender intValue];
//}

-(IBAction) writeChangesToDisk:(id)sender
{
	char filename[256];
	FILE *fptr = NULL;
	struct stat sb;

	sprintf(filename,"%s%s",getenv("HOME"),"/Library/Application Support/Adamem/");

	if (stat(filename, &sb) && !S_ISDIR(sb.st_mode))
	{
		mkdir(filename, 0755);
	}
	sprintf(filename,"%s%s",getenv("HOME"),"/Library/Application Support/Adamem/Adamem.cfg");
	
	fptr = fopen(filename,"w");
	fprintf(fptr,"-ifreq %d\n",[[iFreq selectedCell] tag]);
	fprintf(fptr,"-video %d\n",[[video selectedCell] tag]);
	fprintf(fptr,"-sprite %d\n",[[sprite selectedCell] tag]);
	if (![[adamColecoVision selectedCell] tag]) {
		fprintf(fptr,"-cv\n");
	}
	fprintf(fptr,"-cpuspeed %d\n",[cpuSpeed intValue]);
	fprintf(fptr,"-palette %d\n",[[palette selectedCell] tag]);
	fprintf(fptr,"-uperiod %d\n",[uPeriods intValue]);
	fprintf(fptr,"-sync %d\n",[[sync selectedCell] tag]);
    fprintf(fptr,"-sgm %d\n",[[sgmMode selectedCell] tag]);
    fprintf(fptr,"-comp %d\n",[[videoFilter selectedCell] tag]);

//		"verbose",
//		"cheat","sound","joystick","swapbuttons",
//		"expansion","overscan","volume","soundtrack",
//		"trap","os7","sensitivity",
//		"keys","printer","keypad",
//		"eos","wp","diska","diskb","diskc","diskd",
//		"tapea","tapeb","tapec","taped",
//		"printertype","savecpu",
//		"ram","snap","autosnap","diskspeed","tapespeed","lpt","tdos",
//		"cart","exprom"
	fclose(fptr);
    WindowManagerSetPause(0);

    [panel performClose:self];
}

-(IBAction) resetDefaults:(id)sender
{
	char filename[256];
	sprintf(filename,"%s%s",getenv("HOME"),"/Library/Preferences/Adamem/Adamem.cfg");
	remove(filename);
}

-(IBAction) applyChanges:(id)sender
{
	unsigned int temp = [cpuSpeed intValue];
	IFreq = [[iFreq selectedCell] tag];
	Z80_IPeriod=(3579545*temp)/(100*1000);
	Support5thSprite = [[sprite selectedCell] tag];
    WindowManagerSetPause(0);
    UPeriod = [uPeriods intValue];
    [panel performClose:self];
}

@end
