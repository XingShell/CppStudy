//
// Created by 谢泽星 on 2023/11/14.
//

#ifndef MASTERCPPPLAN_TIMEUTIL_H
#define MASTERCPPPLAN_TIMEUTIL_H


#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

int64_t get_current_millisecond()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000);
}

#endif //MASTERCPPPLAN_TIMEUTIL_H
