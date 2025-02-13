#include <chrono>

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

struct Timer
{
  Timer(double maxTime) : mMaxTime(maxTime),
			  start{Time::now()}
  {}
  bool hasTimedOut() const
  {
    auto now{Time::now()};
    dsec elapsed_time{now - start};
    if(elapsed_time.count()>mMaxTime)
      return true;
    return false;
  }
  double mMaxTime;
  std::chrono::time_point<Time> start;
};
