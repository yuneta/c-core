/****************************************************************************
 *          C_TASK.H
 *          Task GClass.
 *          TODO future work for yuneta 2.0
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

/*^^ ======================== start sphinx doc ================================

Task GClass
==============

    Task of events, filtered by attr's regex or acceptance callback.
    When task reachs his count then a configured event is published.
    Task accepts subscriber.

    WARNING: this gobj is auto-destroyed when reached his final count.


Diagram
=======

    Input Events            Task             Output Events
                    ┌───────────────────┐
        start   ━━━▷│●                  │
                    │-------------------│
                    │                   │
      EV_COUNT ====▷│                   │====▷  EV_FINAL_COUNT
                    │                   │
                    │-------------------│
        stop    ━━━▷│■  ◁---(auto)      │
                    │                   │
                    └───────────────────┘



{
    "info": "xxxx",
    "user_data": 7105288,
    "user_data2": 7076024,
    "final_count": 1,
    "expiration_timeout": 4000,
    "input_schema": [
        {
            "event": "EV_ON_CLOSE_ROUTE",
            "filters": {
                "__route__": 7104008
            }
        }
    ]
}

Attributes
==========

final_count
-----------

    Type: ASN_UINT32

    Final count that activates publishing output event.

input_schema
------------

    Type: ASN_JSON, EvChkItem

    input_schema is a schema to validate and count the input events.
    input_schema is list of EvChkItem json objects.

    EvChkItem: a json **object** with this schema:
        [{
            'event': str,
            'filters': {
                field: str (regex),
                field: int ,
            }
        }]

    Regex filters only can be applied in string attributes.
    For the remain of field type you can use an acceptance callback.


expiration_timeout
------------------

    Timeout in miliseconds to finish the task
    although the final count hasn't been reached.


Input Events
============

EV_COUNT
--------

    Fields:



Output Events
=============

EV_FINAL_COUNT
--------------

    Fields:

        * info: str, User info.
        * jn_info: json, User json info.
        * max_count: int, Desired final count.
        * cur_count: int, count reached.


============================= end sphinx doc ============================= ^^*/


#ifndef _C_TASK_H
#define _C_TASK_H 1

#include <ginsfsm.h>
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_task(void);

#define GCLASS_TASK_NAME "Task"
#define GCLASS_TASK gclass_task()

#ifdef __cplusplus
}
#endif


#endif
