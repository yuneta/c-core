/****************************************************************************
 *          C_TIMER.H
 *          GClass of TIMER uv-mixin
 *
    Input Events                                Output Events
                    ┌───────────────────┐
        start   ━━━▷│●                  │
                    │-------------------│
                    │                   │
        play    ━━━▷│▶                  │
                    │                   │====▷  EV_TIMEOUT
        pause   ━━━▷│❚❚ ◁---(auto)      │
                    │                   │
                    │-------------------│
        stop    ━━━▷│■                  │====▷  EV_STOPPED
                    └───────────────────┘


    Output Events:
    * EV_TIMEOUT    Always is sended to the parent. No use of publish_event.
    * EV_STOPPED    Only NOW you can destroy the gobj.


    There are three helper functions because the high use of this functionality.
      - set_timeout()
      - set_timeout_periodic()
      - clear_timeout()

 *
 *          Copyright (c) 2013-2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "c_yuno.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_timer(void);

#define GCLASS_TIMER_NAME "Timer"
#define GCLASS_TIMER gclass_timer()

PUBLIC void set_timeout(hgobj gobj, int msec);
PUBLIC void set_timeout_periodic(hgobj gobj, int msec);
PUBLIC void clear_timeout(hgobj gobj);

#ifdef __cplusplus
}
#endif
