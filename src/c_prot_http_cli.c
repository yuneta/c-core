/***********************************************************************
 *          C_PROT_HTTP_CLI.C
 *          Prot_http_cli GClass.
 *
 *          Protocol http as client
 *
 *          Copyright (c) 2017-2021 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <inttypes.h>
#include "c_prot_http_cli.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int send_prot_http_cli(hgobj gobj, const char *prot_http_cli);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_INTEGER,     "timeout",          SDF_RD,             10*1000,         "Timeout response"),
SDATA (ASN_OCTET_STR,   "request",          SDF_RD,             "",             "Http request"),
SDATA (ASN_OCTET_STR,   "url",              SDF_RD,             "",             "Url"),
SDATA (ASN_UNSIGNED64,  "__msg_key__",      SDF_RD,             0,              "Request id"),
SDATA (ASN_JSON,        "kw_connex",        SDF_RD,             0,              "Kw to create connex"),
SDATA (ASN_POINTER,     "user_data",        0,                  0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                  0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                  0,              "subscriber of output-events. If it's null then subscriber is the parent."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_MESSAGES = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"messages",                "Trace messages"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    const char *request;
    const char *url;
    int32_t timeout;
    uint64_t __msg_key__;

    GHTTP_PARSER *parsing_response;     // A response parser instance
    hgobj tcp0;
    hgobj timer;
} PRIVATE_DATA;




            /******************************
             *      Framework Methods
             ******************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    priv->timer = gobj_create("prot_http_cli", GCLASS_TIMER, 0, gobj);
    priv->parsing_response = ghttp_parser_create(gobj, HTTP_RESPONSE, FALSE, FALSE);

    /*
     *  CHILD subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(timeout,               gobj_read_int32_attr)
    SET_PRIV(request,               gobj_read_str_attr)
    SET_PRIV(__msg_key__,           gobj_read_uint64_attr)
    SET_PRIV(url,                   gobj_read_str_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout,                 gobj_read_int32_attr)
    ELIF_EQ_SET_PRIV(request,               gobj_read_str_attr)
    ELIF_EQ_SET_PRIV(__msg_key__,           gobj_read_uint64_attr)
    ELIF_EQ_SET_PRIV(url,                   gobj_read_str_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);

    json_t *kw_connex = gobj_read_json_attr(gobj, "kw_connex");
    json_incref(kw_connex);
    priv->tcp0 = gobj_create(gobj_name(gobj), GCLASS_CONNEX, kw_connex, gobj);
    gobj_set_bottom_gobj(gobj, priv->tcp0);
    gobj_write_str_attr(priv->tcp0, "tx_ready_event_name", 0);

    gobj_start(priv->tcp0);

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    clear_timeout(priv->timer);
    gobj_stop(priv->timer);
    if(priv->tcp0) {
        if(gobj_is_running(priv->tcp0))
            gobj_stop(priv->tcp0);
    }
    return 0;
}

/***************************************************************************
 *      Framework Method play
 ***************************************************************************/
PRIVATE int mt_play(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    set_timeout(priv->timer, priv->timeout);

    ghttp_parser_reset(priv->parsing_response);

    /*
     * send the request
     */
    send_prot_http_cli(
        gobj,
        gobj_read_str_attr(gobj, "request")
    );

    gobj_change_state(gobj, "ST_WAIT_RESPONSE");

    return 0;
}

/***************************************************************************
 *      Framework Method pause
 ***************************************************************************/
PRIVATE int mt_pause(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    clear_timeout(priv->timer);

    gobj_change_state(gobj, "ST_CONNECTED");

    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->parsing_response) {
        ghttp_parser_destroy(priv->parsing_response);
        priv->parsing_response = 0;
    }
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *  Send a http message
 ***************************************************************************/
PRIVATE int send_prot_http_cli(hgobj gobj, const char *prot_http_cli)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(empty_string(prot_http_cli)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "request EMPTY",
            NULL
        );
        return -1;
    }
    char host[120];
    parse_http_url(priv->url, 0, 0, host, sizeof(host), 0, 0, FALSE);

    GBUFFER *gbuf = gbuf_create(256, 4*1024, 0,0);
    if(!gbuf) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "gbuf_create() FAILED",
            NULL
        );
        return -1;
    }
    gbuf_printf(gbuf, "GET %s HTTP/1.1\r\n", prot_http_cli);
    gbuf_printf(gbuf, "Host: %s\r\n", host); // TODO priv->url prueba esto para que dé fallo la response;
    gbuf_printf(gbuf, "User-Agent: yuneta-%s\r\n",  __yuneta_version__);
    gbuf_printf(gbuf, "Connection: keep-alive\r\n");
    gbuf_printf(gbuf, "Accept: */*\r\n");
    gbuf_printf(gbuf, "\r\n");

    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        log_debug_gbuf(
            LOG_DUMP_OUTPUT, gbuf, "HTTP Request __msg_key__ %"PRIu64" %s",
            priv->__msg_key__, gobj_short_name(gobj)
        );
    }

    json_t *kw = json_pack("{s:I}",
        "gbuffer", (json_int_t)(size_t)gbuf
    );
    return gobj_send_event(priv->tcp0, "EV_TX_DATA", kw, gobj);
}

/***************************************************************************
 *  Parse a http message
 *  Return -1 if error: you must close the socket.
 *  Return 0 if no new request.
 *  Return 1 if new request available in `request`.
 ***************************************************************************/
PRIVATE int process_http(hgobj gobj, GBUFFER *gbuf, GHTTP_PARSER *parser)
{
    while (gbuf_leftbytes(gbuf)) {
        size_t ln = gbuf_leftbytes(gbuf);
        char *bf = gbuf_cur_rd_pointer(gbuf);
        int n = ghttp_parser_received(parser, bf, ln);
        if (n == -1) {
            // Some error in parsing
            ghttp_parser_reset(parser);
            return -1;
        } else if (n > 0) {
            gbuf_get(gbuf, n);  // take out the bytes consumed
        }

        if (parser->message_completed) {
            /*
             *  The cur_request (with the body) is ready to use.
             */
            return 1;
        }
    }
    return 0; // Paquete parcial?
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *  iam client. send the request
 ***************************************************************************/
PRIVATE int ac_connected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_disconnected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    if(gobj_is_volatil(src)) {
        gobj_set_bottom_gobj(gobj, 0);
    }
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Process http response.
 ***************************************************************************/
PRIVATE int ac_http_response(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);

    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        log_debug_gbuf(
            LOG_DUMP_INPUT, gbuf, "HTTP Response __msg_key__ %"PRIu64" %s",
            priv->__msg_key__, gobj_short_name(gobj)
        );
    }

    int result = process_http(gobj, gbuf, priv->parsing_response);
    if (result < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "process_http() FAILED",
            "key",          "%lu", (unsigned long) priv->__msg_key__,
            NULL
        );
        gbuf_reset_rd(gbuf);
        log_debug_gbuf(LOG_DUMP_INPUT, gbuf, "process_http() FAILED");
        gobj_publish_event(gobj, "EV_HTTP_RESPONSE", 0);

    } else if (result > 0) {
        if (priv->parsing_response->http_parser.status_code == 200) {
            char *body = priv->parsing_response->body;
            json_t *jn_response = legalstring2json(body, TRUE);
            gobj_publish_event(gobj, "EV_HTTP_RESPONSE", jn_response);
        } else {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "NO 200 HTTP Response",
                "status",       "%d", priv->parsing_response->http_parser.status_code,
                "key",          "%lu", (unsigned long) priv->__msg_key__,
                NULL
            );
            gbuf_reset_rd(gbuf);
            log_debug_gbuf(LOG_DUMP_INPUT, gbuf, "NO 200 HTTP Response");
            gobj_publish_event(gobj, "EV_HTTP_RESPONSE", 0);
        }
    }

    gobj_pause(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout_response(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    log_error(0,
        "gobj",         "%s", gobj_full_name(gobj),
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_APP_ERROR,
        "msg",          "%s", "Nominatim Timeout HTTP Response",
        "url",          "%s", gobj_read_str_attr(gobj, "url"),
        "request",      "%s", gobj_read_str_attr(gobj, "request"),
        "key",          "%lu", (unsigned long) priv->__msg_key__,
        NULL
    );
    gobj_publish_event(gobj, "EV_HTTP_RESPONSE", 0);
    gobj_pause(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Child stopped
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    if(gobj_is_volatil(src)) {
        gobj_destroy(src);
    }
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    // bottom input
    {"EV_RX_DATA",          0},
    {"EV_TIMEOUT",          0},
    {"EV_CONNECTED",        0},
    {"EV_DISCONNECTED",     0},
    {"EV_STOPPED",          0},
    // internal
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_HTTP_RESPONSE",    0, 0, 0},
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_DISCONNECTED",
    "ST_CONNECTED",
    "ST_WAIT_RESPONSE",
    NULL
};

PRIVATE EV_ACTION ST_DISCONNECTED[] = {
    {"EV_CONNECTED",        ac_connected,                       "ST_CONNECTED"},
    {"EV_DISCONNECTED",     ac_disconnected,                    0},
    {"EV_STOPPED",          ac_stopped,                         0},
    {0,0,0}
};
PRIVATE EV_ACTION ST_CONNECTED[] = {
    {"EV_DISCONNECTED",     ac_disconnected,                    "ST_DISCONNECTED"},
    {0,0,0}
};
PRIVATE EV_ACTION ST_WAIT_RESPONSE[] = {
    {"EV_RX_DATA",          ac_http_response,                   "ST_CONNECTED"},
    {"EV_TIMEOUT",          ac_timeout_response,                "ST_CONNECTED"},
    {"EV_DISCONNECTED",     ac_disconnected,                    "ST_DISCONNECTED"},
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_DISCONNECTED,
    ST_CONNECTED,
    ST_WAIT_RESPONSE,
    NULL
};

PRIVATE FSM fsm = {
    input_events,
    output_events,
    state_names,
    states,
};

/***************************************************************************
 *              GClass
 ***************************************************************************/
/*---------------------------------------------*
 *              Local methods table
 *---------------------------------------------*/
PRIVATE LMETHOD lmt[] = {
    {0, 0, 0}
};

/*---------------------------------------------*
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_PROT_HTTP_CLI_NAME,      // CHANGE WITH each gclass
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        mt_destroy,
        mt_start,
        mt_stop,
        mt_play,
        mt_pause,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_child_added,
        0, //mt_child_removed,
        0, //mt_stats,
        0, //mt_command_parser,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link
        0, //mt_delete_child_resource_link
        0, //mt_get_resource
        0, //mt_future24,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_future33,
        0, //mt_future34,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_authz_checker,
        0, //mt_future39,
        0, //mt_create_node,
        0, //mt_update_node,
        0, //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_future44,
        0, //mt_unlink_nodes,
        0, //mt_topic_jtree,
        0, //mt_get_node,
        0, //mt_list_nodes,
        0, //mt_snap_nodes,
        0, //mt_set_nodes_snap,
        0, //mt_list_nodes_snaps,
        0, //mt_treedbs,
        0, //mt_treedb_topics,
        0, //mt_topic_desc,
        0, //mt_topic_links,
        0, //mt_topic_hooks,
        0, //mt_node_parents,
        0, //mt_node_childs,
        0, //mt_list_instances,
        0, //mt_node_tree,
        0, //mt_topic_size,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
    s_user_trace_level,
    0,  // cmds
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_prot_http_cli(void)
{
    return &_gclass;
}
