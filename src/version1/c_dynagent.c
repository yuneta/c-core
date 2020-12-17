/***********************************************************************
 *          C_DYNAGENT.C
 *          DynAgent GClass.
 *
 *          Mini Dynamic Agent
 *
 *          Copyright (c) 2015 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include "c_dynagent.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/
#define MAXIMUM_YUNOS   1000  // Will you have enough with 1000 yunos?

/***************************************************************************
 *              Structures
 ***************************************************************************/
typedef struct cnf_cmd_s {
    const char *cmd;
    json_t * (*cmd_handle)(hgobj gobj, char *cmd, char *params, int *result);
    const char *desc;
} cnf_cmd_t;

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE json_t *cmd_list_commands(hgobj gobj, char *cmd, char *params, int *result);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE cnf_cmd_t cmds[] = {
{"list-commands",               cmd_list_commands,      "List available commands"},
{0, 0, 0}
};

PRIVATE sdata_desc_t yuno_child_yunos_it[] = {
SDATA (ASN_UNSIGNED,    "index",            SDF_NOTACCESS, 0,  "table index"),

// Got dynamically
SDATA (ASN_POINTER,     "route",            0, 0,  "route of running yuno"),
SDATA (ASN_UNSIGNED,    "pid",              SDF_RD, 0,  "pid of running yuno"),
SDATA (ASN_UNSIGNED,    "liveTime",         SDF_RD, 0,  "living time of yuno"),
SDATA (ASN_BOOLEAN,     "running",          SDF_RD, 0,  "yuno is running"),
SDATA (ASN_BOOLEAN,     "mustPlay",         SDF_RD|SDF_PERSIST, 0,  "yuno must play"),
SDATA (ASN_BOOLEAN,     "playing",          SDF_RD, 0,  "yuno is playing"),
SDATA (ASN_OCTET_STR,   "yunoRole",         SDF_RD, 0,  "yuno role"),
SDATA (ASN_OCTET_STR,   "yunoName",         SDF_RD, 0,  "yuno name"),
SDATA (ASN_UNSIGNED,    "routerPort",       SDF_RD, 0,  "routerPort of yuno"),
SDATA (ASN_UNSIGNED,    "snmpPort",         SDF_RD, 0,  "snmpPort of yuno"),
SDATA (ASN_OCTET_STR,   "yuno_version",       SDF_RD, 0,  "app version"),
SDATA (ASN_OCTET_STR,   "yunetaVersion",    SDF_RD, 0,  "yuneta version"),
SDATA (ASN_OCTET_STR,   "configfile",       SDF_RD, 0,  "config file parameter to run yuno"),
SDATA (ASN_OCTET_STR,   "roleName",         SDF_RD, 0,  "yuno role^name"),

SDATA_END()
};

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_SCHEMA,      "childyunosTb",     SDF_RD, yuno_child_yunos_it, "Dynamic yunos info table"),
SDATA (ASN_INTEGER,     "check_timeout",    0,  5*1000, "Check living timeout"),
SDATA (ASN_POINTER,     "user_data",        0,  0, "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,  0, "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,  0, "subscriber of output-events. If it's null then subscriber is the parent."),
SDATA_END()
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    int32_t check_timeout;

    htable childyunosTb;
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

    priv->timer = gobj_create("", GCLASS_TIMER, 0, gobj);
    priv->childyunosTb = gobj_read_table_attr(gobj, "childyunosTb");

    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(subscriber) {
        /*
         *  I'm NOT a child gobj
         */
        gobj_subscribe_event(gobj, NULL, NULL, subscriber);
    }
    gobj_subscribe_event(
        gobj_find_service("router", TRUE),
        0,          // event
        0,          // kw
        gobj        // src
    );

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(check_timeout,         gobj_read_int32_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(check_timeout,              gobj_read_int32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->check_timeout);

    // TODO subcribe to route events, to know if child yunos are lived.

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
    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    table_flush(priv->childyunosTb);
}

/***************************************************************************
 *      Framework Method command
 ***************************************************************************/
PRIVATE json_t *mt_command(hgobj gobj, const char *str)
{
    json_t *jn_data = 0;
    // TODO use msg_iev_build_webix

    char *p = (char *)str;
    char *cmd = get_parameter(p, &p);
    char *params = p;

    cnf_cmd_t *cnf_cmd = cmds;
    for(; cnf_cmd->cmd!= 0; cnf_cmd++) {
        if (strcasecmp(cnf_cmd->cmd, cmd) == 0) {
            break;
        }
    }
    if(!cnf_cmd->cmd) {
        jn_data = json_local_sprintf(
            "Command '%s' not available in %s service. Try 'help' command.",
            cmd,
            gobj_short_name(gobj)
        );
        return jn_data;
    }

    if(!gobj_is_running(gobj)) {
        jn_data = json_local_sprintf(
            "GObj '%s' is not running",
            gobj_short_name(gobj)
        );
        return jn_data;
    }

    jn_data = (cnf_cmd->cmd_handle)(gobj, cmd, params, result);
    return jn_data;
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *  Send simple synchronous response
 ***************************************************************************/
PRIVATE int send_simple_sync_response(
    hgobj gobj,
    hsdata src_iev,
    hsdata route,
    BOOL result,
    char *msg,
    ...)
{
    va_list ap;
    #define MAX_TEMP_BUF (16*1024)
    char *temp = gbmem_malloc(MAX_TEMP_BUF);

    va_start(ap, msg);
    vsnprintf(temp, MAX_TEMP_BUF, msg, ap);

    /*
     *  Send response inter_event
     */
    json_t *kw_resp = json_pack("{s:i, s:s}",
        "result", result,
        "info", temp
    );
    hsdata resp_iev = iev_create_response(
        src_iev,
        "EV_AGENT_SYNC_RESPONSE",
        kw_resp,
        gobj_name(gobj)
    );
    send_inter_event_by_route(
        route,
        resp_iev
    );
    va_end(ap);
    gbmem_free(temp);
    return 0;
}

/****************************************************************************
 *  Compare functions
 ****************************************************************************/
PRIVATE int cmp_by_role_name(const void *hrri1_, const void *hrri2_)
{
    hsdata hrri1 = * (hsdata * const *)hrri1_;
    hsdata hrri2 = * (hsdata * const *)hrri2_;

    char *role_name1 = sdata_read_str(hrri1, "roleName");
    char *role_name2 = sdata_read_str(hrri2, "roleName");

    return strcmp(role_name1, role_name2);
}

PRIVATE int cmp_by_routerPort(const void *hrri1_, const void *hrri2_)
{
    hsdata hrri1 = * (hsdata * const *)hrri1_;
    hsdata hrri2 = * (hsdata * const *)hrri2_;

    uint32_t routerPort1 = sdata_read_uint32(hrri1, "routerPort");
    uint32_t routerPort2 = sdata_read_uint32(hrri2, "routerPort");
    if(routerPort1 == routerPort2)
        return 0;
    return (routerPort1 > routerPort2)? 1:-1;
}
PRIVATE int cmp_by_running(const void *hrri1_, const void *hrri2_)
{
    hsdata hrri1 = * (hsdata * const *)hrri1_;
    hsdata hrri2 = * (hsdata * const *)hrri2_;

    BOOL running1 = sdata_read_bool(hrri1, "running");
    BOOL running2 = sdata_read_bool(hrri2, "running");
    return (running1 < running2)? 1:-1;
}

/***************************************************************************
 *  Walk table role_info and return in hsbf the hrri matched
 *  with a maximum of hsbf_size
 *  Order the table if option:
 *      "n" by role_name
 *      "p" by routerPort
 *      "r" by running
 *  Return number of hrri found
 ***************************************************************************/
PRIVATE int select_runTb_table(
    hgobj gobj,
    const char *role_name,
    const char *routerPort,
    hsdata *hsbf,
    int hsbf_size,
    const char *option
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(empty_string(role_name))
        role_name = "*";
    if(empty_string(routerPort))
        routerPort = "*";

    char re_routerPort[80];
    if(strcmp(routerPort, "*")==0 || strcmp(routerPort, "?")==0)
        snprintf(re_routerPort, sizeof(re_routerPort), "^.*$");
    else
        snprintf(re_routerPort, sizeof(re_routerPort), "^%s$", routerPort);

    char temp[80];
    snprintf(temp, sizeof(temp), "%s", role_name);
    change_char(temp, '^', '.');
    char re_role_name[80];
    if(strcmp(temp, "*")==0  || strcmp(temp, "?")==0)
        snprintf(re_role_name, sizeof(re_role_name), "^.*$");
    else
        snprintf(re_role_name, sizeof(re_role_name), "^%s$", temp);

    regex_t _re_role_name;
    regex_t _re_routerPort;

    if(regcomp(&_re_role_name, re_role_name, REG_EXTENDED | REG_NOSUB)!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "regcomp() FAILED",
            "re",           "%s", re_role_name,
           NULL
        );
        return 0;
    }

    if(regcomp(&_re_routerPort, re_routerPort, REG_EXTENDED | REG_NOSUB)!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "regcomp() FAILED",
            "re",           "%s", re_routerPort,
           NULL
        );
        regfree(&_re_role_name);
        return 0;
    }

    int found = 0;
    hsdata hrri;
    hrri = table_first_row(priv->childyunosTb);
    while(hrri) {
        BOOL match_role_name = FALSE;
        BOOL match_routerPort = FALSE;

        /*
         *  Check role_name
         */
        char *_role_name = sdata_read_str(hrri, "roleName");
        if (regexec(&_re_role_name, _role_name, 0, 0, 0)==0) {
            match_role_name = TRUE;
        }

        /*
         *  Check role_name
         */
        uint32_t _routerPort = sdata_read_uint32(hrri, "routerPort");
        char _srouterPort[12];
        snprintf(_srouterPort, sizeof(_srouterPort), "%d", _routerPort);
        if (regexec(&_re_routerPort, _srouterPort, 0, 0, 0)==0) {
            match_routerPort = TRUE;
        }

        /*
         *  Add to list if all match
         */
        if(match_role_name && match_routerPort) {
            if(found < hsbf_size) {
                hsbf[found] = hrri;
                found++;
            }
            if(found >= hsbf_size) {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                    "msg",          "%s", "hsdata array TOO SHORT",
                    "size",         "%d", hsbf_size,
                    NULL
                );
                break;
            }
        }

        hrri = table_next_row(hrri);
    }
    regfree(&_re_role_name);
    regfree(&_re_routerPort);

    /*
     *  Order the array
     */

    if(!empty_string(option)) {
        int (*cmp)(const void *, const void *) = 0;
        if(strcmp(option, "n")==0)
            cmp = cmp_by_role_name;
        else if(strcmp(option, "p")==0)
            cmp = cmp_by_routerPort;
        else if(strcmp(option, "r")==0)
            cmp = cmp_by_running;
        if(cmp)
            qsort(hsbf, found, sizeof(hsdata), cmp);
    }

    return found;
}

/***************************************************************************
 *  CLI command
 ***************************************************************************/
PRIVATE json_t *cmd_list_commands(hgobj gobj, char *cmd, char *params, int *result)
{
#define FORMAT_LIST_COMMANDS "%-26s %s"

    json_t *jn_data = json_array();
    json_array_append_new(
        jn_data,
        json_local_sprintf(FORMAT_LIST_COMMANDS,
            "Command",
            "Description"
        )
    );
    json_array_append_new(
        jn_data,
        json_local_sprintf(FORMAT_LIST_COMMANDS,
            "==========================",
            "======================================="
        )
    );
    cnf_cmd_t *cnf_cmd = cmds;
    for(; cnf_cmd->cmd!= 0; cnf_cmd++) {
        json_array_append_new(
            jn_data,
            json_local_sprintf(FORMAT_LIST_COMMANDS,
                cnf_cmd->cmd,
                cnf_cmd->desc
            )
        );
    }
    return jn_data;
}

/***************************************************************************
 *  get file path of running yuno extended config file
 ***************************************************************************/
PRIVATE char *get_yuno_run_cnf_path(
    hgobj gobj,
    char *bf,
    int bflen,
    const char *role,
    int routerPort,
    BOOL create_directories)
{
// TODO
return 0;
//     char cnffile[80];
//
//     snprintf(cnffile, sizeof(cnffile), "%d.json", routerPort);
//
//     char subdirs[80];
//     snprintf(subdirs, sizeof(subdirs), "%s/dyn", role);
//     return get_yuneta_work_path(
//         bf,
//         bflen,
//         RUN_YUNO_DIR,
//         subdirs,
//         cnffile,
//         create_directories
//     );
}

/***************************************************************************
 *  Build configfile
 ***************************************************************************/
PRIVATE int build_configfile(hgobj gobj, hsdata hrri, char *config)
{
    /*
     *  Inject config by level control
     */
    int yuno_routerPort = sdata_read_uint32(hrri, "routerPort");
    char *yunoRole = sdata_read_str(hrri, "yunoRole");;

    /*
     *  Check fileconfig
     */
    char configfile[PATH_MAX];
    get_yuno_run_cnf_path(
        gobj,
        configfile,
        sizeof(configfile),
        yunoRole,
        yuno_routerPort,
        TRUE
    );

    FILE *file = fopen(configfile, "w");
    if(!file) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "fopen() FAILED",
            "configfile",  "%s", configfile,
            "error",        "%d", errno,
            "strerror",     "%s", strerror(errno),
            NULL
        );
        return -1;
    }
    fprintf(file, "%s\n", config);
    fclose(file);

    sdata_write_str(hrri, "configfile", configfile);
    return 0;
}

/***************************************************************************
 *  Search the role info of routerPort
 ***************************************************************************/
PRIVATE hsdata yuno_info_search_by_routerPort(hgobj gobj, int routerPort)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    hsdata hrri = table_first_row(priv->childyunosTb);
    while(hrri) {
        uint32_t _routerPort = sdata_read_uint32(hrri, "routerPort");
        if(routerPort == _routerPort)
            return hrri;
        hrri = table_next_row(hrri);
    }
    return 0;
}




           /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_add_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    /*
     *  yuno and global dictionaries must exist.
     */
    json_t *jn_global = kw_get_dict(kw, "global", 0, 0);
    json_t *jn_yuno = kw_get_dict(kw, "yuno", 0, 0);
    if(!jn_global || !jn_yuno) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "global and yuno dictionaries MUST exist",
            NULL
        );
        KW_DECREF(kw);
        return -1;
    }
    const char *role_name = kw_get_str(jn_yuno, "__role_name__", "", 0);
    const char *yuno_name = kw_get_str(jn_yuno, "__yuno_name__", "", 0);
    int routerPort = kw_get_int(jn_yuno, "__routerPort__", 0, 0);
    int snmpPort = kw_get_int(jn_yuno, "__snmpPort__", 0, 0);
    if(empty_string(role_name) || !routerPort) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "__role_name__ and __routerPort__ MUST exist",
            NULL
        );
        KW_DECREF(kw);
        return -1;
    }

    if(yuno_info_search_by_routerPort(gobj, routerPort)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "routerPort ALREADY actived",
            "routerPort",   "%d", routerPort,
            NULL
        );
        KW_DECREF(kw);
        return -1;
    }

    hsdata hrri = table_create_row(priv->childyunosTb);
    table_append_row(hrri);

    sdata_write_str(hrri, "yunoRole", role_name);
    sdata_write_str(hrri, "yunoName", yuno_name);
    char roleName[120]; // role^name
    snprintf(roleName, sizeof(roleName), "%s^%s", role_name, yuno_name);
    sdata_write_str(hrri, "roleName", roleName);
    sdata_write_uint32(hrri, "routerPort", routerPort);
    sdata_write_uint32(hrri, "snmpPort", snmpPort);

    char myurl[80];
    snprintf(myurl, sizeof(myurl), "ws://127.0.0.1:%d", 0); // TODO conf_get_routerPort());
    json_t *jn_static_route = json_pack("[{s:s, s:s, s:b, s:b, s:[s]}]",
        "name", gobj_yuno_name(),
        "role", gobj_yuno_role(),
        "controlled_by_dynagent", 1,
        "exit_on_close", 1,
        "urls", myurl
    );
    // TODO debería añadir la ruta. Tal como está borra las que hayan.
    json_object_set_new(jn_global, "Router.static_routes", jn_static_route);

    GBUFFER *gbuf_config = gbuf_create(1024LL, 8*1024LL, 0, 0);
    // TODO size_t flags = JSON_INDENT(4);
    // TODO json2gbuf(gbuf_config, kw, flags); // kw stolen

    char bin_path[256];
    char *config = (char *)gbuf_cur_rd_pointer(gbuf_config);
    build_configfile(gobj, hrri, config);
    char *configfile = sdata_read_str(hrri, "configfile");
    char params[PATH_MAX];
    snprintf(params, sizeof(params),
        "--start -f '%s'",
        configfile
    );

#if defined(__VOS__)
    char out_file_name[80];
    char process_name[80];
    snprintf(out_file_name, sizeof(out_file_name), "%s-%s", role_name, yuno_name);
    snprintf(process_name, sizeof(process_name), "%s^%s", role_name, yuno_name);
    snprintf(bin_path, sizeof(bin_path), "%s/%s.pm",
        "/usr/local/bin/yunos",
        role_name
    );
    int ret = run_command2(
        bin_path,
        params,
        0,
        process_name,
        out_file_name,
        5,
        0
    );
#else
    snprintf(bin_path, sizeof(bin_path), "%s/%s",
        "/usr/local/bin/yunos",
        role_name
    );
    int ret = run_command(
        bin_path,
        params,
        0,
        role_name,
        5,
        0
    );
#endif
    gbuf_decref(gbuf_config);
    return ret;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_open_route(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata src_route = GET_ROUTER_ROUTE(kw);

    BOOL controlled_by_dynagent = kw_get_bool(
        kw,
        "controlled_by_dynagent",
        FALSE,
        FALSE
    );
    if(controlled_by_dynagent) {
        const char *_routerPort = kw_get_str(
            kw,
            "routerPort",
            "",
            0
        );
        int routerPort = atoi(_routerPort);
        hsdata hrri = yuno_info_search_by_routerPort(gobj, routerPort);
        if(!hrri) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "routerPort NOT FOUND",
                "routerPort",   "%d", routerPort,
                NULL
            );
            KW_DECREF(kw);
            return -1;
        }
        sdata_write_pointer(hrri, "route", src_route);
        sdata_write_pointer(src_route, "user_data2", hrri);

        BOOL playing = kw_get_bool(
            kw,
            "playing",
            FALSE,
            0
        );
        const char *yuno_version = kw_get_str(
            kw,
            "yuno_version",
            "",
            0
        );
        const char *yunetaVersion = kw_get_str(
            kw,
            "yunetaVersion",
            "",
            0
        );
        uint32_t pid = kw_get_int(
            kw,
            "pid",
            0,
            0
        );
        uint32_t live_time = kw_get_int(
            kw,
            "live-time",
            0,
            0
        );

        sdata_write_bool(hrri, "running", TRUE);
        sdata_write_bool(hrri, "playing", playing);
        sdata_write_str(hrri, "yuno_version", yuno_version);
        sdata_write_str(hrri, "yunetaVersion", yunetaVersion);
        sdata_write_uint32(hrri, "pid", pid);
        sdata_write_uint32(hrri, "liveTime", live_time);

        if(!playing) {
            BOOL must_play = sdata_read_bool(hrri, "mustPlay");
            if(must_play) {
                char *yunoRole = sdata_read_str(hrri, "yunoRole");
                char *yunoName = sdata_read_str(hrri, "yunoName");
                json_int_t filter_ref = (json_int_t)short_reference();
                json_t *ikw  = json_pack(
                    "{s:I}",
                    "__filter_reference__", (json_int_t)filter_ref
                );
                hsdata inter_event = iev_create(
                    yunoName,   // iev_dst_yuno
                    yunoRole,   // iev_dst_role
                    "router",   // iev_dst_srv
                    "EV_PLAY_YUNO",  // ievent
                    ikw,         // ikw
                    gobj_name(gobj)       // source_hgobj
                );
                if(inter_event && send_inter_event_by_route(src_route, inter_event)==0) {
                    ;
                } else {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                        "msg",          "%s", "Cannot re-play yuno",
                        NULL
                    );
                }
            }
        }

        gobj_publish_event(gobj, "EV_ON_OPEN_ROUTE", kw); // own kw
        kw = 0;
    }

    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_close_route(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    hsdata src_route = GET_ROUTER_ROUTE(kw);
    hsdata hrri = sdata_read_pointer(src_route, "user_data2");
    if(hrri) {
        hsdata ht = table_row_table(hrri);
        if(ht == priv->childyunosTb) {
            gobj_publish_event(gobj, "EV_ON_CLOSE_ROUTE", kw); // own kw
            kw = 0;
            // TODO no debería borrar el registro
            // que debería ser creado en add_yuno
            // para poder relanzar el child-yuno en caso de muerte
            // pero lo dejamos así porque los child-yunos también tienen su watcher.
            table_delete_row(hrri);
        }
    }

    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Return succesful BOOL
 ***************************************************************************/
PRIVATE int ac_play_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *role_name = kw_get_str(kw, "role_name", 0, 0);
    const char *routerPort = kw_get_str(kw, "routerPort", 0, 0);

    hsdata hsbf[MAXIMUM_YUNOS];
    int found = select_runTb_table(
        gobj,
        role_name,
        routerPort,
        hsbf,
        ARRAY_NSIZE(hsbf),
        0
    );
    if(!found) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "Cannot find any yuno to play",
            "role_name",    "%s", role_name,
            "routerPort",   "%s", routerPort,
           NULL
        );
        JSON_DECREF(kw);
        return -1;
    }

    json_t *filterlist = json_array();
    int total_played = 0;
    int total_preplayed = 0;
    for(int i=0; i<found; i++) {
        hsdata hrri = hsbf[i];
        hsdata route = sdata_read_pointer(hrri, "route");
        if(!sdata_read_bool(hrri, "mustPlay")) {
            sdata_write_bool(hrri, "mustPlay", TRUE);
            total_preplayed++;
        }
        BOOL running = sdata_read_bool(hrri, "running");
        if(!running) {
            continue;
        }
        BOOL playing = sdata_read_bool(hrri, "playing");
        if(!playing) {
            char *yunoRole = sdata_read_str(hrri, "yunoRole");
            char *yunoName = sdata_read_str(hrri, "yunoName");
            json_int_t filter_ref = (json_int_t)short_reference();
            json_t *ikw  = json_pack(
                "{s:I}",
                "__filter_reference__", (json_int_t)filter_ref
            );
            hsdata inter_event = iev_create(
                yunoName,       // iev_dst_yuno
                yunoRole,       // iev_dst_role
                "router",       // iev_dst_srv
                "EV_PLAY_YUNO", // ievent
                ikw,            // ikw
                gobj_name(gobj) // source_hgobj
            );
            send_inter_event_by_route(route, inter_event);

            json_t *jn_EvChkItem = json_object();
            json_object_set_new(
                jn_EvChkItem,
                "event",
                json_string("EV_PLAY_YUNO_ACK")
            );
            json_t *jn_filters = json_object();
            json_object_set_new(
                jn_EvChkItem,
                "filters",
                jn_filters
            );
            json_object_set_new(
                jn_filters,
                "__filter_reference__",
                json_integer(filter_ref)
            );
            json_array_append_new(filterlist, jn_EvChkItem);
            total_played++;
        }
    }
    if(!total_played && !total_preplayed) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "Cannot find any yuno to play",
            "role_name",    "%s", role_name,
            "routerPort",   "%s", routerPort,
           NULL
        );
        JSON_DECREF(filterlist);
        JSON_DECREF(kw);
        return -1;
    }

    if(!total_played) {
        JSON_DECREF(filterlist);
        JSON_DECREF(kw);
        return FALSE;
    }
    char info[80];
    snprintf(info, sizeof(info), "found %d to preplay, %d to play",
        total_preplayed,
        total_played);
    json_t *kw_counter = json_pack("{s:s, s:i, s:i, s:o}",
        "info", info,
        "max_count", total_played,
        "expiration_timeout", 2*1000,
        "input_schema", filterlist
    );
    hgobj gobj_counter = gobj_create("", GCLASS_COUNTER, kw_counter, gobj);
    json_t *kw_sub = json_pack("{s:{s:s}}",
        "__config__", "__rename_event_name__", "EV_COUNT"
    );
    gobj_subscribe_event(gobj, "EV_PLAY_YUNO_ACK", kw_sub, gobj_counter);
    gobj_start(gobj_counter);

    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Return succesful BOOL
 ***************************************************************************/
PRIVATE int ac_pause_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *role_name = kw_get_str(kw, "role_name", 0, 0);
    const char *routerPort = kw_get_str(kw, "routerPort", 0, 0);

    hsdata hsbf[MAXIMUM_YUNOS];
    int found = select_runTb_table(
        gobj,
        role_name,
        routerPort,
        hsbf,
        ARRAY_NSIZE(hsbf),
        0
    );
    if(!found) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "Cannot find any yuno to pause",
            "role_name",    "%s", role_name,
            "routerPort",   "%s", routerPort,
           NULL
        );
        JSON_DECREF(kw);
        return -1;
    }

    json_t *filterlist = json_array();
    int total_paused = 0;
    int total_prepaused = 0;
    for(int i=0; i<found; i++) {
        hsdata hrri = hsbf[i];
        hsdata route = sdata_read_pointer(hrri, "route");
        if(sdata_read_bool(hrri, "mustPlay")) {
            sdata_write_bool(hrri, "mustPlay", FALSE);
            total_prepaused++;
        }
        BOOL playing = sdata_read_bool(hrri, "playing");
        if(!playing) {
            continue;
        }

        char *yunoRole = sdata_read_str(hrri, "yunoRole");
        char *yunoName = sdata_read_str(hrri, "yunoName");
        json_int_t filter_ref = (json_int_t)short_reference();
        json_t *ikw  = json_pack(
            "{s:I}",
            "__filter_reference__", (json_int_t)filter_ref
        );
        hsdata inter_event = iev_create(
            yunoName,           // iev_dst_yuno
            yunoRole,           // iev_dst_role
            "router",           // iev_dst_srv
            "EV_PAUSE_YUNO",    // ievent
            ikw,                // ikw
            gobj_name(gobj)     // source_hgobj
        );
        send_inter_event_by_route(route, inter_event);
        json_t *jn_EvChkItem = json_object();
        json_object_set_new(
            jn_EvChkItem,
            "event",
            json_string("EV_PAUSE_YUNO_ACK")
        );
        json_t *jn_filters = json_object();
        json_object_set_new(
            jn_EvChkItem,
            "filters",
            jn_filters
        );
        json_object_set_new(
            jn_filters,
            "__filter_reference__",
            json_integer(filter_ref)
        );
        json_array_append_new(filterlist, jn_EvChkItem);
        total_paused++;
    }

    if(!total_paused && !total_prepaused) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "Cannot find any yuno to pause",
            "role_name",    "%s", role_name,
            "routerPort",   "%s", routerPort,
           NULL
        );
        JSON_DECREF(filterlist);
        JSON_DECREF(kw);
        return -1;
    }
    if(!total_paused) {
        JSON_DECREF(filterlist);
        JSON_DECREF(kw);
        return 0;
    }

    char info[80];
    snprintf(info, sizeof(info), "found %d to prepause, %d to pause",
        total_prepaused,
        total_paused);
    json_t *kw_counter = json_pack("{s:s, s:i, s:i, s:o}",
        "info", info,
        "max_count", total_paused,
        "expiration_timeout", 2*1000,
        "input_schema", filterlist
    );
    hgobj gobj_counter = gobj_create("", GCLASS_COUNTER, kw_counter, gobj);
    json_t *kw_sub = json_pack("{s:{s:s}}",
        "__config__", "__rename_event_name__", "EV_COUNT"
    );
    gobj_subscribe_event(gobj, "EV_PAUSE_YUNO_ACK", kw_sub, gobj_counter);
    gobj_start(gobj_counter);

    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *  Return succesful BOOL
 ***************************************************************************/
PRIVATE int ac_command_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *role_name = kw_get_str(kw, "role_name", 0, 0);
    const char *routerPort = kw_get_str(kw, "routerPort", 0, 0);
    const char *command = kw_get_str(kw, "command", 0, 0);
    const char *service = kw_get_str(kw, "service", 0, 0);
    hsdata src_route = GET_ROUTER_ROUTE(kw);
    hsdata src_iev = GET_INTER_EVENT(kw);

    if(empty_string(service)) {
        service = "";
    }

    /*
     *  Check if destination role is myself
     */
    if(strcmp(role_name, gobj_yuno_role())==0) {
        json_t *kw_resp = json_object();
        // TODO execute_service_command(service, command, kw_resp);

        hsdata resp_iev = iev_create_response(
            src_iev,
            "EV_AGENT_COMMAND_RESPONSE",
            kw_resp,
            gobj_name(gobj)
        );
        send_inter_event_by_route(
            src_route,
            resp_iev
        );
        JSON_DECREF(kw);
        return TRUE;
    }

    hsdata hsbf[MAXIMUM_YUNOS];
    int found = select_runTb_table(
        gobj,
        role_name,
        routerPort,
        hsbf,
        ARRAY_NSIZE(hsbf),
        0
    );
    if(!found) {
        send_simple_sync_response(
            gobj,
            src_iev,
            src_route,
            FALSE,
            "Cannot find any yuno '%s' with routerPort %s to command",
            role_name,
            routerPort
        );
        JSON_DECREF(kw);
        return FALSE;
    }

    int total_stated = 0;
    for(int i=0; i<found; i++) {
        hsdata hrri = hsbf[i];
        hsdata route = sdata_read_pointer(hrri, "route");
        BOOL running = sdata_read_bool(hrri, "running");
        if(!running) {
            continue;
        }
        char *yunoRole = sdata_read_str(hrri, "yunoRole");
        char *yunoName = sdata_read_str(hrri, "yunoName");
        json_int_t filter_ref = (json_int_t)short_reference();
        iev_incref(src_iev);
        json_t *ikw  = json_pack(
            "{s:s, s:s, s:I, s:I, s:I}",
            "command", command,
            "service", service,
            "orig_route", (json_int_t)src_route,
            "orig_iev", (json_int_t)src_iev,
            "__filter_reference__", (json_int_t)filter_ref
        );
        hsdata inter_event = iev_create(
            yunoName,       // iev_dst_yuno
            yunoRole,       // iev_dst_role
            "router",       // iev_dst_srv
            "EV_COMMAND_YUNO", // ievent
            ikw,            // ikw
            gobj_name(gobj) // source_hgobj
        );
        send_inter_event_by_route(route, inter_event);
        total_stated++;
    }
    if(!total_stated) {
        send_simple_sync_response(
            gobj,
            src_iev,
            src_route,
            FALSE,
            "No yuno found to command"
        );
        JSON_DECREF(kw);
        return FALSE;
    }

    JSON_DECREF(kw);
    return TRUE;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_play_yuno_ack(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata src_route = GET_ROUTER_ROUTE(kw);
    hsdata hrri = sdata_read_pointer(src_route, "user_data2");
    if(hrri) {
        sdata_write_bool(hrri, "playing", TRUE);
    }
    gobj_publish_event(gobj, event, kw); // own kw
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_pause_yuno_ack(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata src_route = GET_ROUTER_ROUTE(kw);
    hsdata hrri = sdata_read_pointer(src_route, "user_data2");
    if(hrri) {
        sdata_write_bool(hrri, "playing", FALSE);
    }
    gobj_publish_event(gobj, event, kw); // own kw
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_command_yuno_ack(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    //TODO peligro, debería cotejarse estos punteros recibidos contra
    // una tabla de punteros validos. Por si nos cambian la referencia desde fuera
    // o crear un hash y luego validarlo
    hsdata orig_src_iev = (hsdata)kw_get_int(kw, "orig_iev", 0, FALSE);
    hsdata orig_route = (hsdata)kw_get_int(kw, "orig_route", 0, FALSE);
    int command_result = kw_get_int(kw, "command_result", -1, FALSE);
    json_t *jn_command_data = json_object_get(kw, "command_data");
    if(!jn_command_data)
        jn_command_data = json_string("\nNo command data\n");

    /*
     *  Send response inter_event
     */
    json_t *kw_resp = json_pack("{s:i, s:o}",
        "command_result", command_result,
        "command_data", jn_command_data
    );
    hsdata resp_iev = iev_create_response(
        orig_src_iev,
        "EV_AGENT_COMMAND_RESPONSE",
        kw_resp,
        gobj_name(gobj)
    );
    send_inter_event_by_route(
        orig_route,
        resp_iev
    );
    iev_decref(orig_src_iev);

    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_final_count(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    // TODO check if yunos are lived
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Child stopped
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_ADD_YUNO",         0, 0, 0},
    {"EV_TIMEOUT",          0, 0, 0},
    {"EV_ON_OPEN_ROUTE",    0, 0, 0},
    {"EV_ON_CLOSE_ROUTE",   0, 0, 0},
    {"EV_STOPPED",          0, 0, 0},
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_OPEN_ROUTE",    0, 0, 0},
    {"EV_ON_CLOSE_ROUTE",   0, 0, 0},
    {"EV_PLAY_YUNO_ACK",    0, 0, 0},
    {"EV_PAUSE_YUNO_ACK",   0, 0, 0},
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_ADD_YUNO",         ac_add_yuno,            0},
    {"EV_ON_OPEN_ROUTE",    ac_on_open_route,       0},
    {"EV_ON_CLOSE_ROUTE",   ac_on_close_route,      0},
    {"EV_PLAY_YUNO",        ac_play_yuno,           0},
    {"EV_PAUSE_YUNO",       ac_pause_yuno,          0},
    {"EV_COMMAND_YUNO",     ac_command_yuno,        0},
    {"EV_PLAY_YUNO_ACK",    ac_play_yuno_ack,       0},
    {"EV_PAUSE_YUNO_ACK",   ac_pause_yuno_ack,      0},
    {"EV_COMMAND_YUNO_ACK", ac_command_yuno_ack,    0},
    {"EV_FINAL_COUNT",      ac_final_count,         0},
    {"EV_TIMEOUT",          ac_timeout,             0},
    {"EV_STOPPED",          ac_stopped,             0},
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
    GCLASS_DYNAGENT_NAME,
    &fsm,
    {
        mt_create,
        mt_destroy,
        mt_start,
        mt_stop,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_resource_bin,
        0, //mt_resource_desc,
        0, //mt_play,
        0, //mt_pause,
        0, //mt_resource_data,
        0, //mt_child_added,
        0, //mt_child_removed,
        0, //mt_stats,
        mt_command,
        0, //mt_create2,
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
        0, //mt_authz_allow,
        0, //mt_authz_deny,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_future38,
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
        0, //mt_future61,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0, // __acl__
    0, // s_user_trace_level
    0, // cmds
    0, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_dynagent(void)
{
    return &_gclass;
}
