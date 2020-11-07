/*
 *  Bookmarks.cpp
 *  JsonMockup
 *
 *  Created by Eldan on 6/8/10.
 *  Copyright 2010 Eldan Ben-Haim. All rights reserved.
 *
 */

#include "LinesAndBookmarks.h"

#include <functional>

using namespace std;

LinesAndBookmarks::LinesAndBookmarks()
{
    lineStarts.clear();
    lineStarts.appendMarker(BaseMarker(0));
}

LinesAndBookmarks::~LinesAndBookmarks()
{

}

void LinesAndBookmarks::addBookmark(int aLine)
{
   bookmarks.addMarker((TextCoordinate)aLine);   
   notifyListeners();
}

void LinesAndBookmarks::removeBookmark(int aLine)
{
   bookmarks.removeMarker((TextCoordinate)aLine);
   notifyListeners();
}

bool LinesAndBookmarks::findNextBookmark(int &aLine) const
{
   bool lRet;
   TextCoordinate lCoord = (TextCoordinate)aLine;
   
   lRet = bookmarks.nextMarker(lCoord)!=NULL;
   aLine = (int)lCoord;
   
   return lRet;
}

bool LinesAndBookmarks::findPrevBookmark(int &aLine) const
{
   bool lRet;
   TextCoordinate lCoord = (TextCoordinate)aLine;
   
   lRet = bookmarks.prevMarker(lCoord)!=NULL;
   aLine = (int)lCoord;
   
   return lRet;
}

bool LinesAndBookmarks::hasBookmarkAt(int aLine) const
{
   return bookmarks.hasMarkerAt((TextCoordinate)aLine);
}

void LinesAndBookmarks::addListener(BookmarksChangeListener *aListener)
{
   bookmarksListeners.push_back(aListener);
}


SimpleMarkerList::const_iterator LinesAndBookmarks::begin() const
{
   return bookmarks.begin();
}

SimpleMarkerList::const_iterator LinesAndBookmarks::end() const
{
   return bookmarks.end();
}

   
void LinesAndBookmarks::updateBookmarksByLineSplice(TextCoordinate aStartLine, TextLength aLineCount, TextLength aNewLineCount)
{
    if(aLineCount != aNewLineCount) {
       if(bookmarks.spliceCoordinatesList(aStartLine, aLineCount, aNewLineCount))
       {
          // Notify bookmarks listeners
          notifyListeners();
       }
    }
}

void LinesAndBookmarks::notifyListeners()
{
   for_each(bookmarksListeners.begin(), bookmarksListeners.end(),
            bind2nd(mem_fun(&BookmarksChangeListener::bookmarksChanged), this));
}


void LinesAndBookmarks::updateLineOffsetsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen,
                                                     const char *allText)
{
    SimpleMarkerList newLines;
    
    const char *updateStart = allText+aOffsetStart;
    const char *updateEnd = updateStart + aNewLen;
    
    for(const char *cur = updateStart; cur != updateEnd; cur++) {
        if(*cur == '\n') {
            newLines.appendMarker(cur - allText);
        }
    }

    int lLineDelStart, lLineDelLen;
    if(lineStarts.spliceCoordinatesList(aOffsetStart, aLen, aNewLen,
                                        &newLines, // TODO
                                        &lLineDelStart,
                                        &lLineDelLen))
    {
        updateBookmarksByLineSplice(lLineDelStart, lLineDelLen,
                                    newLines.size() // TODO new lines
                                    );
    }
}

void LinesAndBookmarks::getCoordinateRowCol(TextCoordinate aCoord, unsigned long &aRow, unsigned long &aCol) const
{
   if(aCoord)
   {
      SimpleMarkerList::const_iterator iter = lower_bound(lineStarts.begin(), lineStarts.end(), aCoord);
      
      /*if(iter!=lineStarts.end() &&
         *iter == aCoord)
      {
         iter++;
      }*/
      
      if(iter == lineStarts.begin())
      {
         aRow = 1;
         aCol = aCoord + 1;
         return;
      }

      aRow = iter - lineStarts.begin() + 1;
      iter--;
      aCol = aCoord - *iter;
   } else
   {
      aRow = 1;
      aCol = 1;
   }
}  
