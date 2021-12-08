//
//  JsonDocumentEditingRecorder.hpp
//  Bracez
//
//  Created by Eldan Ben-Haim on 08/12/2021.
//

#ifndef JsonDocumentEditingRecorder_hpp
#define JsonDocumentEditingRecorder_hpp

#include "json_file.h"

class JsonDocumentEditingRecorder {
public:
    virtual ~JsonDocumentEditingRecorder() {}
    
    virtual void prepareToRecordSpliceOnFile(json::JsonFile *file) = 0;
    virtual void recordFastSplice(json::JsonFile *file, TextCoordinate start, TextLength len, const std::wstring &newText) = 0;
    virtual void recordSlowSplice(json::JsonFile *file, TextCoordinate start, TextLength len, const std::wstring &newText) = 0;

public:
    static  JsonDocumentEditingRecorder *createForFile(json::JsonFile *file);
};
#endif /* JsonDocumentEditingRecorder_hpp */
