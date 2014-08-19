/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       fsl.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       FSL
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains internal routines for resolving VFS structures from
*       strings and other objects. 
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       fsl_mte_from_drive_n
*       fsl_mte_from_fqpath
*       fsl_pc_parsedrive
*       fsl_mte_from_name
*       fsl_mte_to_mntname
*                                                                       
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/fsl_extr.h"
#include "storage/util_extr.h"

MTE_S MTE_Table[MTE_MAX_TABLE_SIZE];

/************************************************************************
* FUNCTION                                                              
*     
*   fsl_mte_from_dh
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Converts a disk handle to the mount table entry.
*                                                                       
* INPUTS                                                                
*                                                                       
*   dh                              Disk handle to convert
*                                                                         
* OUTPUTS                                                               
*       
*   MTE_S*                          Pointer to MTE structure
*   NU_NULL                         Disk handle was not converted
*   
*************************************************************************/
MTE_S* fsl_mte_from_dh(UINT16 dh)
{
UINT16  idx;
MTE_S*  mte;

    /* Assume failure */
    mte = NU_NULL;

    /* Search the table for a matching disk handle */
    for(idx = 0; idx < MTE_MAX_TABLE_SIZE; idx++)
    {
        if ( MTE_Table[idx].mte_dh == dh && 
           (MTE_Table[idx].mte_flags & MTE_FL_VALID)) 
        {
            /* Assign the return pointer to the match */
            mte = &MTE_Table[idx];
            break;
        }

    }
    return mte;
}


/************************************************************************
* FUNCTION                                                              
*     
*   fsl_mte_from_drive_n
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Return the mount table entry associated with the drive number
*                                                                       
* INPUTS                                                                
*                                                                       
*   n                               Drive number to convert
*                                                                         
* OUTPUTS                                                               
*       
*   MTE_S*                          Pointer to converted MTE structure
*   NU_NULL                         Drive number could not be converted
*                                                                       
*************************************************************************/
MTE_S* fsl_mte_from_drive_n(INT16 n)
{
UINT16  idx;
MTE_S*  mte;

    /* Initialize mte so we know if the search succeeded */
    mte = NU_NULL;

   if (n != NO_DRIVE)
   {
        /* Search the mount table for a matching drive number */
        for(idx = 0; idx < MTE_MAX_TABLE_SIZE; idx++)
        {
            /* Verify that the entry is value */
            if ( (MTE_Table[idx].mte_flags & MTE_FL_VALID) && 
                 (MTE_Table[idx].mte_drive == n) )
            {
                /* Assign the return value */
                mte = &MTE_Table[idx];
                break;
            }
        }
    }       
    return mte;
}

/************************************************************************
* FUNCTION                                                              
*     
*   fsl_mte_from_fqpath                                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Returns a MTE_S* from a fully qualified path.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   fqpath*                     CHAR* to a fully qualified path   
*                                                                         
* OUTPUTS                                                               
*       
*   MTE_S*                      Pointer to MTE_S structure for the file
*                               system containing path                                                             
*                                                                       
*************************************************************************/
MTE_S* fsl_mte_from_fqpath(CHAR *path)
{
MTE_S*  mte;
INT16   n;

    /* Assume failure */
    mte = NU_NULL;

    /* Verify a path was given */
    if (!path)
        mte = NU_NULL;
    else
    {
        /* Parse the patch to get the drive number */
        n = fsl_pc_parsedrive(path, NU_FALSE);
        if (n != NO_DRIVE)
        {
            /* Convert the drive number to a mount table entry */
            mte = fsl_mte_from_drive_n(n);
        }
    }
    
    return mte;
}

/************************************************************************
* FUNCTION                                                              
*     
*   fsl_pc_parsedrive
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Parse the drive number from a input path string.
*                                                                       
* INPUTS                                                                
*                                                                       
*   *path                           Input path string
*                                                                         
* OUTPUTS                                                               
*       
*   NO_DRIVE                        Convert failed
*   INT16                           Drive number of converted string
*                                                                       
*************************************************************************/
INT16 fsl_pc_parsedrive(CHAR  *path, UINT8 use_default)
{
CHAR        *p = path;
INT16        n;

    /* Init n */
    n = NO_DRIVE;

    /* Get drive number. */
    if ( p && *p && (*(p+1) == ':') )
    {
        if ( ((*p) >= 'A') && ((*p) <= 'Z') )
            n = (INT16) (*p - 'A');

        if ( ((*p) >= 'a') && ((*p) <= 'z') )
            n = (INT16) (*p - 'a');
    }
    else
    {
        if (use_default == NU_TRUE)
        {
            /* Try to use default_drive number.  */
            n = NU_Get_Default_Drive();
        }
    }

    return n;
}

/************************************************************************
* FUNCTION                                                              
*     
*   fsl_mte_from_name
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Convert an input string to the associated mount table entry
*                                                                       
* INPUTS                                                                
*                                                                       
*   *name                           Input string to convert
*                                                                         
* OUTPUTS                                                               
*       
*   MTE_S*                          Pointer to MTE_S
*   NU_NULL                         String could not be converted
*                                                                       
*************************************************************************/
MTE_S* fsl_mte_from_name(CHAR* name)
{
INT16  n;
MTE_S* mte;

    /* Assume failure */
    mte = NU_NULL;

    /* Verify the input string is valid */
    if (!name)
        mte = NU_NULL;
    else
    {
        /* Parse the drive from the input string */
        n = fsl_pc_parsedrive(name, NU_TRUE);

        if (n != NO_DRIVE)
            /* Get the associated mount table entry */        
            mte = fsl_mte_from_drive_n(n);
    }

    return mte;
}
/************************************************************************
* FUNCTION                                                              
*     
*   fsl_mte_to_mntname
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Get the mount name for the given mte
*                                                                       
* INPUTS                                                                
*                                                                       
*   *mte                            MTE_S to convert
*   *mntname                        Point to location to copy mount name
*                                                                         
* OUTPUTS                                                               
*       
*   NU_SUCCESS                      
*   NU_NODRIVE                      MTE_S could not be converted
*   NUF_BADPARM                     Bad parameter given
*                                                                       
*************************************************************************/
STATUS fsl_mte_to_mntname(MTE_S* mte, CHAR* mntname)
{
STATUS  ret_stat = NU_SUCCESS;

    /* Verify the input string is valid */
    if (!mntname)
        ret_stat = NUF_BADPARM;
    else if (!mte)
        ret_stat = NUF_BADPARM;
    else
    {
        NUF_Copybuff(mntname, mte->mte_mount_name, MTE_MAX_MOUNT_NAME);
    }

    return (ret_stat);
}
