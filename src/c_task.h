/****************************************************************************
 *          C_TASK.H
 *          Task GClass.
 *
 *          Tasks
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "c_timer.h"
#include "c_task.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_TASK_NAME "Task"
#define GCLASS_TASK gclass_task()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_task(void);

#ifdef __cplusplus
}
#endif
