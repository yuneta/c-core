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
#include "yuneta_environment.h"

/***************************************************************************
 *  Prototypes
 ***************************************************************************/
PRIVATE char *multiple_dir(char *bf, int bflen, json_t *jn_l);


/***************************************************************************
 *  Data
 ***************************************************************************/
PRIVATE json_t *__jn_config__ = 0;
PRIVATE int __xpermission__ = 0;    // permission for directories and executable files
PRIVATE int __rpermission__ = 0;    // permission for regular files
PRIVATE char __work_dir__[PATH_MAX] = {0};
PRIVATE char __domain_dir__[PATH_MAX] = {0};

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
    if(work_dir) {
        strncpy(
            __work_dir__,
            work_dir,
            sizeof(__work_dir__)-1
        );
    } else {
        memset(__work_dir__, 0, sizeof(__work_dir__));
    }
    if(domain_dir) {
        strncpy(
            __domain_dir__,
            domain_dir,
            sizeof(__domain_dir__)-1
        );
    } else {
        memset(__domain_dir__, 0, sizeof(__domain_dir__));
    }
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
PUBLIC const char *yuneta_domain(void)
{
    return __domain_dir__;
}

/***************************************************************************
 *  Convert json list of names into path
 ***************************************************************************/
PRIVATE char *multiple_dir(char* bf, int bflen, json_t* jn_l)
{
    char *p = bf;
    int ln;

    *bf = 0;

    /*--------------------------------------------------------*
     *  Add domain
     *--------------------------------------------------------*/
    if(jn_l) {
        size_t index;
        json_t *jn_s_domain_name;

        if(!json_is_array(jn_l)) {
            return 0;
        }
        json_array_foreach(jn_l, index, jn_s_domain_name) {
            if(!json_is_string(jn_s_domain_name)) {
                return 0;
            }
            if(index == 0) {
                ln = snprintf(p, bflen, "%s", json_string_value(jn_s_domain_name));
            } else {
                ln = snprintf(p, bflen, "/%s", json_string_value(jn_s_domain_name));
            }
            if(ln<0) {
                *bf = 0;
                return 0;
            }

            p += ln;
            bflen -= ln;
        }
    }
    return bf;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_realm_dir(char *bf_, int bfsize, const char *subdomain, BOOL create)
{
    char *bf = bf_;

    /*
     *  Add work_dir
     */
    const char *work_dir = yuneta_work_dir();
    if(empty_string(work_dir)) {
        // Si no hay work_dir no se usa el system file
        *bf_ = 0;
        return 0;
    }
    int written = snprintf(bf, bfsize, "%s", work_dir);
    if(written > 0) {
        bf += written;
        bfsize -= written;
    }

    /*
     *  Add domain_dir
     */
    const char *domain_dir = __domain_dir__;
    if(!empty_string(domain_dir)) {
        written = snprintf(bf, bfsize, "%s%s", (*domain_dir=='/')?"":"/", domain_dir);
        if(written > 0) {
            bf += written;
            bfsize -= written;
        }
    }

    /*
     *  Add subdomain
     */
    if(!empty_string(subdomain)) {
        written = snprintf(bf, bfsize, "%s%s", (*subdomain=='/')?"":"/", subdomain);
        if(written > 0) {
            bf += written;
            bfsize -= written;
        }
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }
    return bf_;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_realm_file(char *bf_, int bfsize, const char *subdomain, const char *filename, BOOL create)
{
    char *pdir = yuneta_realm_dir(bf_, bfsize, subdomain, FALSE);
    if(empty_string(pdir)) {
        // Si no hay directorio no se usa el system file
        *bf_ = 0;
        return 0;
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }

    if(!empty_string(filename)) {
        int len = strlen(bf_);
        snprintf(bf_ + len, bfsize - len, "%s%s", (*filename=='/')?"":"/", filename);
    }

    return bf_;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_store_dir(char *bf_, int bfsize, const char *dir, const char *subdir, BOOL create)
{
    char *bf = bf_;

    /*
     *  Add work_dir
     */
    const char *work_dir = yuneta_work_dir();
    if(empty_string(work_dir)) {
        // Si no hay work_dir no se usa el system file
        *bf_ = 0;
        return 0;
    }
    int written = snprintf(bf, bfsize, "%s/store", work_dir);
    if(written > 0) {
        bf += written;
        bfsize -= written;
    }

    /*
     *  Add dir
     */
    if(!empty_string(dir)) {
        written = snprintf(bf, bfsize, "%s%s", (*dir=='/')?"":"/", dir);
        if(written > 0) {
            bf += written;
            bfsize -= written;
        }
    }

    /*
     *  Add subdir
     */
    if(!empty_string(subdir)) {
        written = snprintf(bf, bfsize, "%s%s", (*subdir=='/')?"":"/", subdir);
        if(written > 0) {
            bf += written;
            bfsize -= written;
        }
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }
    return bf_;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_store_file(
    char *bf_,
    int bfsize,
    const char *dir,
    const char *subdir,
    const char *filename,
    BOOL create)    // from environment.global_store_dir json config
{
    const char *pdir = yuneta_store_dir(bf_, bfsize, dir, subdir, FALSE);
    if(empty_string(pdir)) {
        // Si no hay directorio no se usa el system file
        *bf_ = 0;
        return 0;
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }

    if(!empty_string(filename)) {
        int len = strlen(bf_);
        snprintf(bf_ + len, bfsize - len, "%s%s", (*filename=='/')?"":"/", filename);
    }

    return bf_;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_public_dir(char *bf_, int bfsize, const char *dir, const char *subdir, BOOL create)
{
    char *bf = bf_;

    /*
     *  Add work_dir
     */
    const char *work_dir = yuneta_work_dir();
    if(empty_string(work_dir)) {
        // Si no hay work_dir no se usa el system file
        *bf_ = 0;
        return 0;
    }
    int written = snprintf(bf, bfsize, "%s/public", work_dir);
    if(written > 0) {
        bf += written;
        bfsize -= written;
    }

    /*
     *  Add dir
     */
    if(!empty_string(dir)) {
        written = snprintf(bf, bfsize, "%s%s", (*dir=='/')?"":"/", dir);
        if(written > 0) {
            bf += written;
            bfsize -= written;
        }
    }

    /*
     *  Add subdir
     */
    if(!empty_string(subdir)) {
        written = snprintf(bf, bfsize, "%s%s", (*subdir=='/')?"":"/", subdir);
        if(written > 0) {
            bf += written;
            bfsize -= written;
        }
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }
    return bf_;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_public_file(
    char *bf_,
    int bfsize,
    const char *dir,
    const char *subdir,
    const char *filename,
    BOOL create)    // from environment.global_store_dir json config
{
    const char *pdir = yuneta_public_dir(bf_, bfsize, dir, subdir, FALSE);
    if(empty_string(pdir)) {
        // Si no hay directorio no se usa el system file
        *bf_ = 0;
        return 0;
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }

    if(!empty_string(filename)) {
        int len = strlen(bf_);
        snprintf(bf_ + len, bfsize - len, "%s%s", (*filename=='/')?"":"/", filename);
    }

    return bf_;
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
    char *bf_,
    int bfsize,
    const char *filename,
    BOOL create)    // from environment.global_store_dir json config
{
    const char *pdir = yuneta_log_dir(bf_, bfsize, FALSE);
    if(empty_string(pdir)) {
        // Si no hay directorio no se usa el system file
        *bf_ = 0;
        return 0;
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }

    if(!empty_string(filename)) {
        int len = strlen(bf_);
        snprintf(bf_ + len, bfsize - len, "%s%s", (*filename=='/')?"":"/", filename);
    }

    return bf_;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_repos_yuno_dir(
    char *bf_,
    int bfsize,
    json_t *jn_classifiers,  // not owned
    const char *yuno_role,
    const char *yuno_version,
    BOOL create)
{
    char *bf = bf_;

    /*
     *  Add work_dir
     */
    const char *work_dir = yuneta_work_dir();
    if(empty_string(work_dir)) {
        // Si no hay work_dir no se usa el system file
        *bf_ = 0;
        return 0;
    }
    int written = snprintf(bf, bfsize, "%s/repos", work_dir);
    if(written > 0) {
        bf += written;
        bfsize -= written;
    }

    /*
     *  Add classifiers
     */
    if(jn_classifiers) {
        char temp[NAME_MAX];
        char *p = multiple_dir(temp, sizeof(temp), jn_classifiers);
        if(!empty_string(p)) {
            written = snprintf(bf, bfsize, "/%s", p);
            if(written > 0) {
                bf += written;
                bfsize -= written;
            }
        }
    }

    /*
     *  Add role
     */
    written = snprintf(bf, bfsize, "/%s", yuno_role);
    if(written > 0) {
        bf += written;
        bfsize -= written;
    }

    /*
     *  Add version
     */
    if(!empty_string(yuno_version)) {
        written = snprintf(bf, bfsize, "/%s", yuno_version);
        if(written > 0) {
            bf += written;
            bfsize -= written;
        }
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }
    return bf_;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC char *yuneta_repos_yuno_file(
    char* bf_,
    int bfsize,
    json_t* jn_classifiers, // not owned
    const char* yuno_role,
    const char* yuno_version,
    const char *filename,
    BOOL create)
{
    const char *pdir = yuneta_repos_yuno_dir(bf_, bfsize,
        jn_classifiers,
        yuno_role,
        yuno_version,
        FALSE
    );
    if(empty_string(pdir)) {
        // Si no hay directorio no se usa el system file
        *bf_ = 0;
        return 0;
    }

    if(create) {
        if(access(bf_, 0)!=0) {
            mkrdir(bf_, 0, yuneta_xpermission());
            if(access(bf_, 0)!=0) {
                *bf_ = 0;
                return 0;
            }
        }
    }

    if(!empty_string(filename)) {
        int len = strlen(bf_);
        snprintf(bf_ + len, bfsize - len, "%s%s", (*filename=='/')?"":"/", filename);
    }

    return bf_;
}
