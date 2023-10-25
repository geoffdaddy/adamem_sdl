#import <Cocoa/Cocoa.h>

@interface Preferences : NSObject {
    IBOutlet NSWindow *panel;
	IBOutlet NSMatrix *iFreq;
	IBOutlet NSMatrix *video;
	IBOutlet NSMatrix *adamColecoVision;
	IBOutlet NSMatrix *sprite;
	IBOutlet NSMatrix *palette;
	IBOutlet NSMatrix *sync;
	IBOutlet NSSlider *cpuSpeed;
	IBOutlet NSSlider *uPeriods;
    IBOutlet NSMatrix *sgmMode;
    IBOutlet NSMatrix *videoFilter;
}

//+ (Preferences *) sharedInstance;
- (IBAction)showPreferences:(id)sender;
//- (IBAction)setInterruptFrequency:(id)sender;
//- (IBAction)setScreenSize:(id)sender;
//- (IBAction)setCv:(id)sender;
//- (IBAction)setCpu:(id)sender;
//- (IBAction)setSpriteMode:(id)sender;
//- (IBAction)setPaletteMode:(id)sender;
- (IBAction)writeChangesToDisk:(id)sender;
- (IBAction)resetDefaults:(id)sender;
- (IBAction)applyChanges:(id)sender;
- (void)updateUI;

@end
