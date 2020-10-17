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

    void getCoordinateRowCol(TextCoordinate aCoord, unsigned long &aRow, unsigned long &aCol) const;
    inline TextCoordinate getLineStart(unsigned long aRow) const { return (aRow-1)<lineStarts.size() ?
                                                                              (TextCoordinate)lineStarts[aRow-1] :
                                                                              -1; }
    inline TextCoordinate getLineEnd(unsigned long aRow) const { return (aRow)<lineStarts.size() ?
                                                                            (TextCoordinate)lineStarts[aRow] :
        endOffset; }

    void updateLineOffsetsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen,
                                      const char *allText);

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
