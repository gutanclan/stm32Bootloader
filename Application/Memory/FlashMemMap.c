//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        FlashMemMap.h
//!    \brief
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "types.h"

#include "../DriversExternalDevices/FlashMem/NandSimu.h"
#include "Flash.h"
#include "FlashMemMap.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    UINT8   bFlashId;
    UINT32  dwMemBlockStart;
    UINT32  dwMemBlockEnd;
}FlashMemMapNodeType;

typedef struct
{
    CHAR const * const		pcDirName;
    FlashMemMapNodeType *	ptMemNodeArray;
	UINT32               	dwMemNodeArrayTotalNodes;
    UINT32               	dwTotalSizeBytes;
	BOOL                 	fIsMemInitSuccess;
}FlashMemMapMemoryType;

//////////////////////////////////////////////////////////////////////////////////////////////////

// MemoryMap 1 (FLASH_MEM_MAP_FILE_SYS_INFO)
static const FlashMemMapNodeType gtNode1Array[] =
{
	// bFlashId				dwMemBlockStart				dwMemBlockEnd
	{FLASH_ID_NAND_SIMU_1,	0,							5},
};
// MemoryMap 2.1 (FLASH_MEM_MAP_TEST_1)
static const FlashMemMapNodeType gtNode2Array[] =
{
	// bFlashId				dwMemBlockStart				dwMemBlockEnd
	{FLASH_ID_NAND_SIMU_1,	6,							9},
	{FLASH_ID_NAND_SIMU_2,	0,							0},
};
// MemoryMap 3 (FLASH_MEM_MAP_TEST_2)
static const FlashMemMapNodeType gtNode3Array[] =
{
	// bFlashId				dwMemBlockStart				dwMemBlockEnd
	{FLASH_ID_NAND_SIMU_2,	1,							9},
};

static FlashMemMapMemoryType tMemArray[] =
{
    {
        //pcDirName                 ptMemoryNode        	dwMemNodeArrayTotalNodes							dwTotalSizeBytes		fIsMemInitSuccess
        "MEM_SYS_INF",				&gtNode1Array[0],   	(sizeof(gtNode1Array)/sizeof(gtNode1Array[0])),		0,						FALSE
    },
    {
        //pcDirName                 ptMemoryNode        	dwMemNodeArrayTotalNodes							dwTotalSizeBytes		fIsMemInitSuccess
        "MEM_TEST_1",				&gtNode2Array[0],   	(sizeof(gtNode2Array)/sizeof(gtNode2Array[0])),		0,						FALSE
    },
    {
        //pcDirName                 ptMemoryNode        	dwMemNodeArrayTotalNodes							dwTotalSizeBytes		fIsMemInitSuccess
        "MEM_TEST_2",				&gtNode3Array[0],   	(sizeof(gtNode3Array)/sizeof(gtNode3Array[0])),		0,						FALSE
    },
};

#define FLASH_MEM_MAP_AMOUNT     ( sizeof(tMemArray) / sizeof(tMemArray[0]) )

//////////////////////////////////////////////////////////////////////////////////////////////////

static UINT32 FlashMemMapCalcSizeBytes( FlashMemMapEnum eMem );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void FlashMemMapInitModule( void )
{
	if( FLASH_MEM_MAP_AMOUNT != FLASH_MEM_MAP_MAX )
    {
        while(1)
        {
            // catch this bug during development time
        }
    }

	// calculate size of each mem section
	UINT64 qwTotalSizeCalc = 0;
	for( FlashMemMapEnum eMem = 0 ; eMem < FLASH_MEM_MAP_AMOUNT ; eMem++ )
	{
		tMemArray[eMem].dwTotalSizeBytes = FlashMemMapCalcSizeBytes(eMem);
		qwTotalSizeCalc += tMemArray[eMem].dwTotalSizeBytes;
	}

	UINT64 qwTotalSize = (FLASH_MAX * FLASH_NUM_BYTES_TOT);
	if( qwTotalSize != qwTotalSizeCalc )
	{
		while(1)
        {
            // catch this bug during development time
        }
	}

	// if pass this point set all memory nodes to valid
	for( FlashMemMapEnum eMem = 0 ; eMem < FLASH_MEM_MAP_AMOUNT ; eMem++ )
	{
		tMemArray[eMem].fIsMemInitSuccess = TRUE;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 FlashMemMapCalcSizeBytes( FlashMemMapEnum eMem )
{
	UINT32 dwSizeBytes = 0;
	UINT32 dwSizeBlocks = 0;

	if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
		for( UINT8 bMemNode = 0 ; bMemNode < tMemArray[eMem].dwMemNodeArrayTotalNodes ; bMemNode++ )
		{
		    dwSizeBlocks = (tMemArray[eMem].ptMemNodeArray[bMemNode].dwMemBlockEnd - tMemArray[eMem].ptMemNodeArray[bMemNode].dwMemBlockStart) + 1;
			dwSizeBytes += dwSizeBlocks * FLASH_BLOCK_SIZE_BYTES;
		}
	}

	return dwSizeBytes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 FlashMemMapSizeBytes( FlashMemMapEnum eMem )
{
    if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
        return tMemArray[eMem].dwTotalSizeBytes;
	}
	else
    {
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockGetFirst( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlockResult )
{
	BOOL fSuccess = FALSE;

	if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
		if( tMemArray[eMem].fIsMemInitSuccess )
		{
			if( ptBlockResult != NULL )
			{
				ptBlockResult->tMemMapIndex.wCurrentIndex = 0;
				ptBlockResult->bFlashId	= tMemArray[eMem].ptMemNodeArray[ptBlockResult->tMemMapIndex.wCurrentIndex].bFlashId;
				ptBlockResult->dwBlock 	= tMemArray[eMem].ptMemNodeArray[ptBlockResult->tMemMapIndex.wCurrentIndex].dwMemBlockStart;

				fSuccess = TRUE;
			}
		}
	}

	return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockIsFirst( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, BOOL *pfBlockIsFirstResult )
{
	BOOL fSuccess = FALSE;

	if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
		if( tMemArray[eMem].fIsMemInitSuccess )
		{
			if
			(
				( ptBlock != NULL ) &&
				( pfBlockIsFirstResult != NULL )
			)
			{
				(*pfBlockIsFirstResult) = TRUE;
				(*pfBlockIsFirstResult) &= (ptBlock->tMemMapIndex.wCurrentIndex == 0);
				(*pfBlockIsFirstResult) &= (ptBlock->bFlashId == tMemArray[eMem].ptMemNodeArray[0].bFlashId);
				(*pfBlockIsFirstResult) &= (ptBlock->dwBlock == tMemArray[eMem].ptMemNodeArray[0].dwMemBlockStart);

				fSuccess = TRUE;
			}
		}
	}

	return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockGetLast( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlockResult )
{
	BOOL fSuccess = FALSE;

	if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
		if( tMemArray[eMem].fIsMemInitSuccess )
		{
			if( ptBlockResult != NULL )
			{
				ptBlockResult->tMemMapIndex.wCurrentIndex = tMemArray[eMem].dwMemNodeArrayTotalNodes-1;
				ptBlockResult->bFlashId	= tMemArray[eMem].ptMemNodeArray[ptBlockResult->tMemMapIndex.wCurrentIndex].bFlashId;
				ptBlockResult->dwBlock 	= tMemArray[eMem].ptMemNodeArray[ptBlockResult->tMemMapIndex.wCurrentIndex].dwMemBlockEnd;

				fSuccess = TRUE;
			}
		}
	}

	return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockIsLast( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, BOOL *pfBlockIsFirstResult )
{
	BOOL fSuccess = FALSE;
	UINT8 bNodeIndex;

	if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
		if( tMemArray[eMem].fIsMemInitSuccess )
		{
			if
			(
				( ptBlock != NULL ) &&
				( pfBlockIsFirstResult != NULL )
			)
			{
				bNodeIndex = tMemArray[eMem].dwMemNodeArrayTotalNodes-1;
				(*pfBlockIsFirstResult) = TRUE;
				(*pfBlockIsFirstResult) &= (ptBlock->tMemMapIndex.wCurrentIndex == bNodeIndex);
				(*pfBlockIsFirstResult) &= (ptBlock->bFlashId == tMemArray[eMem].ptMemNodeArray[bNodeIndex].bFlashId);
				(*pfBlockIsFirstResult) &= (ptBlock->dwBlock == tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockEnd);

				fSuccess = TRUE;
			}
		}
	}

	return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockGetNext( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, FlashMemMapBlockType *ptBlockResult )
{
	BOOL fSuccess = FALSE;
	UINT8 bNodeIndex;
	BOOL fBlockIsLastResult = FALSE;

	if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
		if( tMemArray[eMem].fIsMemInitSuccess )
		{
			if
			(
				( ptBlock != NULL ) &&
				( ptBlockResult != NULL )
			)
			{
				bNodeIndex = ptBlock->tMemMapIndex.wCurrentIndex;

				// check current mem index is valid
				if( bNodeIndex < tMemArray[eMem].dwMemNodeArrayTotalNodes )
				{
					// if block is not last, then advance one more
					FlashMemMapBlockIsLast( eMem, ptBlock, &fBlockIsLastResult );
					if( fBlockIsLastResult == FALSE )
					{
						// check current block is inside memory map block range
						if
						(
							( ptBlock->dwBlock >= tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockStart ) &&
							( ptBlock->dwBlock <= tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockEnd )
						)
						{
							////////////////////////////
							// to move to the next block.
							// the next block could be in this or the next memory map node
							////////////////////////////
							// is the current block at the end of pointer memory map?
							if( ptBlock->dwBlock == tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockEnd )
							{
								// go to next mem block if there is one more available
								if( (bNodeIndex+1) < tMemArray[eMem].dwMemNodeArrayTotalNodes )
								{
									bNodeIndex++;
									ptBlockResult->tMemMapIndex.wCurrentIndex = bNodeIndex;
									ptBlockResult->bFlashId					  = tMemArray[eMem].ptMemNodeArray[bNodeIndex].bFlashId;
									ptBlockResult->dwBlock					  = tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockStart;

									fSuccess = TRUE;
								}
							}
							// not at the end, then just increment one more block
							else
							{
								ptBlockResult->tMemMapIndex.wCurrentIndex 	= ptBlock->tMemMapIndex.wCurrentIndex;
								ptBlockResult->bFlashId 					= ptBlock->bFlashId;
								ptBlockResult->dwBlock  					= ptBlock->dwBlock + 1;

								fSuccess = TRUE;
							}
							////////////////////////////
						}
					}
				}
			}
		}
	}

	return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockGetPrev( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, FlashMemMapBlockType *ptBlockResult )
{
	BOOL fSuccess = FALSE;
	UINT8 bNodeIndex;
	BOOL fBlockIsFirstResult = FALSE;

	if( eMem < FLASH_MEM_MAP_AMOUNT )
	{
		if( tMemArray[eMem].fIsMemInitSuccess )
		{
			if
			(
				( ptBlock != NULL ) &&
				( ptBlockResult != NULL )
			)
			{
				bNodeIndex = ptBlock->tMemMapIndex.wCurrentIndex;

				// check current mem index is valid
				if( bNodeIndex < tMemArray[eMem].dwMemNodeArrayTotalNodes )
				{
					// if block is not last, then advance one more
					FlashMemMapBlockIsFirst( eMem, ptBlock, &fBlockIsFirstResult );
					if( fBlockIsFirstResult == FALSE )
					{
						// check current block is inside memory map block range
						if
						(
							( ptBlock->dwBlock >= tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockStart ) &&
							( ptBlock->dwBlock <= tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockEnd )
						)
						{
							////////////////////////////
							// to move to the prev block.
							// the prev block could be in this or the prev memory map node
							////////////////////////////
							// is the current block at the start of pointer memory map?
							if( ptBlock->dwBlock == tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockStart )
							{
								// go to next mem block if there is one more available
								INT16 iwNodeIndexTest = bNodeIndex;
								if( (iwNodeIndexTest-1) >= 0 )
								{
									bNodeIndex--;
									ptBlockResult->tMemMapIndex.wCurrentIndex = bNodeIndex;
									ptBlockResult->bFlashId					  = tMemArray[eMem].ptMemNodeArray[bNodeIndex].bFlashId;
									ptBlockResult->dwBlock					  = tMemArray[eMem].ptMemNodeArray[bNodeIndex].dwMemBlockEnd;

									fSuccess = TRUE;
								}
							}
							// not at the end, then just increment one more block
							else
							{
								ptBlockResult->tMemMapIndex.wCurrentIndex 	= ptBlock->tMemMapIndex.wCurrentIndex;
								ptBlockResult->bFlashId 					= ptBlock->bFlashId;
								ptBlockResult->dwBlock  					= ptBlock->dwBlock - 1;

								fSuccess = TRUE;
							}
							////////////////////////////
						}
					}
				}
			}
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockPageGetFirst( UINT32 dwBlock, UINT32 *pdwPageResult )
{
    BOOL fSuccess = FALSE;

    if( dwBlock < FLASH_NUM_BLOCKS )
    {
        if( pdwPageResult != NULL )
        {
            (*pdwPageResult) = dwBlock * FLASH_PAGES_PER_BLOCK;

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockPageIsFirst( UINT32 dwBlock, UINT32 dwPage, BOOL *pfIsPageFirstInBlockResult )
{
    BOOL fSuccess = FALSE;

    if( dwBlock < FLASH_NUM_BLOCKS )
    {
        if( dwPage < FLASH_NUM_PAGES )
        {
            if( pfIsPageFirstInBlockResult != NULL )
            {
                (*pfIsPageFirstInBlockResult) = (dwPage == (dwBlock * FLASH_PAGES_PER_BLOCK));

                fSuccess = TRUE;
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockPageGetLast( UINT32 dwBlock, UINT32 *pdwPageResult )
{
    BOOL fSuccess = FALSE;

    if( dwBlock < FLASH_NUM_BLOCKS )
    {
        if( pdwPageResult != NULL )
        {
            (*pdwPageResult) = ( ( ( dwBlock * FLASH_PAGES_PER_BLOCK ) + FLASH_PAGES_PER_BLOCK ) - 1 );

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockPageIsLast( UINT32 dwBlock, UINT32 dwPage, BOOL *pfIsPageLastInBlockResult )
{
    BOOL fSuccess = FALSE;

    if( dwBlock < FLASH_NUM_BLOCKS )
    {
        if( dwPage < FLASH_NUM_PAGES )
        {
            if( pfIsPageLastInBlockResult != NULL )
            {
                (*pfIsPageLastInBlockResult) = ( dwPage == ( ( ( dwBlock * FLASH_PAGES_PER_BLOCK ) + FLASH_PAGES_PER_BLOCK ) - 1 ) );

                fSuccess = TRUE;
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockPageGetNext( UINT32 dwBlock, UINT32 dwPage, UINT32 *pdwPageResult )
{
    BOOL    fSuccess = FALSE;
    UINT32  dwPageLast;

    if( dwBlock < FLASH_NUM_BLOCKS )
    {
        if( dwPage < FLASH_NUM_PAGES )
        {
            if( pdwPageResult != NULL )
            {
                FlashMemMapBlockPageGetLast( dwBlock, &dwPageLast );

                if( (dwPage+1) <= dwPageLast )
                {
                    (*pdwPageResult) = dwPage+1;

                    fSuccess = TRUE;
                }
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashMemMapBlockPageGetPrev( UINT32 dwBlock, UINT32 dwPage, UINT32 *pdwPageResult )
{
    BOOL    fSuccess = FALSE;
    UINT32  dwPageFirst;

    if( dwBlock < FLASH_NUM_BLOCKS )
    {
        if( dwPage < FLASH_NUM_PAGES )
        {
            if( pdwPageResult != NULL )
            {
                FlashMemMapBlockPageGetFirst( dwBlock, &dwPageFirst );

                if( dwPage > 0 )
                {
                    if( (dwPage-1) >= dwPageFirst )
                    {
                        (*pdwPageResult) = dwPage-1;

                        fSuccess = TRUE;
                    }
                }
            }
        }
    }

    return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void FlashMemMapBlockTest( void )
{
    FlashMemMapBlockType    tBlock;
    BOOL                    fSuccess = FALSE;
    FlashMemMapEnum         eMem;
    BOOL                    fBooleanResult;

    FlashMemMapInitModule();

    ////////////////////////////////////////
    eMem = FLASH_MEM_MAP_FILE_SYS_INFO;
    fSuccess = FlashMemMapBlockGetFirst( eMem, &tBlock );
    fSuccess = FlashMemMapBlockIsFirst( eMem, &tBlock, &fBooleanResult );
    fSuccess = FlashMemMapBlockIsLast( eMem, &tBlock, &fBooleanResult );

    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );

    fSuccess = FlashMemMapBlockGetLast( eMem, &tBlock );
    fSuccess = FlashMemMapBlockIsFirst( eMem, &tBlock, &fBooleanResult );
    fSuccess = FlashMemMapBlockIsLast( eMem, &tBlock, &fBooleanResult );

    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    ////////////////////////////////////////
    eMem = FLASH_MEM_MAP_TEST_1;
    fSuccess = FlashMemMapBlockGetFirst( eMem, &tBlock );
    fSuccess = FlashMemMapBlockIsFirst( eMem, &tBlock, &fBooleanResult );
    fSuccess = FlashMemMapBlockIsLast( eMem, &tBlock, &fBooleanResult );

    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );

    fSuccess = FlashMemMapBlockGetLast( eMem, &tBlock );
    fSuccess = FlashMemMapBlockIsFirst( eMem, &tBlock, &fBooleanResult );
    fSuccess = FlashMemMapBlockIsLast( eMem, &tBlock, &fBooleanResult );

    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    ////////////////////////////////////////
    eMem = FLASH_MEM_MAP_TEST_2;
    fSuccess = FlashMemMapBlockGetFirst( eMem, &tBlock );
    fSuccess = FlashMemMapBlockIsFirst( eMem, &tBlock, &fBooleanResult );
    fSuccess = FlashMemMapBlockIsLast( eMem, &tBlock, &fBooleanResult );

    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetNext( eMem, &tBlock, &tBlock );

    fSuccess = FlashMemMapBlockGetLast( eMem, &tBlock );
    fSuccess = FlashMemMapBlockIsFirst( eMem, &tBlock, &fBooleanResult );
    fSuccess = FlashMemMapBlockIsLast( eMem, &tBlock, &fBooleanResult );

    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    fSuccess = FlashMemMapBlockGetPrev( eMem, &tBlock, &tBlock );
    ////////////////////////////////////////

    UINT32 dwPage;
    eMem = FLASH_MEM_MAP_TEST_2;
    fSuccess = FlashMemMapBlockGetFirst( eMem, &tBlock );

    fSuccess = FlashMemMapBlockPageGetFirst( tBlock.dwBlock, &dwPage );
    fSuccess = FlashMemMapBlockPageIsFirst( tBlock.dwBlock, dwPage, &fBooleanResult );
    fSuccess = FlashMemMapBlockPageIsLast( tBlock.dwBlock, dwPage, &fBooleanResult );
    fSuccess = FlashMemMapBlockPageGetPrev( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetNext( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetNext( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetNext( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetNext( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetNext( tBlock.dwBlock, dwPage, &dwPage );

    fSuccess = FlashMemMapBlockGetLast( eMem, &tBlock );
    fSuccess = FlashMemMapBlockPageGetLast( tBlock.dwBlock, &dwPage );
    fSuccess = FlashMemMapBlockPageIsFirst( tBlock.dwBlock, dwPage, &fBooleanResult );
    fSuccess = FlashMemMapBlockPageIsLast( tBlock.dwBlock, dwPage, &fBooleanResult );
    fSuccess = FlashMemMapBlockPageGetNext( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetPrev( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetPrev( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetPrev( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetPrev( tBlock.dwBlock, dwPage, &dwPage );
    fSuccess = FlashMemMapBlockPageGetPrev( tBlock.dwBlock, dwPage, &dwPage );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
