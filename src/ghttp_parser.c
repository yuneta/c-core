/***********************************************************************
 *          GHTTP_PARSER.C
 *          Mixin http_parser - gobj-ecosistema
 *
 *          Copyright (c) 2013 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <ctype.h>
#include "ghttp_parser.h"

/****************************************************************
 *         Constants
 ****************************************************************/

/****************************************************************
 *         Structures
 ****************************************************************/

/****************************************************************
 *         Prototypes
 ****************************************************************/
PRIVATE int on_message_begin(http_parser* http_parser);
PRIVATE int on_headers_complete(http_parser* http_parser);
PRIVATE int on_message_complete(http_parser* http_parser);
PRIVATE int on_url(http_parser* http_parser, const char* at, size_t length);
PRIVATE int on_header_field(http_parser* http_parser, const char* at, size_t length);
PRIVATE int on_header_value(http_parser* http_parser, const char* at, size_t length);
PRIVATE int on_body(http_parser* http_parser, const char* at, size_t length);

/****************************************************************
 *         Data
 ****************************************************************/
PRIVATE http_parser_settings settings = {
  .on_message_begin = on_message_begin,
  .on_url = on_url,
  .on_header_field = on_header_field,
  .on_header_value = on_header_value,
  .on_headers_complete = on_headers_complete,
  .on_body = on_body,
  .on_message_complete = on_message_complete,
};

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC GHTTP_PARSER *ghttp_parser_create(
    hgobj gobj,
    enum http_parser_type type,
    const char *on_message_event,    // Event to publish or send
    BOOL send_event  // TRUE: use gobj_send_event(), FALSE: use gobj_publish_event()
)
{
    GHTTP_PARSER *parser;

    /*--------------------------------*
     *      Alloc memory
     *--------------------------------*/
    parser = gbmem_malloc(sizeof(GHTTP_PARSER));
    if(!parser) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "no memory for sizeof(GHTTP_PARSER)",
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            "sizeof(GHTTP_PARSER)",  "%d", sizeof(GHTTP_PARSER),
            NULL);
        return 0;
    }
    parser->gobj = gobj;
    parser->type = type;
    parser->on_message_event = on_message_event?gbmem_strdup(on_message_event):0;
    parser->send_event = send_event;

    http_parser_init(&parser->http_parser, type);
    parser->http_parser.data = parser;

    return parser;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC void ghttp_parser_destroy(GHTTP_PARSER *parser)
{
    ghttp_parser_reset(parser);
    GBMEM_FREE(parser->on_message_event);
    gbmem_free(parser);
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC void ghttp_parser_reset(GHTTP_PARSER *parser)
{
    parser->headers_completed = 0;
    parser->message_completed = 0;
    parser->body_size = 0;
    GBMEM_FREE(parser->url);
    GBMEM_FREE(parser->body);
    JSON_DECREF(parser->jn_headers);
    GBMEM_FREE(parser->cur_key);
    GBMEM_FREE(parser->last_key);
    http_parser_init(&parser->http_parser, parser->type);
}

/***************************************************************************
 *  Return bytes consumed or -1 if error
 ***************************************************************************/
PUBLIC int ghttp_parser_received(
    GHTTP_PARSER *parser,
    char *buf,
    int recved)
{
    hgobj gobj = parser->gobj;

    size_t nparsed = http_parser_execute(&parser->http_parser, &settings, buf, recved);
    if (parser->http_parser.upgrade) {
        /* handle new protocol */
    } else if (nparsed != recved) {
        /* Handle error. Usually just close the connection. */
        json_t *tt = json_sprintf("%s", http_errno_name(HTTP_PARSER_ERRNO(&parser->http_parser)));
        log_error(0,
            "gobj",         "%s", gobj_short_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PROTOCOL_ERROR,
            "msg",          "%j", tt,
            "desc",         "%s", http_errno_description(HTTP_PARSER_ERRNO(&parser->http_parser)),
            NULL
        );
        JSON_DECREF(tt);
        return -1;
    } else {
        if(HTTP_PARSER_ERRNO(&parser->http_parser) != HPE_OK) {
            json_t *tt = json_sprintf("%s", http_errno_name(HTTP_PARSER_ERRNO(&parser->http_parser)));
            log_error(0,
                "gobj",         "%s", gobj_short_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PROTOCOL_ERROR,
                "msg",          "%j", tt,
                "desc",         "%s", http_errno_description(HTTP_PARSER_ERRNO(&parser->http_parser)),
                NULL
            );
            JSON_DECREF(tt);
            return -1;
        }
    }
    return nparsed;
}

/***************************************************************************
 *  Callbacks must return 0 on success.
 *  Returning a non-zero value indicates error to the parser,
 *  making it exit immediately.
 ***************************************************************************/
PRIVATE int on_message_begin(http_parser* http_parser)
{
//     GHTTP_PARSER *parser = http_parser->data;
//     ghttp_parser_reset(parser);
    return 0;
}

/***************************************************************************
 *  Callbacks must return 0 on success.
 *  Returning a non-zero value indicates error to the parser,
 *  making it exit immediately.
 ***************************************************************************/
PRIVATE int on_headers_complete(http_parser* http_parser)
{
    GHTTP_PARSER *parser = http_parser->data;

    parser->headers_completed = 1;
    return 0;
}

/***************************************************************************
 *  Callbacks must return 0 on success.
 *  Returning a non-zero value indicates error to the parser,
 *  making it exit immediately.
 ***************************************************************************/
PRIVATE int on_message_complete(http_parser* http_parser)
{
    GHTTP_PARSER *parser = http_parser->data;
    hgobj gobj = parser->gobj;

    parser->message_completed = 1;
    if(!parser->jn_headers) {
        parser->jn_headers = json_object();
    }
    if(parser->on_message_event) {
        if(parser->message_completed) {
            /*
             *  The cur_request (with the body) is ready to use.
             */
            json_t *kw_http = json_pack("{s:i, s:s, s:i, s:i, s:O}",
                "http_parser_type",     (int)parser->type,
                "url",                  parser->url?parser->url:"",
                "response_status_code", (int)parser->http_parser.status_code,
                "request_method",       (int)parser->http_parser.method,
                "headers",              parser->jn_headers
            );
            const char *content_type = kw_get_str(parser->jn_headers, "CONTENT-TYPE", "", 0);
            if(!parser->body) {
                json_object_set_new(kw_http, "body", json_string(""));
            } else {
                if(strcasestr(content_type, "application/json")) {
                    json_t *jn_body = nonlegalbuffer2json(parser->body, parser->body_size, TRUE);
                    json_object_set_new(kw_http, "body", jn_body);
                } else {
                    json_object_set_new(kw_http, "body", json_string(parser->body));
                }
            }
            ghttp_parser_reset(parser);
            if(parser->send_event) {
                gobj_send_event(gobj, parser->on_message_event, kw_http, gobj);
            } else {
                gobj_publish_event(gobj, parser->on_message_event, kw_http);
            }
        }
    }
    return 0;
}

/***************************************************************************
 *  Callbacks must return 0 on success.
 *  Returning a non-zero value indicates error to the parser,
 *  making it exit immediately.
 ***************************************************************************/
PRIVATE int on_url(http_parser* http_parser, const char* at, size_t length)
{
    GHTTP_PARSER *parser = http_parser->data;
    hgobj gobj = parser->gobj;

    int pos = 0;
    if(parser->url) {
        pos = strlen(parser->url);
        parser->url = gbmem_realloc(
            parser->url,
            pos + length + 1
        );
    } else {
        parser->url = gbmem_malloc(length+1);
    }

    if(!parser->url) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "no memory for url",
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            "length",       "%d", length,
            NULL);
        return -1;
    }
    memcpy(parser->url + pos, at, length);
    return 0;
}

/***************************************************************************
 *  Callbacks must return 0 on success.
 *  Returning a non-zero value indicates error to the parser,
 *  making it exit immediately.
 ***************************************************************************/
PRIVATE int on_header_field(http_parser* http_parser, const char* at, size_t length)
{
    GHTTP_PARSER *parser = http_parser->data;
    hgobj gobj = parser->gobj;

    if(!parser->jn_headers) {
        parser->jn_headers = json_object();
    }
    if(!parser->jn_headers) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "no memory for url",
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            "length",       "%d", length,
            NULL);
        return -1;
    }

    int pos=0;
    if(parser->cur_key) {
        pos = strlen(parser->cur_key);
        parser->cur_key = gbmem_realloc(
            parser->cur_key,
            pos + length + 1
        );
    } else {
        parser->cur_key = gbmem_malloc(length+1);
    }
    if(!parser->cur_key) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "no memory for header field",
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            "length",       "%d", length,
            NULL);
        return -1;
    }
    memcpy(parser->cur_key + pos, at, length);
    strntoupper(parser->cur_key + pos, length);
    return 0;
}

/***************************************************************************
 *  Callbacks must return 0 on success.
 *  Returning a non-zero value indicates error to the parser,
 *  making it exit immediately.
 ***************************************************************************/
PRIVATE int on_header_value(http_parser* http_parser, const char* at, size_t length)
{
    GHTTP_PARSER *parser = http_parser->data;
    hgobj gobj = parser->gobj;

    if(!parser->jn_headers) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "jn_headers NULL",
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            "length",       "%d", length,
            NULL);
        return -1;
    }
    char add_value = 0;
    if(!parser->cur_key) {
        if(parser->last_key) {
            parser->cur_key = parser->last_key;
            parser->last_key = 0;
            add_value = 1;
        }
        if(!parser->cur_key) {
            log_error(0,
                "gobj",         "%s", __FILE__,
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "cur_key NULL",
                "gclass",       "%s", gobj_gclass_name(gobj),
                "name",         "%s", gobj_name(gobj),
                NULL);
            return -1;
        }
    }

    char *value;
    int pos = 0;
    if(add_value) {
        json_t *jn_partial_value = json_object_get(
            parser->jn_headers,
            parser->cur_key
        );
        const char *partial_value = json_string_value(jn_partial_value);
        pos = strlen(partial_value);
        value = gbmem_malloc(pos + length + 1);
        if(value) {
            memcpy(value, partial_value, pos);
        }
    } else {
        value = gbmem_malloc(length + 1);
    }
    if(!value) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "no memory for header value",
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            "length",       "%d", length,
            NULL);
        return -1;
    }
    memcpy(value+pos, at, length);
    json_object_set_new(parser->jn_headers, parser->cur_key, json_string(value));
    gbmem_free(value);

    if(parser->last_key) {
        gbmem_free(parser->last_key);
        parser->last_key = 0;
    }
    parser->last_key = parser->cur_key;
    parser->cur_key = 0;
    return 0;
}

/***************************************************************************
 *  Callbacks must return 0 on success.
 *  Returning a non-zero value indicates error to the parser,
 *  making it exit immediately.
 ***************************************************************************/
PRIVATE int on_body(http_parser* http_parser, const char* at, size_t length)
{
    GHTTP_PARSER *parser = http_parser->data;
    hgobj gobj = parser->gobj;

    if(parser->body) {
        parser->body = gbmem_realloc(
            parser->body,
            parser->body_size + length + 1
        );
    } else {
        parser->body = gbmem_malloc(length + 1);
    }

    if(!parser->body) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "no memory for body",
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            "body_size",    "%d", parser->body_size,
            "length",       "%d", length,
            NULL);
        return -1;
    }
    memcpy(parser->body + parser->body_size, at, length);
    parser->body_size += length;

    return 0;
}
