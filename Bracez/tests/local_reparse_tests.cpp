//
//  local_reparse_tests.cpp
//  Bracez
//
//  Created by Eldan Ben-Haim on 07/12/2021.
//

#include "json_file.h"
#include "reader.h"
#include "catch2/catch.hpp"
#include <string>

using namespace json;

class JsonEditTestDescription {
public:
    struct EditAction {
        EditAction(int es, int edc, const std::wstring &ei) :
            editStart(es), editDeleteCount(edc), editInsert(ei) {}
        
        void applyToString(std::wstring &str) const {
            str = str.substr(0, editStart) +
                  editInsert +
                  str.substr(editStart+editDeleteCount);
        }
        
        bool applyToJsonFile(JsonFile *file) const {
            return file->fastSpliceTextWithWorkLimit(TextCoordinate(editStart),
                                                     editDeleteCount,
                                                     editInsert,
                                                     256*1024);
        }
        
        int editStart;
        int editDeleteCount;
        std::wstring editInsert;
    };
    
    typedef vector<EditAction> Edits;
    
private:
    JsonEditTestDescription() {
    }
    
public:
    const std::wstring &getStartFile() const { return startFile; }
    const Edits &getEditActions() const { return editActions; }

    std::wstring getEndFile() const {
        std::wstring endFile = startFile;
        for_each(editActions.begin(), editActions.end(), [&endFile](const EditAction &a) {
            a.applyToString(endFile);
        });
        return endFile;
    }
    
public:
    static JsonEditTestDescription fromInlineEditDescription(const std::wstring &desc) {
        int editStart = (int)(desc.find(L"<<<<"));
        
        int editDeleteStart = (int)desc.find(L"====");
        int editDeleteCount = (editDeleteStart - (editStart+4));
        
        int editInsertEnd = (int)desc.find(L">>>>");
        std::wstring editInsert = desc.substr(editDeleteStart+4, editInsertEnd-(editDeleteStart+4));
        
        std::wstring preEdit = desc.substr(0, editStart);
        std::wstring postEdit = desc.substr(editInsertEnd+4);
        
        JsonEditTestDescription ret;
        ret.startFile = preEdit +
                    desc.substr(editStart+4, editDeleteCount) +
                    postEdit;
        ret.editActions.push_back(EditAction(editStart, editDeleteCount, editInsert));
        
        return ret;
    }
    
private:
    std::wstring startFile;
    Edits editActions;
};

static std::wstring debugJsonForDoc(JsonFile* doc) {
    std::unique_ptr<Node> debugDesc(doc->getDom()->createDebugRepresentation());
    std::wstring debugDescJson;
    debugDesc->calculateJsonTextRepresentation(debugDescJson);
    
    return debugDescJson;
}

TEST_CASE("JSON file local reparse text") {
    auto testedEdit = GENERATE(JsonEditTestDescription::fromInlineEditDescription(L"{ \"hello\": <<<<3====22>>>> }"));

    std::unique_ptr<JsonFile> postEditDoc(new JsonFile());
    postEditDoc->setText(testedEdit.getEndFile());
    std::wstring postEditDebugJson = debugJsonForDoc(postEditDoc.get());
    
    // Run edits
    std::unique_ptr<JsonFile> preEditDoc(new JsonFile());
    preEditDoc->setText(testedEdit.getStartFile());
    for_each(testedEdit.getEditActions().begin(), testedEdit.getEditActions().end(),
             [&preEditDoc](const JsonEditTestDescription::EditAction &a) {
        bool spliceResult = a.applyToJsonFile(preEditDoc.get());
        REQUIRE(spliceResult);
    });
    
    REQUIRE(debugJsonForDoc(postEditDoc.get()) == postEditDebugJson);
}


