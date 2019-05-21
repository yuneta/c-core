/****************************************************************************
 *          COMMAND_PARSER.H
 *
 *          Command parser
 *
 *          Copyright (c) 2017 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_COMMAND_PARSER_H
#define _C_COMMAND_PARSER_H 1

#include <ginsfsm.h>
#include "msg_ievent.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************
 *              Prototypes
 **************************************************/
PUBLIC json_t * command_parser(
    hgobj gobj,
    const char *command,
    json_t *kw,
    hgobj src
);
PUBLIC json_t *gobj_build_cmds_doc(hgobj gobj, json_t *kw);

#ifdef __cplusplus
}
#endif

#endif
