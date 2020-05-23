/***********************************************************************
 *          C_NODE.C
 *          Node GClass.
 *
 *          Nodes: resources with treedb
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "c_node.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
// PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_list_keycloak_users(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_list_tasks(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_enable_task(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_disable_task(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
//
// PRIVATE json_t *cmd_print_tranger(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_list_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_list_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_create_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_update_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_delete_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_link_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_unlink_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_find_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_build_validation(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
// PRIVATE json_t *cmd_build_report(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
//
// PRIVATE sdata_desc_t pm_help[] = {
// /*-PM----type-----------name------------flag------------default-----description---------- */
// SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help."),
// SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
// SDATA_END()
// };
//
// PRIVATE sdata_desc_t pm_task[] = {
// /*-PM----type-----------name------------flag------------default-----description---------- */
// SDATAPM (ASN_OCTET_STR, "username",     0,              0,          "User id"),
// SDATAPM (ASN_OCTET_STR, "event",        0,              0,          "Event"),
// SDATAPM (ASN_OCTET_STR, "task_name",    0,              0,          "Task name"),
// SDATA_END()
// };
//
// PRIVATE sdata_desc_t pm_list_topics[] = {
// /*-PM----type-----------name------------flag------------default-----description---------- */
// SDATAPM (ASN_OCTET_STR, "treedb_name",  0,              0,          "Treedb name"),
// SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: 'dict'"),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_list_nodes[] = {
// /*-PM----type-----------name------------flag------------default-----description---------- */
// SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
// SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_create_node[] = {
// /*-PM----type-----------name------------flag------------default-----description---------- */
// SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
// SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Node content"),
// SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
// SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"permissive\""),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_update_node[] = {
// /*-PM----type-----------name------------flag------------default-----description---------- */
// SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
// SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Node content"),
// SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
// SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"create\", \"clean\""),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_delete_node[] = {
// SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
// SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
// SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"force\""),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_link_nodes[] = {
// SDATAPM (ASN_OCTET_STR, "parent",       0,              0,          "Parent node"),
// SDATAPM (ASN_OCTET_STR, "child",        0,              0,          "Child node"),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_find_node[] = {
// SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
// SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_build_validation[] = {
// SDATAPM (ASN_OCTET_STR, "username",     0,              0,          "Username (email)"),
// SDATAPM (ASN_OCTET_STR, "date",         0,              0,          "YYYY-MM-DD"),
// SDATAPM (ASN_OCTET_STR, "force",        0,              0,          "Re-build validation if exists"),
// SDATA_END()
// };
// PRIVATE sdata_desc_t pm_build_report[] = {
// SDATAPM (ASN_OCTET_STR, "username",     0,              0,          "Username (email)"),
// SDATAPM (ASN_OCTET_STR, "date",         0,              0,          "YYYY-MM-DD"),
// SDATAPM (ASN_OCTET_STR, "report",       0,              0,          "Report type: 'month'"),
// SDATA_END()
// };
//
// PRIVATE const char *a_help[] = {"h", "?", 0};
//
// PRIVATE sdata_desc_t command_table[] = {
// /*-CMD---type-----------name----------------alias-------items-------json_fn---------description--*/
// SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,    cmd_help,       "Command's help"),
// SDATACM (ASN_SCHEMA,    "list-keycloak-users",0,        0,          cmd_list_keycloak_users, "List keycloak users"),
// SDATACM (ASN_SCHEMA,    "list-tasks",       0,          0,          cmd_list_tasks, "List task"),
// SDATACM (ASN_SCHEMA,    "enable-task",      0,          pm_task,    cmd_enable_task,"Enable task"),
// SDATACM (ASN_SCHEMA,    "disable-task",     0,          pm_task,    cmd_disable_task,"Disable task"),
//
// /*-CMD---type-----------name------------al--items-----------json_fn-------------description--*/
// SDATACM (ASN_SCHEMA,    "print-tranger",0,  0,              cmd_print_tranger,  "Print tranger"),
// SDATACM (ASN_SCHEMA,    "list-topics",  0,  pm_list_topics, cmd_list_topics,    "List topics"),
// SDATACM (ASN_SCHEMA,    "list-nodes",   0,  pm_list_nodes,  cmd_list_nodes,     "List nodes"),
// SDATACM (ASN_SCHEMA,    "find-node",    0,  pm_find_node,   cmd_find_node,      "Find node"),
// SDATACM (ASN_SCHEMA,    "create-node",  0,  pm_create_node, cmd_create_node,    "Create node"),
// SDATACM (ASN_SCHEMA,    "update-node",  0,  pm_update_node, cmd_update_node,    "Update node"),
// SDATACM (ASN_SCHEMA,    "delete-node",  0,  pm_delete_node, cmd_delete_node,    "Delete node"),
// SDATACM (ASN_SCHEMA,    "link-nodes",   0,  pm_link_nodes,  cmd_link_nodes,     "Link nodes"),
// SDATACM (ASN_SCHEMA,    "unlink-nodes", 0,  pm_link_nodes,  cmd_unlink_nodes,   "Unlink nodes"),
// SDATACM (ASN_SCHEMA,    "build_validation",0,pm_build_validation,cmd_build_validation,"Build validation"),
// SDATACM (ASN_SCHEMA,    "build_report", 0,  pm_build_report,cmd_build_report,   "Build report"),
//
// SDATA_END()
// };
//

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_JSON,        "treedb_schema",    SDF_REQUIRED,       0,              "Treedb schema"),
SDATA (ASN_JSON,        "tranger",          0,                  0,              "TimeRanger"),
SDATA (ASN_OCTET_STR,   "database",         SDF_RD,             0,              "Database name. Not empty to be persistent."),
SDATA (ASN_OCTET_STR,   "service",          SDF_RD,             "",             "Service name for global store"),
SDATA (ASN_OCTET_STR,   "filename_mask",    SDF_RD,            "%Y",            "Treedb filename mask"),
SDATA (ASN_BOOLEAN,     "local_store",      SDF_RD,             0,              "True for local store (not clonable)"),
SDATA (ASN_POINTER,     "kw_match",         0,                  kw_match_simple,"kw_match default function"),
SDATA (ASN_INTEGER,     "exit_on_error",    0,                  LOG_OPT_EXIT_ZERO,"exit on error"),
SDATA (ASN_POINTER,     "user_data",        0,                  0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                  0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                  0,              "subscriber of output-events. Not a child gobj."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_MESSAGES = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"messages",        "Trace messages"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    json_t *treedb_schema;
    kw_match_fn kw_match;

    json_t *tranger;
    int32_t exit_on_error;
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

    priv->treedb_schema = gobj_read_json_attr(gobj, "treedb_schema");
    priv->kw_match = gobj_read_pointer_attr(gobj, "kw_match");

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
    SET_PRIV(exit_on_error,             gobj_read_int32_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(tranger,             gobj_read_json_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *database = gobj_read_str_attr(gobj, "database");
    if(empty_string(database)) {
        log_critical(priv->exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "database name EMPTY",
            NULL
        );
        return -1;
    }
    const char *filename_mask = gobj_read_str_attr(gobj, "filename_mask");
    if(empty_string(filename_mask)) {
        log_critical(priv->exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "filename_mask EMPTY",
            NULL
        );
        return -1;
    }

    BOOL local_store = gobj_read_bool_attr(gobj, "local_store");
    char path[NAME_MAX];
    if(local_store) {
        yuneta_realm_dir(path, sizeof(path), "store", TRUE);
    } else {
        const char *service = gobj_read_str_attr(gobj, "service");
        yuneta_store_dir(path, sizeof(path), "resources", service, TRUE);
    }

    json_t *jn_tranger = json_pack("{s:s, s:s, s:s, s:b}",
        "path", path,
        "database", database,
        "filename_mask", filename_mask,
        "master", 1
    );

    json_t *tranger = tranger_startup(
        jn_tranger // owned
    );

    if(!tranger) {
        log_critical(priv->exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "tranger_startup() FAILED",
            NULL
        );
    }
    gobj_write_json_attr(gobj, "tranger", tranger);
    json_decref(tranger); // HACK solo una copia de tranger

    JSON_INCREF(priv->treedb_schema);
    treedb_open_db( // Return IS NOT YOURS!
        tranger,
        database,
        priv->treedb_schema,  // owned
        "persistent"
    );
    if(tranger) {
//      TODO       create_persistent_resources(gobj); // IDEMPOTENTE.
//             load_persistent_resources(gobj);
    }

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *treedbs = treedb_list_treedb(priv->tranger);

    int idx; json_t *jn_treedb;
    json_array_foreach(treedbs, idx, jn_treedb) {
        treedb_close_db(priv->tranger, json_string_value(jn_treedb));
    }
    JSON_DECREF(treedbs);

    JSON_INCREF(priv->tranger);  // HACK copy of attr, the attr will decref
    EXEC_AND_RESET(tranger_shutdown, priv->tranger);

    gobj_write_json_attr(gobj, "tranger", 0);

    return 0;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_trace_on(hgobj gobj, const char *level, json_t *kw)
{
    treedb_set_trace(TRUE);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_trace_off(hgobj gobj, const char *level, json_t *kw)
{
    treedb_set_trace(FALSE);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_create_node( // Return is NOT YOURS
    hgobj gobj,
    const char *topic_name,
    json_t *kw, // owned
    const char *options // "permissive"
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *record = treedb_create_node( // Return is NOT YOURS
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        topic_name,
        kw, // owned
        options
    );
    return record;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_update_node( // Return is NOT YOURS
    hgobj gobj,
    const char *topic_name,
    json_t *kw,    // owned
    const char *options // "create" ["permissive"], "clean"
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *record = treedb_update_node( // Return is NOT YOURS
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        topic_name,
        kw, // owned
        options
    );
    return record;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_delete_node(hgobj gobj, const char *topic_name, json_t *kw, const char *options)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_delete_node(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        topic_name,
        kw, // owned
        options
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_link_nodes(hgobj gobj, const char *hook, json_t *parent, json_t *child)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_link_nodes(
        priv->tranger,
        hook,
        parent,
        child
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_link_nodes2(hgobj gobj, const char *parent_ref, const char *child_ref)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_link_nodes2(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        parent_ref,
        child_ref
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_unlink_nodes(hgobj gobj, const char *hook, json_t *parent, json_t *child)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_unlink_nodes(
        priv->tranger,
        hook,
        parent,
        child
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_unlink_nodes2(hgobj gobj, const char *parent_ref, const char *child_ref)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_unlink_nodes2(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        parent_ref,
        child_ref
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_get_node(hgobj gobj, const char *topic_name, const char *id)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_get_node(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        topic_name,
        id
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_list_nodes(
    hgobj gobj,
    const char *topic_name,
    json_t *jn_filter,
    json_t *jn_options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_list_nodes(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        topic_name,
        jn_filter,
        jn_options,
        gobj_read_pointer_attr(gobj, "kw_match")
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_snap_nodes(hgobj gobj, const char *tag)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_snap_nodes(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        tag
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_set_nodes_snap(hgobj gobj, const char *tag)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_set_nodes_snap(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        tag
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_list_nodes_snaps(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_list_nodes_snaps(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED) // treedb_name
    );
}




            /***************************
             *      Commands
             ***************************/




            /***************************
             *      Local Methods
             ***************************/




            /***************************
             *      Private Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/



            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
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
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_NODE_NAME,      // CHANGE WITH each gclass
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
        0,
        0, //mt_command,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link,
        0, //mt_delete_child_resource_link,
        0, //mt_get_resource,
        0, //mt_future24,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        mt_trace_on,
        mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_future33,
        0, //mt_future34,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_future38,
        0, //mt_future39,
        mt_create_node,
        mt_update_node,
        mt_delete_node,
        mt_link_nodes,
        mt_link_nodes2,
        mt_unlink_nodes,
        mt_unlink_nodes2,
        mt_get_node,
        mt_list_nodes,
        mt_snap_nodes,
        mt_set_nodes_snap,
        mt_list_nodes_snaps,
        0, //mt_future52,
        0, //mt_future53,
        0, //mt_future54,
        0, //mt_future55,
        0, //mt_future56,
        0, //mt_future57,
        0, //mt_future58,
        0, //mt_future59,
        0, //mt_future60,
        0, //mt_future61,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    0,  // lmt
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
    s_user_trace_level,
    0,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_node(void)
{
    return &_gclass;
}
