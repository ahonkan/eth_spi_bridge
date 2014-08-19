/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       mii.c
*
*   COMPONENT
*
*       MII - Media Independent Interface control
*
*   DESCRIPTION
*
*       This file contains an implementation of the MII auto-negotiation
*       protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       MII_AutoNeg
*
*   DEPENDENCIES
*
*       nu_net.h
*       mii.h
*
*************************************************************************/

                   /* STATUS typedef */
#include "networking/nu_net.h"
#include "networking/mii.h"                    /* MII_AutoNeg, MII_MAX_PHY */


#ifndef NU_ASSERT
#define NU_ASSERT(test) ((VOID) 0)
#endif


#ifndef NU_CHECK
#define NU_CHECK(test, action) ((VOID) 0)
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
*
*   FUNCTION
*
*       MII_AutoNeg
*
*   DESCRIPTION
*
*       Using the mii_ReadMII and mii_WriteMII routines supplied by the driver
*       to access the MII registers, this routine directs the auto-negotiation
*       process of an MII PHY.
*
*   INPUTS
*
*       *deviceP                Pointer to DV_DEVICE_ENTRY for the device
*                               to pass to the supplied mii_ReadMII and
*                               mii_WriteMII routines.
*       phyAddr                 PHY address on MII bus .
*       retries                 Number of times to poll for auto-negotiation
*                               to complete, or zero to poll indefinitely.
*       *isFullDuplexP          Pointer to boolean indicating if link can be
*                               full duplex.
*       *is100Mbps              Pointer to boolean indicating if link can be
*                               100Mbps.
*       miiRead                 Driver supplied  MII Read routine.
*       miiWrite                Driver supplied  MII Write routine.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_TIMEOUT              The negotiation timed out
*       NU_NOT_PRESENT          Physical address in not in the range
*       NU_INVALID_OPERATION    Operation cannot be performed
*       NU_INVALID_FUNCTION     Invalid function to perform
*       NU_INVALID_POINTER      Pointer is invalid
*
*****************************************************************************/
STATUS MII_AutoNeg(DV_DEVICE_ENTRY* deviceP, int phyAddr,
                   unsigned long retries, int* isFullDuplexP, int* is100MbpsP,
                   mii_ReadMII miiRead, mii_WriteMII miiWrite)
{
    unsigned short miiControl;
    unsigned short miiStatus = 0;
    unsigned short miiAutoNeg = MII_ADVR_802_3;
    unsigned long tries = ((unsigned long)(0xFFFFFFFFUL));
    STATUS status = NU_SUCCESS;

    NU_CHECK((deviceP != NU_NULL) && (isFullDuplexP != NU_NULL) &&
             (is100MbpsP != NU_NULL), status = NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        NU_CHECK(((*isFullDuplexP == NU_TRUE) || (*isFullDuplexP == NU_FALSE)) &&
                 ((*is100MbpsP == NU_TRUE) || (*is100MbpsP == NU_FALSE)),
                 status = NU_INVALID_OPERATION);

        if (status == NU_SUCCESS)
        {
            NU_CHECK((miiRead != NU_NULL) && (miiWrite != NU_NULL),
                     status = NU_INVALID_FUNCTION);

            if (status == NU_SUCCESS)
            {
                NU_CHECK((phyAddr >= 0) && (phyAddr < MII_MAX_PHY),
                         status = NU_NOT_PRESENT);
            }
        }
    }

    NU_ASSERT(status == NU_SUCCESS);

    if (status == NU_SUCCESS)               /* Reset PHY via software */
    {
        status = miiRead(deviceP, phyAddr, MII_CONTROL, &miiControl);
        NU_ASSERT(status == NU_SUCCESS);

        if (status == NU_SUCCESS)
        {
            miiControl |= MII_CTRL_RESET;
            status = miiWrite(deviceP, phyAddr, MII_CONTROL, miiControl);
            NU_ASSERT(status == NU_SUCCESS);
        }
    }

    if (status == NU_SUCCESS)               /* Wait for PHY to complete reset */
    {
        do
        {
            status = miiRead(deviceP, phyAddr, MII_CONTROL, &miiControl);
            NU_ASSERT(status == NU_SUCCESS);

        } while( ((miiControl & MII_CTRL_RESET) != 0) &&
                 (status == NU_SUCCESS) );
    }

    if (status == NU_SUCCESS)               /* Power up PHY and set options */
    {
        miiControl &= ~(MII_CTRL_LOOPBACK | MII_CTRL_POWER_DOWN |
                        MII_CTRL_ISOLATE | MII_CTRL_COLL_TEST);

        miiControl |= (MII_CTRL_AUTO_NEG);

        status = miiWrite(deviceP, phyAddr, MII_CONTROL, miiControl);
        NU_ASSERT(status == NU_SUCCESS);
    }

    if (status == NU_SUCCESS)               /* Read PHY status and capabilities */
    {
        status = miiRead(deviceP, phyAddr, MII_STATUS, &miiStatus);
        NU_ASSERT(status == NU_SUCCESS);
    }

    if (status == NU_SUCCESS)
    {
        /* This driver relies on PHY being capable of auto-negotiation */
        if ((miiStatus & MII_STAT_AUTO_NEG) == 0)
            status = NU_INVALID_OPERATION;

        /* Determine PHY capabilities to be advertised during auto-negotiation */
        if ((miiStatus & MII_STAT_TX) && (*is100MbpsP == NU_TRUE))
        {
            miiAutoNeg |= MII_ADVR_TX;

            if ((miiStatus & MII_STAT_TX_FULL_DUPLEX) && (*isFullDuplexP == NU_TRUE))
                miiAutoNeg |= MII_ADVR_TX_FULL_DUPLEX;
        }

        if (miiStatus & MII_STAT_10)
        {
            miiAutoNeg |= MII_ADVR_10;

            if ((miiStatus & MII_STAT_10_FULL_DUPLEX) && (*isFullDuplexP == NU_TRUE))
                miiAutoNeg |= MII_ADVR_10_FULL_DUPLEX;
        }
    }

    if (status == NU_SUCCESS)               /* Indicate advertised capabilities */
    {
        status = miiWrite(deviceP, phyAddr, MII_ADVERTISEMENT, miiAutoNeg);
        NU_ASSERT(status == NU_SUCCESS);
    }

    if (status == NU_SUCCESS)               /* Restart auto-negotiation process */
    {
        status = miiRead(deviceP, phyAddr, MII_CONTROL, &miiControl);
        NU_ASSERT(status == NU_SUCCESS);

        if (status == NU_SUCCESS)
        {
        miiControl |= MII_CTRL_RESTART;
        status = miiWrite(deviceP, phyAddr, MII_CONTROL, miiControl);
        NU_ASSERT(status == NU_SUCCESS);
        }
    }

    if (status == NU_SUCCESS)               /* Wait for PHY to auto-negotiate */
    {
        if (retries != 0)                     /* If not infinite retries... */
            tries = retries;                   /* Initialize attempts remaining */

        do
        {
            status = miiRead(deviceP, phyAddr, MII_STATUS, &miiStatus);
            NU_ASSERT(status == NU_SUCCESS);

            if ((miiStatus & MII_STAT_REMOTE_FAULT) != 0)
                status = NU_INVALID_OPERATION;

            if (retries != 0)                   /* If not infinite retries... */
                tries -= 1;                      /* Decrement attempts remaining */

        } while ( (status == NU_SUCCESS) && (tries != 0) &&
                  ((miiStatus & MII_STAT_AUTO_NEG_DONE) == 0) );
    }

    if (status == NU_SUCCESS)               /* Obtain link partner response */
    {
        status = miiRead(deviceP, phyAddr, MII_LINK_PARTNER, &miiAutoNeg);
        NU_ASSERT(status == NU_SUCCESS);

        if ((miiAutoNeg & MII_LINK_REMOTE_FAULT) != 0)
            status = NU_INVALID_OPERATION;

        NU_ASSERT(miiAutoNeg & MII_LINK_ACK);
    }

    if (status == NU_SUCCESS)               /* Configure PHY for link partner */
    {
        status = miiRead(deviceP, phyAddr, MII_CONTROL, &miiControl);
        NU_ASSERT(status == NU_SUCCESS);
    }

    if (status == NU_SUCCESS)
    {
        /* Select 100 Mbps if this PHY is 100Base-TX capable, the driver asked for
           100Mbps and not only 10Mbps, and the link partner responded with
           100Base-TX or did not respond at all. The "no response" behavior is
           done to allow a wire to be plugged in later and have the PHY negotiate
           the best speed. */
        if ((miiStatus & MII_STAT_TX) && (*is100MbpsP == NU_TRUE) &&
            ((miiAutoNeg & MII_LINK_TX) || (miiAutoNeg & MII_LINK_ACK)))
        {
            miiControl |= MII_CTRL_100MBPS;
            *is100MbpsP = NU_TRUE;
        }
        else            /* Use only 10Mbps, because of options or link partner */
        {
            miiControl &= (~(MII_CTRL_100MBPS));
            *is100MbpsP = NU_FALSE;
        }

        if ((miiAutoNeg & MII_LINK_ACK) && (*isFullDuplexP == NU_TRUE) &&
            (((*is100MbpsP == NU_TRUE) && (miiStatus & MII_STAT_TX_FULL_DUPLEX) &&
              (miiAutoNeg & MII_LINK_TX_FULL_DUPLEX)) ||
             ((*is100MbpsP == NU_FALSE) && (miiStatus & MII_STAT_10_FULL_DUPLEX) &&
              (miiAutoNeg & MII_LINK_10_FULL_DUPLEX))))
        {
            /* Select full duplex if link partner responded and both the link partner
               and this PHY are full duplex capable */
            miiControl |= MII_CTRL_FULL_DUPLEX;
            *isFullDuplexP = NU_TRUE;
        }
        else        /* Use only half duplex, because of options or link partner */
        {
            miiControl &= (~(MII_CTRL_FULL_DUPLEX));
            *isFullDuplexP = NU_FALSE;
        }

        status = miiWrite(deviceP, phyAddr, MII_CONTROL, miiControl);
        NU_ASSERT(status == NU_SUCCESS);
    }

    if (tries == 0)
        status = NU_TIMEOUT;

  return (status);

} /* MII_AutoNeg */


#ifdef __cplusplus
}
#endif
