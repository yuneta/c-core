/****************************************************************************
 *              YUNETA_VERSION.H
 *              Copyright (c) 2013,2021 Niyamaka.
 *              All Rights Reserved.
 ****************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C"{
#endif

/*********************************************************************
 *      Version
 *********************************************************************/
#define __yuneta_version__  "5.4.7"     /* XX__yuneta_version__XX */
#define __yuneta_date__     __DATE__ " " __TIME__

#define __yuneta_long_version__ "yuneta:" __yuneta_version__ " " \
                                "ginsfsm:" __ginsfsm_version__ " " \
                                "ghelpers:" __ghelpers_version__ " "


#ifdef __cplusplus
}
#endif
