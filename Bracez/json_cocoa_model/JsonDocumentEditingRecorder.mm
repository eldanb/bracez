//
//  JsonDocumentEditingRecorder.cpp
//  Bracez
//
//  Created by Eldan Ben-Haim on 08/12/2021.
//

#include "JsonDocumentEditingRecorder.hpp"
#include "json_file.h"

#include "NSString+WStringUtils.h"

#define BRACEZ_RECORD_EDITS

#if defined(BRACEZ_RECORD_EDITS) && defined(DEBUG)

class FileJsonDocumentEditingRecorder : public JsonDocumentEditingRecorder {
public:
    FileJsonDocumentEditingRecorder()
    : needToEmitFileContent(true) {
        
    }
    
    virtual void prepareToRecordSpliceOnFile(json::JsonFile *file) {
        if(needToEmitFileContent) {
            json::ObjectNode commandNode;
            commandNode.domAddMemberNode(L"action", new json::StringNode(L"file_content"));
            commandNode.domAddMemberNode(L"content", new json::StringNode(file->getText()));

            std::wstring command;
            commandNode.calculateJsonTextRepresentation(command);
            
            outputCommand(command);
            
            needToEmitFileContent = false;
        }
    }
    
    virtual void recordFastSplice(json::JsonFile *file, TextCoordinate start, TextLength len, const std::wstring &newText) {
        json::ObjectNode commandNode;
        commandNode.domAddMemberNode(L"action", new json::StringNode(L"splice"));
        commandNode.domAddMemberNode(L"start", new json::NumberNode(start.getAddress()));
        commandNode.domAddMemberNode(L"len", new json::NumberNode(len));
        commandNode.domAddMemberNode(L"new_text", new json::StringNode(newText));

        std::wstring command;
        commandNode.calculateJsonTextRepresentation(command);
        
        outputCommand(command);
    }

    virtual void recordSlowSplice(json::JsonFile *file, TextCoordinate start, TextLength len, const std::wstring &newText) {
        needToEmitFileContent = true;
    }
    
private:
    void outputCommand(const std::wstring &command) {
        NSLog(@"REC>>> %@", [NSString stringWithWstring:command]);
    }
    
    bool needToEmitFileContent;
};


JsonDocumentEditingRecorder *JsonDocumentEditingRecorder::createForFile(json::JsonFile *file) {
    return new FileJsonDocumentEditingRecorder();
}

#else
JsonDocumentEditingRecorder *JsonDocumentEditingRecorder::createForFile(json::JsonFile *file) {
    return NULL;
}
#endif
