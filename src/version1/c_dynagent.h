/****************************************************************************
 *          C_DYNAGENT.H
 *          DynAgent GClass.
 *
 *          Mini Dynamic Agent

 *          Copyright (c) 2015 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_DYNAGENT_H
#define _C_DYNAGENT_H 1

#include <ginsfsm.h>
#include "c_timer.h"
#include "c_router.h"
#include "c_counter.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_dynagent(void);

#define GCLASS_DYNAGENT_NAME "DynAgent"
#define GCLASS_DYNAGENT gclass_dynagent()

#ifdef __cplusplus
}
#endif


#endif
