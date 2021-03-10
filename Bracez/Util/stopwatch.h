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
      lapTime = 0;
      restart();
   }
   
   ~stopwatch()
   {
      stop();
   }
   
   void restart()
   {
      stopped = false;
      startTime = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
   }
   
   void lap(const char *aDesc)
   {
       __uint64_t lCurTime = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
      
      if(lapTime != 0)
      {
         printf("[stopwatch] %s: %s @ %d millisec (%d millisecs from previous lap)\n", 
               name.c_str(), aDesc, 
               (int)((lCurTime-startTime)/1000/1000),
               (int)((lCurTime-lapTime)/1000/1000));
      } else {
         printf("[stopwatch] %s: %s @ %d millisec\n", name.c_str(), aDesc, (int)((lCurTime-startTime)/1000/1000));
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
   __int64_t startTime;
   __int64_t lapTime;
   bool stopped;
   std::string name;
} ;
