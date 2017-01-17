/*
  HawkVoice Direct Interface (HVDI) cross platform network voice library
  Copyright (C) 2001-2004 Phil Frisbie, Jr. (phil@hawksoft.com)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA  02111-1307, USA.

  Or go to http://www.gnu.org/copyleft/lgpl.html
*/

#include "vox.h"

namespace hvdi
{

vox_st* initVOX(int voxspeed, int noisethreshold)
{
    vox_st *vox;

    vox = new vox_st;
    if(vox == NULL)
    {
        return NULL;
    }
    vox->rate = voxspeed;
    vox->noisethreshold = (int) exp(log(32767.0) * ((1000 - noisethreshold) / 1000.0));
    vox->samplecount = 0;

    return vox;
}

int VOX(vox_st *vox, short *buffer, int buflen)
{
    int     i;
    long    level = 0;

    for(i=0;i<buflen;i++)
    {
        long sample = buffer[i];

        if(sample < 0)
        {
            level -= sample;
        }
        else
        {
            level += sample;
        }

    }
    level /= buflen;
    if(level < vox->noisethreshold)
    {
        if (vox->samplecount <= 0)
        {
            return 0;
        }
        vox->samplecount -= buflen;
    }
    else
    {
        vox->samplecount = vox->rate;
    }
    return 1;
}

}
