//
//  local_reparse_tests.cpp
//  Bracez
//
//  Created by Eldan Ben-Haim on 07/12/2021.
//

#include "json_file.h"
#include "reader.h"
#include "catch2/catch.hpp"
#include <fstream>
#include <string>
#include <codecvt>
#include <locale>

using namespace json;

static std::unique_ptr<ObjectNode> readJsonLineFromStream(istream &stream) {
    if(stream.eof()) {
        throw Exception("EOF encountered");
    }
    
    std::string ln;
    std::getline(stream, ln);
    std::wstring wln = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ln);
    
    ObjectNode *o = NULL;
    if(!wln.empty()) {
        Reader::Read(o, wln);
    }
    return std::unique_ptr<ObjectNode>(o);
}

template<class T>
const T &getObjectMemberValue(ObjectNode* o, const std::wstring &memberName) {
    int memIdx = o->getIndexOfMemberWithName(memberName);
    if(memIdx<0) {
        throw Exception(std::string("Missing member"));
    }
    
    ValueNode<T> *node = dynamic_cast<ValueNode<T>*>(o->getChildAt(memIdx));
    if(!node) {
        throw Exception(std::string("Invalid member type"));
    }
    
    return node->getValue();
}


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
    
    
    static JsonEditTestDescription fromEditSessionRecording(std::istream &recordingSessionStream) {
        JsonEditTestDescription ret;
        
        std::unique_ptr<ObjectNode> fileDesc = readJsonLineFromStream(recordingSessionStream);
        ret.startFile = getObjectMemberValue<std::wstring>(fileDesc.get(), L"content");
            
        while(!recordingSessionStream.eof()) {
            std::unique_ptr<ObjectNode> fileDesc = readJsonLineFromStream(recordingSessionStream);
            if(!fileDesc) continue;
            ret.editActions.push_back(EditAction(
                       getObjectMemberValue<double>(fileDesc.get(), L"start"),
                       getObjectMemberValue<double>(fileDesc.get(), L"len"),
                                                 getObjectMemberValue<std::wstring>(fileDesc.get(), L"new_text")));            
        }
        
        return ret;
    }
    
private:
    std::wstring startFile;
    Edits editActions;
};

JsonEditTestDescription loadEditTestDescriptionFromFile(const std::string &fileName) {
    ifstream fileStream;
    std::string absFileName = std::string(SOURCE_ROOT_FOLDER) + "/" + fileName;
    fileStream.open(absFileName);
    if(!fileStream.is_open()) {
        throw Exception("Could not fead file");
    }
    
    return JsonEditTestDescription::fromEditSessionRecording(fileStream);
}

static std::wstring debugJsonForDoc(JsonFile* doc) {
    std::unique_ptr<Node> debugDesc(doc->getDom()->createDebugRepresentation());
    std::wstring debugDescJson;
    debugDesc->calculateJsonTextRepresentation(debugDescJson);
    
    return debugDescJson;
}

TEST_CASE("JSON file local reparse text") {
    auto testedEdit = GENERATE(
                               JsonEditTestDescription::fromInlineEditDescription(L"{ \"hello\": <<<<3====22>>>> }"),
                               
                               loadEditTestDescriptionFromFile( "json_model/tests/fixtures/edit_recording_test_local_reparse_1.txt"),

                               loadEditTestDescriptionFromFile("json_model/tests/fixtures/edit_recording_test_local_reparse_2.txt"),

                               loadEditTestDescriptionFromFile("json_model/tests/fixtures/edit_recording_test_local_reparse_3.txt")
                               );
    
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

TEST_CASE("JSON file local reparse: string unicode literal bug") {
    // Run edits
    std::unique_ptr<JsonFile> preEditDoc(new JsonFile());
    preEditDoc->setText(L"{\"b\":\"\\\"\n}");
    
    // There's an unterminated string here. We should have an error on it
    REQUIRE(preEditDoc->getErrors().size() == 1);
    
    bool fastParseResult = preEditDoc->fastSpliceTextWithWorkLimit(TextCoordinate(7), 0, L"u", 1024);
    REQUIRE(fastParseResult == false);
}
    

TEST_CASE("JSON file local reparse: EOS errors ") {
    // Run edits
    std::unique_ptr<JsonFile> preEditDoc(new JsonFile());
    preEditDoc->setText(L"{\"d\":\"\\\"");
    
    // There's an unterminated string here. We should have an error on it
    REQUIRE(preEditDoc->getErrors().size() == 1);
    
    bool fastParseResult = preEditDoc->fastSpliceTextWithWorkLimit(TextCoordinate(7), 0, L"n", 1024);
    REQUIRE(fastParseResult == false);
}
    



