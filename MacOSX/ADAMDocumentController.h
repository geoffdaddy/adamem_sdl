//
//  ADAMDocumentController.h
//  ADAMEm
//
//  Created by Jennifer and Geoffrey Oltmans on 11/17/08.
//  Copyright 2008 Geoff Oltmans. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ADAMDocumentController : NSDocumentController {
	int lastSelectedDiskNumber;
	int lastSelectedTapeNumber;
}

- (int)runModalOpenPanel:(NSOpenPanel *)openPanel forTypes:(NSArray *)fileNameExtensionsAndHFSFileTypes;

//- (id)openDocumentWithContentsOfURL:(NSURL *)absoluteURL display:(BOOL)displayDocument error:(NSError **)outError;

- (void) openDocumentWithContentsOfURL:(NSURL *)url display:(BOOL)displayDocument completionHandler:(void (^ _Nullable)(NSDocument *, BOOL, NSError * _Nullable))completionHandler;

- (int)lastSelectedDiskNumber;
- (int)lastSelectedTapeNumber;

@end
