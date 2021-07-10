/****************************************************************************
 *          C_DYNAGENT.H
 *          DynAgent GClass.
 *
 *          Mini Dynamic Agent

 *          Copyright (c) 2015 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

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

