//
//  marker_list.h
//  JsonMockup
//
//  Created by Eldan Ben Haim on 17/10/2020.
//

#ifndef marker_list_h
#define marker_list_h

#include <vector>
#include "Exception.hpp"

#define TEXT_COORDINATE_INFINITY ((unsigned int)-1)

typedef unsigned int TextLength;

class TextCoordinate {

public:
    explicit TextCoordinate(unsigned int coord = 0) : coord(coord) {}
    
    inline TextCoordinate &operator+=(int other) {
        if(!infinite()) {
            coord += other;
        }
        
        return *this;
    }

    inline TextCoordinate operator+(int ofs) const {
        if(!infinite()) {
            return TextCoordinate(coord + ofs);
        } else {
            return *this;
        }
    }

    inline TextCoordinate operator+(TextLength ofs) const {
        if(!infinite()) {
            return TextCoordinate(coord + ofs);
        } else {
            return *this;
        }
    }

    inline TextCoordinate operator-(int ofs) const {
        if(infinite()) {
            return *this;
        }
        return TextCoordinate(coord - ofs);
    }

    inline unsigned int getAddress() const {
        if(infinite()) {
            throw Exception("Can't get address for infinite");
        }
        
        return coord;
    }
    
    inline operator unsigned int() const {
        return getAddress();
    }
    
    inline TextLength operator-(TextCoordinate other) const {
        if(infinite() || other.infinite()) {
            throw Exception("Can't compute length between infinites");
        }
        
        return coord - other.coord;
    }

    inline bool operator<(TextCoordinate other) const {
        return coord < other.coord;
    }

    inline bool operator<=(TextCoordinate other) const {
        return coord <= other.coord;
    }

    inline bool operator==(TextCoordinate other) const {
        return coord == other.coord;
    }

    inline bool operator!=(TextCoordinate other) const {
        return coord != other.coord;
    }

    inline bool operator>=(TextCoordinate other) const {
        return coord >= other.coord;
    }

    inline bool operator>(TextCoordinate other) const {
        return coord > other.coord;
    }

    TextCoordinate &operator=(TextCoordinate other) {
        coord = other.coord;
        return *this;
    }
    
    bool infinite() const {
        return coord == TEXT_COORDINATE_INFINITY;
    }

    TextCoordinate relativeTo(TextCoordinate other) const {
        if(other.infinite()) {
            throw Exception("Can't devise coordinate relative to infinite");
        }
        
        if(infinite()) {
            return *this;
        }
        
        return TextCoordinate(coord - other.coord);
    }
    

    
public:
    static TextCoordinate infinity;
    
private:
    unsigned int coord;
};


class BaseMarker
{
public:
   BaseMarker(TextCoordinate aCoord) : coordinate(aCoord) {}
   
   inline operator TextCoordinate() const { return coordinate; }
   
    TextCoordinate getCoordinate() const { return coordinate; }
   inline bool operator<(const BaseMarker &aOther) const { return coordinate < aOther.coordinate; }
   inline bool operator==(const BaseMarker &aOther) const { return coordinate == aOther.coordinate; }
   inline bool operator<=(const BaseMarker &aOther) const { return coordinate <= aOther.coordinate; }
    
   inline bool operator<(const TextCoordinate &aOther) const { return coordinate < aOther; }
   inline bool operator>=(const TextCoordinate &aOther) const { return coordinate >= aOther; }

   inline void adjustCoordinate(int aOfs) { coordinate += aOfs; };
      
protected:

   TextCoordinate coordinate;
} ;

template <class MARKER_TYPE>
class MarkerList
{
public:
   typedef std::vector<MARKER_TYPE> MarkerListType;
   typedef typename MarkerListType::iterator iterator;
   typedef typename MarkerListType::const_iterator const_iterator;
   
   void addMarker(const MARKER_TYPE &aMarker);
   void removeMarker(const MARKER_TYPE aMarker);
   void appendMarker(const MARKER_TYPE &aMarker);
   
   void clear() { markers.clear(); }
   
   const MARKER_TYPE *nextMarker(TextCoordinate &aBookmark) const;
   const MARKER_TYPE *prevMarker(TextCoordinate &aMarker) const;
   
   bool hasMarkerAt(TextCoordinate aCoord) const;
   const MARKER_TYPE &markerAt(TextCoordinate aCoord) const;

   bool spliceCoordinatesList(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen,
                              MarkerList<MARKER_TYPE> *aNewMarkers = NULL,
                               int *aOutEraseStart = NULL, int *aOutEraseLen = NULL);

   size_t size() const { return markers.size(); }

   iterator begin() { return markers.begin(); }
   iterator end() { return markers.end(); }

   const_iterator begin() const { return markers.begin(); }
   const_iterator end() const { return markers.end(); }
         
   const MARKER_TYPE &operator[] (int aIdx) const { return markers[aIdx]; }
   
private:
   MarkerListType markers;
} ;

typedef MarkerList<BaseMarker> SimpleMarkerList;



template <class MARKER_TYPE>
void MarkerList<MARKER_TYPE>::appendMarker(const MARKER_TYPE &aMarker)
{
   assert(!markers.size() || !(aMarker <= markers.back()));
   markers.push_back(aMarker);
}

template <class MARKER_TYPE>
void MarkerList<MARKER_TYPE>::addMarker(const MARKER_TYPE &aMarker)
{
   iterator iter = lower_bound(markers.begin(), markers.end(), aMarker);
   if(iter != markers.end() && *iter == aMarker)
   {
      return;
   }
   
   markers.insert(iter, aMarker);
}

template <class MARKER_TYPE>
void MarkerList<MARKER_TYPE>::removeMarker(const MARKER_TYPE aMarker)
{
   iterator iter = lower_bound(markers.begin(), markers.end(), aMarker);
   if(iter != markers.end() && *iter == aMarker)
   {
      markers.erase(iter);
   }
}

template <class MARKER_TYPE>
const MARKER_TYPE *MarkerList<MARKER_TYPE>::nextMarker(TextCoordinate &aBookmark) const
{
   const_iterator iter = upper_bound(markers.begin(), markers.end(), aBookmark);
   if(iter != markers.end())
   {
      aBookmark = *iter;
      return &(*iter);
   } else {
      return NULL;
   }
}

template <class MARKER_TYPE>
const MARKER_TYPE *MarkerList<MARKER_TYPE>::prevMarker(TextCoordinate &aMarker) const
{
   const_iterator iter = lower_bound(markers.begin(), markers.end(), aMarker);
   if(iter != markers.begin())
   {
      iter--;
      aMarker = *iter;
      return &(*iter);
   } else {
      return nullptr;
   }
}


template <class MARKER_TYPE>
bool MarkerList<MARKER_TYPE>::hasMarkerAt(TextCoordinate aCoord) const
{
   return binary_search(markers.begin(), markers.end(), aCoord);
}

template <class MARKER_TYPE>
const MARKER_TYPE &MarkerList<MARKER_TYPE>::markerAt(TextCoordinate aCoord) const
{
   iterator iter = lower_bound(markers.begin(), markers.end(), aCoord);
   if(iter != markers.end() && *iter == aCoord)
   {
      return *iter;
   } else {
      throw 0;// todo
   }
}

template <class MARKER_TYPE>
bool MarkerList<MARKER_TYPE>::spliceCoordinatesList(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen,
                                                    MarkerList<MARKER_TYPE> *aNewMarkers,
                                                    int *aOutEraseStart, int *aOutEraseLen)
{
   bool lChanged = false;
   
   // Find first element to delete in lines list
   iterator iter = lower_bound(markers.begin(), markers.end(), aOffsetStart);
   if(iter != markers.begin())
   {
      iter--;
   }
   
   while(iter!=markers.end() && *iter < aOffsetStart)
   {
      iter++;
   }
   iterator delstart_iter = iter;
   iterator delend_iter = iter;
   
   // Find last element to delete in lines list
   while(iter!=markers.end() && *iter >= aOffsetStart && *iter < aOffsetStart + aLen)
   {
      iter++;
      delend_iter = iter;
   }
   
   // Adjust elements past deletion range
   lChanged = lChanged || iter!=markers.end();
   int lLenDiff = aNewLen - aLen;
   while(iter != markers.end())
   {
      iter->adjustCoordinate(lLenDiff);
      iter++;
   }
   
   // Delete deletion range
   lChanged = lChanged || delstart_iter!=delend_iter;
   if(aOutEraseStart) *aOutEraseStart = delstart_iter - markers.begin();
   if(aOutEraseLen) *aOutEraseLen = delend_iter - delstart_iter;
   
   markers.erase(delstart_iter, delend_iter);
   
   // Insert splice range
    bool hasNewMarkers = aNewMarkers && aNewMarkers->size();
   lChanged = lChanged || hasNewMarkers;
   if(hasNewMarkers)
   {
      iterator iter = lower_bound(markers.begin(), markers.end(), aNewMarkers->markers.front());
      markers.insert(iter, aNewMarkers->markers.begin(), aNewMarkers->markers.end());
   }
   
   return lChanged;
}

#endif /* marker_lisst_h */
