/***********************************************************************
 *          C_TRANGER.C
 *          Tranger GClass.
 *
 *          Trangers: resources with tranger
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "c_tranger.h"

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
PRIVATE json_t *cmd_save_tranger_schema(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_create_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_update_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_delete_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_list_records(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_record_instances(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_touch_instance(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_record_pkey2s(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE sdata_desc_t pm_help[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help."),
SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
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
PRIVATE sdata_desc_t pm_save_tranger_schema[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_JSON,      "schema",       SDF_REQUIRED,  "",         "Path"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_create_record[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Tranger content"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Tranger content in base64"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"permissive\" \"multiple\""),
SDATA_END()
};
PRIVATE sdata_desc_t pm_update_record[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "content",      0,              0,          "Tranger content"),
SDATAPM (ASN_OCTET_STR, "content64",    0,              0,          "Tranger content in base64"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Options: \"create\", \"clean\""),
SDATA_END()
};
PRIVATE sdata_desc_t pm_delete_record[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_BOOLEAN,   "force",        0,              0,          "Force delete"),
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
PRIVATE sdata_desc_t pm_list_records[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Tree expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_get_record[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "record_id",      0,              0,          "Tranger id"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Tree expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_record_instances[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "record_id",      0,              0,          "Tranger id"),
SDATAPM (ASN_OCTET_STR, "pkey2",        0,              0,          "PKey2 field"),
SDATAPM (ASN_OCTET_STR, "filter",       0,              0,          "Search filter"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "Tree expanded"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_record_pkey2s[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias-------items-------json_fn---------description--*/
SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,    cmd_help,       "Command's help"),

/*-CMD---type-----------name------------al--items-----------json_fn-------------description--*/
SDATACM (ASN_SCHEMA,    "print-tranger",0,  pm_print_tranger, cmd_print_tranger,  "Print tranger"),
SDATACM (ASN_SCHEMA,    "save-tranger-schema",0, pm_save_tranger_schema, cmd_save_tranger_schema,  "Save tranger schema"),
SDATACM (ASN_SCHEMA,    "create-record",  0,  pm_create_record, cmd_create_record,    "Create record"),
SDATACM (ASN_SCHEMA,    "update-record",  0,  pm_update_record, cmd_update_record,    "Update record"),
SDATACM (ASN_SCHEMA,    "delete-record",  0,  pm_delete_record, cmd_delete_record,    "Delete record"),
SDATACM (ASN_SCHEMA,    "trace",        0,  pm_trace,       cmd_trace,          "Set trace"),
SDATACM (ASN_SCHEMA,    "topics",       0,  pm_topics,      cmd_topics,         "List topics"),
SDATACM (ASN_SCHEMA,    "desc",         0,  pm_desc,        cmd_desc,           "Schema of topic or full"),
SDATACM (ASN_SCHEMA,    "records",        0,  pm_list_records,  cmd_list_records,     "List records"),
SDATACM (ASN_SCHEMA,    "record",         0,  pm_get_record,    cmd_get_record,       "Get record by id"),
SDATACM (ASN_SCHEMA,    "instances",    0,  pm_record_instances,cmd_record_instances,"List record's instances"),
SDATACM (ASN_SCHEMA,    "touch-instance",0, pm_record_instances,cmd_touch_instance,"Touch (save) instance to force to be the last instance"),
SDATACM (ASN_SCHEMA,    "pkey2s",       0,  pm_record_pkey2s, cmd_record_pkey2s,    "List record's pkey2"),
SDATA_END()
};


/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "tranger",          SDF_RD,             0,              "Tranger handler"),
SDATA (ASN_OCTET_STR,   "path",             SDF_RD|SDF_REQUIRED,"",             "If database exists then only needs (path,[database]) params"),
SDATA (ASN_OCTET_STR,   "database",         SDF_RD,             "",             "If null, path must contains the 'database'"),
SDATA (ASN_OCTET_STR,   "filename_mask",    SDF_RD|SDF_REQUIRED,"%Y-%m-%d",    "Organization of tables (file name format, see strftime())"),

SDATA (ASN_INTEGER,     "xpermission",      SDF_RD,             02770,          "Use in creation, default 02770"),
SDATA (ASN_INTEGER,     "rpermission",      SDF_RD,             0660,           "Use in creation, default 0660"),
SDATA (ASN_INTEGER,     "on_critical_error",SDF_RD,             LOG_OPT_EXIT_ZERO,"exit on error (Zero to avoid restart)"),
SDATA (ASN_BOOLEAN,     "master",           SDF_RD,             FALSE,          "the master is the only that can write"),
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
//     SET_PRIV(xxx,                   gobj_read_pointer_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//
//     IF_EQ_SET_PRIV(xxx,             gobj_read_pointer_attr)
//     END_EQ_SET_PRIV()
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

    json_t *jn_tranger = json_pack("{s:s, s:s, s:s, s:i, s:i, s:i, s:b}",
        "path", gobj_read_str_attr(gobj, "path"),
        "database", gobj_read_str_attr(gobj, "database"),
        "filename_mask", gobj_read_str_attr(gobj, "filename_mask"),
        "xpermission", (int)gobj_read_int32_attr(gobj, "xpermission"),
        "rpermission", (int)gobj_read_int32_attr(gobj, "rpermission"),
        "on_critical_error", (int)gobj_read_int32_attr(gobj, "on_critical_error"),
        "master", gobj_read_bool_attr(gobj, "master")
    );

    priv->tranger = tranger_startup(
        jn_tranger // owned
    );
    gobj_write_pointer_attr(gobj, "tranger", priv->tranger);

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    EXEC_AND_RESET(tranger_shutdown, priv->tranger);
    gobj_write_pointer_attr(gobj, "tranger", 0);

    return 0;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_trace_on(hgobj gobj, const char *level, json_t *kw)
{
    // TODO treedb_set_trace(TRUE);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_trace_off(hgobj gobj, const char *level, json_t *kw)
{
    // TODO treedb_set_trace(FALSE);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_create_record( // Return is NOT YOURS
    hgobj gobj,
    const char *topic_name,
    json_t *kw, // owned
    const char *options // "permissive" "verbose"
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

// TODO    json_t *record = treedb_create_record( // Return is NOT YOURS
//         priv->tranger,
//         priv->treedb_name,
//         topic_name,
//         kw, // owned
//         options
//     );
//     return record;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_save_record(
    hgobj gobj,
    json_t *record
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

// TODO    return treedb_save_record(priv->tranger, record);
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_update_record( // Return is NOT YOURS
    hgobj gobj,
    const char *topic_name,
    json_t *kw,    // owned
    const char *options // "create" ["permissive"], "clean"
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

// TODO    json_t *record = treedb_update_record( // Return is NOT YOURS
//         priv->tranger,
//         priv->treedb_name,
//         topic_name,
//         kw, // owned
//         options
//     );
//     return record;
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE int mt_delete_record(hgobj gobj, const char *topic_name, json_t *kw, const char *options)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

// TODO    return treedb_delete_record(
//         priv->tranger,
//         priv->treedb_name,
//         topic_name,
//         kw, // owned
//         options
//     );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_get_record(
    hgobj gobj,
    const char *topic_name,
    const char *id,
    json_t *jn_options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

// TODO    return treedb_get_record(
//         priv->tranger,
//         priv->treedb_name,
//         topic_name,
//         id,
//         jn_options
//     );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_list_records(
    hgobj gobj,
    const char *topic_name,
    json_t *jn_filter,
    json_t *jn_options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

// TODO    return treedb_list_records(
//         priv->tranger,
//         priv->treedb_name,
//         topic_name,
//         jn_filter,
//         jn_options,
//         gobj_read_pointer_attr(gobj, "kw_match")
//     );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_record_instances(
    hgobj gobj,
    const char *topic_name,
    const char *pkey2,
    json_t *jn_filter,
    json_t *jn_options
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

// TODO    return treedb_record_instances( // Return MUST be decref
//         priv->tranger,
//         priv->treedb_name,
//         topic_name,
//         pkey2,
//         jn_filter,  // owned
//         jn_options, // owned, "collapsed"
//         gobj_read_pointer_attr(gobj, "kw_match")
//     );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_treedb_topics(hgobj gobj, const char *treedb_name, const char *options)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

//  TODO   return treedb_topics(
//         priv->tranger,
//         empty_string(treedb_name)?priv->treedb_name:treedb_name,
//         options
//     );
}

/***************************************************************************
 *      Framework Method
 ***************************************************************************/
PRIVATE json_t *mt_topic_desc(hgobj gobj, const char *topic_name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!empty_string(topic_name)) {
        return tranger_list_topic_desc(priv->tranger, topic_name);
    } else {
        // WARNING return all topics, not only treedb's topics
        return kw_incref(kw_get_dict(priv->tranger, "topics", 0, KW_REQUIRED));
    }
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
                json_sprintf("Path not found: '%s'", path),
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
PRIVATE json_t *get_schema_topic(json_t *topic_array, const char *topic_name)
{
    int idx; json_t *jn_topic;
    json_array_foreach(topic_array, idx, jn_topic) {
        const char *topic_name_ = kw_get_str(jn_topic, "topic_name", "", KW_REQUIRED);
        if(strcmp(topic_name_, topic_name)==0) {
            return jn_topic;
        }
    }
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_save_tranger_schema(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *schema = kw_get_dict_value(kw, "schema", 0, 0);
    if(!schema) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What schema?"),
            0,
            0,
            kw  // owned
        );
    }

    BOOL master = gobj_read_bool_attr(gobj, "master");
    if(!master) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Only master can write in tranger"),
            0,
            0,
            kw  // owned
        );
    }

    const char *id = kw_get_str(schema, "id", "", 0);
    const char *schema_type = kw_get_str(schema, "schema_type", "", 0);
    int new_schema_version = kw_get_int(schema, "schema_version", 0, KW_WILD_NUMBER);
    json_t *jn_topics = kw_get_list(schema, "topics", 0, 0);

/*
    "id": "control_fichador",
    "schema_type": "msg2dbs",
    "schema_version": 6,
    "topics": [
        {
            "topic_name": "control_validation",
            "topic_version": 4,
            "cols": {
*/

    if(empty_string(id)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("No schema name"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(schema_type)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("No schema type"),
            0,
            0,
            kw  // owned
        );
    }
    if(!new_schema_version) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("No schema version"),
            0,
            0,
            kw  // owned
        );
    }
    if(!jn_topics) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Schema without topics"),
            0,
            0,
            kw  // owned
        );
    }

    char schema_filename[NAME_MAX];
    if(strncmp(schema_type, "treedb", strlen("treedb"))==0) {
        snprintf(schema_filename, sizeof(schema_filename), "%s.treedb_schema.json",
            id
        );
    } else if(strncmp(schema_type, "msg2db", strlen("msg2db"))==0) {
        snprintf(schema_filename, sizeof(schema_filename), "%s.msg2db_schema.json",
            id
        );
    } else {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_PARAMETER_ERROR,
            "msg",              "%s", "Schema type unknown",
            "schema_type",      "%s", schema_type,
            NULL
        );
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Schema type unknown: '%s'", schema_type),
            0,
            0,
            kw  // owned
        );
    }

    char schema_full_path[NAME_MAX*2];
    snprintf(schema_full_path, sizeof(schema_full_path), "%s/%s",
        kw_get_str(priv->tranger, "directory", "", KW_REQUIRED),
        schema_filename
    );

    if(!file_exists(schema_full_path, 0)) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_PARAMETER_ERROR,
            "msg",              "%s", "Schema file not found",
            "path",             "%s", schema_full_path,
            NULL
        );
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Schema file not found: '%s'", schema_filename),
            0,
            0,
            kw  // owned
        );
    }

    json_t *old_jn_schema = load_json_from_file(
        schema_full_path,
        "",
        kw_get_int(priv->tranger, "on_critical_error", 0, KW_REQUIRED)
    );
    if(!old_jn_schema) {
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_PARAMETER_ERROR,
            "msg",              "%s", "Schema json failed",
            "path",             "%s", schema_full_path,
            NULL
        );
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Schema json failed: '%s'", schema_filename),
            0,
            0,
            kw  // owned
        );
    }

    json_int_t old_schema_version = kw_get_int(
        old_jn_schema,
        "schema_version",
        0,
        KW_WILD_NUMBER
    );
    if(new_schema_version <= old_schema_version) {
        JSON_DECREF(old_jn_schema);
        log_error(0,
            "gobj",             "%s", gobj_full_name(gobj),
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_PARAMETER_ERROR,
            "msg",              "%s", "Schema version lower than current",
            "path",             "%s", schema_full_path,
            "new_version",      "%d", (int)new_schema_version,
            "old_version",      "%d", (int)old_schema_version,
            NULL
        );
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Schema version lower than current"),
            0,
            0,
            kw  // owned
        );
    }

    /*
     *  Update topics
     */
    json_t *old_topics = kw_get_list(old_jn_schema, "topics", 0, KW_REQUIRED);

    int idx; json_t *jn_topic;
    json_array_foreach(jn_topics, idx, jn_topic) {
        const char *topic_name = kw_get_str(jn_topic, "topic_name", "", KW_REQUIRED);
        int new_topic_version = kw_get_int(jn_topic, "topic_version", 0, KW_WILD_NUMBER);
        json_t *old_topic = get_schema_topic(old_topics, topic_name);
        if(!old_topic) {
            /*
             *  HACK topic must be created, needs some parameters as pkey, system_flag, etc
             */
            JSON_DECREF(old_jn_schema);
            log_error(0,
                "gobj",             "%s", gobj_full_name(gobj),
                "function",         "%s", __FUNCTION__,
                "msgset",           "%s", MSGSET_PARAMETER_ERROR,
                "msg",              "%s", "Topic not exist",
                "path",             "%s", schema_full_path,
                "topic_name",       "%s", topic_name,
                NULL
            );
            return msg_iev_build_webix(gobj,
                -1,
                json_sprintf("Topic not exist"),
                0,
                0,
                kw  // owned
            );
        }
        int old_topic_version = kw_get_int(old_topic, "topic_version", 0, KW_WILD_NUMBER);
        if(new_topic_version <= old_topic_version) {
            JSON_DECREF(old_jn_schema);
            log_error(0,
                "gobj",             "%s", gobj_full_name(gobj),
                "function",         "%s", __FUNCTION__,
                "msgset",           "%s", MSGSET_PARAMETER_ERROR,
                "msg",              "%s", "Topic version lower than current",
                "path",             "%s", schema_full_path,
                "topic_name",       "%s", topic_name,
                "new_version",      "%d", (int)new_topic_version,
                "old_version",      "%d", (int)old_topic_version,
                NULL
            );
            return msg_iev_build_webix(gobj,
                -1,
                json_sprintf("Topic version lower than current"),
                0,
                0,
                kw  // owned
            );
        }

        /*
         *  Update cols and version
         */
        json_object_set_new(old_topic, "topic_version", json_sprintf("%d", new_topic_version));

        json_t *new_cols = kw_get_dict(jn_topic, "cols", 0, KW_REQUIRED);
        json_object_set(old_topic, "cols", new_cols);
    }

    json_object_set_new(
        old_jn_schema, "schema_version", json_sprintf("%d", new_schema_version)
    );

    /*
     *  Save
     */
    int ret = save_json_to_file(
        kw_get_str(priv->tranger, "directory", 0, KW_REQUIRED),
        schema_filename,
        kw_get_int(priv->tranger, "xpermission", 0, KW_REQUIRED),
        kw_get_int(priv->tranger, "rpermission", 0, KW_REQUIRED),
        kw_get_int(priv->tranger, "on_critical_error", 0, KW_REQUIRED),
        TRUE,           // Create file if not exists or overwrite.
        FALSE,          // only_read
        old_jn_schema   // owned
    );

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        ret,
        json_sprintf("%s", (ret<0)?log_last_message(): "Schema saved"),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_create_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *content = kw_get_str(kw, "content", "", 0);
    const char *content64 = kw_get_str(kw, "content64", "", 0);
    const char *options = kw_get_str(kw, "options", "", 0);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
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
                json_sprintf("Can't decode json content64"),
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
                    json_sprintf("Can't decode json content"),
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
            json_sprintf("What content?"),
            0,
            0,
            kw  // owned
        );
    }

//TODO     json_t *record = gobj_create_record(
//         gobj,
//         topic_name,
//         jn_content, // owned
//         options // options  "permissive"
//     );
//     JSON_INCREF(record);
//     return msg_iev_build_webix(gobj,
//         record?0:-1,
//         json_sprintf(record?"Tranger created!":log_last_message()),
//         0,
//         record,
//         kw  // owned
//     );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_update_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *content = kw_get_str(kw, "content", "", 0);
    const char *content64 = kw_get_str(kw, "content64", "", 0);
    const char *options = kw_get_str(kw, "options", "", 0);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
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
                json_sprintf("Can't decode json content64"),
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
                    json_sprintf("Can't decode json content"),
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
            json_sprintf("What content?"),
            0,
            0,
            kw  // owned
        );
    }

// TODO    json_t *record = gobj_update_record(
//         gobj,
//         topic_name,
//         jn_content, // owned
//         options // "permissive"
//     );
//     JSON_INCREF(record);
//     return msg_iev_build_webix(gobj,
//         record?0:-1,
//         json_sprintf(record?"Tranger update!":log_last_message()),
//         0,
//         record,
//         kw  // owned
//     );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_delete_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);
    BOOL force = kw_get_bool(kw, "force", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
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
                json_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }

    /*
     *  Get a iter of matched resources.
     */
//     json_t *iter = gobj_list_records(
//         gobj,
//         topic_name,
//         jn_filter,  // filter
//         0
//     );
//
//     if(json_array_size(iter)==0) {
//         JSON_DECREF(iter);
//         return msg_iev_build_webix(
//             gobj,
//             -1,
//             json_sprintf("Select one record please"),
//             0,
//             0,
//             kw  // owned
//         );
//     }
//
//     /*
//      *  Delete
//      */
//     json_t *jn_data = json_array();
//     int idx; json_t *record;
//     json_array_foreach(iter, idx, record) {
//         const char *id = kw_get_str(record, "id", "", KW_REQUIRED);
//
//         if(gobj_delete_record(gobj, topic_name, record, force?"force":"")<0) {
//             JSON_DECREF(iter);
//             return msg_iev_build_webix(
//                 gobj,
//                 -1,
//                 json_sprintf("Cannot delete the record %s^%s", topic_name, id),
//                 0,
//                 0,
//                 kw  // owned
//             );
//         }
//         json_array_append_new(jn_data, json_string(id));
//     }
//
//     JSON_DECREF(iter);
//
//     return msg_iev_build_webix(
//         gobj,
//         0,
//         json_sprintf("%d records deleted", idx),
//         0,
//         jn_data,
//         kw  // owned
// TODO    );
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
PRIVATE json_t *cmd_list_records(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
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
                json_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }

// TODO    json_t *records = gobj_list_records(
//         gobj,
//         topic_name,
//         jn_filter,  // owned
//         json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
//     );
//
//     return msg_iev_build_webix(
//         gobj,
//         records?0:-1,
//         json_sprintf("%d records", json_array_size(records)),
//         tranger_list_topic_desc(priv->tranger, topic_name),
//         records,
//         kw  // owned
//     );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_get_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *record_id = kw_get_str(kw, "record_id", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(record_id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What record id?"),
            0,
            0,
            kw  // owned
        );
    }

// TODO    json_t *record = gobj_get_record(
//         gobj,
//         topic_name,
//         record_id,
//         json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
//     );
//
//     return msg_iev_build_webix(gobj,
//         record?0:-1,
//         record?0:json_sprintf("Tranger not found"),
//         tranger_list_topic_desc(priv->tranger, topic_name),
//         kw_incref(record),
//         kw  // owned
//     );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_record_instances(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *record_id = kw_get_str(kw, "record_id", "", 0);
    const char *pkey2 = kw_get_str(kw, "pkey2", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);
    BOOL collapsed = !kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
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
                json_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }
    if(!empty_string(record_id)) {
        if(!jn_filter) {
            jn_filter = json_pack("{s:s}", "id", record_id);
        } else {
            json_object_set_new(jn_filter, "id", json_string(record_id));
        }
    }

// TODO    json_t *instances = gobj_record_instances(
//         gobj,
//         topic_name,
//         pkey2,
//         jn_filter,  // owned
//         json_pack("{s:b}", "collapsed", collapsed)  // jn_options, owned "collapsed"
//     );
//
//     return msg_iev_build_webix(
//         gobj,
//         0,
//         json_sprintf("%d instances", json_array_size(instances)),
//         tranger_list_topic_desc(priv->tranger, topic_name),
//         instances,
//         kw  // owned
//     );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_touch_instance(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    const char *record_id = kw_get_str(kw, "record_id", "", 0);
    const char *pkey2 = kw_get_str(kw, "pkey2", "", 0);
    const char *filter = kw_get_str(kw, "filter", "", 0);

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
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
                json_sprintf("Can't decode filter json"),
                0,
                0,
                kw  // owned
            );
        }
    }
    if(!empty_string(record_id)) {
        if(!jn_filter) {
            jn_filter = json_pack("{s:s}", "id", record_id);
        } else {
            json_object_set_new(jn_filter, "id", json_string(record_id));
        }
    }

// TODO    json_t *instances = gobj_record_instances(
//         gobj,
//         topic_name,
//         pkey2,
//         jn_filter,  // owned
//         0
//     );
//
//     /*
//      *  This is a danger operation, only one instance please
//      */
//     if(json_array_size(instances)!=1) {
//         return msg_iev_build_webix(
//             gobj,
//             -1,
//             json_sprintf("Select only one instance please"),
//             tranger_list_topic_desc(priv->tranger, topic_name),
//             instances,
//             kw  // owned
//         );
//     }
//     int idx; json_t *instance;
//     json_array_foreach(instances, idx, instance) {
//         gobj_save_record(gobj, instance);
//     }
//
//     return msg_iev_build_webix(
//         gobj,
//         0,
//         json_sprintf("%d instances touched", json_array_size(instances)),
//         tranger_list_topic_desc(priv->tranger, topic_name),
//         instances,
//         kw  // owned
//     );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_record_pkey2s(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
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
        json_sprintf("%d pkey2s", (int)json_object_size(pkey2s)),
        0,
        pkey2s,
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
    GCLASS_TRANGER_NAME,      // CHANGE WITH each gclass
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
        mt_create_record,           //mt_create_node,
        mt_update_record,           //mt_update_node,
        mt_delete_record,           //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_link_nodes2,
        0, //mt_unlink_nodes,
        0, //mt_unlink_nodes2,
        mt_get_record,              //mt_get_node,
        mt_list_records,            //mt_list_nodes,
        0, //mt_shoot_snap,
        0, //mt_activate_snap,
        0, //mt_list_snaps,
        0, //mt_treedbs,
        0, // TODO mt_treedb_topics,
        mt_topic_desc,
        0, //mt_topic_links,
        0, //mt_topic_hooks,
        0, //mt_node_parents,
        0, //mt_node_childs,
        mt_record_instances,        //mt_node_instances,
        mt_save_record,             //mt_save_node,
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
PUBLIC GCLASS *gclass_tranger(void)
{
    return &_gclass;
}
