/***********************************************************************
 *          C_TASK.C
 *          Task GClass.
 *
        "jobs": [
            {
                "exec_action", "$local_method",
                "exec_timeout", 10000,
                "exec_result", "$local_method",
            },
            ...
        ]

    being "$local_method" a local method of gobj_jobs
    "exec_timeout" in miliseconds

    if exec_action() or exec_result() return -1 then the job is stopped

 *          Tasks
 *              - gobj_jobs: gobj defining the jobs (local methods)
 *              - gobj_results: GObj executing the jobs,
 *                  - set as bottom gobj
 *                  - inform of results of the jobs with the api
 *                      EV_ON_OPEN
 *                      EV_ON_CLOSE
 *                      EV_ON_MESSAGE ->
 *                          {
 *                              result: 0 or -1,
 *                              data:
 *                          }

 *
 *          HACK if gobj is volatil then will self auto-destroy on stop
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <stdio.h>
#include "c_task.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int execute_action(hgobj gobj);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE sdata_desc_t pm_help[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help."),
SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authzs[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "authz",        0,              0,          "authz about you want help"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias-------items-----------json_fn---------description---------- */
SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,        cmd_help,       "Command's help"),
SDATACM (ASN_SCHEMA,    "authzs",           0,          pm_authzs,      cmd_authzs,     "Authorization's help"),
SDATA_END()
};


/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag------------default---------description---------- */
SDATA (ASN_JSON,        "jobs",             SDF_RD,         0,              "Jobs"),
SDATA (ASN_JSON,        "input_data",       SDF_RD,         "{}",           "Input Jobs Data. Use as you want. Available in exec_action() and exec_result() action methods."),
SDATA (ASN_JSON,        "output_data",      SDF_RD,         "{}",           "Output Jobs Data. Use as you want. Available in exec_action() and exec_result() action methods."),
SDATA (ASN_POINTER,     "gobj_jobs",        0,              0,              "GObj defining the jobs"),
SDATA (ASN_POINTER,     "gobj_results",     0,              0,              "GObj executing the jobs"),
SDATA (ASN_INTEGER,     "timeout",          SDF_PERSIST|SDF_WR,10*1000,     "Action Timeout"),
SDATA (ASN_POINTER,     "user_data",        0,              0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,              0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,              0,              "subscriber of output-events. Not a child gobj."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *  HACK strict ascendent value!
 *  required paired correlative strings
 *  in s_user_trace_level
 *---------------------------------------------*/
enum {
    TRACE_MESSAGES = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"messages",        "Trace messages"},
{0, 0},
};

/*---------------------------------------------*
 *      GClass authz levels
 *---------------------------------------------*/
PRIVATE sdata_desc_t pm_authz_sample[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "param sample",     0,          "",             "Param ..."),
SDATA_END()
};

PRIVATE sdata_desc_t authz_table[] = {
/*-AUTHZ-- type---------name------------flag----alias---items---------------description--*/
SDATAAUTHZ (ASN_SCHEMA, "sample",       0,      0,      pm_authz_sample,    "Permission to ..."),
SDATA_END()
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    int32_t timeout;
    hgobj timer;

    json_t *jobs;
    int max_job;
    int cur_job;

    hgobj gobj_jobs;
    hgobj gobj_results;
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

    priv->timer = gobj_create(gobj_name(gobj), GCLASS_TIMER, 0, gobj);

    /*
     *  SERVICE subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(subscriber) {
        gobj_subscribe_event(gobj, NULL, NULL, subscriber);
    }

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(timeout,               gobj_read_int32_attr)
    SET_PRIV(jobs,                  gobj_read_json_attr)
    SET_PRIV(gobj_results,          gobj_read_pointer_attr)
    SET_PRIV(gobj_jobs,             gobj_read_pointer_attr)

    priv->max_job = json_array_size(priv->jobs);
    gobj_set_bottom_gobj(gobj, priv->gobj_results);
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout,             gobj_read_int32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*
     *  Subscribe to gobj_results events
     *  TODO gobj_results puede dar soporte a varias tasks,
     *  hay que suscribirse con algún tipo de id
     */
    gobj_subscribe_event(priv->gobj_results, NULL, NULL, gobj);

    gobj_start(priv->timer);

    priv->cur_job = 0; // First job at start
    if(gobj_read_bool_attr(gobj, "connected")) {
        execute_action(gobj);
    }

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_unsubscribe_event(priv->gobj_results, NULL, NULL, gobj);
    clear_timeout(priv->timer);
    gobj_stop(priv->timer);
    return 0;
}




            /***************************
             *      Commands
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    KW_INCREF(kw);
    json_t *jn_resp = gobj_build_cmds_doc(gobj, kw);
    return msg_iev_build_webix(
        gobj,
        0,
        jn_resp,
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    return gobj_build_authzs_doc(gobj, cmd, kw, src);
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int stop_task(hgobj gobj, int result)
{
    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        trace_msg("================! stop task, result %d of %s", result, gobj_name(gobj));
    }

    gobj_publish_event(gobj,
        "EV_END_TASK",
        json_pack("{s:i, s:O, s:O}",
            "result", result,
            "input_data", gobj_read_json_attr(gobj, "input_data"),
            "output_data", gobj_read_json_attr(gobj, "output_data")
        )
    );
    gobj_stop(gobj);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int execute_action(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *jn_job_ = json_array_get(priv->jobs, priv->cur_job);
    if(!jn_job_) {
        stop_task(gobj, 0);
        return 0;
    }

    const char *action = kw_get_str(jn_job_, "exec_action", "", KW_REQUIRED);
    json_int_t exec_timeout = kw_get_int(jn_job_, "exec_timeout", priv->timeout, 0);

    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        trace_msg("================> exec ACTION %d: %s of %s", priv->cur_job, action, gobj_name(gobj));
    }

    int ret = (int)(size_t)gobj_exec_internal_method(
        priv->gobj_jobs,
        action,
        0,
        gobj
    );
    if(ret < 0) {
        stop_task(gobj, ret);
    } else {
        set_timeout(priv->timer, exec_timeout);
    }

    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *  Connected
 ***************************************************************************/
PRIVATE int ac_on_open(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    execute_action(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Disconnected
 ***************************************************************************/
PRIVATE int ac_on_close(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_message(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *jn_job_ = json_array_get(priv->jobs, priv->cur_job);

    const char *action = kw_get_str(jn_job_, "exec_result", "", KW_REQUIRED);

    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        trace_msg("================< exec RESULT %d: %s of %s", priv->cur_job, action, gobj_name(gobj));
        log_debug_json(0, kw, "exec RESULT");
    }

    int ret = (int)(size_t)gobj_exec_internal_method(
        priv->gobj_jobs,
        action,
        json_incref(kw),
        gobj
    );
    if(ret < 0) {
        stop_task(gobj, ret);
    } else {
        priv->cur_job++;
        execute_action(gobj);
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    KW_DECREF(kw);
    if(gobj_is_volatil(gobj)) {
        gobj_destroy(gobj);
    }
    return 0;
}

/***************************************************************************
 *  Timeout waiting result
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    json_t *output_data = gobj_read_json_attr(gobj, "output_data");
    json_object_set_new(
        output_data,
        "comment",
        json_sprintf("Task failed by timeout")
    );

    KW_DECREF(kw);
    stop_task(gobj, -1);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    {"EV_ON_MESSAGE",   0,  0,  ""},
    {"EV_ON_OPEN",      0,  0,  ""},
    {"EV_ON_CLOSE",     0,  0,  ""},
    // bottom input
    {"EV_TIMEOUT",      0,  0,  ""},
    {"EV_STOPPED",      0,  0,  ""},
    // internal
    {NULL, 0, 0, ""}
};
PRIVATE const EVENT output_events[] = {
    {"EV_END_TASK",         EVF_PUBLIC_EVENT,   0,  0},
    {NULL, 0, 0, ""}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_ON_MESSAGE",           ac_on_message,          0},
    {"EV_ON_OPEN",              ac_on_open,             0},
    {"EV_ON_CLOSE",             ac_on_close,            0},
    {"EV_TIMEOUT",              ac_timeout,             0},
    {"EV_STOPPED",              ac_stopped,             0},
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_IDLE,
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
    GCLASS_TASK_NAME,
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_play,
        0, //mt_pause,
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
        0, //mt_authorization_parser,
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
        0, //mt_authzs,
        0, //mt_create_node,
        0, //mt_update_node,
        0, //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_link_nodes2,
        0, //mt_unlink_nodes,
        0, //mt_unlink_nodes2,
        0, //mt_get_node,
        0, //mt_list_nodes,
        0, //mt_shoot_snap,
        0, //mt_activate_snap,
        0, //mt_list_snaps,
        0, //mt_treedbs,
        0, //mt_treedb_topics,
        0, //mt_topic_desc,
        0, //mt_topic_links,
        0, //mt_topic_hooks,
        0, //mt_node_parents,
        0, //mt_node_childs,
        0, //mt_node_instances,
        0, //mt_save_node,
        0, //mt_topic_size,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    authz_table,
    s_user_trace_level,
    command_table,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_task(void)
{
    return &_gclass;
}
