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

/*
*  This AGC algorithm was taken from isdn2h323 (http://www.telos.de). It was
*  converted from C++ to C, and modified to add control over the recording level.
*  Converted to fixed point by Phil Frisbie, Jr. 4/12/2003
*/

#include "agc.h"

namespace hvdi
{


agc_st* initAGC(float level)
{
    agc_st *agc = new agc_st;

    if(agc == NULL)
    {
        return NULL;
    }
    agc->sample_max = 1;
    agc->counter = 0;
    agc->igain = 65536;
    if(level > 1.0f)
    {
        level = 1.0f;
    }
    else if(level < 0.5f)
    {
        level = 0.5f;
    }
    agc->ipeak = (int)(SHRT_MAX * level * 65536);
    agc->silence_counter = 0;
    return agc;
}

void AGC(agc_st *agc, short *buffer, int len)
{
    int i;

    for(i=0;i<len;i++)
    {
        long gain_new;
        int sample;

        /* get the abs of buffer[i] */
        sample = buffer[i];
        sample = (sample < 0 ? -(sample):sample);

        if(sample > (int)agc->sample_max)
        {
            /* update the max */
            agc->sample_max = (unsigned int)sample;
        }
        agc->counter ++;

        /* Will we get an overflow with the current gain factor? */
        if (((sample * agc->igain) >> 16) > agc->ipeak)
        {
            /* Yes: Calculate new gain. */
            agc->igain = ((agc->ipeak / agc->sample_max) * 62259) >> 16;
            agc->silence_counter = 0;
            buffer[i] = (short) ((buffer[i] * agc->igain) >> 16);
            continue;
        }

        /* Calculate new gain factor 10x per second */
        if (agc->counter >= 8000/10)
        {
            if (agc->sample_max > 800)        /* speaking? */
            {
                gain_new = ((agc->ipeak / agc->sample_max) * 62259) >> 16;

                if (agc->silence_counter > 40)  /* pause -> speaking */
                    agc->igain += (gain_new - agc->igain) >> 2;
                else
                    agc->igain += (gain_new - agc->igain) / 20;

                agc->silence_counter = 0;
            }
            else   /* silence */
            {
                agc->silence_counter++;
                /* silence > 2 seconds: reduce gain */
                if ((agc->igain > 65536) && (agc->silence_counter >= 20))
                    agc->igain = (agc->igain * 62259) >> 16;
            }
            agc->counter = 0;
            agc->sample_max = 1;
        }
        buffer[i] = (short) ((buffer[i] * agc->igain) >> 16);
    }
}

}
