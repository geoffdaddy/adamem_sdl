//
//  ADAMDocumentController.m
//  ADAMEm
//
//  Copyright 2008-2023 Geoff Oltmans. All rights reserved.
//

#import "ADAMDocumentController.h"
#include <stdio.h>
#include <string.h>
#include "Coleco.h"

extern char cntDiskName[4][200];
extern char cntTapeName[4][200];

@interface ImageSelectAccessoryOwner : NSObject {
@public
	IBOutlet NSView *accessoryView;
	IBOutlet NSPopUpButton *diskButton;
	IBOutlet NSPopUpButton *tapeButton;
}
@end

@implementation ImageSelectAccessoryOwner

@end



@implementation ADAMDocumentController

- (int)runModalOpenPanel:(NSOpenPanel *)openPanel forTypes:(NSArray *)fileNameExtensionsAndHFSFileTypes
{
	int result;
	ImageSelectAccessoryOwner *owner = [[[ImageSelectAccessoryOwner alloc] init] autorelease];

	if (![NSBundle loadNibNamed:@"ImageSelectAccessory" owner:owner]) {
		NSLog(@"Failed to open ImageSelectAccessory.nib");
		return NSCancelButton;
	}
	
	
	[owner->diskButton removeAllItems];
	[owner->diskButton addItemWithTitle:@"Disk 1"];
	[[owner->diskButton lastItem] setTag:0];
	[[owner->diskButton lastItem] setEnabled:YES];
	[owner->diskButton addItemWithTitle:@"Disk 2"];
	[[owner->diskButton lastItem] setTag:1];
	[[owner->diskButton lastItem] setEnabled:YES];
	[owner->diskButton addItemWithTitle:@"Disk 3"];
	[[owner->diskButton lastItem] setTag:2];
	[[owner->diskButton lastItem] setEnabled:YES];
	[owner->diskButton addItemWithTitle:@"Disk 4"];
	[[owner->diskButton lastItem] setTag:3];
	[[owner->diskButton lastItem] setEnabled:YES];

	[owner->tapeButton removeAllItems];
	[owner->tapeButton addItemWithTitle:@"Tape 1"];
	[[owner->tapeButton lastItem] setTag:0];
	[[owner->tapeButton lastItem] setEnabled:YES];
	[owner->tapeButton addItemWithTitle:@"Tape 2"];
	[[owner->tapeButton lastItem] setTag:1];
	[[owner->tapeButton lastItem] setEnabled:YES];
	[owner->tapeButton addItemWithTitle:@"Tape 3"];
	[[owner->tapeButton lastItem] setTag:2];
	[[owner->tapeButton lastItem] setEnabled:YES];
	[owner->tapeButton addItemWithTitle:@"Tape 4"];
	[[owner->tapeButton lastItem] setTag:3];
	[[owner->tapeButton lastItem] setEnabled:YES];
	
	[openPanel setAccessoryView:[owner->accessoryView autorelease]];
	
	result = [super runModalOpenPanel:openPanel forTypes:fileNameExtensionsAndHFSFileTypes];
	if (result == NSOKButton) {
		lastSelectedDiskNumber = [[owner->diskButton selectedItem] tag];
		lastSelectedTapeNumber = [[owner->tapeButton selectedItem] tag];
		
	}
	return result;
}

- (int) lastSelectedDiskNumber
{
	return lastSelectedDiskNumber;
}

- (int) lastSelectedTapeNumber
{
	return lastSelectedTapeNumber;
}

// We can't rely on the normal NSDocumentController to give us what we need for disk and tape images (passes
// an NSData object to the class MyDocument)... although that is sufficient for ROM images. We will handle
// the disk and tape images here, and pass on the rom images to the normal message.
- (void) openDocumentWithContentsOfURL:(NSURL *)url display:(BOOL)displayDocument completionHandler:(void (^ _Nullable)(NSDocument *, BOOL, NSError * _Nullable))completionHandler
{
    NSString *nativeFilename;
	NSString *filenameExtension;
	const char *filename;
	nativeFilename = [url path];
	filenameExtension = [nativeFilename pathExtension];
	filename = [nativeFilename cStringUsingEncoding:NSASCIIStringEncoding];
	
	if (filename != NULL) {

		// Do comparo here to find out if this is a disk/tape image or not...
		if ([filenameExtension caseInsensitiveCompare:@"dsk"] == NSOrderedSame || 
			[filenameExtension caseInsensitiveCompare:@"img" ] == NSOrderedSame) 
		{
			//open new dialog here to determine which disk to change
			int choice = [self lastSelectedDiskNumber];
			DiskClose(choice);
			strncpy(cntDiskName[choice],filename,200);
			DiskName[choice] = cntDiskName[choice];
			DiskOpen(choice);
			[super noteNewRecentDocumentURL:url];
			return;
		}
		else if ([filenameExtension caseInsensitiveCompare:@"ddp"] == NSOrderedSame) 
		{
			int choice = [self lastSelectedTapeNumber];
			TapeClose(choice);
			strncpy(cntTapeName[choice],filename,200);
			TapeName[choice] = cntTapeName[choice];
			TapeOpen(choice);
			[super noteNewRecentDocumentURL:url];
			return;
		} 
			
			
	}
	
	return [super openDocumentWithContentsOfURL:url display:displayDocument completionHandler:completionHandler];
}

- (id)didSucceed:(BOOL)ok {
    return self;
}
@end
