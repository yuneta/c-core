/****************************************************************************
 *          C_DYNRULE.H
 *          DynRule GClass.
 *          Copyright (c) 2015 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <ginsfsm.h>

/**rst**

.. _dynrule-gclass:

**"DynRule"** :ref:`GClass`
===========================

Description
-----------

Apply simple rules in a dict.

The rules can be changed in real time.

Based on the philosophy of the great mawk.

    [(input)?] -true-> (condition)? -true-> assign and quit
                                    [-false-> assign and quit]
    next-rule


Boolean functions
-----------------

.. function:: bool match(string regex, string variable)

   Return true if regular expression *regex* matches *variable*.

   *regex*: a string with a regular expression.

   *variable*: a string or a path-to-dict


.. function:: bool equal(string/number variable1, string/number variable2)

   Return true if *variable1* is equal to *variable2*.

   *variable1*: a string/number or a path-to-dict

   *variable2*: a string/number or a path-to-dict


.. function:: bool not_equal(string/number variable1, string/number variable2)

   Return true if *variable1* is not equal to *variable2*.

   *variable1*: a string/number or a path-to-dict

   *variable2*: a string/number or a path-to-dict


.. function:: bool greater(string/number variable1, string/number variable2)

   Return true if *variable1* is greater than *variable2*.

   *variable1*: a string/number or a path-to-dict

   *variable2*: a string/number or a path-to-dict


.. function:: bool less(string/number variable1, string/number variable2)

   Return true if *variable1* is less than *variable2*.

   *variable1*: a string/number or a path-to-dict

   *variable2*: a string/number or a path-to-dict


Assing functions
----------------

.. function:: void assign(path-to-dict variable1, string/number variable2)

   Assign *variable2* to *variable1*.

   *variable1*: a path-to-dict

   *variable2*: a string/number or a path-to-dict


.. function:: void append(array path-to-dict variable1, string/number variable2)

   Assign *variable2* to *variable1*.

   *variable1*: an array path-to-dict

   *variable2*: a string/number or a path-to-dict or a list of string/number


   ...


Prototypes
----------

.. function:: PUBLIC GCLASS *gclass_dynrule(void)

   Return a pointer to the :ref:`GCLASS` struct defining the :ref:`dynrule-gclass`.


Macros
------

``GCLASS_DYNRULE_NAME``
   Macro of the gclass string name, i.e **"DynRule"**.

``GCLASS_DYNRULE``
   Macro of the :func:`gclass_dynrule()` function.


**rst**/

#ifdef __cplusplus
extern "C"{
#endif

/*
 *  Return the GCLASS struct of "DynRule" gclass.
 */
PUBLIC GCLASS *gclass_dynrule(void);

#define GCLASS_DYNRULE_NAME "DynRule"
#define GCLASS_DYNRULE gclass_dynrule()


#ifdef __cplusplus
}
#endif
