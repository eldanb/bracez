/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton

***********************************************/

#pragma once

#include "json_file.h"
#include <iostream>
#include <vector>

namespace json
{

#define PARSER_ERROR_UNEXPECTED_TOKEN     1
#define PARSER_ERROR_UNEXPECTED_CHARACTER 2
#define PARSER_ERROR_UNEXPECTED_EOS       3
#define PARSER_ERROR_EXPECTED_EOS         4
#define PARSER_ERROR_DUPLICATE_MEMBER     5
#define PARSER_ERROR_MALFORMED_NUMBER     6
#define PARSER_ERROR_INVALID_MEMBER       7


//////////////////////
struct ParseListener
{
    virtual void EndOfLine(TextCoordinate aWhere) = 0;
    virtual void Error(TextCoordinate aWhere, int aCode, const string &aText)=0;
};

class InputStream // would be cool if we could inherit from std::istream & override "get"
{
public:
    InputStream(std::wistream& iStr, ParseListener *aListener = NULL, TextCoordinate aStartLocation = TextCoordinate()) :
          m_iStr(iStr), m_Location(aStartLocation), m_parseListener(aListener) {}

    // protect access to the input stream, so we can keeep track of document/line offsets
    wchar_t Get();
    wchar_t Peek();

    bool EOS() const;

    bool VerifyString(const std::wstring &sExpected);
    const TextCoordinate& GetLocation() const { return m_Location; }

private:
   std::wistream& m_iStr;
   TextCoordinate m_Location;
   ParseListener *m_parseListener;
};

struct Token
{
   enum Type
   {
      TOKEN_UNKNOWN,       //    invalid bareword
      TOKEN_OBJECT_BEGIN,  //    {
      TOKEN_OBJECT_END,    //    }
      TOKEN_ARRAY_BEGIN,   //    [
      TOKEN_ARRAY_END,     //    ]
      TOKEN_NEXT_ELEMENT,  //    ,
      TOKEN_MEMBER_ASSIGN, //    :
      TOKEN_STRING,        //    "xxx"
      TOKEN_NUMBER,        //    [+/-]000.000[e[+/-]000]
      TOKEN_BOOLEAN,       //    true -or- false
      TOKEN_NULL,          //    null
      TOKEN_WHITESPACE,    //    spacebar, newline, tab

   };
    
   enum CharClass {
        CHAR_CLASS_UNKNOWN = 0,
        CHAR_CLASS_NUMERIC = 1,
   };
 
    Token() : nType(TOKEN_UNKNOWN) {}

   Type nType;
   std::wstring sValue;
   std::wstring sOrgText;

   // for malformed file debugging
   TextCoordinate locBegin;
   TextCoordinate locEnd;
};

class TokenStream
{
public:
   TokenStream(InputStream &aInputStream, ParseListener *aListener, bool aSkipWhitespace = true, int startRow = 0, int startCol = 0);

   const Token& Peek();
   const Token& Get();

   bool EOS() const;

    int Row();
    int Col();
    
    const InputStream &getInputStream() const { return inputStream; }
    
private:
   void pumpTokenIfNeeded();
   void pumpToken();
   void EatWhiteSpace();
   void Match4Hex(unsigned int& integer);
   void MatchString();
   void MatchBareWordToken();
   void MatchNumber();
    void updateLineCol(wchar_t forChar);

   bool skipWhitespace;
   bool tokenEaten;
   Token currentToken;
    
   int currentCol;
   int currentRow;
    bool prevNewLine;

   InputStream &inputStream;
   ParseListener *listener;
   
   char tokenTypeLookup[256];
   char charClassLookup[256];
};

class Reader
{
public:
   
   // thrown during the first phase of reading. generally catches low-level problems such
   //  as errant characters or corrupt/incomplete documents
   class ScanException : public Exception
   {
   public:
      ScanException(const std::string& sMessage, const TextCoordinate& locError) :
         Exception(sMessage),
         m_locError(locError) {}

      TextCoordinate m_locError;
   };

   // thrown during the second phase of reading. generally catches higher-level problems such
   //  as missing commas or brackets
   class ParseException : public Exception
   {
   public:
      ParseException(const std::string& sMessage, const TextCoordinate& locTokenBegin, const TextCoordinate& locTokenEnd) :
         Exception(sMessage),
         m_locTokenBegin(locTokenBegin),
         m_locTokenEnd(locTokenEnd) {}

      TextCoordinate m_locTokenBegin;
      TextCoordinate m_locTokenEnd;
   };


   // if you know what the document looks like, call one of these...
   static void Read(ObjectNode *& object, std::wistream& istr);
   static void Read(ArrayNode *& array, std::wistream& istr);
   static void Read(StringNode *& string, std::wistream& istr);
   static void Read(NumberNode *& number, std::wistream& istr);
   static void Read(BooleanNode *& boolean, std::wistream& istr);
   static void Read(NullNode *& null, std::wistream& istr);

   // ...otherwise, if you don't know, call this & visit it
   static void Read(Node *& elementRoot, std::wistream& istr, ParseListener *aParseListener=NULL, bool allowSuffix = false);

private:
   Reader(ParseListener *aParseListener);
    
   template <typename ElementTypeT>   
   static void Read_i(ElementTypeT& element, std::wistream& istr, ParseListener *aListener=NULL, bool allowSuffix = false);

public:
   // parsing token sequence into element structure
   void Parse(Node *& element, TokenStream& tokenStream, TextCoordinate aBaseOfs=TextCoordinate(0));
   void Parse(ObjectNode *& object, TokenStream& tokenStream, TextCoordinate aBaseOfs=TextCoordinate(0));
   void Parse(ArrayNode *& array, TokenStream& tokenStream, TextCoordinate aBaseOfs=TextCoordinate(0));
   void Parse(StringNode *& string, TokenStream& tokenStream, TextCoordinate aBaseOfs=TextCoordinate(0));
   void Parse(NumberNode *& number, TokenStream& tokenStream, TextCoordinate aBaseOfs=TextCoordinate(0));
   void Parse(BooleanNode *& boolean, TokenStream& tokenStream, TextCoordinate aBaseOfs=TextCoordinate(0));
   void Parse(NullNode *& null, TokenStream& tokenStream, TextCoordinate aBaseOfs=TextCoordinate(0));

   Token MatchExpectedToken(Token::Type nExpected, TokenStream& tokenStream);
    
    
private:
    ParseListener *listener;
};


} // End namespace


#include "reader.inl"
