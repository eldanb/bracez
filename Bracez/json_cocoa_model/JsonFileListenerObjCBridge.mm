//
//  JsonFileListenerObjCBridge.mm
//  JsonMockup
//
//  Created by Eldan on 11/28/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonFileListenerObjCBridge.h"

using namespace json;

JsonFileListenerObjCBridge::JsonFileListenerObjCBridge(id<ObjCJsonFileChangeListener> aTarget) 
   : targetListener(aTarget)
{
}
   
void JsonFileListenerObjCBridge::notifyTextSpliced(JsonFile *aSender,
                                                   TextCoordinate aOldOffset, TextLength aOldLength, TextLength aNewLength,
                                                   TextCoordinate aOldLineStart, TextLength aOldLineLength,
                                                   TextLength aNewLineLength)
{
   [targetListener notifyJsonTextSpliced:aSender
                                    from:aOldOffset
                                  length:aOldLength
                               newLength:aNewLength
                       affectedLineStart:aOldLineStart
                       numDeletedLines:aOldLineLength
                           numAddedLines:aNewLineLength];
}

void JsonFileListenerObjCBridge::notifyNodeChanged(JsonFile *aSender, const JsonPath &nodePath) {
    [targetListener notifyNodeInvalidated:aSender nodePath:nodePath];
}

void JsonFileListenerObjCBridge::notifyErrorsChanged(json::JsonFile *aSender)
{
   [targetListener notifyErrorsChanged:aSender];
}
