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
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_print_tranger(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_create_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_update_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_delete_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_link_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_unlink_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_hooks(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_links(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_parents(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_childs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_list_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE sdata_desc_t pm_help[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help."),
SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_create_node[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Node content"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"permissive\""),
SDATA_END()
};
PRIVATE sdata_desc_t pm_update_node[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Node content"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"create\", \"clean\""),
SDATA_END()
};
PRIVATE sdata_desc_t pm_delete_node[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
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
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: 'dict'"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_desc[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_hooks[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Full desc of hooks"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_links[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Full desc of links"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_parents[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "id",           0,              0,          "Node id"),
SDATAPM (ASN_OCTET_STR, "link",         0,              0,          "Link port to parents"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Parent nodes expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_childs[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "id",           0,              0,          "Node id"),
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
SDATAPM (ASN_OCTET_STR, "id",           0,              0,          "Get node by his id"),
SDATAPM (ASN_OCTET_STR, "ref",          0,              0,          "Get node by ref"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Tree expanded"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias-------items-------json_fn---------description--*/
SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,    cmd_help,       "Command's help"),

/*-CMD---type-----------name------------al--items-----------json_fn-------------description--*/
SDATACM (ASN_SCHEMA,    "print-tranger",0,  0,              cmd_print_tranger,  "Print tranger"),
SDATACM (ASN_SCHEMA,    "create-node",  0,  pm_create_node, cmd_create_node,    "Create node"),
SDATACM (ASN_SCHEMA,    "update-node",  0,  pm_update_node, cmd_update_node,    "Update node"),
SDATACM (ASN_SCHEMA,    "delete-node",  0,  pm_delete_node, cmd_delete_node,    "Delete node"),
SDATACM (ASN_SCHEMA,    "link-nodes",   0,  pm_link_nodes,  cmd_link_nodes,     "Link nodes"),
SDATACM (ASN_SCHEMA,    "unlink-nodes", 0,  pm_link_nodes,  cmd_unlink_nodes,   "Unlink nodes"),
SDATACM (ASN_SCHEMA,    "trace",        0,  pm_trace,       cmd_trace,          "Set trace"),
SDATACM (ASN_SCHEMA,    "topics",       0,  pm_topics,      cmd_topics,         "List topics"),
SDATACM (ASN_SCHEMA,    "desc",         0,  pm_desc,        cmd_desc,           "Schema of topic or full"),
SDATACM (ASN_SCHEMA,    "hooks",        0,  pm_hooks,       cmd_hooks,          "Hooks of node"),
SDATACM (ASN_SCHEMA,    "links",        0,  pm_links,       cmd_links,          "Links of node"),
SDATACM (ASN_SCHEMA,    "parents",      0,  pm_parents,     cmd_parents,        "Parents of node"),
SDATACM (ASN_SCHEMA,    "childs",       0,  pm_childs,      cmd_childs,         "Childs of node"),
SDATACM (ASN_SCHEMA,    "list-nodes",   0,  pm_list_nodes,  cmd_list_nodes,     "List nodes"),
SDATACM (ASN_SCHEMA,    "find-node",    0,  pm_get_node,    cmd_get_node,       "Get node by id or ref"),
SDATA_END()
};


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

    json_t *tranger = kw_incref(priv->tranger);
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        tranger,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_create_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *content = kw_get_str(kw, "content", "", 0);
    const char *content64 = kw_get_str(kw, "content64", "", 0);
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

    /*----------------------------------*
     *  Get content
     *  Priority: conten64, content
     *----------------------------------*/
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
        0,
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
    const char *content = kw_get_str(kw, "content", "", 0);
    const char *content64 = kw_get_str(kw, "content64", "", 0);
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

    /*----------------------------------*
     *  Get content
     *  Priority: conten64, content
     *----------------------------------*/
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
        0,
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
     */
    json_t *jn_data = json_array();
    int idx; json_t *node;
    json_array_foreach(iter, idx, node) {
        const char *id = kw_get_str(node, "id", "", KW_REQUIRED);

        if(gobj_delete_node(gobj, topic_name, node, force?"force":"")<0) {
            JSON_DECREF(iter);
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("Cannot delete the node %s^%s", topic_name, id),
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
        0,
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
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *options = kw_get_str(kw, "options", "", 0);

    json_t *topics = treedb_topics(
        priv->tranger,
        kw_get_str(priv->tranger, "database", "", KW_REQUIRED), // treedb_name
        options
    );

    return msg_iev_build_webix(gobj,
        0,
        0,
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
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(!empty_string(topic_name)) {
        json_t *desc = tranger_list_topic_desc(priv->tranger, topic_name);
        return msg_iev_build_webix(
            gobj,
            0,
            0,
            0,
            desc,
            kw  // owned
        );
    } else {
        json_t *schema = kw_get_dict(priv->tranger, "topics", 0, KW_REQUIRED);
        return msg_iev_build_webix(
            gobj,
            0,
            0,
            0,
            kw_incref(schema),
            kw  // owned
        );
    }
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

    json_t *links = treedb_get_topic_links(priv->tranger, topic_name);

    return msg_iev_build_webix(
        gobj,
        0,
        0,
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

    json_t *hooks = treedb_get_topic_hooks(priv->tranger, topic_name);

    return msg_iev_build_webix(
        gobj,
        0,
        0,
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
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *id = kw_get_str(kw, "id", "", 0);
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
    if(empty_string(id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What node id?"),
            0,
            0,
            kw  // owned
        );
    }
    /*
     *  If no link return all links
     */

    /*
     *  Return a list of parent nodes pointed by the link (fkey)
     */
    json_t *node;
    json_t *nodes = treedb_list_parents( // Return MUST be decref
        priv->tranger,
        link, // must be a fkey field
        node, // not owned
        json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
    );

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        nodes,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_childs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *id = kw_get_str(kw, "id", "", 0);
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
    if(empty_string(id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What node id?"),
            0,
            0,
            kw  // owned
        );
    }
    /*
     *  If no hook return all hooks
     */

    /*
     *  Return a list of child nodes of the hook (WARNING ONLY first level)
     */
    json_t *node;
    json_t *nodes = treedb_list_childs(
        priv->tranger,
        hook,
        node, // not owned
        json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
    );

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        nodes,
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
        0,
        json_local_sprintf("%d nodes", json_array_size(nodes)),
        tranger_list_topic_desc(priv->tranger, topic_name),
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
    const char *id = kw_get_str(kw, "id", "", 0);
    const char *ref = kw_get_str(kw, "ref", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(!empty_string(ref)) {
        // TODO get topic_name, id, hook, link
    } else {
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
        if(!empty_string(id)) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("What node id?"),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *node = gobj_get_node(
        gobj,
        topic_name,
        id
    );

    return msg_iev_build_webix(gobj,
        0,
        json_local_sprintf("Node found!"),
        tranger_list_topic_desc(priv->tranger, topic_name),
        kw_incref(node),
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
