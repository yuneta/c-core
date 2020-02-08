/****************************************************************************
 *          C_IVENT_CLI.H
 *          IEvent_cli GClass.
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#ifndef _C_IVENT_CLI_H
#define _C_IVENT_CLI_H 1

#include <ginsfsm.h>
#include "yuneta_version.h"
#include "msglog_yuneta.h"
#include "c_iogate.h"
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif

/**rst**

IEvent_cli
==========

Implement a client of InterEvents.

**rst**/

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_ievent(void);

#define GCLASS_IEVENT_NAME "IEvent_cli"
#define GCLASS_IEVENT_CLI gclass_ievent()

#ifdef __cplusplus
}
#endif


#endif
