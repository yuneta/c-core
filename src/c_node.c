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
PRIVATE int treedb_callback(
    void *user_data,
    json_t *tranger,
    const char *treedb_name,
    const char *topic_name,
    const char *operation,  // "node_updated", "node_deleted"
    json_t *node            // owned
);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_print_tranger(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_create_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_update_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_delete_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_link_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_unlink_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_treedbs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_hooks(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_links(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_parents(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_childs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_list_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_node_instances(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_touch_instance(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_node_pkey2s(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_list_snaps(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_snap_content(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_shoot_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_activate_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_deactivate_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE sdata_desc_t pm_help[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help."),
SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_create_node[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Node content in string"),
SDATAPM (ASN_JSON,      "record",       0,              0,          "Node content in json"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"permissive\" \"multiple\""),
SDATA_END()
};
PRIVATE sdata_desc_t pm_update_node[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Node content in string"),
SDATAPM (ASN_JSON,      "record",       0,              0,          "Node content in json"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"create\", \"clean\""),
SDATA_END()
};
PRIVATE sdata_desc_t pm_delete_node[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_JSON,      "record",       0,              0,          "Node content in json"),
SDATAPM (ASN_BOOLEAN,   "force",        0,              0,          "Force delete"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_link_nodes[] = {
SDATAPM (ASN_OCTET_STR, "parent_ref",   0,              0,          "Parent node ref"),
SDATAPM (ASN_OCTET_STR, "child_ref",    0,              0,          "Child node ref"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_trace[] = {
SDATAPM (ASN_BOOLEAN,   "set",          0,              0,          "Trace: set 1 o 0"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_topics[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "treedb_name",  0,              0,          "Treedb name"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: 'dict'"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_desc[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_hooks[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_links[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_parents[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_OCTET_STR, "link",         0,              0,          "Link port to parents"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Parent nodes expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_childs[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_OCTET_STR, "hook",         0,              0,          "Hook port to childs"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Child nodes expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_list_nodes[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Tree expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_get_node[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Tree expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_node_instances[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_OCTET_STR, "pkey2",        0,              0,          "PKey2 field"),
SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Tree expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_node_pkey2s[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_snap_content[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "snap_name",    0,              0,          "Snap name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_shoot_snap[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "name",         0,              0,          "Snap name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_activate_snap[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "name",          0,              0,          "Snap name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_print_tranger[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "path",         0,              "",         "Path"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "No expanded (default) return [[size]]"),
SDATAPM (ASN_UNSIGNED,  "lists_limit",  0,              0,          "Expand lists only if size < limit. 0 no limit"),
SDATAPM (ASN_UNSIGNED,  "dicts_limit",  0,              0,          "Expand dicts only if size < limit. 0 no limit"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};
PRIVATE const char *a_nodes[] = {"list-nodes", "list-records", 0};
PRIVATE const char *a_node[] = {"get-node", "get-record", 0};
PRIVATE const char *a_create[] = {"create-record", 0};
PRIVATE const char *a_update[] = {"update-record", 0};
PRIVATE const char *a_delete[] = {"delete-record", 0};
PRIVATE const char *a_schemas[] = {"schemas","list-schemas", 0};
PRIVATE const char *a_schema[] = {"schema", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias-------items-------json_fn---------description--*/
SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,    cmd_help,       "Command's help"),

/*-CMD---type-----------name------------al--items-----------json_fn-------------description--*/
SDATACM (ASN_SCHEMA,    "treedbs",      0,  0,              cmd_treedbs,        "List treedb's"),
SDATACM (ASN_SCHEMA,    "topics",       0,  pm_topics,      cmd_topics,         "List topics"),
SDATACM (ASN_SCHEMA,    "create-node",  a_create, pm_create_node, cmd_create_node, "Create node"),
SDATACM (ASN_SCHEMA,    "update-node",  a_update, pm_update_node, cmd_update_node, "Update node"),
SDATACM (ASN_SCHEMA,    "delete-node",  a_delete, pm_delete_node, cmd_delete_node, "Delete node"),
SDATACM (ASN_SCHEMA,    "nodes",        a_nodes, pm_list_nodes, cmd_list_nodes,  "List nodes"),
SDATACM (ASN_SCHEMA,    "node",         a_node, pm_get_node, cmd_get_node,       "Get node by id"),
SDATACM (ASN_SCHEMA,    "instances",    0,  pm_node_instances,cmd_node_instances,"List node's instances"),
SDATACM (ASN_SCHEMA,    "link-nodes",   0,  pm_link_nodes,  cmd_link_nodes,     "Link nodes"),
SDATACM (ASN_SCHEMA,    "unlink-nodes", 0,  pm_link_nodes,  cmd_unlink_nodes,   "Unlink nodes"),
SDATACM (ASN_SCHEMA,    "hooks",        0,  pm_hooks,       cmd_hooks,          "Hooks of node"),
SDATACM (ASN_SCHEMA,    "links",        0,  pm_links,       cmd_links,          "Links of node"),
SDATACM (ASN_SCHEMA,    "parents",      0,  pm_parents,     cmd_parents,        "Parents of node"),
SDATACM (ASN_SCHEMA,    "childs",       0,  pm_childs,      cmd_childs,         "Childs of node"),
SDATACM (ASN_SCHEMA,    "touch-instance",0, pm_node_instances,cmd_touch_instance,"Touch (save) instance to force to be the last instance"),
SDATACM (ASN_SCHEMA,    "snaps",        0,  0,              cmd_list_snaps,     "List snaps"),
SDATACM (ASN_SCHEMA,    "snap-content", 0,  pm_snap_content,cmd_snap_content,   "Show snap content"),
SDATACM (ASN_SCHEMA,    "shoot-snap",   0,  pm_shoot_snap,  cmd_shoot_snap,     "Shoot snap"),
SDATACM (ASN_SCHEMA,    "activate-snap",0,  pm_activate_snap,cmd_activate_snap, "Activate snap"),
SDATACM (ASN_SCHEMA,    "deactivate-snap",0,0,              cmd_deactivate_snap,"De-Activate snap"),
SDATACM (ASN_SCHEMA,    "pkey2s",       0,  pm_node_pkey2s, cmd_node_pkey2s,    "List node's pkey2"),
SDATACM (ASN_SCHEMA,    "desc",         a_schema, pm_desc,  cmd_desc,           "Schema of topic"),
SDATACM (ASN_SCHEMA,    "descs",        a_schemas, 0,       cmd_desc,           "Schema of topics"),
SDATACM (ASN_SCHEMA,    "print-tranger",0,  pm_print_tranger, cmd_print_tranger,  "Print tranger"),
SDATACM (ASN_SCHEMA,    "trace",        0,  pm_trace,       cmd_trace,          "Set trace"),
SDATA_END()
};

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "tranger",          SDF_RD|SDF_REQUIRED,0,              "Tranger handler"),
SDATA (ASN_OCTET_STR,   "treedb_name",      SDF_RD|SDF_REQUIRED,"",             "Treedb name"),
SDATA (ASN_JSON,        "treedb_schema",    SDF_RD|SDF_REQUIRED,0,              "Treedb schema"),
SDATA (ASN_INTEGER,     "exit_on_error",    0,                  LOG_OPT_EXIT_ZERO,"exit on error"),
SDATA (ASN_POINTER,     "kw_match",         0,                  kw_match_simple,"kw_match default function"),
SDATA (ASN_INTEGER,     "timeout",          SDF_RD,             1*1000,         "Timeout"),
SDATA (ASN_COUNTER64,   "txMsgs",           SDF_RD|SDF_PSTATS,  0,              "Messages transmitted"),
SDATA (ASN_COUNTER64,   "rxMsgs",           SDF_RD|SDF_RSTATS,  0,              "Messages receiveds"),

SDATA (ASN_COUNTER64,   "txMsgsec",         SDF_RD|SDF_RSTATS,  0,              "Messages by second"),
SDATA (ASN_COUNTER64,   "rxMsgsec",         SDF_RD|SDF_RSTATS,  0,              "Messages by second"),
SDATA (ASN_COUNTER64,   "maxtxMsgsec",      SDF_WR|SDF_RSTATS,  0,              "Max Tx Messages by second"),
SDATA (ASN_COUNTER64,   "maxrxMsgsec",      SDF_WR|SDF_RSTATS,  0,              "Max Rx Messages by second"),
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
    json_t *tranger;
    const char *treedb_name;
    json_t *treedb_schema;
    int32_t exit_on_error;
    kw_match_fn kw_match;

//     int32_t timeout;
//     hgobj timer;
    uint64_t *ptxMsgs;
    uint64_t *prxMsgs;
    uint64_t txMsgsec;
    uint64_t rxMsgsec;
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

//     priv->timer = gobj_create(gobj_name(gobj), GCLASS_TIMER, 0, gobj);
    priv->ptxMsgs = gobj_danger_attr_ptr(gobj, "txMsgs");
    priv->prxMsgs = gobj_danger_attr_ptr(gobj, "rxMsgs");

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
    SET_PRIV(tranger,                   gobj_read_pointer_attr)
    SET_PRIV(treedb_name,               gobj_read_str_attr)
    SET_PRIV(treedb_schema,             gobj_read_json_attr)
    SET_PRIV(exit_on_error,             gobj_read_int32_attr)
    SET_PRIV(kw_match,                  gobj_read_pointer_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(tranger,             gobj_read_pointer_attr)
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

    if(!priv->tranger) {
        log_critical(priv->exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "tranger NULL",
            NULL
        );
    }
    if(!priv->treedb_schema) {
        log_critical(priv->exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "treedb_schema NULL",
            NULL
        );
    }
    if(empty_string(priv->treedb_name)) {
        log_critical(priv->exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "treedb_name name EMPTY",
            NULL
        );
        return -1;
    }

    treedb_open_db( // Return IS NOT YOURS!
        priv->tranger,
        priv->treedb_name,
        json_incref(priv->treedb_schema),  // owned
        "persistent"
    );

    treedb_set_callback(
        priv->tranger,
        priv->treedb_name,
        treedb_callback,
        gobj
    );


    /*
     *  Periodic timer for tasks
     */
//     gobj_start(priv->timer);
//     set_timeout_periodic(priv->timer, priv->timeout); // La verdadera

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

//     clear_timeout(priv->timer);
//     gobj_stop(priv->timer);

    treedb_close_db(priv->tranger, priv->treedb_name);

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
        priv->treedb_name,
        topic_name,
        kw, // owned
        options
    );
    return record;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_save_node(
    hgobj gobj,
    json_t *node
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_save_node(priv->tranger, node);
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
        priv->treedb_name,
        topic_name,
        kw, // owned
        options
    );
    return record;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_delete_node(
    hgobj gobj,
    const char *topic_name,
    json_t *kw,    // owned
    const char *options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_delete_node(
        priv->tranger,
        priv->treedb_name,
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
        priv->treedb_name,
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
        priv->treedb_name,
        parent_ref,
        child_ref
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_get_node(
    hgobj gobj,
    const char *topic_name,
    const char *id,
    json_t *jn_options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_get_node(
        priv->tranger,
        priv->treedb_name,
        topic_name,
        id,
        jn_options
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
        priv->treedb_name,
        topic_name,
        jn_filter,
        jn_options,
        gobj_read_pointer_attr(gobj, "kw_match")
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_node_instances(
    hgobj gobj,
    const char *topic_name,
    const char *pkey2,
    json_t *jn_filter,
    json_t *jn_options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_node_instances( // Return MUST be decref
        priv->tranger,
        priv->treedb_name,
        topic_name,
        pkey2,
        jn_filter,  // owned
        jn_options, // owned, "collapsed"
        gobj_read_pointer_attr(gobj, "kw_match")
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_shoot_snap(hgobj gobj, const char *tag)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_shoot_snap(
        priv->tranger,
        priv->treedb_name,
        tag
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_activate_snap(hgobj gobj, const char *tag)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_activate_snap(
        priv->tranger,
        priv->treedb_name,
        tag
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_list_snaps(hgobj gobj, json_t *filter)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_list_snaps(
        priv->tranger,
        priv->treedb_name,
        filter
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_treedbs(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_list_treedb(
        priv->tranger
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_treedb_topics(hgobj gobj, const char *treedb_name, const char *options)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_topics(
        priv->tranger,
        empty_string(treedb_name)?priv->treedb_name:treedb_name,
        options
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_topic_desc(hgobj gobj, const char *topic_name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!empty_string(topic_name)) {
        return tranger_topic_desc(priv->tranger, topic_name);
    } else {
        json_t *topics_list = treedb_topics( //Return a list with topic names of the treedb
            priv->tranger,
            priv->treedb_name,
            ""
        );
        json_t *jn_topics_desc = json_object();
        int idx; json_t *jn_topic_name;
        json_array_foreach(topics_list, idx, jn_topic_name) {
            topic_name = json_string_value(jn_topic_name);
            if(strcmp(topic_name, "__schema_version__")==0) {
                continue;
            }
            json_object_set_new(
                jn_topics_desc,
                topic_name,
                tranger_topic_desc(priv->tranger, topic_name)
            );
        }
        json_decref(topics_list);
        return jn_topics_desc;
    }
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_topic_links(hgobj gobj, const char *treedb_name, const char *topic_name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!empty_string(topic_name)) {
        return treedb_get_topic_links(priv->tranger, treedb_name, topic_name);
    }

    // All topics
    json_t *topics = treedb_topics(
        priv->tranger,
        priv->treedb_name,
        0
    );

    json_t *links = json_object();
    int idx; json_t *jn_topic_name;
    json_array_foreach(topics, idx, jn_topic_name) {
        const char *topic_name = json_string_value(jn_topic_name);
        json_object_set_new(
            links,
            topic_name,
            treedb_get_topic_links(priv->tranger, treedb_name, topic_name)
        );
    }
    JSON_DECREF(topics);
    return links;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_topic_hooks(hgobj gobj, const char *treedb_name, const char *topic_name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!empty_string(topic_name)) {
        return treedb_get_topic_hooks(priv->tranger, treedb_name, topic_name);
    }

    // All topics
    json_t *topics = treedb_topics(
        priv->tranger,
        priv->treedb_name,
        0
    );

    json_t *hooks = json_object();
    int idx; json_t *jn_topic_name;
    json_array_foreach(topics, idx, jn_topic_name) {
        const char *topic_name = json_string_value(jn_topic_name);
        json_object_set_new(
            hooks,
            topic_name,
            treedb_get_topic_hooks(priv->tranger, treedb_name, topic_name)
        );
    }

    JSON_DECREF(topics);
    return hooks;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_node_parents(
    hgobj gobj,
    const char *topic_name,
    const char *id,
    const char *link,
    json_t *options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *node = treedb_get_node(
        priv->tranger,
        priv->treedb_name,
        topic_name,
        id,
        0  // jn_options, must be NOT "collapsed"
    );
    if(!node) {
        // silence
        return 0;
    }

    /*
     *  Return a list of parent nodes pointed by the link (fkey)
     */
    if(!empty_string(link)) {
        return treedb_list_parents( // Return MUST be decref
            priv->tranger,
            link, // must be a fkey field
            node, // not owned
            options
        );
    }

    /*
     *  If no link return all links
     */
    json_t *parents = json_array();
    json_t *links = treedb_get_topic_links(priv->tranger, priv->treedb_name, topic_name);
    int idx; json_t *jn_link;
    json_array_foreach(links, idx, jn_link) {
        json_t *parents_ = treedb_list_parents( // Return MUST be decref
            priv->tranger,
            json_string_value(jn_link), // must be a fkey field
            node, // not owned
            json_incref(options)
        );
        json_array_extend(parents, parents_);
        JSON_DECREF(parents_);
    }
    JSON_DECREF(links);
    JSON_DECREF(options);

    return parents;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_node_childs(
    hgobj gobj,
    const char *topic_name,
    const char *id,
    const char *hook,
    json_t *options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *node = treedb_get_node(
        priv->tranger,
        priv->treedb_name,
        topic_name,
        id,
        0  // jn_options, must be NOT "collapsed"
    );
    if(!node) {
        // silence
        return 0;
    }

    /*
     *  Return a list of child nodes of the hook
     */
    if(!empty_string(hook)) {
        return treedb_list_childs( // Return MUST be decref
            priv->tranger,
            hook, // must be a fkey field
            node, // not owned
            options
        );
    }

    /*
     *  If no hook return all hooks
     */
    json_t *childs = json_array();
    json_t *hooks = treedb_get_topic_hooks(priv->tranger, priv->treedb_name, topic_name);
    int idx; json_t *jn_hook;
    json_array_foreach(hooks, idx, jn_hook) {
        json_t *childs_ = treedb_list_childs( // Return MUST be decref
            priv->tranger,
            json_string_value(jn_hook), // must be a fkey field
            node, // not owned
            json_incref(options)
        );
        json_array_extend(childs, childs_);
        JSON_DECREF(childs_);
    }
    JSON_DECREF(hooks);
    JSON_DECREF(options);

    return childs;
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
PRIVATE json_t *cmd_print_tranger(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    BOOL expanded = kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);
    int lists_limit = kw_get_int(kw, "lists_limit", 100, KW_WILD_NUMBER);
    int dicts_limit = kw_get_int(kw, "dicts_limit", 100, KW_WILD_NUMBER);
    const char *path = kw_get_str(kw, "path", "", 0);

    json_t *value = priv->tranger;

    if(!empty_string(path)) {
        value = kw_find_path(value, path, FALSE);
        if(!value) {
            return msg_iev_build_webix(gobj,
                -1,
                json_local_sprintf("Path not found: '%s'", path),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(expanded) {
        if(!lists_limit && !dicts_limit) {
            kw_incref(value); // All
        } else {
            value = kw_collapse(value, lists_limit, dicts_limit);
        }
    } else {
        value = kw_collapse(value, 0, 0);
    }

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        value,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_create_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *content64 = kw_get_str(kw, "content64", "", 0);
    const char *content = kw_get_str(kw, "content", "", 0);
    const char *options = kw_get_str(kw, "options", "", 0);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }

    /*----------------------------------------*
     *  Get content
     *  Priority: content64, content, record
     *----------------------------------------*/
    json_t *jn_content = 0;
    if(!empty_string(content64)) {
        /*
         *  Get content in base64 and decode
         */
        GBUFFER *gbuf_content = gbuf_decodebase64string(content64);
        jn_content = legalstring2json(gbuf_cur_rd_pointer(gbuf_content), TRUE);
        GBUF_DECREF(gbuf_content);
        if(!jn_content) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("Can't decode json content64"),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(!jn_content) {
        if(!empty_string(content)) {
            jn_content = legalstring2json(content, TRUE);
            if(!jn_content) {
                return msg_iev_build_webix(
                    gobj,
                    -1,
                    json_local_sprintf("Can't decode json content"),
                    0,
                    0,
                    kw  // owned
                );
            }
        }
    }

    if(!jn_content) {
        jn_content = kw_incref(kw_get_dict(kw, "record", 0, 0));
    }

    if(!jn_content) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What content?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *node = gobj_create_node(
        gobj,
        topic_name,
        jn_content, // owned
        options // options  "permissive"
    );
    JSON_INCREF(node);
    return msg_iev_build_webix(gobj,
        node?0:-1,
        json_local_sprintf(node?"Node created!":log_last_message()),
        gobj_topic_desc(gobj, topic_name),
        node,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_update_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *content64 = kw_get_str(kw, "content64", "", 0);
    const char *content = kw_get_str(kw, "content", "", 0);
    const char *options = kw_get_str(kw, "options", "", 0);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }

    /*----------------------------------------*
     *  Get content
     *  Priority: content64, content, record
     *----------------------------------------*/
    json_t *jn_content = 0;
    if(!empty_string(content64)) {
        /*
         *  Get content in base64 and decode
         */
        GBUFFER *gbuf_content = gbuf_decodebase64string(content64);
        jn_content = legalstring2json(gbuf_cur_rd_pointer(gbuf_content), TRUE);
        GBUF_DECREF(gbuf_content);
        if(!jn_content) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("Can't decode json content64"),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(!jn_content) {
        if(!empty_string(content)) {
            jn_content = legalstring2json(content, TRUE);
            if(!jn_content) {
                return msg_iev_build_webix(
                    gobj,
                    -1,
                    json_local_sprintf("Can't decode json content"),
                    0,
                    0,
                    kw  // owned
                );
            }
        }
    }

    if(!jn_content) {
        jn_content = kw_incref(kw_get_dict(kw, "record", 0, 0));
    }

    if(!jn_content) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What content?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *node = gobj_update_node(
        gobj,
        topic_name,
        jn_content, // owned
        options // "permissive"
    );
    JSON_INCREF(node);
    return msg_iev_build_webix(gobj,
        node?0:-1,
        json_local_sprintf(node?"Node update!":log_last_message()),
        gobj_topic_desc(gobj, topic_name),
        node,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_delete_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);
    BOOL force = kw_get_bool(kw, "force", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *jn_filter = 0;
    if(!empty_string(filter)) {
        jn_filter = legalstring2json(filter, TRUE);
        if(!jn_filter) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(!jn_filter) {
        jn_filter = kw_incref(kw_get_dict(kw, "record", 0, 0));
    }

    if(!kw_has_key(jn_filter, "id")) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("field 'id' is required to delete nodes"),
            0,
            0,
            kw  // owned
        );
    }

    /*
     *  Get a iter of matched resources.
     */
    json_t *iter = gobj_list_nodes(
        gobj,
        topic_name,
        jn_filter,  // filter
        0
    );

    if(json_array_size(iter)==0) {
        JSON_DECREF(iter);
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Select one node please"),
            0,
            0,
            kw  // owned
        );
    }

    /*
     *  Delete
     *  HACK if there are several nodes to delete then force is true
     *      in order to avoid incongruences in links
     */
    if(json_array_size(iter)>1) {
        force = TRUE;
    }
    json_t *jn_data = json_array();
    int idx; json_t *node;
    json_array_foreach(iter, idx, node) {
        const char *id = kw_get_str(node, "id", "", KW_REQUIRED);
        JSON_INCREF(node);
        if(gobj_delete_node(gobj, topic_name, node, force?"force":"")<0) {
            JSON_DECREF(iter);
            JSON_DECREF(jn_data);
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf(log_last_message()),
                0,
                0,
                kw  // owned
            );
        }
        json_array_append_new(jn_data, json_string(id));
    }

    JSON_DECREF(iter);

    return msg_iev_build_webix(
        gobj,
        0,
        json_local_sprintf("%d nodes deleted", idx),
        gobj_topic_desc(gobj, topic_name),
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_link_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *parent = kw_get_str(kw, "parent", "", 0);
    const char *child = kw_get_str(kw, "child", "", 0);

    if(empty_string(parent)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What parent node?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(child)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What child node?"),
            0,
            0,
            kw  // owned
        );
    }

    int result = gobj_link_nodes2(
        gobj,
        parent,
        child
    );

    return msg_iev_build_webix(gobj,
        result,
        result<0?json_local_sprintf(log_last_message()):json_local_sprintf("Nodes linked!"),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_unlink_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *parent = kw_get_str(kw, "parent", "", 0);
    const char *child = kw_get_str(kw, "child", "", 0);

    if(empty_string(parent)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What parent node?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(child)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What child node?"),
            0,
            0,
            kw  // owned
        );
    }

    int result = gobj_unlink_nodes2(
        gobj,
        parent,
        child
    );

    return msg_iev_build_webix(gobj,
        result,
        result<0?json_local_sprintf(log_last_message()):json_local_sprintf("Nodes unlinked!"),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_treedbs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    json_t *treedbs = gobj_treedbs(gobj);

    return msg_iev_build_webix(gobj,
        treedbs?0:-1,
        0,
        0,
        treedbs,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *treedb_name = kw_get_str(kw, "treedb_name", "", 0);
    const char *options = kw_get_str(kw, "options", "", 0);

    json_t *topics = gobj_treedb_topics(gobj, treedb_name, options);

    return msg_iev_build_webix(gobj,
        topics?0:-1,
        topics?0:json_string(log_last_message()),
        0,
        topics,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    json_t *desc = gobj_topic_desc(gobj, topic_name);

    return msg_iev_build_webix(gobj,
        desc?0:-1,
        desc?0:json_string(log_last_message()),
        0,
        desc,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    BOOL set = kw_get_bool(kw, "set", 0, KW_WILD_NUMBER);

    if(set) {
        treedb_set_trace(TRUE);
    } else {
        treedb_set_trace(FALSE);
    }
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("Set trace %s", set?"on":"false"),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_links(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);

    json_t *links = gobj_topic_links(gobj, priv->treedb_name, topic_name);

    return msg_iev_build_webix(gobj,
        links?0:-1,
        links?0:json_string(log_last_message()),
        0,
        links,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_hooks(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);

    json_t *hooks = gobj_topic_hooks(gobj, priv->treedb_name, topic_name);

    return msg_iev_build_webix(gobj,
        hooks?0:-1,
        hooks?0:json_string(log_last_message()),
        0,
        hooks,
        kw  // owned
    );

}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_parents(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *node_id = kw_get_str(kw, "node_id", "", 0);
    const char *link = kw_get_str(kw, "link", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(node_id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What node id?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *parents = gobj_node_parents( // Return MUST be decref
        gobj,
        topic_name,
        node_id,
        link,
        json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
    );

    return msg_iev_build_webix(
        gobj,
        parents?0:-1,
        parents?0:json_string(log_last_message()),
        0,
        parents,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_childs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *node_id = kw_get_str(kw, "node_id", "", 0);
    const char *hook = kw_get_str(kw, "hook", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(node_id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What node id?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *childs = gobj_node_childs( // Return MUST be decref
        gobj,
        topic_name,
        node_id,
        hook,
        json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
    );

    return msg_iev_build_webix(
        gobj,
        childs?0:-1,
        childs?0:json_string(log_last_message()),
        0,
        childs,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_list_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *jn_filter = 0;
    if(!empty_string(filter)) {
        jn_filter = legalstring2json(filter, TRUE);
        if(!jn_filter) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *nodes = gobj_list_nodes(
        gobj,
        topic_name,
        jn_filter,  // owned
        json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
    );

    return msg_iev_build_webix(
        gobj,
        nodes?0:-1,
        nodes?
            json_local_sprintf("%d nodes", json_array_size(nodes)):
            json_string(log_last_message()),
        nodes?tranger_topic_desc(priv->tranger, topic_name):0,
        nodes,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_get_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *node_id = kw_get_str(kw, "node_id", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(node_id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What node id?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *node = gobj_get_node(
        gobj,
        topic_name,
        node_id,
        json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
    );

    return msg_iev_build_webix(gobj,
        node?0:-1,
        node?0:json_local_sprintf("Node not found"),
        node?tranger_topic_desc(priv->tranger, topic_name):0,
        kw_incref(node),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_node_instances(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *node_id = kw_get_str(kw, "node_id", "", 0);
    const char *pkey2 = kw_get_str(kw, "pkey2", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    json_t *jn_filter = 0;
    if(!empty_string(filter)) {
        jn_filter = legalstring2json(filter, TRUE);
        if(!jn_filter) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }
    if(!empty_string(node_id)) {
        if(!jn_filter) {
            jn_filter = json_pack("{s:s}", "id", node_id);
        } else {
            json_object_set_new(jn_filter, "id", json_string(node_id));
        }
    }

    json_t *instances = gobj_node_instances(
        gobj,
        topic_name,
        pkey2,
        jn_filter,  // owned
        json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
    );

    return msg_iev_build_webix(
        gobj,
        0,
        instances?
            json_local_sprintf("%d instances", json_array_size(instances)):
            json_string(log_last_message()),
        instances?tranger_topic_desc(priv->tranger, topic_name):0,
        instances,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_touch_instance(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *node_id = kw_get_str(kw, "node_id", "", 0);
    const char *pkey2 = kw_get_str(kw, "pkey2", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    json_t *jn_filter = 0;
    if(!empty_string(filter)) {
        jn_filter = legalstring2json(filter, TRUE);
        if(!jn_filter) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }
    if(!empty_string(node_id)) {
        if(!jn_filter) {
            jn_filter = json_pack("{s:s}", "id", node_id);
        } else {
            json_object_set_new(jn_filter, "id", json_string(node_id));
        }
    }

    json_t *instances = gobj_node_instances(
        gobj,
        topic_name,
        pkey2,
        jn_filter,  // owned
        0
    );

    /*
     *  This is a danger operation, only one instance please
     */
    if(json_array_size(instances)!=1) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Select only one instance please"),
            tranger_topic_desc(priv->tranger, topic_name),
            instances,
            kw  // owned
        );
    }
    int idx; json_t *instance;
    json_array_foreach(instances, idx, instance) {
        gobj_save_node(gobj, instance);
    }

    return msg_iev_build_webix(
        gobj,
        0,
        json_local_sprintf("%d instances touched", json_array_size(instances)),
        tranger_topic_desc(priv->tranger, topic_name),
        instances,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_node_pkey2s(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *pkey2s = treedb_topic_pkey2s(
        priv->tranger,
        topic_name
    );

    return msg_iev_build_webix(
        gobj,
        pkey2s?0:-1,
        pkey2s?
            json_local_sprintf("%d pkey2s", json_object_size(pkey2s)):
            json_string(log_last_message()),
        0,
        pkey2s,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_snap_content(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }

    int snap_tag = kw_get_int(kw, "snap_tag", 0, KW_WILD_NUMBER);
    if(!snap_tag) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What snap_tag?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *jn_data = json_array();

    json_t *jn_filter = json_pack("{s:b, s:i}",
        "backward", 1,
        "user_flag", snap_tag
    );
    json_t *jn_list = json_pack("{s:s, s:o}",
        "topic_name", topic_name,
        "match_cond", jn_filter
    );
    json_t *list = tranger_open_list(
        priv->tranger,
        jn_list // owned
    );
    json_array_extend(jn_data, kw_get_list(list, "data", 0, KW_REQUIRED));
    tranger_close_list(priv->tranger, list);

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        tranger_topic_desc(priv->tranger, topic_name),
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_list_snaps(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *jn_data = gobj_list_snaps(
        gobj,
        kw_incref(kw)
    );

    return msg_iev_build_webix(gobj,
        0,
        0,
        tranger_topic_desc(priv->tranger, "__snaps__"),
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_shoot_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *name = kw_get_str(kw, "name", 0, 0);
    if(empty_string(name)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_local_sprintf(
                "What snap name?"
            ),
            0,
            0,
            kw  // owned
        );
    }
    int ret = gobj_shoot_snap(
        gobj,
        name
    );
    json_t *jn_data = 0;
    if(ret == 0) {
        jn_data = gobj_list_snaps(
            gobj,
            json_pack("{s:s}", "name", name)
        );
    }

    return msg_iev_build_webix(gobj,
        ret,
        ret==0?json_sprintf("Snap '%s' shooted", name):json_string(log_last_message()),
        ret==0?tranger_topic_desc(priv->tranger, "__snaps__"):0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_activate_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *name = kw_get_str(kw, "name", 0, 0);
    if(empty_string(name)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_local_sprintf(
                "What snap name?"
            ),
            0,
            0,
            kw  // owned
        );
    }
    int ret = gobj_activate_snap(
        gobj,
        name
    );
    if(ret>=0) {
        // TODO publish changes?
    }
    return msg_iev_build_webix(gobj,
        ret,
        ret>=0?json_sprintf("Snap activated: '%s', check new yuno pids", name):json_string(log_last_message()),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_deactivate_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    int ret = gobj_activate_snap(
        gobj,
        "__clear__"
    );
    return msg_iev_build_webix(gobj,
        ret,
        ret==0?json_sprintf("Snap deactivated"):json_string(log_last_message()),
        0,
        0,
        kw  // owned
    );
}




            /***************************
             *      Local Methods
             ***************************/




            /***************************
             *      Private Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int treedb_callback(
    void *user_data,
    json_t *tranger,
    const char *treedb_name,
    const char *topic_name,
    const char *operation,  // "EV_TREEDB_NODE_UPDATED", "EV_TREEDB_NODE_DELETED"
    json_t *node            // owned
)
{
    json_t *kw = json_pack("{s:s, s:s, s:o}",
        "treedb_name", treedb_name,
        "topic_name", topic_name,
        "node", node
    );

    return gobj_publish_event(user_data, operation, kw);
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    uint64_t maxtxMsgsec = gobj_read_uint64_attr(gobj, "maxtxMsgsec");
    uint64_t maxrxMsgsec = gobj_read_uint64_attr(gobj, "maxrxMsgsec");
    if(priv->txMsgsec > maxtxMsgsec) {
        gobj_write_uint64_attr(gobj, "maxtxMsgsec", priv->txMsgsec);
    }
    if(priv->rxMsgsec > maxrxMsgsec) {
        gobj_write_uint64_attr(gobj, "maxrxMsgsec", priv->rxMsgsec);
    }

    gobj_write_uint64_attr(gobj, "txMsgsec", priv->txMsgsec);
    gobj_write_uint64_attr(gobj, "rxMsgsec", priv->rxMsgsec);

    priv->rxMsgsec = 0;
    priv->txMsgsec = 0;

    KW_DECREF(kw);
    return 0;
}




/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // bottom input
    {"EV_TIMEOUT",              0,  0,  0},
    {"EV_STOPPED",              0,  0,  0},
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_TREEDB_NODE_UPDATED",  EVF_PUBLIC_EVENT,   0,  0},
    {"EV_TREEDB_NODE_DELETED",  EVF_PUBLIC_EVENT,   0,  0},
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_TIMEOUT",                  ac_timeout,                 0},
    {"EV_STOPPED",                  0,                          0},
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
    GCLASS_NODE_NAME,
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
        mt_shoot_snap,
        mt_activate_snap,
        mt_list_snaps,
        mt_treedbs,
        mt_treedb_topics,
        mt_topic_desc,
        mt_topic_links,
        mt_topic_hooks,
        mt_node_parents,
        mt_node_childs,
        mt_node_instances,
        mt_save_node,
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
    command_table,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_node(void)
{
    return &_gclass;
}
