/****************************************************************************
 *          C_PROT_HTTP.H
 *          Prot_http GClass.
 *
 *          Protocol http 1.1
 *
 *          Copyright (c) 2018 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#ifndef _C_PROT_HTTP_H
#define _C_PROT_HTTP_H 1

#include <ginsfsm.h>
#include "ghttp_parser.h"
#include "c_timer.h"
#include "c_connex.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_PROT_HTTP_NAME "Prot_http"
#define GCLASS_PROT_HTTP gclass_prot_http()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_prot_http(void);

#ifdef __cplusplus
}
#endif

#endif
