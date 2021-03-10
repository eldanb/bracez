/*
 *  stopwatch.h
 *  JsonMockup
 *
 *  Created by Eldan on 1/21/11.
 *  Copyright 2011 Eldan Ben-Haim. All rights reserved.
 *
 */


class stopwatch
{
public:
   stopwatch(const char *aName = NULL)
   {
      name=aName;
      memset(&lapTime, 0, sizeof(lapTime));
      restart();
   }
   
   ~stopwatch()
   {
      stop();
   }
   
   void restart()
   {
      stopped = false;
      startTime = UpTime();
   }
   
   void lap(const char *aDesc)
   {
      AbsoluteTime lCurTime = UpTime();
      
      if(lapTime.hi!=0 || lapTime.lo!=0)
      {
         printf("[stopwatch] %s: %s @ %d millisec (%d millisecs from previous lap)\n", 
               name.c_str(), aDesc, 
               (int)(AbsoluteDeltaToNanoseconds(lCurTime, startTime).lo/1000/1000),
               (int)(AbsoluteDeltaToNanoseconds(lCurTime, lapTime).lo/1000/1000));
      } else {
         printf("[stopwatch] %s: %s @ %d millisec\n", name.c_str(), aDesc, (int)(AbsoluteDeltaToNanoseconds(lCurTime, startTime).lo/1000/1000));
      }

      
      lapTime = lCurTime;
   }
   
   void stop()
   {
      if(!stopped)
      {
         lap("stop");
         stopped = true;
      }
   }
   
private:
   AbsoluteTime startTime;
   AbsoluteTime lapTime;
   bool stopped;
   std::string name;
} ;
