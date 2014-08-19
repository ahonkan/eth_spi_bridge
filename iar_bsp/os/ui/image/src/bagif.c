/******************************************************************************
*                                                                             
*              Copyright Mentor Graphics Corporation 2006                     
*                        All Rights Reserved.                                 
*                                                                             
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS       
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS        
*  SUBJECT TO LICENSE TERMS.                                                  
*                                                                             
*                                                                             
*******************************************************************************
*******************************************************************************
*
* FILE NAME                                                             
*
*    bagif.c                                                           
*                                                                        
*  DESCRIPTION                                                           
*                                                                        
*    Does all of the processing of animated Gifs.                        
*                                                                        
*  DATA STRUCTURES                                                       
*                                                                        
*   Animated_Entry                                                           
*   NU_TASK  AnimTask[NUM_ANIMATED_GIFS];                                
*                                                                        
*  FUNCTIONS                                                             
*                                                                        
*   Create_Animated_Gif_Task - Creates a task to handle each animated
*                                  Gif.                                  
*   RS_Delete_Animated_Gif_Task - Deletes each animated Gif task that is 
*                                  currently executing.                  
*   RS_Suspend_Animated_Gif_Task- Suspends Animated Gif Tasks           
*   RS_Resume_Animated_Gif_Task - Resumes suspended animated GIF tasks  
*   AnimateGif                   - Task to handle running through animated
*                                  Gifs and displaying them accordingly. 
*   init_animated_Gif_Entry      - Initializes the animated Gif Structures.
*                                  Also Creates the Animated event Group.
*   clear_animated_Gif           - Initializes the animated Gif Structures.
*   Free_animated_Gif            - Free's all of the image buffers used  
*                                  to hold all of the image data.        
*                                                                        
*  DEPENDENCIES                                                          
*                                                                        
*   image/inc/bagif.h                                                  
*                                                     
******************************************************************************/

/* Gif Header File */
#include    "ui/bagif.h"

char bagif_remove_warning;

#ifdef IMG_INCLUDED
#ifdef NU_SIMULATION
#include "ui/noatom.h"
#endif

/* Misc function prototypes */
#include "ui/rsextern.h"


palData bRGBdata[256];

/* Animated GIF entry, usually more then one */
extern Animated_Entry *Agif;
extern UINT8 agif_first_time;
UINT32 gif_task_i[NUM_ANIMATED_GIFS];
UINT32 gif_task_j=0;
UINT8  is_task_stopped=0;
UINT8  agif_semaphore_entry=0;

/*  Task Handler */
NU_TASK  AnimTask[NUM_ANIMATED_GIFS];

NU_SEMAPHORE  Agif_Semaphore;

/* List of gif task */
Gif_Task_List   agif_task_list[NUM_ANIMATED_GIFS];
/*  event Group to handle animation */
NU_EVENT_GROUP ANIM_Group[NUM_ANIMATED_GIFS];

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      Create_Animated_Gif_Task                                     
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*       Creates the Animated Gif Tasks that will send the Agif Index    
*       Array.                                                          
*                                                                       
*  INPUTS
*                                                                       
*        name - File name
*
*  OUTPUTS
*
*        STATUS
*
*************************************************************************/
STATUS Create_Animated_Gif_Task(CHAR *name)
{

    STATUS	status = NU_TASKS_UNAVAILABLE;
    VOID	*pointer;
    UINT32  i=0,j=0;
    INT8 	buf[8];
    INT8	buf1[8];
    UNICHAR anim_name[4]={'A','N','I','M'};
 
    if(gif_task_j < NUM_ANIMATED_GIFS)
    {
        /* Search for an available TASK name */
        for(i = 0; i < NUM_ANIMATED_GIFS; i++)
        {
            if(gif_task_i[i] == i)
            {
                break;
            }
            else
            {
                continue;
            }
        } 

        if(gif_task_i[i] == i)
        {
            /* TASK is available */

            /* Check if the task name is already taken */
            while(j<NUM_ANIMATED_GIFS)
            {
                if(((STR_str_cmp((UNICHAR *)agif_task_list[j].task_name, (UNICHAR *)name)) == 0))
                {
                    status = NU_DUPLICATE_TASK_NAME;
                    break;
                }
				++j;
            }

            if(status != NU_DUPLICATE_TASK_NAME)
            {
                /*  Create the Animated GIF Task */
                if((status = GRAFIX_Allocation(&System_Memory, &pointer, (GIF_STACK_SIZE * 2), NU_NO_SUSPEND))
                    ==  NU_SUCCESS)
                {
                    /* Send i as the animated Gif to run through */
                    Agif->pointer[gif_task_i[i]] = (VOID *)pointer;
                    STR_str_cpy((UNICHAR *)agif_task_list[i].task_name, (UNICHAR *)name);
                    STR_mem_set(buf, 0, 8);
                    STR_mem_set(buf1, 0, 8);
                    STD_i_toa(gif_task_i[i], (UNICHAR *)buf1, 10);
                    STR_str_cpy((UNICHAR *)buf, (UNICHAR *)anim_name);
                    STR_str_cat((UNICHAR *)buf, (UNICHAR *)buf1);

                    /*  Create the Task */
                    if((status = NU_Create_Task(&AnimTask[gif_task_i[i]], (CHAR *)buf, AnimateGif, (UNSIGNED)gif_task_i[i], NU_NULL, pointer,
                                (GIF_STACK_SIZE * 2) , 3, 5, NU_NO_PREEMPT, NU_START)) == NU_SUCCESS)
                    {
                        ++gif_task_j;                
                        agif_task_list[i].task_number = i;
                        agif_task_list[i].task_pointer = &AnimTask[gif_task_i[i]];                    
                        if((status = NU_Create_Event_Group(&ANIM_Group[gif_task_i[i]],(CHAR *)buf)) == NU_SUCCESS)
                        {
						    gif_task_i[i] = 99;                
                        }                
                    }
                    else
                    {
                        gif_task_i[i] = 0;
                        GRAFIX_Deallocation(Agif->pointer[gif_task_i[i]]);
                    }
                }
            }
        }
    }
    return status;
}

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      RS_Delete_Animated_Gif_Task                                     
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*       Terminates and Deletes the Animated Gif Tasks that will send the
*       Agif Index Array.                                               
*                                                                       
*  INPUTS
*                                                                       
*        none.
*
*  OUTPUTS
*
*        STATUS
*
*************************************************************************/
STATUS RS_Delete_Animated_Gif_Task(CHAR *gif_task_name)
{
    STATUS		status = NU_TASK_NOT_STARTED;
	UINT32		counter, i;
	UNSIGNED	retrieved_events;

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Search for GIF task which has gif_task_name name*/
    for(counter =0; counter <= gif_task_j; counter++)
    {
        if(((STR_str_cmp((UNICHAR *)agif_task_list[counter].task_name, (UNICHAR *)gif_task_name)) == 0))
        {
            i = counter;
            status = NU_SUCCESS;
            break;
        }
        else
        {
            continue;
        }
    }
    if(status == NU_SUCCESS)
    {
	    NU_Set_Events(&ANIM_Group[i], STOP_TASK, NU_OR);
        /*  Check if Task has been started */
        if(Agif->pointer[i] != NU_NULL)
        {
    
            while(Agif->Task_Stopped[i] != 1)
            {
                NU_Sleep(1);  
            }

            /*  Terminate the Task */
            if((status = NU_Terminate_Task(&AnimTask[i])) == NU_SUCCESS)
            {
                 /*  Delete the animated GIf task processor */
                if((status = NU_Delete_Task(&AnimTask[i])) == NU_SUCCESS)
                {
                    /* Now deallocate memory */
			        GRAFIX_Deallocation(Agif->pointer[i]);

                    Agif->pointer[i] = NU_NULL;
                    Agif->agif[i]->CurrentFrame = 0;

                    /* Make this TASK Number available */
                    gif_task_i[i] = i;

                    /* Update Gif Task List */
                    STR_mem_set((UNICHAR *)agif_task_list[counter].task_name, 0, NU_MAX_NAME);
                    agif_task_list[counter].task_pointer = NU_NULL;

                    /* Decrement the task counter */
                    --gif_task_j;
                
                    /*  Delete the Event group */
                    NU_Delete_Event_Group(&ANIM_Group[i]);

                    /*  Retrieve the last event and Consume it reset the event group */
                    NU_Retrieve_Events(&ANIM_Group[i],STOP_TASK,NU_AND_CONSUME,
                                    &retrieved_events,NU_SUSPEND);
                                                
				    /* Deallocate the allocate image frame memory */
				    for(counter =0; counter < Agif->agif[i]->NumFrames; counter++)
				    {
					    if(Agif->agif[i]->imgframe[counter] != NU_NULL)
					    {
						    GRAFIX_Deallocation(Agif->agif[i]->imgframe[counter]);
						    Agif->agif[i]->imgframe[counter] = NU_NULL;
					    }
					    if(Agif->agif[i]->Lct[counter] != NU_NULL)
					    {
						    GRAFIX_Deallocation(Agif->agif[i]->Lct[counter]);
						    Agif->agif[i]->Lct[counter] = NU_NULL;
					    }
				    }
				    if(gif_task_j == 0)
				    {
					    /* No task are active */
					
					    GRAFIX_Deallocation(Agif->agif[i]);
					    /* Deallocate Memory */
					    GRAFIX_Deallocation(Agif);

					    /* Reset the global */
					    agif_first_time = 0;

				    }
				}
            }
        }
    }    

    /* Return to user mode */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      RS_Suspend_Animated_Gif_Task                                      
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*      Suspends the animated Gif tasks.  This is used for scrolling or  
*      going to hyperlinks within a page.                               
*                                                                       
*  INPUTS
*                                                                       
*        none.
*
*  OUTPUTS
*
*        STATUS
*
*************************************************************************/
STATUS RS_Suspend_Animated_Gif_Task(CHAR *gif_task_name)
{
    STATUS		status = NU_TASK_NOT_STARTED;
    UINT32      counter;

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Search for GIF task which has gif_task_name name*/
    for(counter =0; counter < gif_task_j; counter++)
    {
        if((STR_str_cmp((UNICHAR *)agif_task_list[counter].task_name, (UNICHAR *)gif_task_name)) == 0)
        {
            status = NU_SUCCESS;
            break;
        }
        else
        {
            continue;
        }
    }

    if(status == NU_SUCCESS)
    {
        status = NU_Set_Events(&ANIM_Group[counter], SUSPEND_TASK, NU_OR);

        Agif->Suspended = 1;
    }
    /* Return to user mode */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      RS_Resume_Animated_Gif_Task                                     
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*      Resumes any animated GIf suspended tasks                         
*                                                                       
*  INPUTS
*                                                                       
*        none.
*
*  OUTPUTS
*
*        STATUS
*
*************************************************************************/
STATUS RS_Resume_Animated_Gif_Task(CHAR *gif_task_name)
{
    STATUS		status = NU_TASK_NOT_STARTED;
    UINT32      counter;

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Search for GIF task which has gif_task_name name*/
    for(counter =0; counter < gif_task_j; counter++)
    {
        if((STR_str_cmp((UNICHAR *)agif_task_list[counter].task_name, (UNICHAR *)gif_task_name)) == 0)
        {
            status = NU_SUCCESS;
            break;
        }
        else
        {
            continue;
        }
    }

    if(status == NU_SUCCESS)
    {
        status = NU_Set_Events(&ANIM_Group[counter], TASK_START_AGAIN, NU_OR);

        Agif->Suspended = 0;
    }
    /* Return to user mode */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      AnimateGif                                                       
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*      Task that controls the displaying of animated Gifs.  The task is 
*      controlled by waiting for an event For it to Wake up.  Then it   
*      will process until an event that tells the task to Stop.  Then   
*      All of the memory is freed and initialized and it sends to the   
*      event sending task an event that is through.                     
*                                                                       
*  INPUTS
*                                                                       
*        argc - unsigned argument
*        argv - pointer argument
*
*  OUTPUTS
*
*        none.
*
*************************************************************************/
void AnimateGif(UNSIGNED argc, VOID *argv)
{
    STATUS		status = 0;
    INT			cindex = 0;
    SIGNED		XltTable[COLOR_PALETTE_ARRAY_SIZE];
    SIGNED		MonoTable[COLOR_PALETTE_ARRAY_SIZE];
    image       *dstPtr = 0;
    image       *monoImg;
    grafPort    *scrPort, *transPort;
    rect        transRect;
    UINT8       bLCT = NU_TRUE;
    UINT16      prev_disposal = 0;
    image       *ba_tempimage = NU_NULL;
    image       *bagif_image = NU_NULL;
    imageHeader *srcHdr;
    rect		tempRect;
    rect		temp1rect;
    INT			image_bytes;
    INT			trans_set = 0;
    UINT16		anim_index;
    UNSIGNED	retrieved_events;
    UNSIGNED    sem_count;
    VOID        *reusable;
    PenSetUp    newPenSetUp;

#ifdef NU_SIMULATION    
    UINT16      delay_time = 0;
#endif
	NU_SUPERV_USER_VARIABLES

	NU_SUPERVISOR_MODE();

    anim_index = (UINT16)argc;
    
    while(1)
    {
    	/* Transparency settings for current frame */
    	Agif->agif[anim_index]->TransParent = Agif->agif[anim_index]->transparent[Agif->agif[anim_index]->CurrentFrame];

        /*  What we must do is take all of the Animated Gifs and run through the first  */
        /*  Frames and find the max Delay of the Animated Gifs.  Then use that max delay*/
        /*  as the delay for the rotating through the images.  The sequence will then be*/ 
        /*  done where all the frames are done for each image and then it continues     */
        /*  rotating as long as no events are pressed.  Then the deallocation of memory */ 
        /*  will occur and get ready for the next onslaught of images.                  */
            
            /* Get the semaphore information */
            NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                     (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
                                  
            if(sem_count == 1)
			{
				NU_Obtain_Semaphore(&Agif_Semaphore,NU_SUSPEND);
			}

#if (BPP==8)
                
                ReadPalette(0,0,255,bRGBdata);
                
#endif
                /* Check if local color table needs to be loaded */
                if(Agif->agif[anim_index]->LCT[Agif->agif[anim_index]->CurrentFrame] == 1)
                {
                    bLCT = NU_TRUE;

                    /*  Check if the Current Frame has a local color palette */
                    XlateColors(XltTable,8,1,bRGBdata,Agif->agif[anim_index]->Lct[Agif->agif[anim_index]->CurrentFrame]->Locol_Ct);
                }
                /*
                 * Reload Global Color Table only in following cases
                 * i  - For first use
                 * ii - If Local Color Table was used last time
                 * */
                else if(Agif->agif[anim_index]->GCT == 1 && bLCT)
                {
                    bLCT = NU_FALSE;

                    /*  Check if the Current Frame has a global color palette */    
                    XlateColors(XltTable,8,1,bRGBdata,Agif->agif[anim_index]->Global_Ct);
                }
                
                if(Agif->agif[anim_index]->TransParent)
                {
                    XltTable[Agif->agif[anim_index]->transparent_Index[Agif->agif[anim_index]->CurrentFrame]]
							            = (SIGNED)0;
                    trans_set = 0;
                   
                    /*now Copy over to a Monochrome Table and set the Bit Mask Table */
                    if(!trans_set)
                    {
                        for (cindex = 0;cindex < COLOR_PALETTE_ARRAY_SIZE;cindex++)
                        {
                            MonoTable[cindex] = XltTable[cindex];
                            if(cindex != Agif->agif[anim_index]->transparent_Index[Agif->agif[anim_index]->CurrentFrame])
                            {
                                MonoTable[cindex] = 1;
                            }
                            else
                            {
                                MonoTable[cindex] = 0;
                            }                         
                        }
                    }
                }
      
                srcHdr = (imageHeader *) Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame];

                /*  We Must Modify the Header for Each Animated GIf FRAME */
                srcHdr->imWidth = Agif->agif[anim_index]->imwidth[Agif->agif[anim_index]->CurrentFrame];
                srcHdr->imHeight = Agif->agif[anim_index]->imheight[Agif->agif[anim_index]->CurrentFrame];
                srcHdr->imRowBytes = Agif->agif[anim_index]->row_bytes[Agif->agif[anim_index]->CurrentFrame];

	            status = GRAFIX_Allocation(&System_Memory,(VOID *)&bagif_image,
		                    sizeof(image),NU_NO_SUSPEND);

                tempRect = Agif->agif[anim_index]->aRect;
               
#if (BPP==8)                    
                /*  Find out how many bytes we have in the animated Gif */                                                                           
                image_bytes = XlateImage((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame],NU_NULL,8,1,NU_NULL);
	            status = GRAFIX_Allocation(&System_Memory,(VOID *)&bagif_image->imData,
		                    image_bytes - sizeof(imageHeader),NU_NO_SUSPEND);
#endif

#if (BPP==16)
                /*  Find out how many bytes we have in the animated Gif */                                                                           
                image_bytes = XlateImage((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame],NU_NULL,16,1,NU_NULL);
	            status = GRAFIX_Allocation(&System_Memory,(VOID *)&bagif_image->imData,
		                    (image_bytes - sizeof(imageHeader)) * 2,NU_NO_SUSPEND);
#endif

#if (BPP==24)
                /*  Find out how many bytes we have in the animated Gif */                                                                           
                image_bytes = XlateImage((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame],NU_NULL,24,1,NU_NULL);
	            status = GRAFIX_Allocation(&System_Memory,(VOID *)&bagif_image->imData,
		                    (image_bytes - sizeof(imageHeader)) * 3,NU_NO_SUSPEND);
#endif
#if (BPP==32)
                /*  Find out how many bytes we have in the animated Gif */                                                                           
                image_bytes = XlateImage((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame],NU_NULL,32,1,NU_NULL);
	            status = GRAFIX_Allocation(&System_Memory,(VOID *)&bagif_image->imData,
		                    (image_bytes - sizeof(imageHeader)) * 4,NU_NO_SUSPEND);
#endif

                status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstPtr,sizeof(image),NU_NO_SUSPEND);
                if(status != NU_SUCCESS)
                {
                    /* Get the semaphore information */
                    NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                             (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
                    
                    /*BRW_Show_Errors(&qdop,status);*/
					if(sem_count == 0)
					{
						NU_Release_Semaphore(&Agif_Semaphore);
					}
                    return;
                }

                status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstPtr->imData,image_bytes,NU_NO_SUSPEND);    
                if(status != NU_SUCCESS)
                {
                
                    /* Get the semaphore information */
                    NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                             (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
                                             
                    /*BRW_Show_Errors(&qdop,status);*/
					if(sem_count == 0)
					{
						NU_Release_Semaphore(&Agif_Semaphore);
					}
                    return;
                }

                status = GRAFIX_Allocation(&System_Memory,(VOID *)&ba_tempimage,sizeof(image),NU_NO_SUSPEND);
                if(status != NU_SUCCESS)
                {
                    /* Get the semaphore information */
                    NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                             (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
                
                    /*BRW_Show_Errors(&qdop,status);*/
					if(sem_count == 0)
					{
						NU_Release_Semaphore(&Agif_Semaphore);
					}
                    return;
                }

                ba_tempimage->imWidth = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imWidth;
                ba_tempimage->imHeight = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imHeight;
                ba_tempimage->imAlign = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imAlign;
                ba_tempimage->imFlags = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imFlags;
                ba_tempimage->pad = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->pad;
                ba_tempimage->imBytes = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imBytes;
                ba_tempimage->imBits = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imBits;
                ba_tempimage->imPlanes = ((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imPlanes;
                ba_tempimage->imData = (UINT8 *)(&(((image *)Agif->agif[anim_index]->imgframe[Agif->agif[anim_index]->CurrentFrame])->imData));

                STR_mem_set(dstPtr->imData, 0, image_bytes);

                if(status != NU_SUCCESS)
                {
                    /* Get the semaphore information */
                    NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                             (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
					
					if(sem_count == 0)
					{
						NU_Release_Semaphore(&Agif_Semaphore);
					}
                    return;
                }
                
                if((Agif->agif[anim_index]->TransParent) && (trans_set == 0))
                {

                    status = GRAFIX_Allocation(&System_Memory,(VOID *)&monoImg,
                                             image_bytes,NU_NO_SUSPEND);
                    if (status != NU_SUCCESS)
                    {
                        /* Get the semaphore information */
                        NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                                 (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
                                             
						if(sem_count == 0)
						{
							NU_Release_Semaphore(&Agif_Semaphore);
						}
                        return;
                    }

                    status = GRAFIX_Allocation(&System_Memory,(VOID *)&monoImg->imData,
                                             image_bytes,NU_NO_SUSPEND);
                
                    if (status != NU_SUCCESS)
                    {
                        /* Get the semaphore information */
                        NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                                 (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
                                                 
                        /*BRW_Show_Errors(&qdop,status);*/
						if(sem_count == 0)
						{
							NU_Release_Semaphore(&Agif_Semaphore);
						}
                        return;
                    }

                    STR_mem_set(monoImg->imData, 0, image_bytes);
                    XlateImage(ba_tempimage,monoImg,1,1,MonoTable);
                }
                
#if (BPP == 8)
                /*  Translate the destination Image */
                XlateImage(ba_tempimage,dstPtr,8,1,XltTable);
#endif

#if (BPP == 16)
                /*  Translate the destination Image */
                XlateImage(ba_tempimage,dstPtr,16,1,XltTable);
#endif

#if (BPP == 24)
                /*  Translate the destination Image */
                XlateImage(ba_tempimage,dstPtr,24,1,XltTable);
#endif
#if (BPP == 32)
                /*  Translate the destination Image */
                XlateImage(ba_tempimage,dstPtr,32,1,XltTable);
#endif
                
                /*  Setup the Rectangle */
                tempRect.Xmin += Agif->agif[anim_index]->x_origin[Agif->agif[anim_index]->CurrentFrame];
                tempRect.Xmax = tempRect.Xmin+srcHdr->imWidth;
                tempRect.Ymin += Agif->agif[anim_index]->y_origin[Agif->agif[anim_index]->CurrentFrame];
                tempRect.Ymax = tempRect.Ymin +srcHdr->imHeight;

                /* Check disposal method of previous frame */
                if( RESTORE_BACKGROUND == prev_disposal )
                    RS_Rectangle_Draw(ERASE,&temp1rect,-1,0,0);
                else if(RESTORE_PREVIOUS == prev_disposal )
                    WriteImage(&temp1rect,bagif_image);

                /*RasterOp(zREPz);*/
                ReadImage(&tempRect, bagif_image);
                if(Agif->agif[anim_index]->TransParent && trans_set==0)
                {
                    transRect.Xmin = 0;
                    transRect.Ymin = 0;
                    transRect.Xmax = srcHdr->imWidth;
                    transRect.Ymax = srcHdr->imHeight;

                    /* Get current screen port and create its copy for off-screen drawing */
                    GetPort(&scrPort);
                    transPort = CreateBitmap(cMEMORY, srcHdr->imWidth, srcHdr->imHeight);
                    CopyBlit(scrPort, transPort, &tempRect, &transRect);
                    SetPort(transPort);

                    /* Draw monochrome image off-screen and XOR with source image for transparency */
                    RS_Get_Pen_Setup(&newPenSetUp);
                    RS_Pen_Setup(&newPenSetUp, BRW_MONOCHROME_TRANSPARENT_COLOR);
                    RasterOp(xREPx);
                    WriteImage(&transRect,monoImg);
                    RasterOp(zXORz);
                    WriteImage(&transRect,dstPtr);
                    RasterOp(zREPz);

                    /* Set the screen port to original and copy the off-screen drawn image */
                    SetPort(scrPort);
                    CopyBlit(transPort, scrPort, &transRect, &tempRect);
                    DestroyBitmap(transPort);
                }
                else
                {
                    if(Agif->agif[anim_index]->TransParent)
                    {
                        
                        if(Agif->agif[anim_index]->CurrentFrame == 0)
                        {
                            temp1rect = tempRect;
                        }
                        
                        if(tempRect.Xmax > temp1rect.Xmax)
                        {
                            RS_Get_Pen_Setup(&newPenSetUp);
                            RS_Pen_Setup(&newPenSetUp, Black);
                            RS_Rectangle_Draw( FILL,(&(Agif->agif[anim_index]->aRect)),1,0,0);
                            RS_Pen_Setup(&newPenSetUp, BRW_MONOCHROME_TRANSPARENT_COLOR);
                        }
                    }
                    /*  Or just write the Image */
                    WriteImage(&tempRect,dstPtr);
                }
                
                /* Free the Memory */
                if(Agif->agif[anim_index]->TransParent)
                {
                    GRAFIX_Deallocation(monoImg->imData);
                    GRAFIX_Deallocation(monoImg);
                }
                GRAFIX_Deallocation(dstPtr->imData);
				GRAFIX_Deallocation(dstPtr);
                GRAFIX_Deallocation(ba_tempimage);
                dstPtr = NU_NULL;
              
              /*  Sleep for the Max delay */
              Agif->agif[anim_index]->delay[Agif->agif[anim_index]->CurrentFrame]=(Agif->agif[anim_index]->delay[Agif->agif[anim_index]->CurrentFrame] & 0xff);
              
              /* Get the semaphore information */
              NU_Semaphore_Information( &Agif_Semaphore, (CHAR *)&reusable, &sem_count, 
                                       (OPTION *) &reusable, (UNSIGNED *) &reusable, (NU_TASK **) &reusable);
                                       
			  if(sem_count == 0)
			  {
				  NU_Release_Semaphore(&Agif_Semaphore);
			  }
#ifdef NU_SIMULATION			  
              delay_time = Agif->agif[anim_index]->delay[Agif->agif[anim_index]->CurrentFrame]>>3;
              NU_Sleep((UNSIGNED)delay_time);                       
#else
              NU_Sleep((UNSIGNED)BRW_HUNDREDTH_SECOND * Agif->agif[anim_index]->delay[Agif->agif[anim_index]->CurrentFrame]);
#endif             
              /*  Store the current Image Rect and Store it for a later comparison */
              temp1rect = tempRect;
             
              /*  Bump the Current Frame of the Current Image */
              Agif->agif[anim_index]->CurrentFrame += 1;

              if(Agif->agif[anim_index]->CurrentFrame >= Agif->agif[anim_index]->NumFrames)
              {
                  /*  If the Frame is equal to the Max Frame then set it back to 0 */
                  Agif->agif[anim_index]->CurrentFrame = 0;
              }

              /* Save the disposal method of this frame to be used in drawing next frame */
              prev_disposal = Agif->agif[anim_index]->disposal[Agif->agif[anim_index]->CurrentFrame];

              GRAFIX_Deallocation(bagif_image->imData);
              GRAFIX_Deallocation(bagif_image); 

              if(NU_Retrieve_Events(&ANIM_Group[anim_index],STOP_TASK,NU_AND_CONSUME,
                  &retrieved_events,NU_NO_SUSPEND) == NU_SUCCESS)
              {
                  
                  Agif->Task_Stopped[anim_index] = 1;
                  while(1)
                  {
                      NU_Sleep(1);
					  
                  }
              }
              else if(NU_Retrieve_Events(&ANIM_Group[anim_index],SUSPEND_TASK,NU_AND_CONSUME,
                  &retrieved_events,NU_NO_SUSPEND) == NU_SUCCESS)
              {
                  NU_Retrieve_Events(&ANIM_Group[anim_index],TASK_START_AGAIN,NU_AND_CONSUME,
                      &retrieved_events,NU_SUSPEND);
                  Agif->agif[anim_index]->CurrentFrame= 0;
              }			  
    }/* While (1)*/
}

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      init_animated_Gif_Entry                                                
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*      Initializes the animated Gif Structure.                             
*                                                                       
*  INPUTS
*                                                                       
*        none.
*
*  OUTPUTS
*
*        none.
*
*************************************************************************/
STATUS init_animated_Gif_Entry(void)
{
	STATUS  status = 0;
	int i=0;

	if((status = GRAFIX_Allocation(&System_Memory,(VOID **)&Agif,
		                     sizeof(Animated_Entry), NU_NO_SUSPEND)) == NU_SUCCESS)
    {
        for (i=0;i <NUM_ANIMATED_GIFS; i++)
        {
            gif_task_i[i] = i;
            Agif->agif[i] = NU_NULL;
            
            STR_mem_set((UNICHAR *)agif_task_list[i].task_name, 0, NU_MAX_NAME);
            agif_task_list[i].task_number = 0;
            agif_task_list[i].task_pointer = NU_NULL;
        }
        Agif->NumAGifs=0;
    }

    if(status == NU_SUCCESS)
    {
        /* Create the synchronization semaphore only once */
        if(agif_semaphore_entry == 0)
        {
            /* Create Display synchronization semaphore.  */
            if((status = NU_Create_Semaphore(&Agif_Semaphore, "Display", 1, NU_FIFO)) == NU_SUCCESS)
            {
                agif_semaphore_entry = 1;
            }
        }
    }

    return status;
}

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      init_animated_Gif                                                
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*      Initializes the animated Gif Structure.                             
*                                                                       
*  INPUTS
*                                                                       
*        none.
*
*  OUTPUTS
*
*        none.
*
*************************************************************************/
VOID init_animated_Gif(UINT32 i)
{
	int j=0;
    for(j=0;j<NUM_ANIMATED_FRAMES;j++)
    {
        Agif->agif[i]->imgframe[j]= NU_NULL;
		Agif->agif[i]->Lct[j]= NU_NULL;
        Agif->agif[i]->x_origin[j]= 0;
        Agif->agif[i]->y_origin[j]= 0;
        Agif->agif[i]->imwidth[j]= 0;
        Agif->agif[i]->imheight[j]= 0;
        Agif->agif[i]->row_bytes[j]= 0;
        Agif->agif[i]->delay[j]=0;
        Agif->agif[i]->transparent_Index[j]=0;        
    }

    Agif->agif[i]->TransParent= 0;
    Agif->agif[i]->aRect.Xmin= 0;        
    Agif->agif[i]->aRect.Xmax= 0;        
    Agif->agif[i]->aRect.Ymin= 0;        
    Agif->agif[i]->aRect.Ymax= 0;        
    Agif->agif[i]->GCT= 0;
    Agif->agif[i]->NumFrames= 0;
    Agif->agif[i]->Size_difference= 0;        
    Agif->agif[i]->BackGround=0;
    Agif->agif[i]->CurrentFrame=0;
}
/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      clear_animated_Gif                                               
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*      Clears the animated Gif Structure.                                  
*                                                                       
*  INPUTS
*                                                                       
*        none.
*
*  OUTPUTS
*
*        none.
*
*************************************************************************/
void clear_animated_Gif(void)
{
     /*UTL_Zero((VOID*)Agif,sizeof(Animated_Entry));*/
}

/*************************************************************************
*                                                                       
*  FUNCTION                                                             
*                                                                       
*      Free_animated_Gif                                                
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*      Free's any buffer's that are allocated in the Animated Gif        
*      structure.                                                       
*                                                                       
*  INPUTS
*                                                                       
*        none.
*
*  OUTPUTS
*
*        none.
*
*************************************************************************/
void Free_animated_Gif(void)
{
	INT i; 
	INT j;

    for (i = 0;i < Agif->NumAGifs; i++)
    {

        for(j = 0;j < Agif->agif[i]->NumFrames; j++)
        {
            if(Agif->agif[i]->imgframe[j] != NU_NULL)
            {
				GRAFIX_Deallocation(Agif->agif[i]->imgframe[j]);
                Agif->agif[i]->imgframe[j] = NU_NULL;
            }
			if(Agif->agif[i]->Lct[j] != NU_NULL)
			{
				GRAFIX_Deallocation((Agif->agif[i]->Lct[j]));
				Agif->agif[i]->Lct[j] = NU_NULL;
			}
        }
		GRAFIX_Deallocation(Agif->agif[i]);
    }
    /*  Initialize the Animated Gif structure */
    clear_animated_Gif();
}

#endif /* IMG_INCLUDED */
