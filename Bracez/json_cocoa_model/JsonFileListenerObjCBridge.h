//
//  JsonFileListenerObjCBridge.h
//  JsonMockup
//
//  Created by Eldan on 11/28/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "json_file.h"
#import "marker_list.h"

@protocol ObjCJsonFileChangeListener

-(void)notifyJsonTextSpliced:(json::JsonFile*)aSender from:(TextCoordinate)aOldOffset length:(TextLength)aOldLength newLength:(TextLength)aNewLength;
-(void)notifyErrorsChanged:(json::JsonFile*)aSender;

@end

class JsonFileListenerObjCBridge : public json::JsonFileChangeListener
{
public:
   JsonFileListenerObjCBridge(id<ObjCJsonFileChangeListener> aTarget);   
   void notifyTextSpliced(json::JsonFile *aSender, TextCoordinate aOldOffset, TextLength aOldLength, TextLength aNewLength);
   void notifyErrorsChanged(json::JsonFile *aSender);
private:
   id<ObjCJsonFileChangeListener> targetListener;
};

