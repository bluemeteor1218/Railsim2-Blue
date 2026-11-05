#ifndef HIGHTIMER_H_INCLUDED
#define HIGHTIMER_H_INCLUDED

bool InitHighTimer();
LONGLONG HighTimer();
double FromHighTimerCountToMs(LONGLONG t);

#endif
