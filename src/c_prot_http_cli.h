/****************************************************************************
 *          C_PROT_HTTP_CLI.H
 *          Prot_http_cli GClass.
 *
 *          Protocol http as client
 *
 *          Copyright (c) 2017-2021 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

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
#define GCLASS_PROT_HTTP_CLI_NAME "Prot_http_cli"
#define GCLASS_PROT_HTTP_CLI gclass_prot_http_cli()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_prot_http_cli(void);

#ifdef __cplusplus
}
#endif
