#include <CEventTimer.h>
#include <iostream>
#include <unistd.h>

using namespace std;

void *timedOutCb(void *pArgs, long long int time)
{

    printf("Time=%lld\n", time);
    return nullptr;

}


int main()
{
    cout << "Hello World!" << endl;
    long long int timerBase;
    CEventTimer *timer = new CEventTimer(EEventTimerModePeriodic);

    timer->registerTimeoutCb(timedOutCb, nullptr);

    timer->startTimer(25000000000L, &timerBase);
    printf("Start Timer base=%lld\n", timerBase);
    sleep(5);
    timer->updateTimer(timerBase, 250000000L);
    sleep(5);
    delete timer;
    return 0;
}
