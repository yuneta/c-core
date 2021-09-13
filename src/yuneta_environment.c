/****************************************************************************
 *              YUNETA_ENVIRONMENT.C
 *              Yuneta
 *
 *              Copyright (c) 2014-2015 Niyamaka.
 *              All Rights Reserved.
 ****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uuid/uuid.h>
#include "yuneta_environment.h"

/***************************************************************************
 *  Prototypes
 ***************************************************************************/

/***************************************************************************
 *  Data
 ***************************************************************************/
PRIVATE json_t *__jn_config__ = 0;
PRIVATE int __xpermission__ = 0;    // permission for directories and executable files
PRIVATE int __rpermission__ = 0;    // permission for regular files
PRIVATE char __work_dir__[PATH_MAX] = {0};
PRIVATE char __domain_dir__[PATH_MAX] = {0};
PRIVATE char uuid[256] = {0};

/***************************************************************************
 *  Register environment
 ***************************************************************************/
PUBLIC int register_yuneta_environment(
    const char *work_dir,
    const char *domain_dir,
    int xpermission,
    int rpermission,
    json_t *jn_config)
{
    __xpermission__ = xpermission;
    __rpermission__ = rpermission;

    snprintf(__work_dir__, sizeof(__work_dir__), "%s", work_dir?work_dir:"");
    snprintf(__domain_dir__, sizeof(__domain_dir__), "%s", domain_dir?domain_dir:"");

    strtolower(__work_dir__);
    strtolower(__domain_dir__);

    __jn_config__ = jn_config;

    return 0;
}


/***************************************************************************
 *
 ***************************************************************************/
PUBLIC json_t *yuneta_json_config(void)
{
    return __jn_config__;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int yuneta_xpermission(void)
{
    return __xpermission__;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int yuneta_rpermission(void)
{
    return __rpermission__;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC const char *yuneta_work_dir(void)
{
    return __work_dir__;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC const char *yuneta_domain_dir(void)
{
    return __domain_dir__;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_realm_dir(
    char *bf,
    int bfsize,
    const char *subdomain,
    BOOL create
)
{
    if(empty_string(yuneta_work_dir()) || empty_string(yuneta_domain_dir())) {
        *bf = 0;
        return 0;
    }

    build_path3(bf, bfsize, yuneta_work_dir(), yuneta_domain_dir(), subdomain);

    if(create) {
        if(access(bf, 0)!=0) {
            mkrdir(bf, 0, yuneta_xpermission());
            if(access(bf, 0)!=0) {
                *bf = 0;
                return 0;
            }
        }
    }
    return bf;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_realm_file(
    char *bf,
    int bfsize, const char *subdomain,
    const char *filename,
    BOOL create
)
{
    char realm_path[PATH_MAX];
    if(!yuneta_realm_dir(realm_path, sizeof(realm_path), subdomain, create)) {
        *bf = 0;
        return 0;
    }

    build_path2(bf, bfsize, realm_path, filename);

    return bf;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_store_dir(
    char *bf,
    int bfsize,
    const char *dir,
    const char *subdir,
    BOOL create
)
{
    if(empty_string(yuneta_work_dir())) {
        *bf = 0;
        return 0;
    }

    build_path4(bf, bfsize, yuneta_work_dir(), "store", dir, subdir);

    if(create) {
        if(access(bf, 0)!=0) {
            mkrdir(bf, 0, yuneta_xpermission());
            if(access(bf, 0)!=0) {
                *bf = 0;
                return 0;
            }
        }
    }
    return bf;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_store_file(
    char *bf,
    int bfsize,
    const char *dir,
    const char *subdir,
    const char *filename,
    BOOL create)    // from environment.global_store_dir json config
{
    char store_path[PATH_MAX];
    if(!yuneta_store_dir(store_path, sizeof(store_path), dir, subdir, create)) {
        *bf = 0;
        return 0;
    }

    build_path2(bf, bfsize, store_path, filename);

    return bf;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_log_dir(char *bf, int bfsize, BOOL create)
{
    return yuneta_realm_dir(bf, bfsize, "logs", create);
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_log_file(
    char *bf,
    int bfsize,
    const char *filename,
    BOOL create)    // from environment.global_store_dir json config
{
    char log_path[PATH_MAX];
    if(!yuneta_log_dir(log_path, sizeof(log_path), create)) {
        *bf = 0;
        return 0;
    }

    build_path2(bf, bfsize, log_path, filename);

    return bf;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_realm_store_dir(
    char *bf,
    int bfsize,
    const char *service,
    const char *owner,
    const char *realm_id,
    const char *dir,
    BOOL create
)
{
    if(empty_string(yuneta_work_dir())) {
        *bf = 0;
        return 0;
    }

    build_path6(bf, bfsize, yuneta_work_dir(), "store", service, owner, realm_id, dir);

    if(create) {
        if(access(bf, 0)!=0) {
            mkrdir(bf, 0, yuneta_xpermission());
            if(access(bf, 0)!=0) {
                *bf = 0;
                return 0;
            }
        }
    }
    return bf;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC const char *get_random_uuid(void)
{
    static char uuid_[256] = {0};

    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, uuid_);

    return uuid_;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void save_uuid(const char *uuid_)
{
    char *directory = "/yuneta/store/agent/uuid";

    json_t *jn_uuid = json_object();
    json_object_set_new(jn_uuid, "uuid", json_string(uuid_));

    save_json_to_file(
        directory,
        "uuid.json",
        02770,
        0660,
        0,
        TRUE,   //create
        FALSE,  //only_read
        jn_uuid // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
uint64_t timespec2nsec(const struct timespec *timespec)
{
    return (timespec->tv_sec * (uint64_t)1000000000) + timespec->tv_nsec;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC const char *node_uuid(void)
{
    const char *root_dir = "/dev/disk/by-uuid";
    struct dirent *dent;
    struct stat st;
    DIR *dir;
    uint64_t low_fecha = 0;
    char path[PATH_MAX];

    if(!empty_string(uuid)) {
        return uuid;
    }

    if (!(dir = opendir(root_dir))) {
        save_uuid("????1");
        return "????1";
    }

    snprintf(uuid, sizeof(uuid), "%s", "????2");

    while ((dent = readdir(dir))) {
        char *dname = dent->d_name;
        if (strlen(dname)!=36) {
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", root_dir, dname);
        if(stat(path, &st) != 0) {
            continue;
        }
        if(low_fecha == 0) {
            low_fecha = timespec2nsec(&st.st_mtim);
            snprintf(uuid, sizeof(uuid), "%s", dname);
        } else {
            if(timespec2nsec(&st.st_mtim) < low_fecha) {
                low_fecha = timespec2nsec(&st.st_mtim);
                snprintf(uuid, sizeof(uuid), "%s", dname);
            }
        }
    }

    save_uuid(uuid);
    closedir(dir);
    return uuid;
}

