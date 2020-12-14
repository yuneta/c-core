/***********************************************************************
 *          TREEDBSIMPLE.H
 *
 *          Simple TreeDB file for persistent attributes
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <string.h>
#include <unistd.h>
#include "treedbsimple.h"

/***************************************************************
 *              Constants
 ***************************************************************/

/***************************************************************
 *              Structures
 ***************************************************************/

/***************************************************************
 *              Prototypes
 ***************************************************************/
PRIVATE char *get_persistent_sdata_full_filename(
    hgobj gobj,
    char *bf,
    int bflen,
    const char *label,
    BOOL create_directories
);

/***************************************************************
 *              Data
 ***************************************************************/

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE char *get_persistent_sdata_full_filename(
    hgobj gobj,
    char *bf,
    int bflen,
    const char *label,
    BOOL create_directories)
{
    char filename[NAME_MAX];
    snprintf(filename, sizeof(filename), "%s-%s-%s.json",
        gobj_gclass_name(gobj),
        gobj_name(gobj),
        label
    );

    return yuneta_realm_file(bf, bflen, "data", filename, create_directories);
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int treedb_load_persistent_attrs(hgobj gobj)
{
    char path[PATH_MAX];
    get_persistent_sdata_full_filename(gobj, path, sizeof(path), "persistent-attrs", FALSE);
    if(empty_string(path) || access(path, 0)!=0) {
        // No persistent attrs saved
        return 0;
    }
    int ret = sdata_load_persistent(gobj_hsdata(gobj), path);
    if(ret < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "dynamic data table loaded WITH ERRORS",
            "path",         "%s", path,
            NULL
        );
    }
    return ret;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int treedb_save_persistent_attrs(hgobj gobj)
{
    char path[PATH_MAX];
    get_persistent_sdata_full_filename(gobj, path, sizeof(path), "persistent-attrs", TRUE);
    if(empty_string(path)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "persistent-attrs path NULL",
            NULL
        );
        return -1;
    }

    return sdata_save_persistent(gobj_hsdata(gobj), path, yuneta_rpermission());
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int treedb_remove_persistent_attrs(hgobj gobj)
{
    char path[PATH_MAX];
    get_persistent_sdata_full_filename(gobj, path, sizeof(path), "persistent-attrs", FALSE);
    if(empty_string(path)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "path NULL",
            NULL
        );
        return -1;
    }

    if(access(path, 0)==0) {
        if(unlink(path)<0) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "unlink() FAILED",
                "path",         "%s", path,
                "errno",        "%d", errno,
                "strerror",     "%s", strerror(errno),
                NULL
            );
            return -1;
        }
    }
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE BOOL read_json_cb(
    void *user_data,
    wd_found_type type,     // type found
    char *fullpath,         // directory+filename found
    const char *directory,  // directory of found filename
    char *name,             // dname[255]
    int level,              // level of tree where file found
    int index               // index of file inside of directory, relative to 0
)
{
    json_t *jn_dict = user_data;
    size_t flags = 0;
    json_error_t error;
    json_t *jn_attr = json_load_file(fullpath, flags, &error);
    if(jn_attr) {
        json_object_set_new(jn_dict, fullpath, jn_attr);
    } else {
        jn_attr = json_local_sprintf("Invalid json in '%s' file, error '%s'", fullpath, error.text);
        json_object_set_new(jn_dict, fullpath, jn_attr);
    }
    return TRUE; // to continue
}

PUBLIC json_t * treedb_list_persistent_attrs(void)
{
    char path[PATH_MAX];
    yuneta_realm_dir(path, sizeof(path), "data", TRUE);
    json_t *jn_dict = json_object();

    walk_dir_tree(
        path,
        ".*persistent-attrs.json",
        WD_RECURSIVE|WD_MATCH_REGULAR_FILE,
        read_json_cb,
        jn_dict
    );

    return jn_dict;
}

