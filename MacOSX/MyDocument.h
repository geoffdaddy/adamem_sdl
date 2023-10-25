//
//  MyDocument.h
//  ADAMEm
//
//  Copyright 2008-2023 Geoff Oltmans. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface MyDocument : NSDocument {
	NSURL *file;
}

-(void)setFileURL:(NSURL *) newFile;
@end
