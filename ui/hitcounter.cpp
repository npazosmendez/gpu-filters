//
// Created by nicolas on 08/07/18.
//

#include <time.h>

#include "hitcounter.h"

#define NULL 0

HitCounter::HitCounter() : hitsSinceActualSecond(0), actualSecond(time(NULL)), lastHPS(0) {}

void HitCounter::hit(){
    hitsSinceActualSecond++;
}

float HitCounter::hitsPerSecond(){
    int rightNowSecond = time(NULL);
    if(actualSecond == rightNowSecond){
        return lastHPS;
    }else{
        lastHPS =  float(hitsSinceActualSecond)/(rightNowSecond - actualSecond);
        actualSecond = rightNowSecond;
        hitsSinceActualSecond = 0;
        return lastHPS;
    }

}
