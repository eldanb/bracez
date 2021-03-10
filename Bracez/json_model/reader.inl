/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton

***********************************************/

#include <cassert>
#include <set>
#include <sstream>
#include <codecvt>

/*  

TODO:
* better documentation
* unicode character decoding

*/


// convert wstring to UTF-8 string
static std::string wstring_to_utf8 (const std::wstring& str)
{
    std::wstring_convert< std::codecvt_utf8<wchar_t> > myconv;
    return myconv.to_bytes(str);
}

namespace json
{


//////////////////////
// Reader::InputStream

   

inline wchar_t InputStream::Peek() {
  assert(m_iStr.eof() == false); // enforce reading of only valid stream data
  return m_iStr.peek();
}

inline bool InputStream::EOS() const {
  m_iStr.peek(); // apparently eof flag isn't set until a character read is attempted. whatever.
  return m_iStr.eof();
}

inline bool InputStream::VerifyString(const std::wstring &sExpected)
{
  bool lRet=true;
  int lPerformedGetCount  = 0;
  std::wstring::const_iterator it(sExpected.begin()),
                               itEnd(sExpected.end());
  for ( ; lRet && it != itEnd; ++it) {
      if (EOS() ||      // did we reach the end before finding what we're looking for...
          (lPerformedGetCount++, m_iStr.get()) != *it) // ...or did we find something different?
      {
         lRet=false;
      }
   }
   
   while(lPerformedGetCount--)
   {
      m_iStr.unget();
   }
   
   return lRet;
}

inline wchar_t InputStream::Get()
{
   assert(m_iStr.eof() == false); // enforce reading of only valid stream data 
   wchar_t c = m_iStr.get();
   
   m_Location = m_Location + 1;
   if (c == L'\n') {
      if(m_parseListener)
      {
         m_parseListener->EndOfLine(m_Location-1);
      }
   }

   return c;
}





inline TokenStream::TokenStream(InputStream& aInputStream,
                                ParseListener *aListener,
                                bool aSkipWhitespace,
                                int startRow,
                                int startCol) :
    inputStream(aInputStream),
    tokenEaten(true),
    listener(aListener),
    skipWhitespace(aSkipWhitespace),
    currentRow(startRow),
    currentCol(startCol)
{
    // Prepare lookup tables
    memset(tokenTypeLookup, Token::TOKEN_UNKNOWN, 256);
    
    
    tokenTypeLookup['\n'] = Token::TOKEN_WHITESPACE;
    tokenTypeLookup['\t'] = Token::TOKEN_WHITESPACE;
    tokenTypeLookup[' '] = Token::TOKEN_WHITESPACE;
    
    tokenTypeLookup['{'] = Token::TOKEN_OBJECT_BEGIN;
    tokenTypeLookup['}'] = Token::TOKEN_OBJECT_END;
    tokenTypeLookup['['] = Token::TOKEN_ARRAY_BEGIN;
    tokenTypeLookup[']'] = Token::TOKEN_ARRAY_END;
    tokenTypeLookup[','] = Token::TOKEN_NEXT_ELEMENT;
    tokenTypeLookup[':'] = Token::TOKEN_MEMBER_ASSIGN;
    
    tokenTypeLookup['-'] = Token::TOKEN_NUMBER;
    for(int i='0'; i<='9'; i++) tokenTypeLookup[i] = Token::TOKEN_NUMBER;
    
    tokenTypeLookup['"'] =  Token::TOKEN_STRING;
    
    
    memset(charClassLookup, Token::CHAR_CLASS_UNKNOWN, 256);
    const char lNumericChars[] = "0123456789.eE-+";
    for(const char *lChar = lNumericChars; *lChar; lChar++) charClassLookup[*lChar] = Token::CHAR_CLASS_NUMERIC;
    
    
    EatWhiteSpace();              // ignore any leading white space...
}

inline const Token& TokenStream::Peek() {
   pumpTokenIfNeeded();
    
   return currentToken;
}

inline const Token& TokenStream::Get() {
   assert(!EOS());
   pumpTokenIfNeeded();
   tokenEaten = true;
   
   return currentToken; 
}

inline bool TokenStream::EOS() const {
   return tokenEaten && inputStream.EOS(); 
}

inline int TokenStream::Row() {
    return currentRow;
}

inline int TokenStream::Col() {
    return currentCol;
}

inline void TokenStream::pumpTokenIfNeeded()
{
    if(tokenEaten)
    {
       assert(!EOS());
       pumpToken();
       tokenEaten = false;
    }
}

inline void TokenStream::pumpToken()
{
  // Mark token start
  currentToken.locBegin = inputStream.GetLocation();
  
  // Get current token type (good guess...)
  wchar_t sChar = inputStream.Peek();
  currentToken.nType = (Token::Type)tokenTypeLookup[sChar];

  switch (currentToken.nType)
  {
     case Token::TOKEN_NUMBER:
        MatchNumber();
        break;
        
     case Token::TOKEN_STRING:
        MatchString();
        break;
        
     case Token::TOKEN_UNKNOWN:
        MatchBareWordToken();
        break;
                  
     default:   // Default case is for simple tokens
        currentToken.sOrgText = inputStream.Get();
        updateLineCol(sChar);
  }
  
  currentToken.locEnd = inputStream.GetLocation();
  
  EatWhiteSpace();              // Skip to next real token
}

inline void TokenStream::EatWhiteSpace()
{
    if(skipWhitespace) {
       while (inputStream.EOS() == false &&
              ::isspace(inputStream.Peek()))
           updateLineCol(inputStream.Get());
    }
}

inline void TokenStream::Match4Hex(unsigned int& integer) {
   std::wstringstream ss;
   int i;
   for (i = 0; (i < 4) && (inputStream.EOS() == false) && isxdigit(inputStream.Peek()); ++i)
      ss << inputStream.Get();

   listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected a hex digit");

   ss << std::hex;
   ss >> integer;
}

inline void TokenStream::MatchString()
{
   // Eat starting "\""
   inputStream.Get();
   
   // Initialize value
   currentToken.sValue.clear();
   currentToken.sOrgText = L"\"";

   while (inputStream.EOS() == false &&
          inputStream.Peek() != L'"')
   {
      wchar_t c = inputStream.Get();
      currentToken.sOrgText.push_back(c);
       
      updateLineCol(c);
       
      // escape?
      if (c == L'\\' &&
          inputStream.EOS() == false) // shouldn't have reached the end yet
      {
         c = inputStream.Get();
         currentToken.sOrgText.push_back(c);
          
         switch (c) {
            case L'/':      currentToken.sValue.push_back(L'/');     break;
            case L'"':      currentToken.sValue.push_back(L'"');     break;
            case L'\\':     currentToken.sValue.push_back(L'\\');    break;
            case L'b':      currentToken.sValue.push_back(L'\b');    break;
            case L'f':      currentToken.sValue.push_back(L'\f');    break;
            case L'n':      currentToken.sValue.push_back(L'\n');    break;
            case L'r':      currentToken.sValue.push_back(L'\r');    break;
            case L't':      currentToken.sValue.push_back(L'\t');    break;
            case L'u':
                {
                    unsigned int codepoint = 0, surrogate = 0;
                    Match4Hex(codepoint);
                    if ((codepoint & 0xFC00) == 0xD800) { // Surrogate?
                        if(inputStream.Peek() == L'\\')
                        {
                            inputStream.Get(); 
                            if(inputStream.Peek() == L'u')
                            {
                                Match4Hex(surrogate);
                                codepoint = (((codepoint & 0x3F) << 10) | 
                                    ((((codepoint >> 6) & 0xF) + 1) << 16) | 
                                    (surrogate & 0x3FF));
                            } else
                            {
                                listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected \\u after surrogate character.");
                            }
                        } else
                        {
                            listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected \\u after surrogate character.");
                        }                                
                    }

                    currentToken.sValue.push_back(codepoint);
                    /*
                    // To UTF8
                    if (codepoint < 0x80) {
                        currentToken.sValue.push_back((char)codepoint);
                    } else if (codepoint < 0x0800) {
                        currentToken.sValue.push_back((char)((codepoint >> 6) | 0xC0));
                        currentToken.sValue.push_back((char)((codepoint & 0x3F) | 0x80));
                    } else if (codepoint < 0x10000) {
                        currentToken.sValue.push_back((char)((codepoint >> 12) | 0xE0));
                        currentToken.sValue.push_back((char)(((codepoint >> 6) & 0x3F) | 0x80));
                        currentToken.sValue.push_back((char)((codepoint & 0x3F) | 0x80));
                    } else if (codepoint < 0x200000) {
                        currentToken.sValue.push_back((char)((codepoint >> 18) | 0xF0));
                        currentToken.sValue.push_back((char)(((codepoint >> 12) & 0x3F) | 0x80));
                        currentToken.sValue.push_back((char)(((codepoint >> 6) & 0x3F) | 0x80));
                        currentToken.sValue.push_back((char)((codepoint & 0x3F) | 0x80));
                    } else {
                        currentToken.sValue.push_back('?');
                    }*/
                    break;
                }
            default: {
               std::string sMessage = "Unrecognized escape sequence found in string: \\" + wstring_to_utf8(wstring(&c, 1));
               listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, sMessage);
            }
         }
      }
      else {
         currentToken.sValue.push_back(c);
      }
   }

    if(inputStream.EOS() == false) {
        // eat the last '"' that we just peeked        
        currentToken.sOrgText.push_back(inputStream.Get());
    } else {
        std::string sMessage = "Unterminated string constant.";
        listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, sMessage);
    }
}

inline void TokenStream::MatchBareWordToken()
{
    currentToken.sValue.clear();
    
    wchar_t c;
    while(!inputStream.EOS() &&     
          !::isspace(c=inputStream.Peek()) &&
          c != L']' && c != L'}' && c != L',' && c != L':'  // Pretty ugly hack
            )
    {
        updateLineCol(c);
        currentToken.sValue.push_back(inputStream.Get());
    }
    
    if(currentToken.sValue == L"true" || currentToken.sValue == L"false")
    {
        currentToken.nType = Token::TOKEN_BOOLEAN;
    } else
    if(currentToken.sValue == L"null")
    {
        currentToken.nType = Token::TOKEN_NULL;
    } else
    {
        
        std::string sErrorMessage = "Unknown token in stream: " + wstring_to_utf8(currentToken.sValue);
        listener->Error(currentToken.locBegin, PARSER_ERROR_UNEXPECTED_CHARACTER, sErrorMessage);
    }
    
    currentToken.sOrgText = currentToken.sValue;
}

inline void TokenStream::MatchNumber()
{
   currentToken.sValue.clear();
      
   while (inputStream.EOS() == false &&
          charClassLookup[inputStream.Peek()] == Token::CHAR_CLASS_NUMERIC)
   {
       updateLineCol(inputStream.Peek());
      currentToken.sValue.push_back(inputStream.Get());   
   }
    
    currentToken.sOrgText = currentToken.sValue;
}

inline void TokenStream::updateLineCol(wchar_t forChar) {
    if(prevNewLine) {
        currentCol = 0;
        currentRow++;
        prevNewLine = false;
    } else {
        currentCol++;
        if(forChar == '\n') {
            prevNewLine = true;
        }
    }
}

///////////////////
// Reader (finally)

inline Reader::Reader(ParseListener *aParseListener) : listener(aParseListener)
{
}

inline void Reader::Read(ObjectNode*& object, std::wistream& istr)                { Read_i(object, istr); }
inline void Reader::Read(ArrayNode*& array, std::wistream& istr)                  { Read_i(array, istr); }
inline void Reader::Read(StringNode*& string, std::wistream& istr)                { Read_i(string, istr); }
inline void Reader::Read(NumberNode*& number, std::wistream& istr)                { Read_i(number, istr); }
inline void Reader::Read(BooleanNode*& boolean, std::wistream& istr)              { Read_i(boolean, istr); }
inline void Reader::Read(NullNode*& null, std::wistream& istr)                    { Read_i(null, istr); }
inline void Reader::Read(Node*& unknown, std::wistream& istr, ParseListener *aParseListener, bool allowSuffix)       { Read_i(unknown, istr, aParseListener, allowSuffix); }


template <typename ElementTypeT>   
void Reader::Read_i(ElementTypeT& element, std::wistream& istr, ParseListener *aParseListener, bool allowSuffix)
{
   Reader reader(aParseListener);

   InputStream inputStream(istr, aParseListener);
   TokenStream tokenStream(inputStream, aParseListener);
   reader.Parse(element, tokenStream);

   if (tokenStream.EOS() == false && !allowSuffix)
   {
      const Token& token = tokenStream.Peek();
      std::string sMessage = "Expected End of token stream; found " + wstring_to_utf8(token.sValue);
      aParseListener->Error(token.locBegin, PARSER_ERROR_EXPECTED_EOS, sMessage);
   }
}




inline void Reader::Parse(Node*& element, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   if (tokenStream.EOS()) {
      std::string sMessage = "Unexpected end of token stream";
      listener->Error(TextCoordinate(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of token stream");
      return;
   }

   const Token& token = tokenStream.Peek();
   switch (token.nType) {
      case Token::TOKEN_OBJECT_BEGIN:
      {
         // implicit non-const cast will perform conversion for us (if necessary)
         ObjectNode* object;
         Parse(object, tokenStream, aBaseOfs);
         element = object;
         break;
      }

      case Token::TOKEN_ARRAY_BEGIN:
      {
         ArrayNode* array;
         Parse(array, tokenStream, aBaseOfs);
         element = array;
         break;
      }

      case Token::TOKEN_STRING:
      {
         StringNode* string;
         Parse(string, tokenStream, aBaseOfs);
         element = string;
         break;
      }

      case Token::TOKEN_NUMBER:
      {
         NumberNode* number;
         Parse(number, tokenStream, aBaseOfs);
         element = number;
         break;
      }

      case Token::TOKEN_BOOLEAN:
      {
         BooleanNode* boolean;
         Parse(boolean, tokenStream, aBaseOfs);
         element = boolean;
         break;
      }

      case Token::TOKEN_NULL:
      {
         NullNode* null;
         Parse(null, tokenStream, aBaseOfs);
         element = null;
         break;
      }

      default:
      {
         std::string sMessage = "Unexpected token: " + wstring_to_utf8(token.sValue);
         listener->Error(token.locBegin, PARSER_ERROR_UNEXPECTED_TOKEN, sMessage);
         tokenStream.Get(); 
      }
   }
}


inline void Reader::Parse(ObjectNode*& object, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextCoordinate lBegin(tokenStream.Peek().locBegin);
    
   MatchExpectedToken(Token::TOKEN_OBJECT_BEGIN, tokenStream);

   object = new ObjectNode();
   
   bool bContinue = (tokenStream.EOS() == false &&
                     tokenStream.Peek().nType != Token::TOKEN_OBJECT_END);
   while (bContinue)
   {
      // first the member name. save the token in case we have to throw an exception
      Token tokenName = tokenStream.Peek();
      MatchExpectedToken(Token::TOKEN_STRING, tokenStream);

      // ...then the key/value separator...
      MatchExpectedToken(Token::TOKEN_MEMBER_ASSIGN, tokenStream);

      // ...then the value itself (can be anything).
      Node *nodeVal = NULL;
      Parse(nodeVal, tokenStream, lBegin);
       
       // try adding it to the object (this could throw)
       if(nodeVal) {
           try
           {
              ObjectNode::Member &member = object->DomAddMemberNode(tokenName.sValue, nodeVal);
              member.nameRange = TextRange(tokenName.locBegin.relativeTo(lBegin),
                                           tokenName.locEnd.relativeTo(lBegin));
           }
           catch (Exception&)
           {
              // must be a duplicate name
              std::string sMessage = "Duplicate object member: " + wstring_to_utf8(tokenName.sValue);
              listener->Error(tokenName.locBegin, PARSER_ERROR_DUPLICATE_MEMBER, sMessage);
           }
       } else {
           std::string sMessage = "Could not parse member value '" + wstring_to_utf8(tokenName.sValue) + "'";
           listener->Error(tokenName.locBegin, PARSER_ERROR_INVALID_MEMBER, sMessage);
       }
       
      Token lTok;
      while(!tokenStream.EOS() &&
            (lTok = tokenStream.Peek(), lTok.nType != Token::TOKEN_NEXT_ELEMENT && lTok.nType != Token::TOKEN_OBJECT_END))
      {
         listener->Error(lTok.locBegin, PARSER_ERROR_UNEXPECTED_TOKEN, "Expecting \",\" or \"}\"");
         tokenStream.Get();
      }

      if(lTok.nType == Token::TOKEN_NEXT_ELEMENT)
      {
         MatchExpectedToken(lTok.nType, tokenStream);
         bContinue = !tokenStream.EOS();
      } else
      {
         bContinue = false;
      }
      
   }

   if(tokenStream.EOS())
   {
      listener->Error(tokenStream.getInputStream().GetLocation(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of file");
      object->textRange = TextRange(lBegin.relativeTo(aBaseOfs), TextCoordinate::infinity);
   } else
   {
       TextCoordinate lEndCoord = MatchExpectedToken(Token::TOKEN_OBJECT_END, tokenStream).locEnd;
       object->textRange = TextRange(lBegin.relativeTo(aBaseOfs), lEndCoord.relativeTo(aBaseOfs));
   }

   
}


inline void Reader::Parse(ArrayNode*& array, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextCoordinate lBegin(tokenStream.Peek().locBegin);
      
   MatchExpectedToken(Token::TOKEN_ARRAY_BEGIN, tokenStream);

   array = new ArrayNode();
   
   bool bContinue = (tokenStream.EOS() == false &&
                     tokenStream.Peek().nType != Token::TOKEN_ARRAY_END);
   while (bContinue)
   {
      Node *elemVal = NULL;
      Parse(elemVal, tokenStream, lBegin);

     if(elemVal) {
         array->DomAddElementNode(elemVal);
     }

      bContinue = (tokenStream.EOS() == false &&
                   tokenStream.Peek().nType == Token::TOKEN_NEXT_ELEMENT);
      if (bContinue)
         MatchExpectedToken(Token::TOKEN_NEXT_ELEMENT, tokenStream);
   }

   if(tokenStream.EOS()) {
       listener->Error(TextCoordinate(0), PARSER_ERROR_UNEXPECTED_EOS, "Expecting \",\" or \"]\"");
       array->textRange = TextRange(lBegin.relativeTo(aBaseOfs), TextCoordinate::infinity);
   } else {
       TextCoordinate lEnd = MatchExpectedToken(Token::TOKEN_ARRAY_END, tokenStream).locEnd;
       array->textRange = TextRange(lBegin.relativeTo(aBaseOfs), lEnd.relativeTo(aBaseOfs));
   }

    
}


inline void Reader::Parse(StringNode*& string, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextRange lTokRange(tokenStream.Peek().locBegin.relativeTo(aBaseOfs), tokenStream.Peek().locEnd.relativeTo(aBaseOfs));
   string = new StringNode(MatchExpectedToken(Token::TOKEN_STRING, tokenStream).sValue);
   string->textRange = lTokRange;
}


inline void Reader::Parse(NumberNode*& number, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   const Token& currentToken = tokenStream.Peek(); // might need this later for throwing exception
   TextRange lTokRange(currentToken.locBegin.relativeTo(aBaseOfs),
                       currentToken.locEnd.relativeTo(aBaseOfs));
    
   const std::wstring& sValue = MatchExpectedToken(Token::TOKEN_NUMBER, tokenStream).sValue;

   std::wistringstream iStr(sValue);
   double dValue;
   iStr >> dValue;

   // did we consume all characters in the token?
   if (iStr.eof() == false)
   {
      std::string sMessage = std::string("Unexpected character in NUMBER token: ") + std::string(1, iStr.peek());
      listener->Error(currentToken.locBegin, PARSER_ERROR_MALFORMED_NUMBER, sMessage);
   }

   number = new NumberNode(dValue);
   number->textRange = lTokRange;
}


inline void Reader::Parse(BooleanNode*& boolean, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextRange lTokRange(tokenStream.Peek().locBegin.relativeTo(aBaseOfs),
                       tokenStream.Peek().locEnd.relativeTo(aBaseOfs));
   const std::wstring& sValue = MatchExpectedToken(Token::TOKEN_BOOLEAN, tokenStream).sValue;
   
   boolean = new BooleanNode(sValue == L"true");
   boolean->textRange = lTokRange;
}


inline void Reader::Parse(NullNode*& aNull, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextRange lTokRange(tokenStream.Peek().locBegin.relativeTo(aBaseOfs),
                       tokenStream.Peek().locEnd.relativeTo(aBaseOfs));
   MatchExpectedToken(Token::TOKEN_NULL, tokenStream);
   aNull = new NullNode();
   aNull->textRange = lTokRange;
}


inline Token Reader::MatchExpectedToken(Token::Type nExpected, TokenStream& tokenStream)
{
   static wstring lEmptyStr;
   
   if (tokenStream.EOS())
   {
      std::string sMessage = "Unexpected end of token stream";
      listener->Error(TextCoordinate(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of token stream");
      return Token();
   }

   const Token& token = tokenStream.Peek();
   if (token.nType != nExpected)
   {
     static const char *lTypeNames[] = { 
          "'{'",
          "'}'",
          "'['",
          "']'",
          "','",
          "':'",
          "string",
          "number",
          "'true' or 'false'",
          "'null'"          
      };

      std::string sMessage = "Unexpected token: " + wstring_to_utf8(token.sValue) + "; expecting " + lTypeNames[nExpected];
      listener->Error(token.locBegin-1, PARSER_ERROR_UNEXPECTED_TOKEN, sMessage);
   } else
   {   
      tokenStream.Get();
   }
   
   return token;
}

} // End namespace
