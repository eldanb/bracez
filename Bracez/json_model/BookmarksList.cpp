/*
 *  Bookmarks.cpp
 *  JsonMockup
 *
 *  Created by Eldan on 6/8/10.
 *  Copyright 2010 Eldan Ben-Haim. All rights reserved.
 *
 */

#include "BookmarksList.h"

#include <functional>

using namespace std;

BookmarksList::BookmarksList()
{
}

BookmarksList::~BookmarksList()
{

}

void BookmarksList::addBookmark(int aLine)
{
   bookmarks.addMarker((TextCoordinate)aLine);   
   notifyListeners();
}

void BookmarksList::removeBookmark(int aLine)
{
   bookmarks.removeMarker((TextCoordinate)aLine);
   notifyListeners();
}

bool BookmarksList::findNextBookmark(int &aLine) const
{
   bool lRet;
   TextCoordinate lCoord = (TextCoordinate)aLine;
   
   lRet = bookmarks.nextMarker(lCoord)!=NULL;
   aLine = (int)lCoord.getAddress();
   
   return lRet;
}

bool BookmarksList::findPrevBookmark(int &aLine) const
{
   bool lRet;
   TextCoordinate lCoord = (TextCoordinate)aLine;
   
   lRet = bookmarks.prevMarker(lCoord)!=NULL;
   aLine = (int)lCoord.getAddress();
   
   return lRet;
}

bool BookmarksList::hasBookmarkAt(int aLine) const
{
   return bookmarks.hasMarkerAt((TextCoordinate)aLine);
}

void BookmarksList::addListener(BookmarksChangeListener *aListener)
{
   bookmarksListeners.push_back(aListener);
}


SimpleMarkerList::const_iterator BookmarksList::begin() const
{
   return bookmarks.begin();
}

SimpleMarkerList::const_iterator BookmarksList::end() const
{
   return bookmarks.end();
}

   
void BookmarksList::updateBookmarksByLineSplice(TextCoordinate aStartLine, TextLength aLineCount, TextLength aNewLineCount)
{
    if(aLineCount != aNewLineCount) {
       if(bookmarks.spliceCoordinatesList(aStartLine, aLineCount, aNewLineCount))
       {
          // Notify bookmarks listeners
          notifyListeners();
       }
    }
}

void BookmarksList::notifyListeners()
{
   for_each(bookmarksListeners.begin(), bookmarksListeners.end(),
            bind(&BookmarksChangeListener::bookmarksChanged, placeholders::_1, this));
}

