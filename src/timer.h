#ifndef _TIMER_H_
#define _TIMER_H_ 1

#include <boost/thread/thread_time.hpp>

class Timer;

/** A base class for jobs to be scheduled by the timer. */
class TimerJob {
protected:
    // Method to be executed. Override with an actual implementation.
    virtual void Run() = 0;

    // Destructor. This will automatically unschedule if necessary.
    virtual ~TimerJob();

    // Constructor for a job. Set autodelete to true for a fire-and-forget
    // new TimerJobChild();.
    TimerJob(bool fAutoDelete = false);

    // Constructor for a job which automatically schedules it.
    TimerJob(const boost::system_time &time, bool autodelete = false);

public:
    // Unschedule this job. This will block while the job is running.
    // Returns whether the job was scheduled before.
    bool Unschedule();

    // Schedule this job at the specified time. This will unschedule if
    // necessary first. It is allowed to schedule a job while it is already
    // running.
    void Schedule(const boost::system_time &time);
    void Schedule(const boost::posix_time::time_duration &dur);

private:
    // these internal fields are only modified by Timer itself
    friend class Timer;
    boost::system_time _time;
    bool _running; // whether this job is running
    bool _scheduled; // whether this job is scheduled for running
    bool _autodelete; // whether this job is to be delete'd after running
};

/** Start the global timer thread */
void StartTimer();

/** Stop the global timer thread */
void StopTimer();

#endif
