#ifndef VOX_H
#define VOX_H

#include <QObject>
#include <unistd.h>
#include <math.h>
#include "config_defines.h"

namespace hvdi
{
    typedef struct hvdi_vox_st {
        int     rate;           /* HVDI_VOX_FAST, HVDI_VOX_MEDIUM, or HVDI_VOX_SLOW */
        int     noisethreshold; /* The actual threshold used by hvdiVOX */
        int     samplecount;    /* init to 0; used internally by hvdiVOX */
    } hvdi_vox_t;

    typedef struct hvdi_vox_st vox_st;
    vox_st* initVOX(int voxspeed, int noisethreshold);
    int VOX(vox_st *vox, short *buffer, int buflen);

}

#endif // VOX_H
