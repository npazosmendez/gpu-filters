//
// Created by nicolas on 08/07/18.
//

#ifndef GPU_FILTERS_HITCOUNTER_H
#define GPU_FILTERS_HITCOUNTER_H

class HitCounter{
public:
    HitCounter();
    void hit();
    float hitsPerSecond();
private:
    int hitsSinceActualSecond;
    int actualSecond;
    float lastHPS;
};

#endif //GPU_FILTERS_HITCOUNTER_H
