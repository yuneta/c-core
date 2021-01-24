/****************************************************************************
 *              YUNETA_VERSION.H
 *              Copyright (c) 2013,2018 Niyamaka.
 *              All Rights Reserved.
 ****************************************************************************/
#ifndef _YUNETA_VERSION_H
#define _YUNETA_VERSION_H 1

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      Version
 *********************************************************************/
#define __yuneta_version__  "4.7.5"
#define __yuneta_date__     __DATE__ " " __TIME__

#define __yuneta_long_version__ "yuneta:" __yuneta_version__ " " \
                                "ginsfsm:" __ginsfsm_version__ " " \
                                "ghelpers:" __ghelpers_version__ " "


#ifdef __cplusplus
}
#endif

#endif /* _YUNETA_VERSION_H */
