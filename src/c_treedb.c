/***********************************************************************
 *          C_TREEDB.C
 *          Treedb GClass.
 *
 *          Management of treedb's
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <stdio.h>
#include "c_treedb.h"

#include "treedb_system_schema.c"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE json_t *get_client_treedb_schema(
    hgobj gobj,
    const char *treedb_name,
    json_t *jn_client_treedb_schema // not owned
);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_open_treedb(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_close_treedb(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

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

PRIVATE sdata_desc_t pm_open_treedb[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "filename_mask",0,              "%Y-%m-%d",    "Organization of tables (file name format, see strftime())"),
SDATAPM (ASN_INTEGER,   "exit_on_error",0,              0,          "exit on error"),
SDATAPM (ASN_OCTET_STR, "treedb_name",  0,              0,          "Treedb name"),
SDATAPM (ASN_JSON,      "treedb_schema",0,              0,          "Initial treedb schema"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_close_treedb[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "treedb_name",  0,              0,          "Treedb name"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias-------items-----------json_fn---------description---------- */
SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,        cmd_help,       "Command's help"),
SDATACM (ASN_SCHEMA,    "authzs",           0,          pm_authzs,      cmd_authzs,     "Authorization's help"),

/*-CMD2---type----------name------------flag------------ali-items-----------json_fn-------------description--*/
SDATACM2 (ASN_SCHEMA,   "open-treedb",  SDF_AUTHZ_X,    0, pm_open_treedb,  cmd_open_treedb, "Open treedb"),
SDATACM2 (ASN_SCHEMA,   "close-treedb", SDF_AUTHZ_X,    0, pm_close_treedb, cmd_close_treedb, "Close treedb"),

SDATA_END()
};


/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "tranger",          0,                  0,              "Tranger handler"),
SDATA (ASN_OCTET_STR,   "path",             SDF_RD|SDF_REQUIRED,"",             "Path of treedbs"),
SDATA (ASN_OCTET_STR,   "filename_mask",    SDF_RD|SDF_REQUIRED,"%Y-%m",        "System organization of tables (file name format, see strftime())"),
SDATA (ASN_BOOLEAN,     "master",           SDF_RD,             FALSE,          "the master is the only that can write"),
SDATA (ASN_INTEGER,     "xpermission",      SDF_RD,             02770,          "Use in creation, default 02770"),
SDATA (ASN_INTEGER,     "rpermission",      SDF_RD,             0660,           "Use in creation, default 0660"),
SDATA (ASN_INTEGER,     "exit_on_error",    0,                  LOG_OPT_EXIT_ZERO,"exit on error"),
SDATA (ASN_POINTER,     "user_data",        0,                  0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                  0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                  0,              "subscriber of output-events. Not a child gobj."),
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
PRIVATE sdata_desc_t pm_authz_open[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATA_END()
};
PRIVATE sdata_desc_t pm_authz_create[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "treedb_name",      0,          "",             "Treedb name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authz_write[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "treedb_name",      0,          "",             "Treedb name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authz_read[] = {
/*-PM-----type--------------name----------------flag--------authpath--------description-- */
SDATAPM0 (ASN_OCTET_STR,    "treedb_name",      0,          "",             "Treedb name"),
SDATA_END()
};

PRIVATE sdata_desc_t authz_table[] = {
/*-AUTHZ-- type---------name------------flag----alias---items---------------description--*/
SDATAAUTHZ (ASN_SCHEMA, "open-close",   0,      0,      pm_authz_open,      "Permission to open-close treedb's"),
SDATAAUTHZ (ASN_SCHEMA, "create-delete",0,      0,      pm_authz_create,    "Permission to create-delete topics"),
SDATAAUTHZ (ASN_SCHEMA, "write",        0,      0,      pm_authz_write,     "Permission to write"),
SDATAAUTHZ (ASN_SCHEMA, "read",         0,      0,      pm_authz_read,      "Permission to read"),
SDATA_END()
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    hgobj gobj_tranger_system;
    hgobj gobj_node_system;
    json_t *tranger_system_;
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

    /*-----------------------------------*
     *      Create System Timeranger
     *-----------------------------------*/
    const char *filename_mask = gobj_read_str_attr(gobj, "filename_mask");
    BOOL master = gobj_read_bool_attr(gobj, "master");
    int exit_on_error = gobj_read_int32_attr(gobj, "exit_on_error");
    int xpermission = gobj_read_int32_attr(gobj, "xpermission");
    int rpermission = gobj_read_int32_attr(gobj, "rpermission");

    char path[PATH_MAX];
    build_path2(
        path,
        sizeof(path),
        gobj_read_str_attr(gobj, "path"),
        "__system__"
    );

    json_t *kw_tranger = json_pack("{s:s, s:s, s:b, s:i, s:i, s:i}",
        "path", path,
        "filename_mask", filename_mask,
        "master", master,
        "on_critical_error", exit_on_error,
        "xpermission", xpermission,
        "rpermission", rpermission
    );
    priv->gobj_tranger_system = gobj_create_service(
        "tranger_system_schema",
        GCLASS_TRANGER,
        kw_tranger,
        gobj
    );

    priv->tranger_system_ = gobj_read_pointer_attr(priv->gobj_tranger_system, "tranger");
    if(!priv->tranger_system_) {
        log_critical(priv->exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "tranger NULL",
            NULL
        );
    }

    /*-------------------------------*
     *      Create System Treedb
     *-------------------------------*/
    helper_quote2doublequote(treedb_system_schema);
    json_t *jn_treedb_system_schema;
    jn_treedb_system_schema = legalstring2json(treedb_system_schema, TRUE);
    if(!jn_treedb_system_schema) {
        exit(-1);
    }

    const char *treedb_name = kw_get_str(
        jn_treedb_system_schema,
        "id",
        "treedb_system_schema",
        KW_REQUIRED
    );
    json_t *kw_resource = json_pack("{s:I, s:s, s:o, s:i}",
        "tranger", (json_int_t)(size_t)priv->tranger_system_,
        "treedb_name", treedb_name,
        "treedb_schema", jn_treedb_system_schema,
        "exit_on_error", LOG_OPT_EXIT_ZERO
    );

    priv->gobj_node_system = gobj_create_service(
        treedb_name,
        GCLASS_NODE,
        kw_resource,
        gobj
    );

    /*
     *  HACK pipe inheritance
     */
    gobj_set_bottom_gobj(priv->gobj_node_system, priv->gobj_tranger_system);
    gobj_set_bottom_gobj(gobj, priv->gobj_node_system);

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
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

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

    /*----------------------------------------*
     *  Check AUTHZS
     *----------------------------------------*/
    const char *permission = "read";
    if(!gobj_user_has_authz(gobj, permission, json_incref(kw), src)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("No permission to '%s'", permission),
            0,
            0,
            kw  // owned
        );
    }

    return treedb_list_treedb(
        priv->tranger_system_,
        kw
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
PRIVATE json_t *cmd_open_treedb(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *filename_mask = kw_get_str(kw, "filename_mask", "", 0);
    int exit_on_error = kw_get_int(kw, "exit_on_error", 0, 0);
    const char *treedb_name = kw_get_str(kw, "treedb_name", "", 0);
    json_t *_jn_treedb_schema = kw_get_dict(kw, "treedb_schema", 0, 0);

    /*----------------------------------------*
     *  Check AUTHZS
     *----------------------------------------*/
    const char *permission = "open-close";
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

    /*-----------------------------------*
     *      Check parameters
     *-----------------------------------*/
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
    if(empty_string(filename_mask)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("What filename_mask?"),
            0,
            0,
            kw  // owned
        );
    }

    /*-----------------------------------*
     *      Get schema of __system__
     *-----------------------------------*/
    json_t *jn_client_treedb_schema = get_client_treedb_schema(
        gobj,
        treedb_name,
        _jn_treedb_schema // not owned
    );
    if(!jn_client_treedb_schema) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("treedb_schema not found: '%s'", treedb_name),
            0,
            0,
            kw  // owned
        );
    }

    /*-----------------------------------*
     *      Create Client Timeranger
     *-----------------------------------*/
    BOOL master = gobj_read_bool_attr(gobj, "master");
    int xpermission = gobj_read_int32_attr(gobj, "xpermission");
    int rpermission = gobj_read_int32_attr(gobj, "rpermission");

    char path[PATH_MAX];
    build_path2(
        path,
        sizeof(path),
        gobj_read_str_attr(gobj, "path"),
        treedb_name
    );

    json_t *kw_client_tranger = json_pack("{s:s, s:s, s:b, s:i, s:i, s:i}",
        "path", path,
        "filename_mask", filename_mask,
        "master", master,
        "on_critical_error", exit_on_error,
        "xpermission", xpermission,
        "rpermission", rpermission
    );
    // TODO crea como servicio hasta que se pueda integrar el bottom como propio
    char tranger_name[NAME_MAX];
    snprintf(tranger_name, sizeof(tranger_name), "tranger_%s", treedb_name);
    hgobj gobj_client_tranger = gobj_create_service(
        tranger_name,
        GCLASS_TRANGER,
        kw_client_tranger,
        gobj
    );

    json_t *tranger_client = gobj_read_pointer_attr(gobj_client_tranger, "tranger");
    if(!tranger_client) {
        log_critical(exit_on_error,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "tranger client NULL",
            NULL
        );

        JSON_DECREF(jn_client_treedb_schema);
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Internal error, tranger client NULL"),
            0,
            0,
            kw  // owned
        );
    }

    /*-------------------------------*
     *      Create Client Treedb
     *-------------------------------*/
    json_t *kw_resource = json_pack("{s:I, s:s, s:o, s:i}",
        "tranger", (json_int_t)(size_t)tranger_client,
        "treedb_name", treedb_name,
        "treedb_schema", jn_client_treedb_schema,
        "exit_on_error", exit_on_error
    );

    hgobj gobj_client_node = gobj_create_service(
        treedb_name,
        GCLASS_NODE,
        kw_resource,
        gobj
    );

    /*
     *  HACK pipe inheritance
     */
    gobj_set_bottom_gobj(gobj_client_node, gobj_client_tranger);

    gobj_start(gobj_client_tranger);
    gobj_start(gobj_client_node);

    return msg_iev_build_webix(gobj,
        gobj_client_node?0:-1,
        json_local_sprintf(gobj_client_node?"Treedb opened!":log_last_message()),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_close_treedb(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *treedb_name = kw_get_str(kw, "treedb_name", "", 0);

    /*----------------------------------------*
     *  Check AUTHZS
     *----------------------------------------*/
    const char *permission = "open-close";
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

    /*-----------------------------------*
     *      Check parameters
     *-----------------------------------*/
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

    /*----------------------*
     *  Close gclass Node
     *----------------------*/
    hgobj gobj_client_node = gobj_find_service(treedb_name, FALSE);
    if(!gobj_client_node) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_local_sprintf("Treedb_name not found: '%s'", treedb_name),
            0,
            0,
            kw  // owned
        );
    }

    hgobj gobj_client_tranger = gobj_bottom_gobj(gobj_client_node);
    if(gobj_is_running(gobj_client_tranger)) {
        gobj_stop(gobj_client_tranger);
    }

    if(gobj_is_running(gobj_client_node)) {
        gobj_stop(gobj_client_node);
    }
    gobj_destroy(gobj_client_tranger);
    gobj_destroy(gobj_client_node);

    return msg_iev_build_webix(gobj,
        0,
        json_local_sprintf("Treedb closed!"),
        0,
        0,
        kw  // owned
    );
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *  Build new schema, it must not exist
 ***************************************************************************/
PRIVATE int build_new_treedb_schema(
    hgobj gobj,
    const char *treedb_name,
    json_t *kw // not owned
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_int_t schema_version = kw_get_int(kw, "schema_version", 1, KW_WILD_NUMBER);

    json_t *treedb = gobj_create_node(
        priv->gobj_node_system,
        "treedbs",
        json_pack("{s:s, s:I}",
            "id", treedb_name,
            "schema_version", (json_int_t )schema_version
        ),
        json_pack("{s:b}", "refs", 1),          // fkey,hook options
        gobj
    );
    if(!treedb) {
        return -1;
    }

    json_t *jn_topics = kw_get_list(kw, "topics", 0, 0);
    int idx; json_t *jn_topic;
    json_array_foreach(jn_topics, idx, jn_topic) {
        const char *topic_name = kw_get_str(jn_topic, "id", "", 0);
        if(empty_string(topic_name)) {
            topic_name = kw_get_str(jn_topic, "topic_name", "", KW_REQUIRED);
        }
        if(empty_string(topic_name)) {
            continue;
        }
        const char *pkey = kw_get_str(jn_topic, "pkey", "id", 0);
        const char *tkey = kw_get_str(jn_topic, "tkey", "", 0);
        const char *system_flag = kw_get_str(jn_topic, "system_flag", "sf_string_key", 0);
        json_int_t topic_version = kw_get_int(jn_topic, "topic_version", 1, KW_WILD_NUMBER);
        json_t *topic_pkey2s_ = kw_get_dict_value(jn_topic, "topic_pkey2s", 0, 0);

        json_t *topic = gobj_create_node(
            priv->gobj_node_system,
            "topics",
            json_pack("{s:s, s:s, s:s, s:s, s:I}",
                "id", topic_name,
                "pkey", pkey,
                "system_flag", system_flag,
                "tkey", tkey,
                "topic_version", (json_int_t )topic_version
            ),
            json_pack("{s:b}", "refs", 1),          // fkey,hook options
            gobj
        );
        if(!topic) {
            continue;
        }
        if(topic_pkey2s_) {
            json_object_set(topic, "topic_pkey2s", topic_pkey2s_);
        }

        gobj_link_nodes(
            priv->gobj_node_system,
            "topics",               // hook
            "treedbs",              // parent_topic_name,
            json_incref(treedb),    // parent_record,owned
            "topics",               // child_topic_name,
            json_incref(topic),     // child_record,owned
            gobj
        );

        json_t *jn_cols = kwid_new_list("", jn_topic, "cols");
        if(!jn_cols) {
            json_decref(topic);
            continue;
        }

        int idx2; json_t *jn_col;
        json_array_foreach(jn_cols, idx2, jn_col) {
            const char *col_name = kw_get_str(jn_col, "id", "", KW_REQUIRED);
            if(empty_string(col_name)) {
                continue;
            }
            const char *header = kw_get_str(jn_col, "header", col_name, 0);
            json_int_t fillspace = kw_get_int(jn_col, "fillspace", 10, 0);
            const char *type = kw_get_str(jn_col, "type", "", KW_REQUIRED);
            if(empty_string(type)) {
                continue;
            }
            json_t *flag_ = kw_get_list(jn_col, "flag", json_array(), 0);
            json_t *hook_ = kw_get_dict_value(jn_col, "hook", 0, 0);
            json_t *default_ = kw_get_dict_value(jn_col, "default", 0, 0);
            const char *description = kw_get_str(jn_col, "description", 0, 0);
            json_t *properties_ = kw_get_dict_value(jn_col, "properties", 0, 0);

            json_t *kw_col = json_pack("{s:s, s:s, s:I, s:s, s:O}",
                "_id_", col_name,
                "header", header,
                "fillspace", (json_int_t)fillspace,
                "type", type,
                "flag", flag_
            );
            if(hook_) {
                json_object_set(kw_col, "hook", hook_);
            }
            if(default_) {
                json_object_set(kw_col, "default", default_);
            }
            if(description) {
                json_object_set_new(kw_col, "description", json_string(description));
            }
            if(properties_) {
                json_object_set(kw_col, "properties", properties_);
            }

            // TODO comprueba que no existe la row, solo tienes la pkey2!
            json_t *col;
//             col = gobj_get_node(
//                 priv->gobj_node_system,
//                 "cols",
//                 kw_col,
//                 json_pack("{s:b}", "refs", 1),  // fkey,hook options
//                 gobj
//             );
//             if(col) {
//                 log_error(0,
//                     "gobj",         "%s", gobj_full_name(gobj),
//                     "function",     "%s", __FUNCTION__,
//                     "msgset",       "%s", MSGSET_TREEDB_ERROR,
//                     "msg",          "%s", "Column alreade defined",
//                     "topic_name",   "%s", topic_name,
//                     NULL
//                 );
//                 json_decref(col);
//                 continue;
//             }

            col = gobj_create_node(
                priv->gobj_node_system,
                "cols",
                kw_col,
                json_pack("{s:b}", "refs", 1),  // fkey,hook options
                gobj
            );
            if(!col) {
                continue;
            }

            gobj_link_nodes(
                priv->gobj_node_system,
                "cols",                 // hook
                "topics",               // parent_topic_name,
                json_incref(topic),     // parent_record,owned
                "cols",                 // child_topic_name,
                json_incref(col),       // child_record,owned
                gobj
            );

            /*
             *  free
             */
            json_decref(col);
        }

        /*
         *  free
         */
        json_decref(jn_cols);
        json_decref(topic);
    }

    /*
     *  free
     */
    json_decref(treedb);

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *get_treedb_schema(
    hgobj gobj,
    const char *treedb_name
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *treedb = gobj_node_tree(
        priv->gobj_node_system,
        "treedbs",
        json_pack("{s:s}", "id", treedb_name),
        0,
        gobj
    );
    if(!treedb) {
        return 0;
    }

    json_t *topics = kw_get_dict(treedb, "topics", 0, 0);
    const char *topic_name; json_t *topic;
    json_object_foreach(topics, topic_name, topic) {
        json_t *cols = kw_get_dict(topic, "cols", 0, KW_EXTRACT|KW_REQUIRED);
        if(!cols) {
            continue;
        }
        /*
         *  TODO delete fkey's
         */
        json_object_del(topic, "treedbs");

        /*
         *  HACK It's a topic with rowid and pkey2
         *  TODO en este tipo de tablas en el frontend
         *      tienen que salvar sin "id" (porque es rowid)
         *
         */
        json_t *new_cols = json_object();
        const char *col_name; json_t *col;
        json_object_foreach(cols, col_name, col) {
            /*
             *  TODO get pkey2 and interchange pkey by pkey2
             *
             *  HACK The id of a rowid record is his pkey2
             */
            const char *_id_ = kw_get_str(col, "_id_", 0, KW_REQUIRED);
            if(!_id_) {
                continue;
            }
            json_object_set_new(col, "id", json_string(_id_));
            json_object_del(col, "_id_");

            /*
             *  TODO delete fkey's
             */
            json_object_del(col, "topics");

            /*
             *  Add new column, by pkey2
             */
            json_object_set(new_cols, _id_, col);
        }

        /*
         *  Set new checked cols
         */
        json_object_set_new(topic, "cols", new_cols);

        json_decref(cols);
    }

    return treedb;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *get_client_treedb_schema(
    hgobj gobj,
    const char *treedb_name,
    json_t *jn_client_treedb_schema // not owned
)
{
    /*
     *  Get the current schema
     */
    json_t *client_treedb_schema = get_treedb_schema(gobj, treedb_name);
    if(client_treedb_schema) {
        return client_treedb_schema;
    } else {
        /*
         *  Build new schema
         */
        build_new_treedb_schema(
            gobj,
            treedb_name,
            jn_client_treedb_schema
        );

        client_treedb_schema = get_treedb_schema(gobj, treedb_name);
        if(client_treedb_schema) {
            return client_treedb_schema;
        }

        /*
         *  All fails, return input parameter
         */
        return json_incref(jn_client_treedb_schema);
    }
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_open_treedb(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    // TODO
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_close_treedb(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    // TODO
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    {"EV_OPEN_TREEDB",      EVF_PUBLIC_EVENT,   0,      0},
    {"EV_CLOSE_TREEDB",     EVF_PUBLIC_EVENT,   0,      0},
    // bottom input
    {"EV_STOPPED",      0,  0,  ""},
    // internal
    {NULL, 0, 0, ""}
};
PRIVATE const EVENT output_events[] = {
    {NULL, 0, 0, ""}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_OPEN_TREEDB",          ac_open_treedb,         0},
    {"EV_CLOSE_TREEDB",         ac_close_treedb,        0},
    {"EV_STOPPED",              0,                      0},
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
    GCLASS_TREEDB_NAME,
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
        mt_treedbs,
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
PUBLIC GCLASS *gclass_treedb(void)
{
    return &_gclass;
}
