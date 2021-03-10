/*
 *  Bookmarks.h
 *  JsonMockup
 *
 *  Created by Eldan on 6/8/10.
 *  Copyright 2010 Eldan Ben-Haim. All rights reserved.
 *
 */

#ifndef __bookmarks_h__
#define __bookmarks_h__

#include <vector>
#include "marker_list.h"

class LinesAndBookmarks;

struct BookmarksChangeListener 
{
   virtual void bookmarksChanged(LinesAndBookmarks * aSender) = 0;
} ;

typedef MarkerList<BaseMarker> LineMarkerList;     // This is a marker list where coordinates are actually lines.
                                                   // Somewhat of a hack, yes.

class LinesAndBookmarks
{
public:
    LinesAndBookmarks();
   ~LinesAndBookmarks();
   
   void addBookmark(int aLine);
   void removeBookmark(int aLine);
   
   bool findNextBookmark(int &aLine) const;
   bool findPrevBookmark(int &aLine) const;
   
   bool hasBookmarkAt(int aLine) const;
         
   void addListener(BookmarksChangeListener *aListener);
   
   LineMarkerList::const_iterator begin() const;
   LineMarkerList::const_iterator end() const;

    void getCoordinateRowCol(TextCoordinate aCoord, int &aRow, int &aCol) const;
    inline TextCoordinate getLineStart(unsigned long aRow) const {
        if(aRow <= 1) {
            return TextCoordinate(0);
        } else
        if(aRow-2 > lineStarts.size()) {
            return TextCoordinate::infinity;
        } else {
            return (TextCoordinate)lineStarts[(int)(aRow-2)];
        }
    }
    
    inline TextCoordinate getLineFirstCharacter(unsigned long aRow) const {
        return getLineStart(aRow) + (aRow>1 ? 1 : 0);
    }
    
    inline TextCoordinate getLineEnd(unsigned long aRow) const {
        if(aRow-1 < lineStarts.size()) {
            return TextCoordinate(lineStarts[(int)(aRow-1)].getCoordinate()-1);
        } else {
            return endOffset;
        }
    }


    unsigned long numLines() { return lineStarts.size()+1; }
    void updateLineOffsetsAfterSplice(TextCoordinate aOffsetStart,
                                      TextLength aLen,
                                      TextLength aNewLen,
                                      const wchar_t *allText);

private:
   void notifyListeners();
   void updateBookmarksByLineSplice(TextCoordinate aStartLine, TextLength aLineCount, TextLength aNewLineCount);

   
private:
    TextCoordinate endOffset;
   LineMarkerList bookmarks;
   SimpleMarkerList lineStarts;
   
   typedef std::vector<BookmarksChangeListener*> BookmarksListeners;
   BookmarksListeners bookmarksListeners;
} ;

#endif
