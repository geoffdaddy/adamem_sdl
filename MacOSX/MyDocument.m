//
//  MyDocument.m
//  ADAMEm
//
//  Created by Jennifer and Geoffrey Oltmans on 11/12/08.
//  Copyright 2008 Geoff Oltmans. All rights reserved.
//

#import "MyDocument.h"
#include "Coleco.h"

//typedef unsigned long NSUInteger;

@implementation MyDocument

- (id)init
{
    self = [super init];
    if (self) {
		
        // Add your subclass-specific initialization here.
        // If an error occurs here, send a [self release] message and return nil.
		
    }
    return self;
}

- (NSString *)windowNibName {
    // Implement this to return a nib to load OR implement -makeWindowControllers to manually create your controllers.
    return @"MyDocument";
}

- (void)windowControllerDidLoadNib:(NSWindowController *) aController
{
    [super windowControllerDidLoadNib:aController];
    // Add any code here that needs to be executed once the windowController has loaded the document's window.
}

- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError
{
    // Insert code here to write your document to data of the specified type. If the given outError != NULL, ensure that you set *outError when returning nil.

    // You can also choose to override -fileWrapperOfType:error:, -writeToURL:ofType:error:, or -writeToURL:ofType:forSaveOperation:originalContentsURL:error: instead.

    // For applications targeted for Panther or earlier systems, you should use the deprecated API -dataRepresentationOfType:. In this case you can also choose to override -fileWrapperRepresentationOfType: or -writeToFile:ofType: instead.

    return nil;
}

- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError
{
	NSUInteger cartSize;
	int i;
    // For applications targeted for Panther or earlier systems, you should use the deprecated API -loadDataRepresentation:ofType. In this case you can also choose to override -readFromFile:ofType: or -loadFileWrapperRepresentation:ofType: instead.
    if (![typeName compare:@"ROM Image"]) {
		cartSize = [data length];
        if (!CART)
        {
            return true;
        }
		if (cartSize <= 1024*1024)
		{
			[data getBytes:CART length:cartSize];
            CartSize = cartSize;
            if ( CartSize > 0x8000 )
            {
                CartBanks = CartSize/(16*1024);
                CurrCartBank = CartBanks-1;
            }
            
            for(i = 0x8000; i <=0xffff; i+=256)
            {
                if (CartSize > 0x8000) {
                    if (i<0xC000) {
                        AddrTabl[i>>8]=&CART[CartSize-0x4000 + (i-0x8000)];
                    }
                    else {
                        AddrTabl[i>>8]=&CART[CartSize-0x4000 + (i-0xc000)];
                    }
                } else {
                    AddrTabl[i>>8] = &CART[i-0x8000];
                }
            }
		}
		ResetColeco(1);
	} else if (![typeName compare:@"Disk Image"]) {
		NSLog(@"This is a disk image!\n");
		NSLog([file relativeString]);
		if (DiskName[0] != 0) {
			DiskClose(0);
		}
//		DiskName
		ResetColeco(0);
	}
		
    return YES;
}

-(void)setFileURL:(NSURL *) newFile
{
	file = newFile;
}


@end
