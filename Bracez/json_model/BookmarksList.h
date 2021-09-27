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

class BookmarksList;

struct BookmarksChangeListener 
{
   virtual void bookmarksChanged(BookmarksList * aSender) = 0;
} ;

typedef MarkerList<BaseMarker> LineMarkerList;     // This is a marker list where coordinates are actually lines.
                                                   // Somewhat of a hack, yes.

class BookmarksList
{
public:
    BookmarksList();
   ~BookmarksList();
   
   void addBookmark(int aLine);
   void removeBookmark(int aLine);
   
   bool findNextBookmark(int &aLine) const;
   bool findPrevBookmark(int &aLine) const;
   
   bool hasBookmarkAt(int aLine) const;
         
   void addListener(BookmarksChangeListener *aListener);
   
   LineMarkerList::const_iterator begin() const;
   LineMarkerList::const_iterator end() const;

    void updateBookmarksByLineSplice(TextCoordinate aStartLine, TextLength aLineCount, TextLength aNewLineCount);

private:
   void notifyListeners();

   
private:
   LineMarkerList bookmarks;
   
   typedef std::vector<BookmarksChangeListener*> BookmarksListeners;
   BookmarksListeners bookmarksListeners;
} ;

#endif
