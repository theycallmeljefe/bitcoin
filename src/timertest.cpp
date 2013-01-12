#include "timer.h"
#include <iostream>
#include <string>
#include <boost/thread.hpp>

using namespace std;

class COutJob : public TimerJob
{
private:
    string text;
    int repeat;
    
public:
    COutJob(const string& _text, int repeatMs = 0) : TimerJob(true), text(_text), repeat(repeatMs) { }
    
    void Run()
    {
        if (repeat)
            Schedule(boost::posix_time::milliseconds(repeat));

        cout << text << endl;
    }
};

int main()
{
    StartTimer();
    COutJob* pJob1 = new COutJob("I'm the first job - I happen in 1.5 seconds!");
    COutJob* pJob2 = new COutJob("I'm the second job - I happen in 0.9 second and 1.8 seconds!", 900);
    COutJob* pJob3 = new COutJob("I'm the third job - I happen in 1.2 seconds!");
    COutJob* pJob4 = new COutJob("I'm the fourth job - I don't happen");
    COutJob* pJob5 = new COutJob("I'm the fifth job - I happen after shutdown");
    pJob4->Schedule(boost::posix_time::milliseconds(100));
    pJob5->Schedule(boost::posix_time::milliseconds(2100));
    delete pJob4;
    pJob1->Schedule(boost::posix_time::milliseconds(1500));
    pJob2->Schedule(boost::posix_time::milliseconds(900));
    sleep(1);
    pJob3->Schedule(boost::posix_time::milliseconds(200));
    sleep(1);
    StopTimer();
}
