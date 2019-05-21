/****************************************************************************
 *          GHTTP_PARSER.H
 *          Mixin http_parser - gobj-ecosistema
 *
 *          Copyright (c) 2013 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _ROLE_REGISTER_H
#define _ROLE_REGISTER_H 1

#include <ginsfsm.h>
#include "msglog_yuneta.h"

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      Structures
 *********************************************************************/
typedef struct _GHTTP_PARSER {
    http_parser http_parser;
    hgobj gobj;
    char *on_message_event;

    enum http_parser_type type;
    char message_completed;
    char headers_completed;

    char *url;
    json_t *jn_headers;
    char *body;
    int  body_size;

    char *cur_key;  // key can arrive in several callbacks
    char *last_key; // save last key for the case value arriving in several callbacks
} GHTTP_PARSER;

/*********************************************************************
 *      Prototypes
 *********************************************************************/
PUBLIC GHTTP_PARSER *ghttp_parser_create(
    hgobj gobj,
    enum http_parser_type type,
    const char *on_message_event    // Not empty if you want publish event
);
PUBLIC int ghttp_parser_received( /* Return bytes consumed or -1 if error */
    GHTTP_PARSER *parser,
    char *bf,
    int len
);
PUBLIC void ghttp_parser_destroy(GHTTP_PARSER *parser);

PUBLIC void ghttp_parser_reset(GHTTP_PARSER *parser);

#ifdef __cplusplus
}
#endif


#endif

