/**
 * @file orxFX.c
 */

/***************************************************************************
 orxFX.c
 FX module
 begin                : 30/06/2008
 author               : (C) Arcallians
 email                : iarwain@arcallians.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License           *
 *   as published by the Free Software Foundation; either version 2.1      *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 ***************************************************************************/


#include "render/orxFX.h"

#include "debug/orxDebug.h"
#include "memory/orxMemory.h"
#include "core/orxConfig.h"
#include "core/orxClock.h"
#include "object/orxStructure.h"
#include "utils/orxHashTable.h"
#include "utils/orxString.h"


/** Module flags
 */
#define orxFX_KU32_STATIC_FLAG_NONE             0x00000000

#define orxFX_KU32_STATIC_FLAG_READY            0x00000001

#define orxFX_KU32_STATIC_MASK_ALL              0xFFFFFFFF


/** Flags
 */
#define orxFX_KU32_FLAG_NONE                    0x00000000  /**< No flags */

#define orxFX_KU32_FLAG_ENABLED                 0x10000000  /**< Enabled flag */

#define orxFX_KU32_MASK_ALL                     0xFFFFFFFF  /**< All mask */


/** Misc defines
 */
#define orxFX_KU32_REFERENCE_TABLE_SIZE         32

#define orxFX_KU32_SLOT_NUMBER                  8


/***************************************************************************
 * Structure declaration                                                   *
 ***************************************************************************/

/** FX type enum
 */
typedef enum __orxFX_TYPE_t
{
	orxFX_TYPE_ALPHA_FADE = 0,
	orxFX_TYPE_COLOR_BLEND,
	orxFX_TYPE_ROTATION,
	orxFX_TYPE_SCALE,
	orxFX_TYPE_TRANSLATION,

	orxFX_TYPE_NUMBER,

	orxFX_TYPE_MAX_NUMBER = 16,

	orxFX_TYPE_NONE = orxENUM_NONE

} orxFX_TYPE;

/** FX slot
 */
typedef struct __orxFX_SLOT_t
{
	orxFX_TYPE  eFXType;                          /**< Type : 4 */
  orxFLOAT    fStartTime;                       /**< Start Time : 8 */
  orxFLOAT    fEndTime;                         /**< End Time : 12 */
  orxFLOAT    fCyclePeriod;                     /**< Cycle period : 16 */
  orxFLOAT    fCyclePhasis;                     /**< Cycle phasis : 20 */

  union
  {
    struct
    {
      orxFLOAT fStartAlpha;                     /**< Alpha start value : 24 */
      orxFLOAT fEndAlpha;                       /**< Alpha end value : 28 */
    };                                          /**< Alpha Fade  : 28 */

    struct
    {
      orxVECTOR vStartColor;                    /**< ColorBlend start value : 32 */
      orxVECTOR vEndColor;                      /**< ColorBlend end value : 44 */
    };                                          /** Color blend : 44 */

    struct
    {
      orxFLOAT fStartRotation;                  /**< Rotation start value : 24 */
      orxFLOAT fEndRotation;                    /**< Rotation end value : 28 */
    };                                          /**< Scale : 28 */

    struct
    {
      orxVECTOR vStartScale;                    /**< Scale start value : 32 */
      orxVECTOR vEndScale;                      /**< Scale end value : 44 */
    };                                          /**< Scale : 44 */

    struct
    {
      orxVECTOR vStartPosition;                 /**< Translation vector : 32 */
      orxVECTOR vEndPosition;                   /**< Translation end position : 44 */
    };                                          /**< Translation : 44 */
  };

  orxU32 u32Flags;                              /**< Flags : 48 */

} orxFX_SLOT;

/** FX structure
 */
struct __orxFX_t
{
  orxSTRUCTURE  stStructure;                            /**< Public structure, first structure member : 16 */
  orxFX_SLOT    astFXSlotList[orxFX_KU32_SLOT_NUMBER];  /**< FX slot list : 64 */
  orxU32        u32ID;                                  /**< FX ID : 68 */
  orxFLOAT      fDuration;                              /**< FX duration : 72 */

  /* Padding */
  orxPAD(72)
};

/** Static structure
 */
typedef struct __orxFX_STATIC_t
{
  orxHASHTABLE *pstReferenceTable;              /**< Reference hash table */
  orxU32        u32Flags;                       /**< Control flags */

} orxFX_STATIC;


/***************************************************************************
 * Static variables                                                        *
 ***************************************************************************/

/** Static data
 */
orxSTATIC orxFX_STATIC sstFX;


/***************************************************************************
 * Private functions                                                       *
 ***************************************************************************/

/** Finds the first empty slot
 * @param[in] _pstFX            Concerned FX
 * @return orxU32 / orxU32_UNDEFINED
 */
orxSTATIC orxINLINE orxU32 orxFX_FindEmptySlotIndex(orxCONST orxFX *_pstFX)
{
  orxU32 i, u32Result = orxU32_UNDEFINED;

  /* Checks */
  orxSTRUCTURE_ASSERT(_pstFX);

  /* For all slots */
  for(i = 0; i < orxFX_KU32_SLOT_NUMBER; i++)
  {
    /* Empty? */
    if(!orxFLAG_TEST(_pstFX->astFXSlotList[i].u32Flags, orxFX_SLOT_KU32_FLAG_DEFINED))
    {
      /* Updates result */
      u32Result = i;
      break;
    }
  }

  /* Done! */
  return u32Result;
}

/** Deletes all the FXs
 */
orxSTATIC orxINLINE orxVOID orxFX_DeleteAll()
{
  orxFX *pstFX;

  /* Gets first FX */
  pstFX = orxFX(orxStructure_GetFirst(orxSTRUCTURE_ID_FX));

  /* Non empty? */
  while(pstFX != orxNULL)
  {
    /* Deletes it */
    orxFX_Delete(pstFX);

    /* Gets first FX */
    pstFX = orxFX(orxStructure_GetFirst(orxSTRUCTURE_ID_FX));
  }

  return;
}


/***************************************************************************
 * Public functions                                                        *
 ***************************************************************************/

/** FX module setup
 */
orxVOID orxFX_Setup()
{
  /* Adds module dependencies */
  orxModule_AddDependency(orxMODULE_ID_FX, orxMODULE_ID_MEMORY);
  orxModule_AddDependency(orxMODULE_ID_FX, orxMODULE_ID_STRUCTURE);
  orxModule_AddDependency(orxMODULE_ID_FX, orxMODULE_ID_CONFIG);
  orxModule_AddDependency(orxMODULE_ID_FX, orxMODULE_ID_HASHTABLE);

  return;
}

/** Inits the FX module
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFX_Init()
{
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(orxFX_TYPE_NUMBER <= orxFX_TYPE_MAX_NUMBER);

  /* Not already Initialized? */
  if(!(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY))
  {
    /* Cleans static controller */
    orxMemory_Zero(&sstFX, sizeof(orxFX_STATIC));

    /* Creates reference table */
    sstFX.pstReferenceTable = orxHashTable_Create(orxFX_KU32_REFERENCE_TABLE_SIZE, orxHASHTABLE_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);

    /* Valid? */
    if(sstFX.pstReferenceTable != orxNULL)
    {
      /* Registers structure type */
      eResult = orxSTRUCTURE_REGISTER(FX, orxSTRUCTURE_STORAGE_TYPE_LINKLIST, orxMEMORY_TYPE_MAIN, orxNULL);
    }
    else
    {
      /* !!! MSG !!! */
    }
  }
  else
  {
    /* !!! MSG !!! */

    /* Already initialized */
    eResult = orxSTATUS_SUCCESS;
  }

  /* Initialized? */
  if(eResult == orxSTATUS_SUCCESS)
  {
    /* Inits Flags */
    sstFX.u32Flags = orxFX_KU32_STATIC_FLAG_READY;
  }

  /* Done! */
  return eResult;
}

/** Exits from the FX module
 */
orxVOID orxFX_Exit()
{
  /* Initialized? */
  if(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY)
  {
    /* Deletes FX list */
    orxFX_DeleteAll();

    /* Unregisters structure type */
    orxStructure_Unregister(orxSTRUCTURE_ID_FX);

    /* Deletes reference table */
    orxHashTable_Delete(sstFX.pstReferenceTable);

    /* Updates flags */
    sstFX.u32Flags &= ~orxFX_KU32_STATIC_FLAG_READY;
  }
  else
  {
    /* !!! MSG !!! */
  }

  return;
}

/** Creates an empty FX
 * @return      Created orxFX / orxNULL
 */
orxFX *orxFX_Create()
{
  orxFX *pstResult;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);

  /* Creates FX */
  pstResult = orxFX(orxStructure_Create(orxSTRUCTURE_ID_FX));

  /* Created? */
  if(pstResult != orxNULL)
  {
    /* Inits flags */
    orxStructure_SetFlags(pstResult, orxFX_KU32_FLAG_ENABLED, orxFX_KU32_MASK_ALL);
  }
  else
  {
    /* !!! MSG !!! */
  }

  /* Done! */
  return pstResult;
}

/** Creates an FX from config
 * @param[in]   _zConfigID            Config ID
 * @ return orxFX / orxNULL
 */
orxFX *orxFASTCALL orxFX_CreateFromConfig(orxCONST orxSTRING _zConfigID)
{
  orxU32  u32ID;
  orxFX  *pstResult;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxASSERT((_zConfigID != orxNULL) && (*_zConfigID != *orxSTRING_EMPTY));

  /* Gets FX ID */
  u32ID = orxString_ToCRC(_zConfigID);

  /* Search for reference */
  pstResult = orxHashTable_Get(sstFX.pstReferenceTable, u32ID);

  /* Not already created? */
  if(pstResult == orxNULL)
  {
    orxSTRING zPreviousSection;

    /* Gets previous config section */
    zPreviousSection = orxConfig_GetCurrentSection();

    /* Selects section */
    if((orxConfig_HasSection(_zConfigID) != orxFALSE)
    && (orxConfig_SelectSection(_zConfigID) != orxSTATUS_FAILURE))
    {
      /* Creates FX */
      pstResult = orxFX_Create();

      /* Valid? */
      if(pstResult != orxNULL)
      {
        /* Stores its ID */
        pstResult->u32ID = orxString_ToCRC(_zConfigID);

        //! TODO
      }

      /* Restores previous section */
      orxConfig_SelectSection(zPreviousSection);
    }
    else
    {
      /* !!! MSG !!! */

      /* Updates result */
      pstResult = orxNULL;
    }
  }
  else
  {
    /* Updates reference counter */
    orxStructure_IncreaseCounter(pstResult);
  }

  /* Done! */
  return pstResult;
}

/** Deletes an FX
 * @param[in] _pstFX            Concerned FX
 * @return orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxFX_Delete(orxFX *_pstFX)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Not referenced? */
  if(orxStructure_GetRefCounter(_pstFX) == 0)
  {
    /* Deletes structure */
    orxStructure_Delete(_pstFX);
  }
  else
  {
    /* !!! MSG !!! */

    /* Referenced by others */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Applies FX on object
 * @param[in] _pstFX            FX to apply
 * @param[in] _pstObject        Object on which to apply the FX
 * @param[in] _fStartTime       FX local application start time
 * @param[in] _fEndTime         FX local application end time
 * @return    orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxFX_Apply(orxCONST orxFX *_pstFX, orxOBJECT *_pstObject, orxFLOAT _fStartTime, orxFLOAT _fEndTime)
{
  orxU32    i;
  orxRGBA   stColor;
  orxBOOL   bAlphaLock = orxFALSE, bColorBlendLock = orxFALSE, bRotationLock = orxFALSE, bScaleLock = orxFALSE, bTranslationLock = orxFALSE;
  orxBOOL   bAlphaUpdate = orxFALSE, bColorBlendUpdate = orxFALSE, bRotationUpdate = orxFALSE, bScaleUpdate = orxFALSE, bTranslationUpdate = orxFALSE;
  orxFLOAT  fAlpha = orxFLOAT_0, fRotation = orxFLOAT_0;
  orxVECTOR vColor, vScale, vPosition;
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxSTRUCTURE_ASSERT(_pstFX);
  orxSTRUCTURE_ASSERT(_pstObject);
  orxASSERT(_fStartTime >= orxFLOAT_0);
  orxASSERT(_fEndTime > _fStartTime);

  /* Clears color, scale and position vectors */
  orxVector_SetAll(&vColor, orxFLOAT_0);
  orxVector_SetAll(&vScale, orxFLOAT_1);
  orxVector_SetAll(&vPosition, orxFLOAT_0);

  /* Gets object color */
  stColor = (orxObject_HasColor(_pstObject) != orxFALSE) ? orxObject_GetColor(_pstObject) : orx2RGBA(0xFF, 0xFF, 0xFF, 0xFF);

  /* For all slots */
  for(i = 0; i< orxFX_KU32_SLOT_NUMBER; i++)
  {
    orxCONST orxFX_SLOT *pstFXSlot;

    /* Gets the slot */
    pstFXSlot = &(_pstFX->astFXSlotList[i]);

    /* Is defined? */
    if(orxFLAG_TEST(pstFXSlot->u32Flags, orxFX_SLOT_KU32_FLAG_DEFINED))
    {
      orxFLOAT fStartTime, fEndTime, fPeriod, fFrequency, fStartCoef, fEndCoef;

      /* Gets corrected start and end time */
      fStartTime  = orxMAX(_fStartTime, pstFXSlot->fStartTime);
      fEndTime    = orxMIN(_fEndTime, pstFXSlot->fEndTime);

      /* Is this slot active in the time period? */
      if(fEndTime > fStartTime)
      {
        /* Is FX type not blocked? */
        if(((pstFXSlot->eFXType == orxFX_TYPE_ALPHA_FADE) && (bAlphaLock == orxFALSE))
        || ((pstFXSlot->eFXType == orxFX_TYPE_COLOR_BLEND) && (bColorBlendLock == orxFALSE))
        || ((pstFXSlot->eFXType == orxFX_TYPE_ROTATION) && (bRotationLock == orxFALSE))
        || ((pstFXSlot->eFXType == orxFX_TYPE_SCALE) && (bScaleLock == orxFALSE))
        || ((pstFXSlot->eFXType == orxFX_TYPE_TRANSLATION) && (bTranslationLock == orxFALSE)))
        {
          /* Has a valid cycle period? */
          if(pstFXSlot->fCyclePeriod != orxFLOAT_0)
          {
            /* Gets it */
            fPeriod = pstFXSlot->fCyclePeriod;
          }
          else
          {
            /* Gets whole duration as period */
            fPeriod = pstFXSlot->fEndTime - pstFXSlot->fStartTime;
          }

          /* Gets its corresponding frequency */
          fFrequency = orxFLOAT_1 / fPeriod;

          /* Depending on blend curve */
          switch(pstFXSlot->u32Flags & orxFX_SLOT_KU32_MASK_BLEND_CURVE)
          {
            case orxFX_SLOT_KU32_FLAG_BLEND_CURVE_LINEAR:
            {
              /* Gets linear start coef in period [0.0; 1.0] starting at given phasis */
              fStartCoef = (fStartTime * fFrequency) + pstFXSlot->fCyclePhasis;

              /* Non zero? */
              if(fStartCoef != orxFLOAT_0)
              {
                /* Gets its modulo */
                fStartCoef = orxMath_Mod(fStartCoef, orxFLOAT_1);

                /* Zero? */
                if(fStartCoef == orxFLOAT_0)
                {
                  /* Sets it at max value */
                  fStartCoef = orxFLOAT_1;
                }
              }

              /* Gets linear end coef in period [0.0; 1.0] starting at given phasis */
              fEndCoef = (fEndTime * fFrequency) + pstFXSlot->fCyclePhasis;

              /* Non zero? */
              if(fEndCoef != orxFLOAT_0)
              {
                /* Gets its modulo */
                fEndCoef = orxMath_Mod(fEndCoef, orxFLOAT_1);

                /* Zero? */
                if(fEndCoef == orxFLOAT_0)
                {
                  /* Sets it at max value */
                  fEndCoef = orxFLOAT_1;
                }
              }

              break;
            }

            case orxFX_SLOT_KU32_FLAG_BLEND_CURVE_SAW:
            {
              /* Gets linear coef in period [0.0; 2.0] starting at given phasis */
              fStartCoef = (fStartTime * fFrequency) + pstFXSlot->fCyclePhasis;
              fStartCoef = orxMath_Mod(fStartCoef, orxFLOAT_1);

              /* Gets symetric coef between 1.0 & 2.0 */
              if(fStartCoef > orxFLOAT_1)
              {
                fStartCoef = orx2F(2.0f) - fStartCoef;
              }

              /* Gets linear coef in period [0.0; 2.0] starting at given phasis */
              fEndCoef = (fEndCoef * fFrequency) + pstFXSlot->fCyclePhasis;
              fEndCoef = orxMath_Mod(fEndCoef, orxFLOAT_1);

              /* Gets symetric coef between 1.0 & 2.0 */
              if(fEndCoef > orxFLOAT_1)
              {
                fEndCoef = orx2F(2.0f) - fEndCoef;
              }

              break;
            }

            case orxFX_SLOT_KU32_FLAG_BLEND_CURVE_SINE:
            {
              /* Gets sine coef starting at given phasis * 2Pi - Pi/2 */
              fStartCoef = (orxMath_Sin((orxMATH_KF_2_PI * (fStartTime + (fPeriod * (pstFXSlot->fCyclePhasis - orx2F(0.25f))))) * fFrequency) + orxFLOAT_1) * orx2F(0.5f);

              /* Gets sine coef starting at given phasis * 2Pi - Pi/2 */
              fEndCoef = (orxMath_Sin((orxMATH_KF_2_PI * (fEndTime + (fPeriod * (pstFXSlot->fCyclePhasis - orx2F(0.25f))))) * fFrequency) + orxFLOAT_1) * orx2F(0.5f);

              break;
            }

            default:
            {
              /* !!! MSG !!! */

              /* Skips it */
              continue;
            }
          }

          /* Using a square curve? */
          if(orxFLAG_TEST(pstFXSlot->u32Flags, orxFX_SLOT_KU32_FLAG_BLEND_SQUARE))
          {
            /* Squares both coefs */
            fStartCoef *= fStartCoef;
            fEndCoef   *= fEndCoef;
          }

          /* Clamps the coefs */
          fStartCoef  = orxCLAMP(fStartCoef, orxFLOAT_0, orxFLOAT_1);
          fEndCoef    = orxCLAMP(fEndCoef, orxFLOAT_0, orxFLOAT_1);

          /* Depending on FX type */
          switch(pstFXSlot->eFXType)
          {
            case orxFX_TYPE_ALPHA_FADE:
            {
              /* Absolute ? */
              if(orxFLAG_TEST(pstFXSlot->u32Flags, orxFX_SLOT_KU32_FLAG_ABSOLUTE))
              {
                /* Overrides value */
                fAlpha = orxLERP(pstFXSlot->fStartAlpha, pstFXSlot->fEndAlpha, fEndCoef);

                /* Locks it */
                bAlphaLock = orxTRUE;
              }
              else
              {
                orxFLOAT fStartAlpha, fEndAlpha;

                /* Gets alpha values */
                fStartAlpha  = orxLERP(pstFXSlot->fStartAlpha, pstFXSlot->fEndAlpha, fStartCoef);
                fEndAlpha    = orxLERP(pstFXSlot->fStartAlpha, pstFXSlot->fEndAlpha, fEndCoef);

                /* Updates global alpha value */
                fAlpha += fEndAlpha - fStartAlpha;
              }

              /* Updates alpha status */
              bAlphaUpdate = orxTRUE;

              break;
            }

            case orxFX_TYPE_COLOR_BLEND:
            {
              /* Absolute ? */
              if(orxFLAG_TEST(pstFXSlot->u32Flags, orxFX_SLOT_KU32_FLAG_ABSOLUTE))
              {
                /* Overrides values */
                orxVector_Lerp(&vColor, &(pstFXSlot->vStartColor), &(pstFXSlot->vEndColor), fEndCoef);

                /* Locks it */
                bColorBlendLock = orxTRUE;

              }
              else
              {
                orxVECTOR vStartColor, vEndColor;

                /* Gets alpha values */
                orxVector_Lerp(&vStartColor, &(pstFXSlot->vStartColor), &(pstFXSlot->vEndColor), fStartCoef);
                orxVector_Lerp(&vEndColor, &(pstFXSlot->vStartColor), &(pstFXSlot->vEndColor), fEndCoef);

                /* Updates global color value */
                orxVector_Add(&vColor, &vColor, orxVector_Sub(&vEndColor, &vEndColor, &vStartColor));
              }

              /* Updates color blend status */
              bColorBlendUpdate = orxTRUE;

              break;
            }

            case orxFX_TYPE_ROTATION:
            {
              /* Absolute ? */
              if(orxFLAG_TEST(pstFXSlot->u32Flags, orxFX_SLOT_KU32_FLAG_ABSOLUTE))
              {
                /* Overrides value */
                fRotation = orxLERP(pstFXSlot->fStartRotation, pstFXSlot->fEndRotation, fEndCoef);

                /* Locks it */
                bRotationLock = orxTRUE;
              }
              else
              {
                orxFLOAT fStartRotation, fEndRotation;

                /* Gets rotation values */
                fStartRotation  = orxLERP(pstFXSlot->fStartRotation, pstFXSlot->fEndRotation, fStartCoef);
                fEndRotation    = orxLERP(pstFXSlot->fStartRotation, pstFXSlot->fEndRotation, fEndCoef);

                /* Updates global alpha value */
                fRotation += fEndRotation - fStartRotation;
              }

              /* Updates rotation status */
              bRotationUpdate = orxTRUE;

              break;
            }

            case orxFX_TYPE_SCALE:
            {
              /* Absolute ? */
              if(orxFLAG_TEST(pstFXSlot->u32Flags, orxFX_SLOT_KU32_FLAG_ABSOLUTE))
              {
                /* Overrides values */
                orxVector_Lerp(&vScale, &(pstFXSlot->vStartScale), &(pstFXSlot->vEndScale), fEndCoef);

                /* Locks it */
                bScaleLock = orxTRUE;

              }
              else
              {
                orxVECTOR vStartScale, vEndScale;

                /* Gets scale values */
                orxVector_Lerp(&vStartScale, &(pstFXSlot->vStartScale), &(pstFXSlot->vEndScale), fStartCoef);
                orxVector_Lerp(&vEndScale, &(pstFXSlot->vStartScale), &(pstFXSlot->vEndScale), fEndCoef);

                /* Updates global scale value */
                orxVector_Mul(&vScale, &vScale, orxVector_Div(&vEndScale, &vEndScale, &vStartScale));
              }

              /* Updates scale status */
              bScaleUpdate = orxTRUE;

              break;
            }

            case orxFX_TYPE_TRANSLATION:
            {
              /* Absolute ? */
              if(orxFLAG_TEST(pstFXSlot->u32Flags, orxFX_SLOT_KU32_FLAG_ABSOLUTE))
              {
                /* Overrides values */
                orxVector_Lerp(&vPosition, &(pstFXSlot->vStartPosition), &(pstFXSlot->vEndPosition), fEndCoef);

                /* Locks it */
                bTranslationLock = orxTRUE;

              }
              else
              {
                orxVECTOR vStartPosition, vEndPosition;

                /* Gets position values */
                orxVector_Lerp(&vStartPosition, &(pstFXSlot->vStartPosition), &(pstFXSlot->vEndPosition), fStartCoef);
                orxVector_Lerp(&vEndPosition, &(pstFXSlot->vStartPosition), &(pstFXSlot->vEndPosition), fEndCoef);

                /* Updates global position value */
                orxVector_Add(&vPosition, &vPosition, orxVector_Sub(&vEndPosition, &vEndPosition, &vStartPosition));
              }

              /* Updates translation status */
              bTranslationUpdate = orxTRUE;

              break;
            }

            default:
            {
              /* !!! MSG !!! */

              break;
            }
          }
          
          /* Update alpha? */
          if(bAlphaUpdate != orxFALSE)
          {
            /* Non absolute? */
            if(bAlphaLock == orxFALSE)
            {
              /* Updates alpha with previous one */
              fAlpha += (orxU2F(orxRGBA_A(stColor)) * orxRGBA_NORMALIZER);
            }

            /* Clamps alpha */
            fAlpha = orxCLAMP(fAlpha, orxFLOAT_0, orxFLOAT_1);

            /* Updates color */
            stColor = orx2RGBA(orxRGBA_R(stColor), orxRGBA_G(stColor), orxRGBA_B(stColor), (orxU8)orxF2U(orx2F(255.0f) * fAlpha));
          }
        }
      }
    }
  }

  /* Done! */
  return eResult;
}

/** Enables/disables an FX
 * @param[in]   _pstFX        Concerned FX
 * @param[in]   _bEnable      enable / disable
 */
orxVOID orxFASTCALL orxFX_Enable(orxFX *_pstFX, orxBOOL _bEnable)
{
  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Enable? */
  if(_bEnable != orxFALSE)
  {
    /* Updates status flags */
    orxStructure_SetFlags(_pstFX, orxFX_KU32_FLAG_ENABLED, orxFX_KU32_FLAG_NONE);
  }
  else
  {
    /* Updates status flags */
    orxStructure_SetFlags(_pstFX, orxFX_KU32_FLAG_NONE, orxFX_KU32_FLAG_ENABLED);
  }

  return;
}

/** Is FX enabled?
 * @param[in]   _pstFX        Concerned FX
 * @return      orxTRUE if enabled, orxFALSE otherwise
 */
orxBOOL orxFASTCALL orxFX_IsEnabled(orxCONST orxFX *_pstFX)
{
  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Done! */
  return(orxStructure_TestFlags((orxFX *)_pstFX, orxFX_KU32_FLAG_ENABLED));
}

/** Adds alpha fade to an FX
 * @param[in]   _pstFX          Concerned FX
 * @param[in]   _fStartTime     Time start
 * @param[in]   _fEndTime       Time end
 * @param[in]   _fCyclePeriod   Cycle period
 * @param[in]   _fCyclePhasis   Cycle phasis (at start)
 * @param[in]   _fStartAlpha    Starting alpha value
 * @param[in]   _fEndAlpha      Ending alpha value
 * @param[in]   _u32Flags       Param flags
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxFX_AddAlphaFade(orxFX *_pstFX, orxFLOAT _fStartTime, orxFLOAT _fEndTime, orxFLOAT _fCyclePeriod, orxFLOAT _fCyclePhasis, orxFLOAT _fStartAlpha, orxFLOAT _fEndAlpha, orxU32 _u32Flags)
{
  orxU32    u32Index;
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxASSERT((_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) == _u32Flags);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Finds empty slot index */
  u32Index = orxFX_FindEmptySlotIndex(_pstFX);

  /* Valid? */
  if(u32Index != orxU32_UNDEFINED)
  {
    orxFX_SLOT *pstFXSlot;

    /* Gets the slot */
    pstFXSlot = &(_pstFX->astFXSlotList[u32Index]);

    /* Updates its parameters */
    pstFXSlot->eFXType      = orxFX_TYPE_ALPHA_FADE;
    pstFXSlot->fStartTime   = _fStartTime;
    pstFXSlot->fEndTime     = _fEndTime;
    pstFXSlot->fCyclePeriod = _fCyclePeriod;
    pstFXSlot->fCyclePhasis = _fCyclePhasis;
    pstFXSlot->fStartAlpha = _fStartAlpha;
    pstFXSlot->fEndAlpha   = _fEndAlpha;
    pstFXSlot->u32Flags     = (_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) | orxFX_SLOT_KU32_FLAG_DEFINED;

    /* Is longer than current FX duration? */
    if(_fEndTime > _pstFX->fDuration)
    {
      /* Updates it */
      _pstFX->fDuration = _fEndTime;
    }
  }

  /* Done! */
  return eResult;
}

/** Adds color blend to an FX
 * @param[in]   _pstFX          Concerned FX
 * @param[in]   _fStartTime     Time start
 * @param[in]   _fEndTime       Time end
 * @param[in]   _fCyclePeriod   Cycle period
 * @param[in]   _fCyclePhasis   Cycle phasis (at start)
 * @param[in]   _stStartColor   Starting color value
 * @param[in]   _stEndColor     Ending color value
 * @param[in]   _u32Flags       Param flags
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxFX_AddColorBlend(orxFX *_pstFX, orxFLOAT _fStartTime, orxFLOAT _fEndTime, orxFLOAT _fCyclePeriod, orxFLOAT _fCyclePhasis, orxRGBA _stStartColor, orxRGBA _stEndColor, orxU32 _u32Flags)
{
  orxU32    u32Index;
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxASSERT((_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) == _u32Flags);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Finds empty slot index */
  u32Index = orxFX_FindEmptySlotIndex(_pstFX);

  /* Valid? */
  if(u32Index != orxU32_UNDEFINED)
  {
    orxFX_SLOT *pstFXSlot;

    /* Gets the slot */
    pstFXSlot = &(_pstFX->astFXSlotList[u32Index]);

    /* Updates its parameters */
    pstFXSlot->eFXType      = orxFX_TYPE_COLOR_BLEND;
    pstFXSlot->fStartTime   = _fStartTime;
    pstFXSlot->fEndTime     = _fEndTime;
    pstFXSlot->fCyclePeriod = _fCyclePeriod;
    pstFXSlot->fCyclePhasis = _fCyclePhasis;
    orxVector_Set(&(pstFXSlot->vStartColor), orxRGBA_NORMALIZER * orxU2F(orxRGBA_R(_stStartColor)), orxRGBA_NORMALIZER * orxU2F(orxRGBA_G(_stStartColor)), orxRGBA_NORMALIZER * orxU2F(orxRGBA_B(_stStartColor)));
    orxVector_Set(&(pstFXSlot->vEndColor), orxRGBA_NORMALIZER * orxU2F(orxRGBA_R(_stEndColor)), orxRGBA_NORMALIZER * orxU2F(orxRGBA_G(_stEndColor)), orxRGBA_NORMALIZER * orxU2F(orxRGBA_B(_stEndColor)));
    pstFXSlot->u32Flags     = (_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) | orxFX_SLOT_KU32_FLAG_DEFINED;

    /* Is longer than current FX duration? */
    if(_fEndTime > _pstFX->fDuration)
    {
      /* Updates it */
      _pstFX->fDuration = _fEndTime;
    }
  }

  /* Done! */
  return eResult;
}

/** Adds rotation to an FX
 * @param[in]   _pstFX          Concerned FX
 * @param[in]   _fStartTime     Time start
 * @param[in]   _fEndTime       Time end
 * @param[in]   _fCyclePeriod   Cycle period
 * @param[in]   _fCyclePhasis   Cycle phasis (at start)
 * @param[in]   _fStartRotation Starting rotation value
 * @param[in]   _fEndRotation   Ending rotation value
 * @param[in]   _u32Flags       Param flags
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxFX_AddRotation(orxFX *_pstFX, orxFLOAT _fStartTime, orxFLOAT _fEndTime, orxFLOAT _fCyclePeriod, orxFLOAT _fCyclePhasis, orxFLOAT _fStartRotation, orxFLOAT _fEndRotation, orxU32 _u32Flags)
{
  orxU32    u32Index;
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxASSERT((_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) == _u32Flags);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Finds empty slot index */
  u32Index = orxFX_FindEmptySlotIndex(_pstFX);

  /* Valid? */
  if(u32Index != orxU32_UNDEFINED)
  {
    orxFX_SLOT *pstFXSlot;

    /* Gets the slot */
    pstFXSlot = &(_pstFX->astFXSlotList[u32Index]);

    /* Updates its parameters */
    pstFXSlot->eFXType        = orxFX_TYPE_ROTATION;
    pstFXSlot->fStartTime     = _fStartTime;
    pstFXSlot->fEndTime       = _fEndTime;
    pstFXSlot->fCyclePeriod   = _fCyclePeriod;
    pstFXSlot->fCyclePhasis   = _fCyclePhasis;
    pstFXSlot->fStartRotation = _fStartRotation;
    pstFXSlot->fEndRotation   = _fEndRotation;
    pstFXSlot->u32Flags       = (_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) | orxFX_SLOT_KU32_FLAG_DEFINED;

    /* Is longer than current FX duration? */
    if(_fEndTime > _pstFX->fDuration)
    {
      /* Updates it */
      _pstFX->fDuration = _fEndTime;
    }
  }

  /* Done! */
  return eResult;
}

/** Adds scale to an FX
 * @param[in]   _pstFX          Concerned FX
 * @param[in]   _fStartTime     Time start
 * @param[in]   _fEndTime       Time end
 * @param[in]   _fCyclePeriod   Cycle period
 * @param[in]   _fCyclePhasis   Cycle phasis (at start)
 * @param[in]   _pvStartScale   Starting scale value
 * @param[in]   _pvEndScale     Ending scale value
 * @param[in]   _u32Flags       Param flags
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxFX_AddScale(orxFX *_pstFX, orxFLOAT _fStartTime, orxFLOAT _fEndTime, orxFLOAT _fCyclePeriod, orxFLOAT _fCyclePhasis, orxCONST orxVECTOR *_pvStartScale, orxCONST orxVECTOR *_pvEndScale, orxU32 _u32Flags)
{
  orxU32    u32Index;
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxASSERT((_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) == _u32Flags);
  orxSTRUCTURE_ASSERT(_pstFX);
  orxASSERT(_pvStartScale != orxNULL);
  orxASSERT(_pvEndScale != orxNULL);

  /* Finds empty slot index */
  u32Index = orxFX_FindEmptySlotIndex(_pstFX);

  /* Valid? */
  if(u32Index != orxU32_UNDEFINED)
  {
    orxFX_SLOT *pstFXSlot;

    /* Gets the slot */
    pstFXSlot = &(_pstFX->astFXSlotList[u32Index]);

    /* Updates its parameters */
    pstFXSlot->eFXType      = orxFX_TYPE_SCALE;
    pstFXSlot->fStartTime   = _fStartTime;
    pstFXSlot->fEndTime     = _fEndTime;
    pstFXSlot->fCyclePeriod = _fCyclePeriod;
    pstFXSlot->fCyclePhasis = _fCyclePhasis;
    orxVector_Copy(&(pstFXSlot->vStartScale), _pvStartScale);
    orxVector_Copy(&(pstFXSlot->vEndScale), _pvEndScale);
    pstFXSlot->u32Flags     = (_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) | orxFX_SLOT_KU32_FLAG_DEFINED;

    /* Is longer than current FX duration? */
    if(_fEndTime > _pstFX->fDuration)
    {
      /* Updates it */
      _pstFX->fDuration = _fEndTime;
    }
  }

  /* Done! */
  return eResult;
}

/** Adds translation to an FX
 * @param[in]   _pstFX          Concerned FX
 * @param[in]   _fStartTime     Time start
 * @param[in]   _fEndTime       Time end
 * @param[in]   _fCyclePeriod   Cycle period
 * @param[in]   _fCyclePhasis   Cycle phasis (at start)
 * @param[in]   _pvStartPosition Starting position value
 * @param[in]   _pvEndPosition  Ending position value
 * @param[in]   _u32Flags       Param flags
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxFX_AddTranslation(orxFX *_pstFX, orxFLOAT _fStartTime, orxFLOAT _fEndTime, orxFLOAT _fCyclePeriod, orxFLOAT _fCyclePhasis, orxCONST orxVECTOR *_pvStartPosition, orxCONST orxVECTOR *_pvEndPosition, orxU32 _u32Flags)
{
  orxU32    u32Index;
  orxSTATUS eResult = orxSTATUS_FAILURE;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxASSERT((_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) == _u32Flags);
  orxSTRUCTURE_ASSERT(_pstFX);
  orxASSERT(_pvStartPosition != orxNULL);
  orxASSERT(_pvEndPosition != orxNULL);

  /* Finds empty slot index */
  u32Index = orxFX_FindEmptySlotIndex(_pstFX);

  /* Valid? */
  if(u32Index != orxU32_UNDEFINED)
  {
    orxFX_SLOT *pstFXSlot;

    /* Gets the slot */
    pstFXSlot = &(_pstFX->astFXSlotList[u32Index]);

    /* Updates its parameters */
    pstFXSlot->eFXType      = orxFX_TYPE_TRANSLATION;
    pstFXSlot->fStartTime   = _fStartTime;
    pstFXSlot->fEndTime     = _fEndTime;
    pstFXSlot->fCyclePeriod = _fCyclePeriod;
    pstFXSlot->fCyclePhasis = _fCyclePhasis;
    orxVector_Copy(&(pstFXSlot->vStartPosition), _pvStartPosition);
    orxVector_Copy(&(pstFXSlot->vEndPosition), _pvEndPosition);
    pstFXSlot->u32Flags     = (_u32Flags & orxFX_SLOT_KU32_MASK_USER_ALL) | orxFX_SLOT_KU32_FLAG_DEFINED;

    /* Is longer than current FX duration? */
    if(_fEndTime > _pstFX->fDuration)
    {
      /* Updates it */
      _pstFX->fDuration = _fEndTime;
    }
  }

  /* Done! */
  return eResult;
}

/** Gets FX duration
 * @param[in]   _pstFX          Concerned FX
 * @return      orxFLOAT
 */
orxFLOAT orxFASTCALL orxFX_GetDuration(orxCONST orxFX *_pstFX)
{
  orxFLOAT fResult;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Updates result */
  fResult = _pstFX->fDuration;

  /* Done! */
  return fResult;
}

/** Tests FX name against given one
 * @param[in]   _pstFX          Concerned FX
 * @param[in]   _zName          Name to test
 * @return      orxTRUE if it's FX name, orxFALSE otherwise
 */
orxBOOL orxFASTCALL orxFX_IsName(orxCONST orxFX *_pstFX, orxCONST orxSTRING _zName)
{
  orxBOOL bResult;

  /* Checks */
  orxASSERT(sstFX.u32Flags & orxFX_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstFX);

  /* Updates result */
  bResult = (orxString_ToCRC(_zName) == _pstFX->u32ID) ? orxTRUE : orxFALSE;

  /* Done! */
  return bResult;
}
