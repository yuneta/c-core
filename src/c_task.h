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
#include "msglog_yuneta.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define CONTINUE_TASK() \
    return (void *)(size_t)0;

#define STOP_TASK() \
    return (void *)(size_t)-1;

#define GCLASS_TASK_NAME "Task"
#define GCLASS_TASK gclass_task()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_task(void);

#ifdef __cplusplus
}
#endif
