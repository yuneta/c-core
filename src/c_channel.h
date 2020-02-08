/****************************************************************************
 *          C_CHANNEL.H
 *          Channel GClass.
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/


#ifndef _C_CHANNEL_H
#define _C_CHANNEL_H 1

/*
 *  See c_iogate.h for documentation.
 */

#include <ginsfsm.h>
#include "c_tcp0.h"
#include "c_timer.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_channel(void);

#define GCLASS_CHANNEL_NAME "Channel"
#define GCLASS_CHANNEL gclass_channel()

#ifdef __cplusplus
}
#endif


#endif
