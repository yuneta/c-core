/****************************************************************************
 *          C_FS.H
 *          Watch file system events uv-mixin.
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_FS_H
#define _C_FS_H 1

#include <ginsfsm.h>
#include "msglog_yuneta.h"
#include "c_timer.h"
#include "c_yuno.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_fs(void);

#define GCLASS_FS_NAME "Fs"
#define GCLASS_FS gclass_fs()

#ifdef __cplusplus
}
#endif


#endif
