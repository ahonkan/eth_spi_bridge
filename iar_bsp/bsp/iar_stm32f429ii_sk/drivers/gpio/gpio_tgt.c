/*
 * Avinash Honkan   Terabit Radios
 * 17 Jul 2014
 *
 * GPIO to Nucleus interface for initializing gpios
 * 
 *
 */


/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"


/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/

/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_gpio_init
*
*   DESCRIPTION
*
*       GPIO entry function
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*
*   INPUTS
*
*       CHAR    *key                        - Path to registry
*       INT     startstop                   - Option to Register or Unregister
*
*
*   OUTPUTS
*
*       STATUS
*
***********************************************************************/
VOID nu_bsp_drvr_gpio_init(const CHAR * key, INT startstop)
{
    VOID                        (*setup_fn)(VOID) = NU_NULL;
    STATUS                      reg_status;

    if (key != NU_NULL)
    {
        if (startstop)
        {
          /* Get setup function */
          /* If there is a setup function, call it */
          reg_status = REG_Get_UINT32_Value(key, "/setup", (UINT32 *)&setup_fn);

          if (reg_status == NU_SUCCESS && setup_fn != NU_NULL)
          {
            setup_fn();
          }
        }
    }

}

/******************************* END OF FILE *********************************/


