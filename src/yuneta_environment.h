/****************************************************************************
 *              YUNETA_ENVIRONMENT.H
 *              Includes
 *              Copyright (c) 2014-2015 Niyamaka.
 *              All Rights Reserved.
 ****************************************************************************/

#ifndef _YUNETA_ENVIRONMENT_H
#define _YUNETA_ENVIRONMENT_H 1

#ifdef __cplusplus
extern "C"{
#endif

#include <ginsfsm.h>

/*********************************************************************
 *  Prototypes
 *********************************************************************/
PUBLIC int register_yuneta_environment(
    const char *work_dir,
    const char *domain_dir,
    int xpermission,    // permission for directories and executable files
    int rpermission,    // permission for regular files
    json_t *jn_config
);
PUBLIC json_t *yuneta_json_config(void);
PUBLIC int yuneta_xpermission(void);    // permission for directories and executable files
PUBLIC int yuneta_rpermission(void);    // permission for regular files

PUBLIC const char *yuneta_work_dir(void); // from environment.work_dir json config
PUBLIC const char *yuneta_domain(void);

/*
 *  Final user functions
 *  Return in `bf` the full path of file or directories of **realms**.
 *  The paths are       /work_dir/domain_dir/subdomain or
 *                      /work_dir/domain_dir/subdomain/filename
 *
 *  work_dir            from environment.work_dir json config
 *  domain_dir          from environment.domain_dir json config

 *  Schema examples for domain_dir:
 *      universe/galaxy/empire/realm
 *      domain/realm_role/realm_name
 *
 */
PUBLIC char *yuneta_realm_dir(char *bf, int bfsize, const char *subdomain, BOOL create);
PUBLIC char *yuneta_realm_file(
    char *bf,
    int bfsize,
    const char *subdomain,
    const char *filename,
    BOOL create
);

PUBLIC char *yuneta_store_dir(char *bf, int bfsize, const char *dir, const char *subdir, BOOL create);
PUBLIC char *yuneta_store_file(
    char *bf,
    int bfsize,
    const char *dir,
    const char *subdir,
    const char *filename,
    BOOL create
);

PUBLIC char *yuneta_log_dir(char *bf, int bfsize, BOOL create);
PUBLIC char *yuneta_log_file(
    char *bf,
    int bfsize,
    const char *filename,
    BOOL create
);


#ifdef __cplusplus
}
#endif


#endif

