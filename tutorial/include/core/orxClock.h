/* Orx - Portable Game Engine
 *
 * Orx is the legal property of its developers, whose names
 * are listed in the COPYRIGHT file distributed 
 * with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file orxClock.h
 * @date 28/01/2004
 * @author iarwain@orx-project.org
 * 
 * @todo 
 * - Add internal/external dependency system
 * - Optimize dependencies storage
 * - Add freezing behaviour
 */

/**
 * @addtogroup orxClock
 * 
 * Clock module
 * Module that handles clocks which is the low level kernel part of orx execution.
 * @{
 */

#ifndef _orxCLOCK_H_
#define _orxCLOCK_H_


#include "orxInclude.h"
#include "core/orxSystem.h"


#define orxCLOCK_KU32_CLOCK_BANK_SIZE                 16          /**< Clock bank size */

#define orxCLOCK_KU32_FUNCTION_BANK_SIZE              16          /**< Function bank size */


/** Clock type enum
 */
typedef enum __orxCLOCK_TYPE_t
{
  orxCLOCK_TYPE_CORE = 0,
  orxCLOCK_TYPE_RENDER,
  orxCLOCK_TYPE_PHYSICS,
  orxCLOCK_TYPE_USER,

  orxCLOCK_TYPE_SECOND,

  orxCLOCK_TYPE_NUMBER,

  orxCLOCK_TYPE_NONE = orxENUM_NONE

} orxCLOCK_TYPE;


/** Clock mod type enum
 */
typedef enum __orxCLOCK_MOD_TYPE_t
{
  orxCLOCK_MOD_TYPE_FIXED = 0,                        /**< The given DT will always be constant (= modifier value) */
  orxCLOCK_MOD_TYPE_MULTIPLY,                         /**< The given DT will be the real one * modifier */
  orxCLOCK_MOD_TYPE_MAXED,                            /**< The given DT will be the real one maxed by the modifier value */

  orxCLOCK_MOD_TYPE_NUMBER,

  orxCLOCK_MOD_TYPE_NONE = orxENUM_NONE,
    
} orxCLOCK_MOD_TYPE;


/** Clock info structure
 */
typedef struct __orxCLOCK_INFO_t
{
  orxCLOCK_TYPE     eType;                            /**< Clock type : 4 */
  orxFLOAT          fTickSize;                        /**< Clock tick size (in seconds) : 8 */
  orxCLOCK_MOD_TYPE eModType;                         /**> Clock mod type : 12 */
  orxFLOAT          fModValue;                        /**> Clock mod value : 16 */
  orxFLOAT          fDT;                              /**> Clock DT (time ellapsed between 2 clock calls in seconds) : 20 */
  orxFLOAT          fTime;                            /**> Clock time : 24 */ 

} orxCLOCK_INFO;


/** Clock structure */
typedef struct __orxCLOCK_t                           orxCLOCK;

/** Clock callback function type to use with clock bindings */
typedef orxVOID (orxFASTCALL *orxCLOCK_FUNCTION)(orxCONST orxCLOCK_INFO *_pstClockInfo, orxVOID *_pstContext);


/** Clock module setup
 */
extern orxDLLAPI orxVOID                              orxClock_Setup();

/** Inits the clock module
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
extern orxDLLAPI orxSTATUS                            orxClock_Init();

/** Exits from the clock module
 */
extern orxDLLAPI orxVOID                              orxClock_Exit();


/** Updates the clock system
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
extern orxDLLAPI orxSTATUS                            orxClock_Update();

/** Creates a clock
 * @param[in]   _fTickSize                            Tick size for the clock (in seconds)
 * @param[in]   _eType                                Type of the clock
 * @return      orxCLOCK / orxNULL
 */
extern orxDLLAPI orxCLOCK *orxFASTCALL                orxClock_Create(orxFLOAT _fTickSize, orxCLOCK_TYPE _eType);

/** Deletes a clock
 * @param[in]   _pstClock                             Concerned clock
 */
extern orxDLLAPI orxVOID orxFASTCALL                  orxClock_Delete(orxCLOCK *_pstClock);

/** Resyncs a clock (accumulated DT => 0)
 */
extern orxDLLAPI orxVOID                              orxClock_Resync();

/** Pauses a clock
 * @param[in]   _pstClock                             Concerned clock
 */
extern orxDLLAPI orxVOID orxFASTCALL                  orxClock_Pause(orxCLOCK *_pstClock);

/** Unpauses a clock
 * @param[in]   _pstClock                             Concerned clock
 */
extern orxDLLAPI orxVOID orxFASTCALL                  orxClock_Unpause(orxCLOCK *_pstClock);

/** Is a clock paused?
 * @param[in]   _pstClock                             Concerned clock
 * @return      orxTRUE if paused, orxFALSE otherwise
 */
extern orxDLLAPI orxBOOL orxFASTCALL                  orxClock_IsPaused(orxCONST orxCLOCK *_pstClock);

/** Gets clock info
 * @param[in]   _pstClock                             Concerned clock
 * @return      orxCLOCK_INFO / orxNULL
 */
extern orxDLLAPI orxCONST orxCLOCK_INFO *orxFASTCALL  orxClock_GetInfo(orxCONST orxCLOCK *_pstClock);


/** Sets a clock modifier
 * @param[in]   _pstClock                             Concerned clock
 * @param[in]   _eModType                             Modifier type
 * @param[in]   _fModValue                            Modifier value
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
extern orxDLLAPI orxSTATUS orxFASTCALL                orxClock_SetModifier(orxCLOCK *_pstClock, orxCLOCK_MOD_TYPE _eModType, orxFLOAT _fModValue);


/** Registers a callback function to a clock
 * @param[in]   _pstClock                             Concerned clock
 * @param[in]   _pfnCallback                          Callback to register
 * @param[in]   _pstContext                           Context that will be transmitted to the callback when called
 * @param[in]   _eModuleID                            IF of the module related to this callback
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
extern orxDLLAPI orxSTATUS orxFASTCALL                orxClock_Register(orxCLOCK *_pstClock, orxCONST orxCLOCK_FUNCTION _pfnCallback, orxVOID *_pstContext, orxMODULE_ID _eModuleID);

/** Unregisters a callback function from a clock
 * @param[in]   _pstClock                             Concerned clock
 * @param[in]   _pfnCallback                          Callback to remove
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
extern orxDLLAPI orxSTATUS orxFASTCALL                orxClock_Unregister(orxCLOCK *_pstClock, orxCONST orxCLOCK_FUNCTION _pfnCallback);

/** Gets a callback function context
 * @param[in]   _pstClock                             Concerned clock
 * @param[in]   _pfnCallback                          Concerned callback
 * @return      Registered context
 */
extern orxDLLAPI orxVOID  *orxFASTCALL                orxClock_GetContext(orxCONST orxCLOCK *_pstClock, orxCONST orxCLOCK_FUNCTION _pfnCallback);

/** Sets a callback function context
 * @param[in]   _pstClock                             Concerned clock
 * @param[in]   _pfnCallback                          Concerned callback
 * @param[in]   _pstContext                           Context that will be transmitted to the callback when called
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
extern orxDLLAPI orxSTATUS orxFASTCALL                orxClock_SetContext(orxCLOCK *_pstClock, orxCONST orxCLOCK_FUNCTION _pfnCallback, orxVOID *_pstContext);


/** Finds a clock given its tick size and its type
 * @param[in]   _fTickSize                            Tick size of the desired clock (in seconds)
 * @param[in]   _eType                                Type of the desired clock
 * @return      orxCLOCK / orxNULL
 */
extern orxDLLAPI orxCLOCK *orxFASTCALL                orxClock_FindFirst(orxFLOAT _fTickSize, orxCLOCK_TYPE _eType);

/** Finds next clock of same type/tick size
 * @param[in]   _pstClock                             Concerned clock
 * @return      orxCLOCK / orxNULL
 */
extern orxDLLAPI orxCLOCK *orxFASTCALL                orxClock_FindNext(orxCONST orxCLOCK *_pstClock);

/** Gets next existing clock in list (can be used to parse all existing clocks)
 * @param[in]   _pstClock                             Concerned clock
 * @return      orxCLOCK / orxNULL
 */
extern orxDLLAPI orxCLOCK *orxFASTCALL                orxClock_GetNext(orxCONST orxCLOCK *_pstClock);


#endif /* _orxCLOCK_H_ */

/** @} */
