//
//  JsonIndentFormatter.hpp
//  Bracez
//
//  Created by Eldan Ben Haim on 01/03/2021.
//

#ifndef JsonIndentFormatter_hpp
#define JsonIndentFormatter_hpp

#include <string>
#include "json_file.h"
#include "LinesAndBookmarks.h"
#include "reader.h"

struct TokenStreamAndUnderlying {
    TokenStreamAndUnderlying(const std::wstring &text,
                             unsigned long startOffset,
                             int startRow, int startCol)
        : stringStream(text),
          inputStream(stringStream, NULL, TextCoordinate(startOffset)),
          tokenStream(inputStream, NULL, false, startRow, startCol)
    { 
        stringStream.seekg(startOffset);
    }
    
    std::basic_stringstream<wchar_t> stringStream;
    json::InputStream inputStream;
    json::TokenStream tokenStream;
};

class JsonIndentationContext {
public:
    int currentIndent();

    void pushIndentLevel();
    void popIndentLevel();

public:
    static JsonIndentationContext approximateBySingleLine(const std::wstring &text, LinesAndBookmarks &linesAndBookmarks,
                                                          TextCoordinate aOffset, int aIndentSize);
    static JsonIndentationContext approximateWithTokenStream(json::TokenStream &aTokStream, TextCoordinate aOffset, int aIndentSize);
    
    static JsonIndentationContext approximateWithDocument(const json::JsonFile &file,
                                                          const LinesAndBookmarks &linesAndBookmarks,
                                                          TextCoordinate aOffset, int aIndentSize);

private:
    JsonIndentationContext(int initialIndent, int indentSize);
    JsonIndentationContext(const std::vector<int> &&indentLevels, int indentSize);
    
    void setInitialIndentLevel(int level);
    
    std::vector<int> indentLevelStack;
    int indentSize;
};

class JsonIndentFormatter {
public:
    JsonIndentFormatter(const std::wstring &text,
                        const json::JsonFile &jsonFile,
                        LinesAndBookmarks &linesAndBookmarks,
                        TextCoordinate aOffsetStart, TextLength aLen, int indentSize);
    
    const std::wstring &getIndented();
    
private:
    void reindent();
    
    void skipInputWhitespace();
    void outputIndent();
    void copyAndEnsureNewLine();

    void outputString(const std::wstring &string);
    
private:
    
    TokenStreamAndUnderlying tokStreamAndCo;
    JsonIndentationContext indentationContext;
    
    TextCoordinate startOffset;
    TextCoordinate endOffset;
    
    unsigned int outputCol;    
    std::wstring output;
};

#endif /* JsonIndentFormatter_hpp */
