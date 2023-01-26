//
//  AdamemController.h
//  Adamem
//
//  Copyright 2008 Geoff Oltmans. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface AdamemController : NSObject {
	int iFreq;
	IBOutlet NSMenuItem *diskA;
	IBOutlet NSMenuItem *pause;
}

-(IBAction)togglePause:(id)x;

-(IBAction)resetComputer:(id)x;
-(IBAction)resetGame:(id)x;
-(IBAction)setIFreq60:(id)x;
-(IBAction)setIFreq50:(id)x;
-(IBAction)attachDiskImage:(id)sender;	
-(IBAction)detachDiskImage:(id)sender;	
-(IBAction)attachTapeImage:(id)sender;	
-(IBAction)detachTapeImage:(id)sender;	
-(IBAction)fullScreen:(id)sender;	

@end
