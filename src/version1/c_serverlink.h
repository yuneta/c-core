/****************************************************************************
 *          C_SERVERLINK.H
 *          A end point link acting as server.
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_GSERVERLINK_H
#define _C_GSERVERLINK_H 1

#include <ginsfsm.h>
#include "c_tcp_s0.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_gserverlink(void);

#define GCLASS_GSERVERLINK_NAME "GServerLink"
#define GCLASS_GSERVERLINK gclass_gserverlink()

#ifdef __cplusplus
}
#endif


#endif
