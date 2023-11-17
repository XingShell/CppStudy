//
// Created by 谢泽星 on 2023/11/15.
//

#ifndef MASTERCPPPLAN_SYSTEMUTIL_H
#define MASTERCPPPLAN_SYSTEMUTIL_H

#include <unistd.h>
#include <iostream>

using namespace std;

void printCpu() {
    int num = sysconf(_SC_NPROCESSORS_CONF);
    cout << "cpu num=" << num << endl;
}

#endif //MASTERCPPPLAN_SYSTEMUTIL_H
