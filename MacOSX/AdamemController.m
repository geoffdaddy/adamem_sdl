//
//  AdamemController.m
//  Adamem
//
//  Copyright 2008-2023 Geoff Oltmans. All rights reserved.
//

#import "AdamemController.h"
#import "SDLMain.h"
#import "Coleco.h"
#import "AdamemSDL.h"


extern int PausePressed;
char cntDiskName[4][200];
char cntTapeName[4][200];

@implementation AdamemController

-(IBAction)togglePause:(id)x
{
	int i;
	i = WindowManagerSetPause(!PausePressed);
	if (!i) {
		[pause setState:NSOffState];
	}
	else {
		[pause setState:NSOnState];
	}
	if (DiskName[0]) {
		[diskA setTitle:[NSString stringWithFormat:@"Disk 1 - %s",DiskName[0]]];
	} else {
		[diskA setTitle:@"Disk 1"];
	}
	
		
}

-(IBAction)resetComputer:(id)x
{
//	WindowManagerSetPause(0);
	ResetColeco(0);
}
-(IBAction)resetGame:(id)x
{
//	WindowManagerSetPause(0);
	ResetColeco(1);
}

-(IBAction)setIFreq50:(id)x
{
	IFreq = 50;
}

-(IBAction)setIFreq60:(id)x
{
	IFreq = 60;
}

-(IBAction)attachDiskImage:(id)sender
{
	int disknum = [sender tag];
	NSOpenPanel *thePanel = [NSOpenPanel openPanel];
	//NSArray *types = [NSArray arrayWithObjects:@"dsk", @"img", nil];
	
	[thePanel setAllowsMultipleSelection:NO];
    [thePanel setCanChooseFiles:YES];
    [thePanel setCanChooseDirectories:NO];
    [thePanel setTitle:@"Attach Disk Image"];    
    
    [thePanel beginWithCompletionHandler:^(NSInteger result) {
        if(result == NSFileHandlingPanelOKButton) {
            DiskClose(disknum);
            NSURL*  theFile = [[thePanel URLs] objectAtIndex:0];

            strncpy(cntDiskName[disknum],[[theFile path] UTF8String],200);
            DiskName[disknum] = cntDiskName[disknum];
            DiskOpen(disknum);
        }
    }];
    
}

-(IBAction)detachDiskImage:(id)sender
{

	int disknum = [sender tag];
	switch (disknum) {
		case 0:
			[diskA setTitle:@"Disk 1 - No disk"];
			
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
	}
	DiskClose(disknum);
}

-(IBAction)attachTapeImage:(id)sender
{
	int tapenum = [sender tag];
	NSOpenPanel *thePanel = [NSOpenPanel openPanel];
	//NSArray *types = [NSArray arrayWithObjects:@"ddp", nil];
	
	[thePanel setAllowsMultipleSelection:NO];
    [thePanel setCanChooseFiles:YES];
    [thePanel setCanChooseDirectories:NO];
    [thePanel setTitle:@"Attach Tape Image"];    
    
    [thePanel beginWithCompletionHandler:^(NSInteger result) {
        if(result == NSFileHandlingPanelOKButton) {
            TapeClose(tapenum);
            NSURL*  theFile = [[thePanel URLs] objectAtIndex:0];

            strncpy(cntTapeName[tapenum],[[theFile path] UTF8String],200);
            TapeName[tapenum] = cntTapeName[tapenum];
            TapeOpen(tapenum);
        }
    }];
	
}

-(IBAction)detachTapeImage:(id)sender
{
	
	int tapenum = [sender tag];
	TapeClose(tapenum);
}

-(IBAction)fullScreen:(id)sender
{
	//[NSAlert alertWithMessageText:@"To return to windowed mode, press COMMMAND-F" defaultButton:@"OK"];
	WindowManagerSetPause(0);
	setFullScreen(1);
}
@end
