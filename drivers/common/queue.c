//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        queue.h
//!    \brief       queue module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

// Module based on tutorial:
// http://www.simplyembedded.org/tutorials/interrupt-free-ring-buffer/

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "./Types.h"
#include "./queue.h"
#include "./queueDataTypes.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    void *          pvQueue;		    /* body of queue                */
    UINT32          dwMaxItems;         /* max elements allowed in queue*/
    UINT32          dwItemSize_bytes;   /* size of each item in bytes   */
    volatile UINT32 dwStart;            /* number of queue elements     */
    volatile UINT32 dwEnd;              /* number of queue elements     */
}queueType;



// declare number of elements per queue
//#define QUEUE_NUM_OF_ITEMS_USART3_RX         (256)
//#define QUEUE_NUM_OF_ITEMS_USART3_TX         (256)

#define QUEUE_NUM_OF_ITEMS_USART3_RX         (1024)
#define QUEUE_NUM_OF_ITEMS_USART3_TX         (1024)

// declare buffer of required type
static UINT8          			gbQueueBufferUsart3Rx[QUEUE_NUM_OF_ITEMS_USART3_RX];
static UINT8          			gbQueueBufferUsart3Tx[QUEUE_NUM_OF_ITEMS_USART3_TX];

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// This table must be in the same order as QueueEnum in queue.h
static queueType gtQueueLookupArray[] =
{
    //	*pvQUEUE					    MAX ITEMS 								                                ITEM SIZE (BYTES)
    {   &gbQueueBufferUsart3Rx[0],      (sizeof(gbQueueBufferUsart3Rx)/sizeof(gbQueueBufferUsart3Rx[0])),       sizeof( gbQueueBufferUsart3Rx[0] )   },
	{   &gbQueueBufferUsart3Tx[0],      (sizeof(gbQueueBufferUsart3Tx)/sizeof(gbQueueBufferUsart3Tx[0])),       sizeof( gbQueueBufferUsart3Tx[0] )   },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
};

#define QUEUE_ARRAY_MAX	( sizeof(gtQueueLookupArray) / sizeof( gtQueueLookupArray[0] ) )

static  BOOL    QueueIsBufferFull       ( const queueType *const ptQueue );
static  BOOL    QueueIsBufferEmpty      ( const queueType *const ptQueue );

///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void QueueModuleInit( void )
//!
//! \brief  Initialize queue variables
//!
//! \param[in]  void
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL QueueModuleInit( void )
{
    BOOL fIsError = FALSE;

    if( QUEUE_MAX != QUEUE_ARRAY_MAX )
    {
        // definition mismatch error
        while(0);
    }
    else
    {
        for( UINT32 dwQCounter = 0 ; dwQCounter < QUEUE_ARRAY_MAX ; dwQCounter++ )
        {
            // make sure size is greater than 0
            if( ( gtQueueLookupArray[dwQCounter].pvQueue != NULL ) && (gtQueueLookupArray[dwQCounter].dwMaxItems > 0) )
            {
                // make sure the size of the buffer is power of 2
                // modulus operation can be represented by a only logical AND operator a simple subtraction
                if( ( ( gtQueueLookupArray[dwQCounter].dwMaxItems -1 ) & gtQueueLookupArray[dwQCounter].dwMaxItems ) == 0 )
                {
                    if( QueueReset( dwQCounter ) == FALSE )
                    {
                        fIsError = TRUE;
                    }
                }
                else
                {
                    fIsError = TRUE;
                }
            }
            else
            {
                fIsError = TRUE;
            }
        }
    }

    if( fIsError )
    {
        while(1);
    }

    return !fIsError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL QueueIsBufferFull( const queueType * const ptQueue )
{
    if( ( ptQueue->dwStart - ptQueue->dwEnd ) == ptQueue->dwMaxItems )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL QueueIsBufferEmpty( const queueType *const ptQueue )
{
    if( ( ptQueue->dwStart - ptQueue->dwEnd ) == 0 )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL QueueSend( QueueEnum eQueue, const void * pvItemToQueue )
//!
//! \brief  send an item to the queue
//!
//! \param[in]  eQueue          queue selected
//! \param[in]  pvItemToQueue   pointer to item to be queued
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL QueueSend( QueueEnum eQueue, const void * pvItemToQueue )
{
    BOOL    fSuccess = FALSE;

    UINT32  dwOffset = 0;

    if( NULL != pvItemToQueue )
    {
        if( eQueue < QUEUE_ARRAY_MAX )
        {
            if( QueueIsBufferFull( &gtQueueLookupArray[eQueue] ) == FALSE )
            {
                // modulus operation can be represented by a only logical AND operator a simple subtraction
                dwOffset = (gtQueueLookupArray[eQueue].dwStart & (gtQueueLookupArray[eQueue].dwMaxItems -1)) * gtQueueLookupArray[eQueue].dwItemSize_bytes;
                memcpy( &gtQueueLookupArray[eQueue].pvQueue[ dwOffset ], pvItemToQueue, gtQueueLookupArray[eQueue].dwItemSize_bytes );
                gtQueueLookupArray[eQueue].dwStart++;

                fSuccess = TRUE;
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL QueueReceive( QueueEnum eQueue, void * pvBuffer, UINT32 dwBufferSize )
//!
//! \brief  return and remove item from queue
//!
//! \param[in]  eQueue          queue selected
//! \param[in]  pvItemToQueue   pointer to item to be dequeued
//! \param[in]  dwBufferSize    size of buffer to hold the item to be dequeued
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL QueueReceive( QueueEnum eQueue, void * pvBuffer, UINT32 dwBufferSize )
{
    BOOL    fSuccess = FALSE;

    UINT32  dwOffset = 0;

    if( NULL != pvBuffer )
    {
        if( eQueue < QUEUE_ARRAY_MAX )
        {
            if( QueueIsBufferEmpty( &gtQueueLookupArray[eQueue] ) == FALSE )
            {
                if( dwBufferSize == gtQueueLookupArray[eQueue].dwItemSize_bytes )
                {
                    // modulus operation can be represented by a only logical AND operator a simple subtraction
                    dwOffset = (gtQueueLookupArray[eQueue].dwEnd & (gtQueueLookupArray[eQueue].dwMaxItems -1)) * gtQueueLookupArray[eQueue].dwItemSize_bytes;
                    memcpy( pvBuffer, &gtQueueLookupArray[eQueue].pvQueue[ dwOffset ], dwBufferSize );
                    gtQueueLookupArray[eQueue].dwEnd++;
                    fSuccess = TRUE;
                }
            }
        }
    }

    return fSuccess;
}

//

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL QueuePeek( QueueEnum eQueue, void * pvBuffer, UINT32 dwBufferSize )
//!
//! \brief  return the item from queue. it doesn't remove it from queue.
//!
//! \param[in]  eQueue          queue selected
//! \param[in]  pvItemToQueue   pointer to item to be dequeued
//! \param[in]  dwBufferSize    size of buffer to hold the item to be dequeued
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL QueuePeek( QueueEnum eQueue, void * pvBuffer, UINT32 dwBufferSize )
{
    BOOL    fSuccess    = FALSE;

    UINT32  dwOffset;

    if( NULL != pvBuffer )
    {
        if( eQueue < QUEUE_ARRAY_MAX )
        {
            if( QueueIsBufferEmpty( &gtQueueLookupArray[eQueue] ) == FALSE )
            {
                if( dwBufferSize == gtQueueLookupArray[eQueue].dwItemSize_bytes )
                {
                    // modulus operation can be represented by a only logical AND operator a simple subtraction
                    dwOffset = (gtQueueLookupArray[eQueue].dwEnd & (gtQueueLookupArray[eQueue].dwMaxItems -1)) * gtQueueLookupArray[eQueue].dwItemSize_bytes;
                    memcpy( pvBuffer, &gtQueueLookupArray[eQueue].pvQueue[ dwOffset ], dwBufferSize );
                    fSuccess = TRUE;
                }
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL QueueReset( QueueEnum eQueue )
//!
//! \brief  set queue item counter to 0;
//!
//! \param[in]  eQueue          queue selected
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL QueueReset( QueueEnum eQueue )
{
    BOOL fSuccess = FALSE;

    if( eQueue < QUEUE_ARRAY_MAX )
    {
        gtQueueLookupArray[eQueue].dwEnd    = 0;
        gtQueueLookupArray[eQueue].dwStart  = 0;

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 QueueGetItemsMax( QueueEnum eQueue )
{
    if( eQueue < QUEUE_ARRAY_MAX )
    {
        return gtQueueLookupArray[eQueue].dwMaxItems;
    }
    else
    {
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 QueueGetItemsWaiting( QueueEnum eQueue )
{
    if( eQueue < QUEUE_ARRAY_MAX )
    {
        return (gtQueueLookupArray[eQueue].dwStart - gtQueueLookupArray[eQueue].dwEnd);
    }
    else
    {
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void QueueTest( void )
{
    QueueModuleInit();

    CHAR cChar;
    BOOL fRes;

    cChar = 'a';
    fRes = QueueSend( QUEUE_USART3_TX, &cChar );

    cChar = 'b';
    fRes = QueueSend( QUEUE_USART3_TX, &cChar );

    cChar = 'c';
    fRes = QueueSend( QUEUE_USART3_TX, &cChar );

    cChar = 'd';
    fRes = QueueSend( QUEUE_USART3_TX, &cChar );

    // - - - -

    fRes = QueueReceive( QUEUE_USART3_TX, &cChar, sizeof(cChar) );

    fRes = QueueReceive( QUEUE_USART3_TX, &cChar, sizeof(cChar) );

    fRes = QueueReceive( QUEUE_USART3_TX, &cChar, sizeof(cChar) );

    fRes = QueueReceive( QUEUE_USART3_TX, &cChar, sizeof(cChar) );

}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////


