/****************************************************************************
 *          C_AUTHZ.H
 *          Authz GClass.
 *
 *          Authorization Manager
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>
#include "c_tranger.h"
#include "c_node.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************
 *              Constants
 ***************************************************************/
#define GCLASS_AUTHZ_NAME "Authz"
#define GCLASS_AUTHZ gclass_authz()

/***************************************************************
 *              Prototypes
 ***************************************************************/
PUBLIC GCLASS *gclass_authz(void);

PUBLIC BOOL authz_checker(hgobj gobj, const char *authz, json_t *kw, hgobj src);


#ifdef __cplusplus
}
#endif
