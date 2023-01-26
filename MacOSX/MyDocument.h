//
//  MyDocument.h
//  ADAMEm
//
//  Created by Jennifer and Geoffrey Oltmans on 11/12/08.
//  Copyright 2008 Geoff Oltmans. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface MyDocument : NSDocument {
	NSURL *file;
}

-(void)setFileURL:(NSURL *) newFile;
@end
