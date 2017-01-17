#ifndef AGC_H
#define AGC_H

#include <QObject>
#include <unistd.h>
#include <math.h>
#include "config_defines.h"

namespace hvdi
{
    typedef struct hvdi_agc_st {
        unsigned int    sample_max;
        int             counter;
        long            igain;
        int             ipeak;
        int             silence_counter;
    } hvdi_agc_t;

    typedef struct hvdi_agc_st agc_st;

    agc_st *initAGC(float level);
    void AGC(agc_st *agc, short *buffer, int len);
}

#endif // AGC_H
