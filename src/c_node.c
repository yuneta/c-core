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
    const char *operation,  // "EV_TREEDB_NODE_UPDATED",
                            // "EV_TREEDB_NODE_UPDATED",
                            // "EV_TREEDB_NODE_DELETED"
    json_t *node            // owned
);

PRIVATE json_t *fetch_node(  // WARNING Return is NOT YOURS, pure node
    hgobj gobj,
    const char *topic_name,
    json_t *kw  // NOT owned, 'id' and pkey2s fields are used to find the node
);

PRIVATE int export_treedb(
    hgobj gobj,
    const char *path,
    BOOL with_metadata,
    BOOL without_rowid,
    hgobj src
);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_create_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_update_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_delete_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_link_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_unlink_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_treedbs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_jtree(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_hooks(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_links(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_parents(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_childs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_list_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_node_instances(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_node_pkey2s(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_list_snaps(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_snap_content(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_shoot_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_activate_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_deactivate_snap(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_import_db(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_export_db(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t* cmd_system_topic_schema(hgobj gobj, const char* cmd, json_t* kw, hgobj src);

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

PRIVATE sdata_desc_t pm_jtree[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_OCTET_STR, "hook",         0,              0,          "Hook to build the tree"),
SDATAPM (ASN_OCTET_STR, "rename_hook",  0,              0,          "Rename the hook field in the response"),
SDATAPM (ASN_JSON,      "filter",       0,              0,          "Filter to childs"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: refs, hook_refs, fkey_refs, only_id, hook_only_id, fkey_only_id, list_dict, hook_list_dict, fkey_list_dict, size, hook_size"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_create_node[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
SDATAPM (ASN_JSON,      "record",       0,              0,          "Node content in json"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: refs, hook_refs, fkey_refs, only_id, hook_only_id, fkey_only_id, list_dict, hook_list_dict, fkey_list_dict, size, hook_size"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_update_node[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Node content in base64"),
SDATAPM (ASN_JSON,      "record",       0,              0,          "Node content in json"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: create, autolink, volatil, refs, hook_refs, fkey_refs, only_id, hook_only_id, fkey_only_id, list_dict, hook_list_dict, fkey_list_dict, size, hook_size"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_delete_node[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_JSON,      "record",       0,              0,          "Node content in json"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: 'force'"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_link_nodes[] = {
SDATAPM (ASN_OCTET_STR, "parent_ref",   0,              0,          "Parent node ref (parent_topic_name^parent_id^hook_name)"),
SDATAPM (ASN_OCTET_STR, "child_ref",    0,              0,          "Child node ref (child_topic_name^child_id)"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: create, autolink, volatil, refs, hook_refs, fkey_refs, only_id, hook_only_id, fkey_only_id, list_dict, hook_list_dict, fkey_list_dict, size, hook_size"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_trace[] = {
SDATAPM (ASN_BOOLEAN,   "set",          0,              0,          "Trace: set 1 o 0"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_topics[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "treedb_name",  0,              0,          "Treedb name"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: 'dict'"),
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
SDATAPM (ASN_OCTET_STR, "link",         0,              0,          "Link port (fkey) to parents"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: refs, fkey_refs, only_id, fkey_only_id, list_dict, fkey_list_dict"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_childs[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_OCTET_STR, "hook",         0,              0,          "Hook port to childs"),
SDATAPM (ASN_JSON,      "filter",       0,              0,          "Filter to childs"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: recursive, refs, hook_refs, only_id, hook_only_id, list_dict, fkey_list_dict, size, hook_size"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_list_nodes[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_JSON,      "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: refs, hook_refs, fkey_refs, only_id, hook_only_id, fkey_only_id, list_dict, hook_list_dict, fkey_list_dict, size, hook_size"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_get_node[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: refs, hook_refs, fkey_refs, only_id, hook_only_id, fkey_only_id, list_dict, hook_list_dict, fkey_list_dict, size, hook_size"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_node_instances[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "node_id",      0,              0,          "Node id"),
SDATAPM (ASN_OCTET_STR, "pkey2",        0,              0,          "PKey2 field"),
SDATAPM (ASN_JSON,      "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_JSON,      "options",      0,              0,          "Options: only_id, list_dict, size"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_node_pkey2s[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_snap_content[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "snap_id",      0,              0,          "Snap id"),
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_shoot_snap[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "name",         0,              0,          "Snap name"),
SDATAPM (ASN_OCTET_STR, "description",  0,              0,          "Snap description"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_activate_snap[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "name",         0,              0,          "Snap name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_import_db[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Content in base64"),
SDATAPM (ASN_OCTET_STR, "if-resource-exists", 0,        0,          "abort, skip, overwrite"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_export_db[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "filename",     0,              0,          "Filename to save db"),
SDATAPM (ASN_BOOLEAN,   "overwrite",    0,              0,          "Overwrite the file if it exits"),
SDATAPM (ASN_BOOLEAN,   "with_metadata",0,              0,          "Write metadata"),
SDATAPM (ASN_BOOLEAN,   "without_rowid",0,              "0",        "Without id in records with rowid id"),
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
SDATACM (ASN_SCHEMA,    "authzs",           0,          pm_authzs,  cmd_authzs,     "Authorization's help"),

/*-CMD2---type----------name------------flag------------ali-items-----------json_fn-------------description--*/
SDATACM2 (ASN_SCHEMA,   "treedbs",      SDF_AUTHZ_X,    0,  0,              cmd_treedbs,        "List treedb's"),
SDATACM2 (ASN_SCHEMA,   "topics",       SDF_AUTHZ_X,    0,  pm_topics,      cmd_topics,         "List topics"),
SDATACM2 (ASN_SCHEMA,   "jtree",        SDF_AUTHZ_X,    0,  pm_jtree,       cmd_jtree,          "List hierarchical tree (topic with self-link). Webix option return dict-list else return list of dicts. Always with __path__ "),
SDATACM2 (ASN_SCHEMA,   "create-node",  SDF_AUTHZ_X,    a_create, pm_create_node, cmd_create_node, "Create node"),
SDATACM2 (ASN_SCHEMA,   "update-node",  SDF_AUTHZ_X,    a_update, pm_update_node, cmd_update_node, "Update node"),
SDATACM2 (ASN_SCHEMA,   "delete-node",  SDF_AUTHZ_X,    a_delete, pm_delete_node, cmd_delete_node, "Delete node"),
SDATACM2 (ASN_SCHEMA,   "nodes",        SDF_AUTHZ_X,    a_nodes, pm_list_nodes, cmd_list_nodes,  "List nodes"),
SDATACM2 (ASN_SCHEMA,   "node",         SDF_AUTHZ_X,    a_node, pm_get_node, cmd_get_node,       "Get node by id"),
SDATACM2 (ASN_SCHEMA,   "instances",    SDF_AUTHZ_X,    0,  pm_node_instances,cmd_node_instances,"List node's instances"),
SDATACM2 (ASN_SCHEMA,   "link-nodes",   SDF_AUTHZ_X,    0,  pm_link_nodes,  cmd_link_nodes,     "Link nodes"),
SDATACM2 (ASN_SCHEMA,   "unlink-nodes", SDF_AUTHZ_X,    0,  pm_link_nodes,  cmd_unlink_nodes,   "Unlink nodes"),
SDATACM2 (ASN_SCHEMA,   "hooks",        SDF_AUTHZ_X,    0,  pm_hooks,       cmd_hooks,          "Hooks of node"),
SDATACM2 (ASN_SCHEMA,   "links",        SDF_AUTHZ_X,    0,  pm_links,       cmd_links,          "Links of node"),
SDATACM2 (ASN_SCHEMA,   "parents",      SDF_AUTHZ_X,    0,  pm_parents,     cmd_parents,        "Parent Refs of node"),
SDATACM2 (ASN_SCHEMA,   "childs",       SDF_AUTHZ_X,    0,  pm_childs,      cmd_childs,         "Childs of node"),
SDATACM2 (ASN_SCHEMA,   "snaps",        SDF_AUTHZ_X,    0,  0,              cmd_list_snaps,     "List snaps"),
SDATACM2 (ASN_SCHEMA,   "snap-content", SDF_AUTHZ_X,    0,  pm_snap_content,cmd_snap_content,   "Show snap content"),
SDATACM2 (ASN_SCHEMA,   "shoot-snap",   SDF_AUTHZ_X,    0,  pm_shoot_snap,  cmd_shoot_snap,     "Shoot snap"),
SDATACM2 (ASN_SCHEMA,   "activate-snap",SDF_AUTHZ_X,    0,  pm_activate_snap,cmd_activate_snap, "Activate snap"),
SDATACM2 (ASN_SCHEMA,   "deactivate-snap",SDF_AUTHZ_X,  0,  0,              cmd_deactivate_snap,"De-Activate snap"),
SDATACM2 (ASN_SCHEMA,   "import-db",    SDF_AUTHZ_X,    0,  pm_import_db,   cmd_import_db, "Import db"),
SDATACM2 (ASN_SCHEMA,   "export-db",    SDF_AUTHZ_X,    0,  pm_export_db,   cmd_export_db, "Export db"),
SDATACM2 (ASN_SCHEMA,   "pkey2s",       SDF_AUTHZ_X,    0,  pm_node_pkey2s, cmd_node_pkey2s,    "List node's pkey2"),
SDATACM2 (ASN_SCHEMA,   "desc",         SDF_AUTHZ_X,    a_schema, pm_desc,  cmd_desc,           "Schema of topic"),
SDATACM2 (ASN_SCHEMA,   "descs",        SDF_AUTHZ_X,    a_schemas, 0,       cmd_desc,           "Schema of topics"),
SDATACM (ASN_SCHEMA,    "system-schema",0,              0,  cmd_system_topic_schema, "Get system schema"),
SDATACM2 (ASN_SCHEMA,   "trace",        SDF_AUTHZ_X,    0,  pm_trace,       cmd_trace,          "Set trace"),
SDATA_END()
};

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "tranger",          0,                  0,              "Tranger handler"),
SDATA (ASN_OCTET_STR,   "treedb_name",      SDF_RD|SDF_REQUIRED,"",             "Treedb name"),
SDATA (ASN_JSON,        "treedb_schema",    SDF_RD|SDF_REQUIRED,0,              "Treedb schema"),
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
 *      GClass authz levels
 *---------------------------------------------*/

PRIVATE sdata_desc_t pm_authz_create[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "treedb_name",      0,          "",             "Treedb name"),
SDATAPM0 (ASN_OCTET_STR,    "topic_name",       0,          "",             "Topic name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authz_write[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "treedb_name",      0,          "",             "Treedb name"),
SDATAPM0 (ASN_OCTET_STR,    "topic_name",       0,          "",             "Topic name"),
SDATAPM0 (ASN_OCTET_STR,    "id",               0,          "record`id",    "Node Id"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authz_read[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "treedb_name",      0,          "__md_treedb__`treedb_name",             "Treedb name"),
SDATAPM0 (ASN_OCTET_STR,    "topic_name",       0,          "__md_treedb__`topic_name", "Topic name"),
SDATAPM0 (ASN_OCTET_STR,    "id",               0,          "id",           "Node Id"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authz_delete[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "treedb_name",      0,          "",             "Treedb name"),
SDATAPM0 (ASN_OCTET_STR,    "topic_name",       0,          "",             "Topic name"),
SDATAPM0 (ASN_OCTET_STR,    "id",               0,          "record`id",    "Node Id"),
SDATA_END()
};

PRIVATE sdata_desc_t authz_table[] = {
/*-AUTHZ-- type---------name------------flag----alias---items---------------description--*/
SDATAAUTHZ (ASN_SCHEMA, "create",       0,      0,      pm_authz_create,    "Permission to create nodes"),
SDATAAUTHZ (ASN_SCHEMA, "update",       0,      0,      pm_authz_write,     "Permission to update nodes"),
SDATAAUTHZ (ASN_SCHEMA, "read",         0,      0,      pm_authz_read,      "Permission to read nodes"),
SDATAAUTHZ (ASN_SCHEMA, "delete",       0,      0,      pm_authz_delete,    "Permission to delete nodes"),
SDATA_END()
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    json_t *tranger;
    const char *treedb_name;
    json_t *treedb_schema;
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
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
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

    json_t *treedb = treedb_open_db( // Return IS NOT YOURS!
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

    return treedb?0:-1;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

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
PRIVATE json_t *mt_treedbs(
    hgobj gobj,
    json_t *kw,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_list_treedb(
        priv->tranger,
        kw
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_treedb_topics(
    hgobj gobj,
    const char *treedb_name,
    json_t *options, // "dict" return list of dicts, otherwise return list of strings
    hgobj src
)
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
        /*-----------------------------------*
         *      Check appropiate topic
         *-----------------------------------*/
        if(!treedb_is_treedbs_topic(
            priv->tranger,
            priv->treedb_name,
            topic_name
        )) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_TREEDB_ERROR,
                "msg",          "%s", "Topic name not found in treedbs",
                "treedb_name",  "%s", priv->treedb_name,
                "topic_name",   "%s", topic_name,
                NULL
            );
            return 0;
        }
        return tranger_topic_desc(priv->tranger, topic_name);

    } else {
        json_t *topics_list = treedb_topics( //Return a list with topic names of the treedb
            priv->tranger,
            priv->treedb_name,
            0
        );
        json_t *jn_topics_desc = json_object();
        int idx; json_t *jn_topic_name;
        json_array_foreach(topics_list, idx, jn_topic_name) {
            topic_name = json_string_value(jn_topic_name);
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
PRIVATE json_t *mt_topic_links(
    hgobj gobj,
    const char *treedb_name,
    const char *topic_name,
    json_t *kw,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!empty_string(topic_name)) {
        /*-----------------------------------*
         *      Check appropiate topic
         *-----------------------------------*/
        if(!treedb_is_treedbs_topic(
            priv->tranger,
            priv->treedb_name,
            topic_name
        )) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_TREEDB_ERROR,
                "msg",          "%s", "Topic name not found in treedbs",
                "treedb_name",  "%s", priv->treedb_name,
                "topic_name",   "%s", topic_name,
                NULL
            );
            KW_DECREF(kw);
            return 0;
        }

        KW_DECREF(kw);
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
    KW_DECREF(kw);
    return links;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_topic_hooks(
    hgobj gobj,
    const char *treedb_name,
    const char *topic_name,
    json_t *kw,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!empty_string(topic_name)) {
        /*-----------------------------------*
         *      Check appropiate topic
         *-----------------------------------*/
        if(!treedb_is_treedbs_topic(
            priv->tranger,
            priv->treedb_name,
            topic_name
        )) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_TREEDB_ERROR,
                "msg",          "%s", "Topic name not found in treedbs",
                "treedb_name",  "%s", priv->treedb_name,
                "topic_name",   "%s", topic_name,
                NULL
            );
            KW_DECREF(kw);
            return 0;
        }

        KW_DECREF(kw);
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
    KW_DECREF(kw);
    return hooks;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_create_node( // Return is YOURS
    hgobj gobj,
    const char *topic_name,
    json_t *kw,
    json_t *jn_options, // fkey,hook options
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_options);
        KW_DECREF(kw);
        return 0;
    }

    json_t *node = treedb_create_node( // Return is NOT YOURS
        priv->tranger,
        priv->treedb_name,
        topic_name,
        kw // owned
    );
    if(!node) {
        JSON_DECREF(jn_options);
        return 0;
    }

    return node_collapsed_view( // Return MUST be decref
        priv->tranger,
        node, // not owned
        jn_options // owned fkey,hook options
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE size_t mt_topic_size(
    hgobj gobj,
    const char *topic_name
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_topic_size(priv->tranger, priv->treedb_name, topic_name);
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_update_node( // Return is YOURS
    hgobj gobj,
    const char *topic_name,
    json_t *kw,         // 'id' and pkey2s fields are used to find the node
    json_t *jn_options, // "create" "autolink" "volatil" fkey,hook options
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    BOOL volatil = kw_get_bool(jn_options, "volatil", 0, KW_WILD_NUMBER);
    BOOL create = kw_get_bool(jn_options, "create", 0, KW_WILD_NUMBER);
    BOOL autolink = kw_get_bool(jn_options, "autolink", 0, KW_WILD_NUMBER);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_options);
        KW_DECREF(kw);
        return 0;
    }

    json_t *node = fetch_node(gobj, topic_name, kw);
    if(!node) {
        if(create) {
            node = treedb_create_node( // Return is NOT YOURS
                priv->tranger,
                priv->treedb_name,
                topic_name,
                kw_incref(kw)
            );
        }
        if(!node) {
            log_error(LOG_OPT_TRACE_STACK,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_TREEDB_ERROR,
                "msg",          "%s", "node not found",
                "treedb_name",  "%s", priv->treedb_name,
                "topic_name",   "%s", topic_name,
                NULL
            );
            log_debug_json(0, kw, "node not found");
            JSON_DECREF(jn_options);
            KW_DECREF(kw);
            return 0;
        }
    } else {
        /*
         *  If node exists then it's an update
         */
        create = 0;
    }

    if(volatil) {
        set_volatil_values(
            priv->tranger,
            topic_name,
            node,  // NOT owned
            kw // NOT owned
        );

    } else {
        if(!create) {
            treedb_update_node( // Return is NOT YOURS
                priv->tranger,
                node,
                json_incref(kw),
                autolink?FALSE:TRUE
            );
        }
        if(autolink) {
            treedb_clean_node(priv->tranger, node, FALSE);  // remove current links
            treedb_autolink(priv->tranger, node, json_incref(kw), FALSE);
            treedb_save_node(priv->tranger, node);
        }
    }

    json_decref(kw);

    return node_collapsed_view( // Return MUST be decref
        priv->tranger,
        node, // not owned
        jn_options // owned fkey,hook options
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_delete_node(
    hgobj gobj,
    const char *topic_name,
    json_t *kw,         // 'id' and pkey2s fields are used to find the node
    json_t *jn_options, // "force"
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_options);
        KW_DECREF(kw);
        return 0;
    }

    const char *id = kw_get_str(kw, "id", 0, 0);
    if(empty_string(id)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "id required to delete node",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(kw);
        JSON_DECREF(jn_options);
        return -1;
    }

    json_t *main_node = treedb_get_node( // WARNING Return is NOT YOURS, pure node
        priv->tranger,
        priv->treedb_name,
        topic_name,
        id
    );
    if(!main_node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "node not found",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            "id",           "%s", id,
            NULL
        );
        JSON_DECREF(kw);
        JSON_DECREF(jn_options);
        return -1;
    }

    json_t *node = 0;
    int ret = 0;
    json_t *jn_filter = json_object();

    /*
     *  Check if has secondary keys
     */
    json_t *pkey2s_list = treedb_topic_pkey2s(priv->tranger, topic_name);
    int idx; json_t *jn_pkey2_name;
    json_array_foreach(pkey2s_list, idx, jn_pkey2_name) {
        const char *pkey2_name = json_string_value(jn_pkey2_name);
        const char *key2 = kw_get_str(kw, pkey2_name, 0, 0);
        node = treedb_get_instance( // WARNING Return is NOT YOURS, pure node
            priv->tranger,
            priv->treedb_name,
            topic_name,
            pkey2_name,
            id,     // primary key
            key2    // secondary key
        );
        if(node && node != main_node) {
            json_object_set_new(jn_filter, pkey2_name, json_string(key2));
            ret += treedb_delete_instance(
                priv->tranger,
                node,
                pkey2_name,
                json_incref(jn_options)
            );
        }
    }
    json_decref(pkey2s_list);

    if(kw_match_simple(main_node, jn_filter)) {
        int r = treedb_delete_node(
            priv->tranger,
            main_node,
            json_incref(jn_options)
        );
        ret += r;
    }

    JSON_DECREF(kw);
    JSON_DECREF(jn_options);
    return ret;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_link_nodes(
    hgobj gobj,
    const char *hook,
    const char *parent_topic_name,
    json_t *parent_record,  // owned
    const char *child_topic_name,
    json_t *child_record,  // owned
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        parent_topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", parent_topic_name,
            NULL
        );
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        child_topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", child_topic_name,
            NULL
        );
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    /*-------------------------------*
     *      Recover nodes
     *-------------------------------*/
    //const char *parent_topic_name = kw_get_str(parent_record, "__md_treedb__`topic_name", 0, 0);
    //const char *child_topic_name = kw_get_str(child_record, "__md_treedb__`topic_name", 0, 0);

    json_t *parent_node = fetch_node( // WARNING Return is NOT YOURS, pure node
        gobj,
        parent_topic_name,
        parent_record
    );
    if(!parent_node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "parent node not found",
            NULL
        );
        log_debug_json(0, parent_record, "node not found");
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    json_t *child_node = fetch_node( // WARNING Return is NOT YOURS, pure node
        gobj,
        child_topic_name,
        child_record
    );
    if(!child_node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "child node not found",
            NULL
        );
        log_debug_json(0, child_record, "node not found");
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    JSON_DECREF(parent_record);
    JSON_DECREF(child_record);
    return treedb_link_nodes(
        priv->tranger,
        hook,
        parent_node,
        child_node
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_unlink_nodes(
    hgobj gobj,
    const char *hook,
    const char *parent_topic_name,
    json_t *parent_record,  // owned
    const char *child_topic_name,
    json_t *child_record,  // owned
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        parent_topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", parent_topic_name,
            NULL
        );
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        child_topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", child_topic_name,
            NULL
        );
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    /*-------------------------------*
     *      Recover nodes
     *-------------------------------*/
    //const char *parent_topic_name = kw_get_str(parent_record, "__md_treedb__`topic_name", 0, 0);
    //const char *child_topic_name = kw_get_str(child_record, "__md_treedb__`topic_name", 0, 0);

    json_t *parent_node = fetch_node( // WARNING Return is NOT YOURS, pure node
        gobj,
        parent_topic_name,
        parent_record
    );
    if(!parent_node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "parent node not found",
            NULL
        );
        log_debug_json(0, parent_record, "node not found");
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    json_t *child_node = fetch_node( // WARNING Return is NOT YOURS, pure node
        gobj,
        child_topic_name,
        child_record
    );
    if(!child_node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "child node not found",
            NULL
        );
        log_debug_json(0, child_record, "node not found");
        JSON_DECREF(parent_record);
        JSON_DECREF(child_record);
        return -1;
    }

    JSON_DECREF(parent_record);
    JSON_DECREF(child_record);
    return treedb_unlink_nodes(
        priv->tranger,
        hook,
        parent_node,
        child_node
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_get_node(
    hgobj gobj,
    const char *topic_name,
    json_t *kw,         // 'id' and pkey2s fields are used to find the node
    json_t *jn_options, // fkey,hook options
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_options);
        KW_DECREF(kw);
        return 0;
    }

    json_t *node = fetch_node(gobj, topic_name, kw);
    if(!node) {
        // Silence
        JSON_DECREF(jn_options);
        KW_DECREF(kw);
        return 0;
    }

    KW_DECREF(kw);
    return node_collapsed_view( // Return MUST be decref
        priv->tranger, // not owned
        node, // not owned
        jn_options // owned
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_list_nodes(
    hgobj gobj,
    const char *topic_name,
    json_t *jn_filter,
    json_t *jn_options, // "include-instances" fkey,hook options
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        return 0;
    }

    BOOL include_instances = kw_get_bool(jn_options, "include-instances", 0, KW_WILD_NUMBER);

    /*
     *  Search in main list
     */
    json_t *iter = treedb_list_nodes(
        priv->tranger,
        priv->treedb_name,
        topic_name,
        json_incref(jn_filter),
        0
    );

    if(json_array_size(iter)==0 && include_instances) {
        /*
         *  Search in instances
         */
        json_decref(iter);
        iter = treedb_list_instances(
            priv->tranger,
            priv->treedb_name,
            topic_name,
            "",
            json_incref(jn_filter),
            0
        );
    }

    json_t *list = json_array();
    int idx; json_t *node;
    json_array_foreach(iter, idx, node) {
        /*----------------------------------------*
         *  Check AUTHZS
         *----------------------------------------*/
        /*
         *  En node tengo __treedb_name__, __topic_name__, "id"
         */
        const char *permission = "read";
        if(gobj_user_has_authz(gobj, permission, json_incref(node), src)) {
            json_array_append_new(
                list,
                node_collapsed_view( // TODO añade opción "expand"
                    priv->tranger,
                    node,
                    json_incref(jn_options)
                )
            );
        }
    }
    json_decref(iter);

    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);

    return list;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_list_instances(
    hgobj gobj,
    const char *topic_name,
    const char *pkey2,
    json_t *jn_filter,
    json_t *jn_options, // fkey,hook options
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        return 0;
    }

    // TODO Filtra la lista con los nodos con permiso para leer

    json_t *iter = treedb_list_instances( // Return MUST be decref
        priv->tranger,
        priv->treedb_name,
        topic_name,
        pkey2,
        json_incref(jn_filter),
        0
    );

    json_t *list = json_array();
    int idx; json_t *node;
    json_array_foreach(iter, idx, node) {
        json_array_append_new(
            list,
            node_collapsed_view(
                priv->tranger,
                node,
                json_incref(jn_options)
            )
        );
    }
    json_decref(iter);

    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    return list;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_node_parents(
    hgobj gobj,
    const char *topic_name,
    json_t *kw,         // 'id' and pkey2s fields are used to find the node
    const char *link,   // fkey
    json_t *jn_options, // owned , fkey options
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    json_t *node = fetch_node(gobj, topic_name, kw);
    if(!node) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Node not found",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            "kw",           "%j", kw,
            NULL
        );
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    /*
     *  Return a list of parent nodes pointed by the link (fkey)
     */
    if(!empty_string(link)) {
        JSON_DECREF(kw);
        return treedb_parent_refs( // Return MUST be decref
            priv->tranger,
            link, // must be a fkey field
            node, // not owned
            jn_options
        );
    }

    /*
     *  If no link return all links
     */
    json_t *parents = json_array();
    json_t *links = treedb_get_topic_links(priv->tranger, priv->treedb_name, topic_name);
    int idx; json_t *jn_link;
    json_array_foreach(links, idx, jn_link) {
        json_t *parents_ = treedb_parent_refs( // Return MUST be decref
            priv->tranger,
            json_string_value(jn_link), // must be a fkey field
            node, // not owned
            json_incref(jn_options)
        );
        json_array_extend(parents, parents_);
        JSON_DECREF(parents_);
    }
    JSON_DECREF(links);

    JSON_DECREF(jn_options);
    JSON_DECREF(kw);
    return parents;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_node_childs(
    hgobj gobj,
    const char *topic_name,
    json_t *kw,         // 'id' and pkey2s fields are used to find the node
    const char *hook,
    json_t *jn_filter,  // filter to childs tree
    json_t *jn_options, // fkey,hook options, "recursive"
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    json_t *node = fetch_node(gobj, topic_name, kw);
    if(!node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Node not found",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            "kw",           "%j", kw,
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    /*
     *  Return a list of child nodes of the hook
     */
    json_t *iter = treedb_node_childs( // Return MUST be decref
        priv->tranger,
        hook, // must be a hook field
        node, // not owned
        json_incref(jn_filter),
        json_incref(jn_options)
    );

    if(!iter) {
        // Error already logged
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    json_t *childs = json_array();

    int idx; json_t *child;
    json_array_foreach(iter, idx, child) {
        json_array_append_new(
            childs,
            node_collapsed_view(
                priv->tranger,
                child,
                json_incref(jn_options)
            )
        );
    }
    json_decref(iter);

    JSON_DECREF(jn_filter);
    JSON_DECREF(jn_options);
    JSON_DECREF(kw);
    return childs;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_topic_jtree(
    hgobj gobj,
    const char *topic_name,
    const char *hook,   // hook to build the hierarchical tree
    const char *rename_hook, // change the hook name in the tree response
    json_t *kw,         // 'id' and pkey2s fields are used to find the root node
    json_t *jn_filter,  // filter to match records
    json_t *jn_options, // fkey,hook options
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    /*
     *  If root node is not specified then the first with no parent is used
     */
    json_t *node = fetch_node(gobj, topic_name, kw);
    if(!node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Node not found",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            "kw",           "%j", kw,
            NULL
        );
        JSON_DECREF(jn_filter);
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    /*
     *  Return a tree of child nodes of the hook
     */
    JSON_DECREF(kw);
    return treedb_node_jtree( // Return MUST be decref
        priv->tranger,
        hook, // must be a hook field
        rename_hook,
        node, // not owned
        jn_filter,
        jn_options
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_node_tree(
    hgobj gobj,
    const char *topic_name,
    json_t *kw,         // 'id' and pkey2s fields are used to find the root node
    json_t *jn_options, // "with_metatada"
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*-----------------------------------*
     *      Check appropiate topic
     *-----------------------------------*/
    if(!treedb_is_treedbs_topic(
        priv->tranger,
        priv->treedb_name,
        topic_name
    )) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Topic name not found in treedbs",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            NULL
        );
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    /*
     *  If root node is not specified then the first with no parent is used
     */
    json_t *node = fetch_node(gobj, topic_name, kw);
    if(!node) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Node not found",
            "treedb_name",  "%s", priv->treedb_name,
            "topic_name",   "%s", topic_name,
            "kw",           "%j", kw,
            NULL
        );
        JSON_DECREF(jn_options);
        JSON_DECREF(kw);
        return 0;
    }

    /*
     *  Return the duplicated full node
     */
    JSON_DECREF(jn_options);
    JSON_DECREF(kw);

    BOOL with_metadata = kw_get_bool(jn_options, "with_metadata", 0, KW_WILD_NUMBER);

    if(with_metadata) {
        return json_deep_copy(node);
    } else {
        return kw_filter_metadata(json_incref(node));
    }
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_shoot_snap(
    hgobj gobj,
    const char *name,
    json_t *kw,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *description = kw_get_str(kw, "description", "", 0);
    int ret = treedb_shoot_snap(
        priv->tranger,
        priv->treedb_name,
        name,
        description
    );
    KW_DECREF(kw);
    return ret;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_activate_snap(
    hgobj gobj,
    const char *tag,
    json_t *kw,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    KW_DECREF(kw);

    return treedb_activate_snap(
        priv->tranger,
        priv->treedb_name,
        tag
    );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_list_snaps(
    hgobj gobj,
    json_t *filter,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return treedb_list_snaps(
        priv->tranger,
        priv->treedb_name,
        filter
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
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    return gobj_build_authzs_doc(gobj, cmd, kw, src);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_create_node(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *content64 = kw_get_str(kw, "content64", "", 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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
     *  Priority: content64, record
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
        jn_content = kw_incref(kw_get_dict(kw, "record", 0, 0));
    } else {
        // To authz
        json_object_set(kw, "record", jn_content);
    }

    if(!jn_content) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What record?"),
            0,
            0,
            kw  // owned
        );
    }

    /*----------------------------------------*
     *  Check AUTHZS
     *----------------------------------------*/
    const char *permission = "create";
    if(!gobj_user_has_authz(gobj, permission, kw_incref(kw), src)) {
        json_decref(jn_content);
        return msg_iev_build_webix(
            gobj,
            -403,
            json_local_sprintf("No permission to '%s'", permission),
            0,
            0,
            kw  // owned
        );
    }

    json_t *node = gobj_create_node(
        gobj,
        topic_name,
        jn_content, // owned
        json_incref(_jn_options),
        src
    );
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
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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
     *  Priority: content64, record
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
        jn_content = json_incref(kw_get_dict(kw, "record", 0, 0));
    } else {
        // To authz
        json_object_set(kw, "record", jn_content);
    }

    if(!jn_content) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What record?"),
            0,
            0,
            kw  // owned
        );
    }

    /*----------------------------------------*
     *  Check AUTHZS
     *----------------------------------------*/
    const char *permission = "update";
    if(!gobj_user_has_authz(gobj, permission, kw_incref(kw), src)) {
        json_decref(jn_content);
        return msg_iev_build_webix(
            gobj,
            -403,
            json_local_sprintf("No permission to '%s'", permission),
            0,
            0,
            kw  // owned
        );
    }

    json_t *node = gobj_update_node(
        gobj,
        topic_name,
        jn_content, // owned
        json_incref(_jn_options),
        src
    );

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
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);
    json_t *_jn_record = kw_get_dict_value(kw, "record", 0, 0);

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

    if(!kw_has_key(_jn_record, "id")) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("field 'id' is required to delete nodes"),
            0,
            0,
            kw  // owned
        );
    }

    /*----------------------------------------*
     *  Check AUTHZS
     *----------------------------------------*/
    const char *permission = "delete";
    if(!gobj_user_has_authz(gobj, permission, kw_incref(kw), src)) {
        return msg_iev_build_webix(
            gobj,
            -403,
            json_local_sprintf("No permission to '%s'", permission),
            0,
            0,
            kw  // owned
        );
    }

    /*
     *  Get a iter of matched resources.
     */
    json_t *node = gobj_get_node(
        gobj,
        topic_name,
        json_incref(_jn_record),
        0,
        src
    );
    if(!node) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Node not found"),
            0,
            0,
            kw  // owned
        );
    }

    JSON_INCREF(node);
    if(gobj_delete_node(
            gobj,
            topic_name,
            node,
            json_incref(_jn_options),
            src
    )<0) {
        JSON_DECREF(node);
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf(log_last_message()),
            0,
            0,
            kw  // owned
        );
    }

    return msg_iev_build_webix(
        gobj,
        0,
        json_local_sprintf("Node deleted"),
        gobj_topic_desc(gobj, topic_name),
        node,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_link_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *parent_ref = kw_get_str(kw, "parent_ref", "", 0);
    const char *child_ref = kw_get_str(kw, "child_ref", "", 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

    if(empty_string(parent_ref)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What parent ref?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(child_ref)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What child ref?"),
            0,
            0,
            kw  // owned
        );
    }

    char parent_topic_name[NAME_MAX];
    char parent_id[NAME_MAX];
    char hook_name[NAME_MAX];
    if(!decode_parent_ref(
        parent_ref,
        parent_topic_name, sizeof(parent_topic_name),
        parent_id, sizeof(parent_id),
        hook_name, sizeof(hook_name)
    )) {
        /*
         *  It's not a fkey.
         *  It's not an error, it happens when it's a hook and fkey field.
         */
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Wrong parent ref"),
            0,
            0,
            kw  // owned
        );
    }

    char child_topic_name[NAME_MAX];
    char child_id[NAME_MAX];

    if(!decode_child_ref(
        child_ref,
        child_topic_name, sizeof(child_topic_name),
        child_id, sizeof(child_id)
    )) {
        // It's not a child ref
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Wrong child ref"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *parent_node = gobj_get_node(
        gobj,
        parent_topic_name,
        json_pack("{s:s}", "id", parent_id),
        0,
        src
    );
    if(!parent_node) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Parent not found"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *child_node = gobj_get_node(
        gobj,
        child_topic_name,
        json_pack("{s:s}", "id", child_id),
        0,
        src
    );
    if(!child_node) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Parent not found"),
            0,
            0,
            kw  // owned
        );
    }

    int result = gobj_link_nodes(
        gobj,
        hook_name,
        parent_topic_name,
        parent_node,
        child_topic_name,
        child_node,
        src
    );

    child_node = gobj_get_node(
        gobj,
        child_topic_name,
        json_pack("{s:s}", "id", child_id),
        json_incref(_jn_options),
        src
    );

    return msg_iev_build_webix(gobj,
        result,
        result<0?json_local_sprintf(log_last_message()):json_local_sprintf("Nodes linked!"),
        gobj_topic_desc(gobj, child_topic_name),
        child_node,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_unlink_nodes(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *parent_ref = kw_get_str(kw, "parent_ref", "", 0);
    const char *child_ref = kw_get_str(kw, "child_ref", "", 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

    if(empty_string(parent_ref)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What parent ref?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(child_ref)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What child ref?"),
            0,
            0,
            kw  // owned
        );
    }

    char parent_topic_name[NAME_MAX];
    char parent_id[NAME_MAX];
    char hook_name[NAME_MAX];
    if(!decode_parent_ref(
        parent_ref,
        parent_topic_name, sizeof(parent_topic_name),
        parent_id, sizeof(parent_id),
        hook_name, sizeof(hook_name)
    )) {
        /*
         *  It's not a fkey.
         *  It's not an error, it happens when it's a hook and fkey field.
         */
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Wrong parent ref"),
            0,
            0,
            kw  // owned
        );
    }

    char child_topic_name[NAME_MAX];
    char child_id[NAME_MAX];

    if(!decode_child_ref(
        child_ref,
        child_topic_name, sizeof(child_topic_name),
        child_id, sizeof(child_id)
    )) {
        // It's not a child ref
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Wrong child ref"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *parent_node = gobj_get_node(
        gobj,
        parent_topic_name,
        json_pack("{s:s}", "id", parent_id),
        0,
        src
    );
    if(!parent_node) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Parent not found"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *child_node = gobj_get_node(
        gobj,
        child_topic_name,
        json_pack("{s:s}", "id", child_id),
        0,
        src
    );
    if(!child_node) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Parent not found"),
            0,
            0,
            kw  // owned
        );
    }

    int result = gobj_unlink_nodes(
        gobj,
        hook_name,
        parent_topic_name,
        parent_node,
        child_topic_name,
        child_node,
        src
    );

    child_node = gobj_get_node(
        gobj,
        child_topic_name,
        json_pack("{s:s}", "id", child_id),
        json_incref(_jn_options),
        src
    );

    return msg_iev_build_webix(gobj,
        result,
        result<0?json_local_sprintf(log_last_message()):json_local_sprintf("Nodes unlinked!"),
        gobj_topic_desc(gobj, child_topic_name),
        child_node,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_treedbs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    json_incref(kw);
    json_t *treedbs = gobj_treedbs(gobj, kw, src);

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
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

    if(empty_string(treedb_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What treedb_name?"),
            0,
            0,
            kw  // owned
        );
    }
    json_t *topics = gobj_treedb_topics(gobj, treedb_name, json_incref(_jn_options), src);

    return msg_iev_build_webix(gobj,
        topics?0:-1,
        topics?0:json_string(log_last_message()),
        0,
        topics,
        kw  // owned
    );
}

/***************************************************************************
 *  Return a hierarchical tree of the self-link topic
 *  If "webix" option is true return webix style (dict-list),
 *      else list of dict's
 *  __path__ field in all records (id`id`... style)
 *  If root node is not specified then the first with no parent is used
 ***************************************************************************/
PRIVATE json_t *cmd_jtree(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *node_id = kw_get_str(kw, "node_id", "", 0);
    const char *hook = kw_get_str(kw, "hook", "", 0);
    const char *rename_hook = kw_get_str(kw, "rename_hook", "", 0);
    json_t *_jn_filter = kw_get_dict(kw, "filter", 0, 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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
    if(empty_string(hook)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What hook?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(node_id)) {
        do {
            // Search the first "fkey" field
            json_t *links = gobj_topic_links(
                gobj,
                priv->treedb_name,
                topic_name,
                0,
                src
            );
            if(json_array_size(links)==0) {
                json_decref(links);
                break;
            }

            const char *link = json_string_value(json_array_get(links, 0));
            // Search the first "root" node (without parents);
            json_t *nodes = gobj_list_nodes(
                gobj,
                topic_name,
                0, // filter
                0, // options
                src
            );
            int idx; json_t *node;
            json_array_foreach(nodes, idx, node) {
                json_t *jn_hook = kw_get_list(node, link, 0, KW_REQUIRED);
                if(json_array_size(jn_hook)==0) {
                    node_id = kw_get_str(node, "id", "", KW_REQUIRED);
                    break;
                }
            }
            json_decref(nodes);
            json_decref(links);
        } while(0);

        if(empty_string(node_id)) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_local_sprintf("What node_id?"),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jtree = gobj_topic_jtree(
        gobj,
        topic_name,
        hook,                       // hook to build the hierarchical tree
        rename_hook,                // change the hook name in the tree response
        json_pack("{s:s}", "id", node_id),
        json_incref(_jn_filter),    // filter to match records
        json_incref(_jn_options),   // "webix", fkey,hook options
        src
    );

    return msg_iev_build_webix(gobj,
        jtree?0:-1,
        jtree?0:json_string(log_last_message()),
        0,
        jtree,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(strcmp(cmd, "desc")==0) {
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
    }

    json_t *desc = gobj_topic_desc(gobj, topic_name); // Empty topic_name -> return all

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
PRIVATE json_t* cmd_system_topic_schema(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        _treedb_create_topic_cols_desc(),
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

    json_incref(kw);
    json_t *links = gobj_topic_links(gobj, priv->treedb_name, topic_name, kw, src);

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

    json_incref(kw);
    json_t *hooks = gobj_topic_hooks(gobj, priv->treedb_name, topic_name, kw, src);

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
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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
        json_pack("{s:s}", "id", node_id),
        link,
        json_incref(_jn_options),
        src
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
    json_t *_jn_filter = kw_get_dict(kw, "filter", 0, 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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
    if(empty_string(hook)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What hook?"),
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
        json_pack("{s:s}", "id", node_id),
        hook,
        json_incref(_jn_filter),
        json_incref(_jn_options),
        src
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
    json_t *_jn_filter = kw_get_dict(kw, "filter", 0, 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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

    json_t *nodes = gobj_list_nodes(
        gobj,
        topic_name,
        json_incref(_jn_filter),
        json_incref(_jn_options),
        src
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
    const char *id = kw_get_str(kw, "node_id", "", 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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
    json_object_set_new(kw, "id", json_string(id)); // HACK remove 'id' field of cli

    json_t *node = gobj_get_node(
        gobj,
        topic_name,
        json_incref(kw),
        json_incref(_jn_options),
        src
    );

    return msg_iev_build_webix(gobj,
        node?0:-1,
        node?0:json_local_sprintf("Node not found"),
        node?tranger_topic_desc(priv->tranger, topic_name):0,
        node,
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
    json_t *jn_filter = json_incref(kw_get_dict(kw, "filter", 0, 0));
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0);

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
    if(!empty_string(node_id)) {
        if(!jn_filter) {
            jn_filter = json_pack("{s:s}", "id", node_id);
        } else {
            json_object_set_new(jn_filter, "id", json_string(node_id));
        }
    }

    json_t *instances = gobj_list_instances(
        gobj,
        topic_name,
        pkey2,
        jn_filter,
        json_incref(_jn_options),
        src
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
        0,
        json_local_sprintf("%d pkey2s", json_array_size(pkey2s)),
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

    int snap_id = kw_get_int(kw, "snap_id", 0, KW_WILD_NUMBER);
    if(!snap_id) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What snap_id?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *jn_data = json_array();

    json_t *jn_filter = json_pack("{s:b, s:i}",
        "backward", 1,
        "user_flag", snap_id
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
        tranger_list_topic_desc(priv->tranger, topic_name),
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
        kw_incref(kw),
        src
    );

    return msg_iev_build_webix(gobj,
        0,
        0,
        tranger_list_topic_desc(priv->tranger, "__snaps__"),
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
        name,
        kw_incref(kw),
        src
    );
    json_t *jn_data = 0;
    if(ret == 0) {
        jn_data = gobj_list_snaps(
            gobj,
            json_pack("{s:s}", "name", name),
            src
        );
    }

    return msg_iev_build_webix(gobj,
        ret,
        ret==0?json_sprintf("Snap '%s' shooted", name):json_string(log_last_message()),
        ret==0?tranger_list_topic_desc(priv->tranger, "__snaps__"):0,
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
        name,
        kw_incref(kw),
        src
    );
    return msg_iev_build_webix(gobj,
        ret,
        ret>=0?json_sprintf("Snap activated: '%s'", name):json_string(log_last_message()),
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
        "__clear__",
        kw_incref(kw),
        src
    );
    return msg_iev_build_webix(gobj,
        ret,
        ret==0?json_sprintf("Snap deactivated"):json_string(log_last_message()),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_export_db(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *filename = kw_get_str(kw, "filename", "", 0);
    BOOL overwrite = kw_get_bool(kw, "overwrite", 0, KW_WILD_NUMBER);
    BOOL with_metadata = kw_get_bool(kw, "with_metadata", 0, KW_WILD_NUMBER);
    BOOL without_rowid = kw_get_bool(kw, "without_rowid", 0, KW_WILD_NUMBER);

    char path[PATH_MAX];
    char name[NAME_MAX];
    char date[100];

    if(empty_string(filename)) {
        current_timestamp(date, sizeof(date));
        time_t t;
        struct tm *tm;
        time(&t);
        tm = localtime(&t);
        strftime(date, sizeof(date), "%Y-%m-%d", tm);
        snprintf(name, sizeof(name), "%s-%s-%s.trdb.json",
            priv->treedb_name,
            kw_get_str(priv->treedb_schema, "schema_version", "", KW_REQUIRED),
            date
        );
    } else {
        snprintf(name, sizeof(name), "%s.trdb.json", filename);
    }

    yuneta_realm_file(path, sizeof(path), "temp", "", TRUE);

    json_t *jn_data = json_pack("{s:s, s:s}",
        "path", path,
        "filename", name
    );

    yuneta_realm_file(path, sizeof(path), "temp", name, TRUE);

    if(access(path, 0)==0) {
        if(!overwrite) {
            json_decref(jn_data);
            return msg_iev_build_webix(gobj,
                -1,
                json_local_sprintf("File '%s' already exists. Use overwrite option", name),
                0,
                0,
                kw  // owned
            );
        }
    }

    int ret = export_treedb(gobj, path, with_metadata, without_rowid, src);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        ret,
        json_local_sprintf("Treedb exported %s", ret==0?"ok":"failed"),
        0,
        jn_data, // owned
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_import_db(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *content64 = kw_get_str(kw, "content64", "", 0);

    int if_resource_exists = 0; // abort by default
    const char *if_resource_exists_ = kw_get_str(kw, "if-resource-exists", "", 0);

    if(strcasecmp(if_resource_exists_, "skip")==0) {
        if_resource_exists = 1;
    } else if(strcasecmp(if_resource_exists_, "overwrite")==0) {
        if_resource_exists = 2;
    }

    /*------------------------------------------------*
     *  Firstly get content in base64 and decode
     *------------------------------------------------*/
    json_t *jn_db;
    if(empty_string(content64)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What content64?"),
            0,
            0,
            kw  // owned
        );
    }

    GBUFFER *gbuf_content = gbuf_decodebase64string(content64);
    jn_db = gbuf2json(
        gbuf_content,  // owned
        2
    );
    if(!jn_db) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Bad json in content64"),
            0,
            0,
            kw  // owned
        );
    }

    /*-----------------------------*
     *  Load records
     *  If a resource exists
     *      0 abort (default)
     *      1 skip
     *      2 overwrite
     *-----------------------------*/
    int ret = 0;
    int new = 0;
    int overwrite = 0;
    int ignored = 0;
    int failure = 0;
    int abort = 0;
    int link_failure = 0;

    json_t *jn_loaded = json_object();
    json_t *jn_errores = json_object();

    // TODO Chequear permisos

    BOOL fin = FALSE;
    const char *topic_name;
    json_t *topic_records;
    json_object_foreach(jn_db, topic_name, topic_records) {
        if(fin) {
            break;
        }
        json_t *list_records = kw_get_dict_value(
            jn_loaded,
            topic_name,
            json_array(),
            KW_CREATE
        );

        int idx; json_t *record;
        json_array_foreach(topic_records, idx, record) {
            if(fin) {
                break;
            }
            json_t *node = gobj_create_node( // Return is NOT YOURS
                gobj,
                topic_name,
                json_incref(record),
                0,
                src
            );
            if(node) {
                json_decref(node);
                json_array_append(
                    list_records,
                    record
                );
                new++;
            } else {
                int n = kw_get_int(jn_errores, log_last_message(), 0, KW_CREATE);
                n++;
                json_object_set_new(jn_errores, log_last_message(), json_integer(n));

                if(if_resource_exists==1) {
                    // Skip
                    ignored++;
                } else if(if_resource_exists==2) {
                    // Overwrite
                    node = gobj_get_node(
                        gobj,
                        topic_name,
                        json_incref(record),
                        0,
                        src
                    );
                    if(node) {
                        json_decref(node);
                        json_array_append(
                            list_records,
                            record
                        );
                        overwrite++;
                    } else {
                        int n = kw_get_int(jn_errores, log_last_message(), 0, KW_CREATE);
                        n++;
                        json_object_set_new(jn_errores, log_last_message(), json_integer(n));

                        failure++;
                    }
                } else {
                    // abort
                    abort++;
                    fin = TRUE;
                    log_debug_json(0, record, "%s", log_last_message());
                    break;
                }
            }
        }
    }

    /*
     *  Create links
     */
    json_object_foreach(jn_loaded, topic_name, topic_records) {
        int idx; json_t *record;
        json_array_foreach(topic_records, idx, record) {
            json_t *node = gobj_update_node(
                gobj,
                topic_name,
                json_incref(record),
                json_pack("{s:b}", "autolink", 1),
                src
            );
            if(node) {
                //log_debug_json(0, node, "node added");
                json_decref(node);
            } else {
                link_failure++;
                log_debug_json(0, record, "link_failure");
            }
        }
    }

    json_decref(jn_db);
    json_decref(jn_loaded);

    /*
     *  Inform
     */
    json_t *jn_data = json_pack("{s:i, s:i, s:i, s:i, s:i, s:i, s:o}",
        "abort", abort,
        "added", new,
        "overwrite", overwrite,
        "ignored", ignored,
        "failule", failure,
        "link failure", link_failure,
        "errores", jn_errores
    );

    return msg_iev_build_webix(gobj,
        ret,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
// PRIVATE json_t *cmd_tree(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
// {
//     do {
//         if(!fichajes_roles ||
//                 !(json_str_in_list(fichajes_roles, "admin", FALSE) ||
//                 json_str_in_list(fichajes_roles, "gestor", FALSE))) {
//             jn_comment = json_string("User has not 'admin' or 'gestor' role");
//             result = -1;
//             break;
//         }
//
//         /*
//          *  Get lista departments
//          */
//         const char *department = kw_get_str(kw, "department", 0, 0);
//         if(!department) {
//             department = "direccion";
//         }
//         json_t *node = gobj_get_node( // Return is NOT YOURS
//             priv->treedb_gest,
//             "gest_departments",
//             json_pack("{s:s}", "id", department),
//             0,
//             src
//         );
//         if(node) {
//             json_t *filter = json_pack("{s:s, s:s, s:s}", // Fields to include
//                 "name", "value", // HACK rename field name->value
//                 "users", "",
//                 "managers", ""
//             );
//             jn_data = webix_new_list_tree(node, "departments", "data", filter, "");
//         } else {
//             result = -1;
//             jn_comment = json_string(log_last_message());
//         }
//     } while(0);
/*
}*/




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
    const char *operation,  // "EV_TREEDB_NODE_UPDATED",
                            // "EV_TREEDB_NODE_UPDATED",
                            // "EV_TREEDB_NODE_DELETED"
    json_t *node            // owned
)
{
    json_t *collapse_node = node_collapsed_view( // Return MUST be decref
        tranger,
        node, // not owned
        json_pack("{s:b}",
            "list_dict", 1  // HACK always list_dict
        )
    );

    json_t *kw = json_pack("{s:s, s:s, s:o}",
        "treedb_name", treedb_name,
        "topic_name", topic_name,
        "node", collapse_node
    );
    json_decref(node);

    return gobj_publish_event(user_data, operation, kw);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *fetch_node(
    hgobj gobj,
    const char *topic_name,
    json_t *kw  // NOT owned, 'id' and pkey2s fields are used to find the node
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *id = kw_get_str(kw, "id", 0, 0);
    if(!id) {
        // Silence
        return 0;
    }

    json_t *jn_filter = treedb_topic_pkey2s_filter(
        priv->tranger,
        topic_name,
        kw, // NO owned
        id
    );

    /*
     *  Busca en indice principal
     */
    json_t *node = treedb_get_node(
        priv->tranger,
        priv->treedb_name,
        topic_name,
        id
    );
    if(!node) {
        // Error already logged
        JSON_DECREF(jn_filter);
        return 0;
    }
    if(node && kw_match_simple(node, json_incref(jn_filter))) {
        JSON_DECREF(jn_filter);
        return node;
    }

    json_t *iter_pkey2s = treedb_topic_pkey2s(priv->tranger, topic_name);
    int idx; json_t *jn_pkey2_name;
    json_array_foreach(iter_pkey2s, idx, jn_pkey2_name) {
        const char *pkey2_name = json_string_value(jn_pkey2_name);
        if(empty_string(pkey2_name)) {
            continue;
        }

        const char *pkey2_value = kw_get_str(kw, pkey2_name, "", 0);

        node = treedb_get_instance( // WARNING Return is NOT YOURS, pure node
            priv->tranger,
            priv->treedb_name,
            topic_name,
            pkey2_name, // required
            id,         // primary key
            pkey2_value // secondary key
        );

        if(node && kw_match_simple(node, json_incref(jn_filter))) {
            break;
        } else {
            node = 0;
        }
    }
    json_decref(iter_pkey2s);

    JSON_DECREF(jn_filter);
    return node;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int export_treedb(
    hgobj gobj,
    const char *path,
    BOOL with_metadata,
    BOOL without_rowid,
    hgobj src
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    FILE *file = fopen(path, "w");
    if(!file) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_TREEDB_ERROR,
            "msg",          "%s", "Cannot create file",
            "path",         "%s", path,
            NULL
        );
        return -1;
    }

    json_t *jn_db = json_object();

    json_t *topics_list = gobj_treedb_topics(
        gobj,
        priv->treedb_name,
        0,
        src
    );
    int idx; json_t *jn_topic_name;
    json_array_foreach(topics_list, idx, jn_topic_name) {
        const char *topic_name = json_string_value(jn_topic_name);
        if(strcmp(topic_name, "__snaps__")==0) {
            continue;
        }
        json_t *nodes = gobj_list_nodes( // Return MUST be decref
            gobj,
            topic_name,
            0,
            json_pack("{s:b, s:b}",
                "with_metadata", with_metadata,
                "without_rowid", without_rowid
            ),
            src
        );
        json_object_set_new(jn_db, topic_name, nodes);
    }

    json_decref(topics_list);

    json_dump_file(jn_db, path, JSON_INDENT(4));
    json_decref(jn_db);

    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *  HACK bypass authz control, only internal use
 ***************************************************************************/
PRIVATE int ac_treedb_update_node(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *  Get parameters
     */
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    json_t *record = kw_get_dict(kw, "record", 0, 0);
    json_t *_jn_options = kw_get_dict(kw, "options", 0, 0); // "create", "auto-link"

    json_t *node = gobj_update_node( // Return is YOURS
        gobj,
        topic_name,
        json_incref(record),
        json_incref(_jn_options),
        src
    );
    json_decref(node); // return something? de momento no, uso interno.

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = { // HACK System gclass, not public events TODO o si?
    {"EV_TREEDB_UPDATE_NODE",   EVF_PUBLIC_EVENT,   0,      0},
    // bottom input
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = { // HACK System gclass, not public events TODO o si?
    {"EV_TREEDB_NODE_CREATED",  EVF_PUBLIC_EVENT|EVF_NO_WARN_SUBS,  0,  0},
    {"EV_TREEDB_NODE_UPDATED",  EVF_PUBLIC_EVENT|EVF_NO_WARN_SUBS,  0,  0},
    {"EV_TREEDB_NODE_DELETED",  EVF_PUBLIC_EVENT|EVF_NO_WARN_SUBS,  0,  0},
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_TREEDB_UPDATE_NODE",   ac_treedb_update_node,      0},
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
        0, //mt_stats
        0, //mt_command_parser,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link,
        0, //mt_delete_child_resource_link,
        0, //mt_get_resource,
        0, //mt_authorization_parser,
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
        0, //mt_authz_checker,
        0, //mt_future39,
        mt_create_node,
        mt_update_node,
        mt_delete_node,
        mt_link_nodes,
        0, //mt_future44,
        mt_unlink_nodes,
        mt_topic_jtree,
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
        mt_list_instances,
        mt_node_tree, //mt_node_tree,
        mt_topic_size,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    0,  // lmt
    tattr_desc,
    sizeof(PRIVATE_DATA),
    authz_table,  // acl
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
