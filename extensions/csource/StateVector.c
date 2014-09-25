/*------------------------------------------------------------------------------
! . File      : StateVector.c
! . Program   : pDynamo-1.8.0                           (http://www.pdynamo.org)
! . Copyright : CEA, CNRS, Martin  J. Field  (2007-2012),
!                          Mikolaj J. Feliks (2014)
! . License   : CeCILL French Free Software License     (http://www.cecill.info)
!-----------------------------------------------------------------------------*/
#include "StateVector.h"


StateVector *StateVector_Allocate (const Integer length) {
  StateVector *self = NULL;

  MEMORY_ALLOCATE (self, StateVector);
  if (self != NULL) {
    self->vector    = NULL;
    self->minvector = NULL;
    self->maxvector = NULL;
    self->substate  = NULL;
    self->length    = length;
    self->slength   = 0;

    if (length > 0) {
      MEMORY_ALLOCATEARRAY (self->vector, length, Integer);
      if (self->vector == NULL) {
        MEMORY_DEALLOCATE (self);
      }
      else {
        MEMORY_ALLOCATEARRAY (self->minvector, length, Integer);
        if (self->minvector == NULL) {
          MEMORY_DEALLOCATE (self->vector);
          MEMORY_DEALLOCATE (self);
        }
        else {
          MEMORY_ALLOCATEARRAY (self->maxvector, length, Integer);
          if (self->maxvector == NULL) {
            MEMORY_DEALLOCATE (self->minvector);
            MEMORY_DEALLOCATE (self->vector);
            MEMORY_DEALLOCATE (self);
          }
        }
      }
    }
  }
  return self;
}

void StateVector_Deallocate (StateVector *self) {
  if (self != NULL) {
    MEMORY_DEALLOCATE (self->maxvector);
    MEMORY_DEALLOCATE (self->minvector);
    MEMORY_DEALLOCATE (self->vector);

    /* Deallocate substate, if exists */
    if (self->substate != NULL) {
      MEMORY_DEALLOCATE (self->substate);
    }

    MEMORY_DEALLOCATE (self);
  }
}

void StateVector_Reset (const StateVector *self) {
  Integer   i;
  Integer   *v = self->vector, *m = self->minvector;
  for (i = 0; i < self->length; i++, v++, m++) {
    *v = *m;
  }
}

void StateVector_ResetToMaximum (const StateVector *self) {
  Integer   i;
  Integer   *v = self->vector, *m = self->maxvector;
  for (i = 0; i < self->length; i++, v++, m++) {
    *v = *m;
  }
}

/*
 Get the local index of an instance of a site, usually 0 and 1 for most sites or 0, 1, 2, 3 for histidines
*/
Integer StateVector_GetItem (const StateVector *self, const Integer index) {
  if (index < 0 || index > (self->length - 1)) {
    return -1;
  }
  else {
    return (self->vector[index] - self->minvector[index]);
  }
}

Boolean StateVector_SetItem (const StateVector *self, const Integer index, const Integer value) {
  Integer valueActual;

  if (index < 0 || index > (self->length - 1)) {
    return False;
  }
  else {
    valueActual = value + self->minvector[index];
    if (valueActual < self->minvector[index] || valueActual > self->maxvector[index]) {
      return False;
    }
    else {
      self->vector[index] = valueActual;
      return True;
    }
  }
}

/*
 Get the actual content of the state vector, i.e. global index of an instance in the central arrays (_protons, _intrinsic, _interactions)
*/
Integer StateVector_GetActualItem (const StateVector *self, const Integer index) {
  if (index < 0 || index > (self->length - 1)) {
    return -1;
  }
  else {
    return self->vector[index];
  }
}

Boolean StateVector_SetActualItem (const StateVector *self, const Integer index, const Integer value) {
  if (index < 0 || index > (self->length - 1)) {
    return False;
  }
  else if (value < self->minvector[index] || value > self->maxvector[index]) {
    return False;
  }
  else {
    self->vector[index] = value;
    return True;
  }
}

/* Incrementation algorithm by Timm Essigke 

One could write a code that prevents zeroing the state vector after the last iteration
*/
Boolean StateVector_Increment (const StateVector *self) {
  Integer i;
  Integer *v = self->vector, *minv = self->minvector, *maxv = self->maxvector;

  for (i = 0; i < self->length; i++, v++, minv++, maxv++) {
    if ((*v) < (*maxv)) {
      (*v)++;
      return True;
    }
    else {
      *v = *minv;
    }
  }
  return False;
}

/*-----------------------------------------------------------------------------
  Substate-related functions
-----------------------------------------------------------------------------*/
Boolean StateVector_AllocateSubstate (StateVector *self, const Integer nsites) {
  if (self->substate != NULL) {
    /* Substate already allocated */
    return False;
  }
  else {
    MEMORY_ALLOCATEARRAY (self->substate, nsites, Integer);
    if (self->substate == NULL) {
      /* Substate allocation failed */
      return False;
    }
    self->slength = nsites;
    return True;
  }
}

/*  
    |selectedSiteIndex| is an index of the site to increment within the substate

    |index| is an index in the substate's array of selectedSiteIndices
*/
Boolean StateVector_SetSubstateItem (const StateVector *self, const Integer selectedSiteIndex, const Integer index) {
  if (index < 0 || index > (self->slength - 1)) {
    return False;
  }
  else if (selectedSiteIndex < 0 || (selectedSiteIndex > self->length - 1)) {
    return False;
  }
  else {
    self->substate[index] = selectedSiteIndex;
    return True;
  }
}

Integer StateVector_GetSubstateItem (const StateVector *self, const Integer index) {
  if (index < 0 || index > (self->slength - 1)) {
    return -1;
  }
  else {
    return self->substate[index];
  }
}

void StateVector_ResetSubstate (const StateVector *self) {
  Integer i;
  Integer *siteIndex = self->substate;

  if (self->substate != NULL) {
    for (i = 0; i < self->slength; i++, siteIndex++) {
      self->vector[*siteIndex] = self->minvector[*siteIndex];
    }
  }
}

Boolean StateVector_IncrementSubstate (const StateVector *self) {
  Integer i, site, maxsite;
  Integer *siteIndex = self->substate;

  if (self->substate != NULL) {
    for (i = 0; i < self->slength; i++, siteIndex++) {
      site    = self->vector    [*siteIndex];
      maxsite = self->maxvector [*siteIndex];
  
      if (site < maxsite) {
        site++;
        self->vector[*siteIndex] = site;
        return True;
      }
      else {
        self->vector[*siteIndex] = self->minvector[*siteIndex];
      }
    }
    return False;
  }
  else {
    return False;
  }
}

/*-----------------------------------------------------------------------------
  Calculating microstate energy
-----------------------------------------------------------------------------*/
Real StateVector_CalculateMicrostateEnergy (const StateVector *self, const Integer1DArray *protons, const Real1DArray *intrinsic, const Real2DArray *interactions, const Real pH, const Real temperature) {
  Real Gintr = 0.0, W = 0.0;
  Integer nprotons = 0, siteIndex, siteIndexInner, *instanceIndex, *instanceIndexInner;

  for (siteIndex = 0, instanceIndex = self->vector; siteIndex < self->length; siteIndex++, instanceIndex++) {
    nprotons += Integer1DArray_Item (protons, *instanceIndex);
    Gintr += Real1DArray_Item (intrinsic, *instanceIndex);

    for (siteIndexInner = 0, instanceIndexInner = self->vector; siteIndexInner < siteIndex; siteIndexInner++, instanceIndexInner++) {
      W += Real2DArray_Item (interactions, *instanceIndex, *instanceIndexInner);
    }
  }
  return (Gintr - nprotons * (-CONSTANT_MOLAR_GAS_KCAL_MOL * temperature * CONSTANT_LN10 * pH) + W);
}

/*-----------------------------------------------------------------------------
  Calculating probabilities analytically
-----------------------------------------------------------------------------*/
Boolean StateVector_CalculateProbabilitiesAnalytically (const StateVector *self, const Integer1DArray *protons, const Real1DArray *intrinsic, const Real2DArray *interactions, const Real pH, const Real temperature, const Integer nstates, Real1DArray *probabilities) {
  Real1DArray *bfactors;
  Real        *bfactor;
  Real         energy, energyZero, bsum;
  Integer     *activeInstanceGlobalIndex;
  Integer      stateIndex, siteIndex;
  Status       status;

  bfactors = Real1DArray_Allocate (nstates, &status);
  if (bfactors == NULL) {
    return False;
  }

  for (stateIndex = 0, bfactor = bfactors->data; stateIndex < nstates; stateIndex++, bfactor++) {
    energy = StateVector_CalculateMicrostateEnergy (self, protons, intrinsic, interactions, pH, temperature);

    if (stateIndex < 1) {
      energyZero = energy;
    }
    else {
      if (energy < energyZero) {
        energyZero = energy;
      }
    }

    *bfactor = energy;
    StateVector_Increment (self);
  }

  Real1DArray_AddScalar (bfactors, -energyZero);
  Real1DArray_Scale (bfactors, -1. / (CONSTANT_MOLAR_GAS_KCAL_MOL * temperature));
  Real1DArray_Exp (bfactors);

  Real1DArray_Set (probabilities, 0.);
  StateVector_Reset (self);

  for (stateIndex = 0, bfactor = bfactors->data; stateIndex < nstates; stateIndex++, bfactor++) {
    for (siteIndex = 0, activeInstanceGlobalIndex = self->vector; siteIndex < self->length; siteIndex++, activeInstanceGlobalIndex++) {
      probabilities->data[*activeInstanceGlobalIndex] += *bfactor;
    }
    StateVector_Increment (self);
  }

  bsum = Real1DArray_Sum (bfactors);
  Real1DArray_Scale (probabilities, 1. / bsum);

  Real1DArray_Deallocate (&bfactors);

  return True;
}


/*
StateVector *StateVector_Clone (const StateVector *self) {
  StateVector *clone = NULL;
  if (self != NULL) {
    clone = StateVector_Allocate (self->length);
    StateVector_CopyTo (self, clone);
  }

  return clone;
}

void StateVector_CopyTo (const StateVector *self, StateVector *other) {
  Integer i, length, *src, *dst;

  if (self != NULL && other != NULL) {
    other->length    = self->length;
    other->slength   = self->slength;

    length = self->length;
    if (length > other->length) {
      length = other->length;
    }

    for (i = 0, src = self->vector, dst = other->vector; i < length; i++, src++, dst++) {
      *dst = *src;
    }
    for (i = 0, src = self->minvector, dst = other->minvector; i < length; i++, src++, dst++) {
      *dst = *src;
    }
    for (i = 0, src = self->maxvector, dst = other->maxvector; i < length; i++, src++, dst++) {
      *dst = *src;
    }

    if (self->substate != NULL && other->substate != NULL) {
      for (i = 0, src = self->substate, dst = other->substate; i < length; i++, src++, dst++) {
        *dst = *src;
      }
    }
  }
}
*/
