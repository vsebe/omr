/*******************************************************************************
 * Copyright (c) 2000, 2016 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef OMR_MACHINE_BASE_INCL
#define OMR_MACHINE_BASE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_MACHINEBASE_CONNECTOR
#define OMR_MACHINEBASE_CONNECTOR
namespace OMR { class Machine; }
namespace OMR { typedef OMR::Machine MachineConnector; }
#endif

#include <stddef.h>                            // for NULL
#include <stdint.h>                            // for uint8_t, uint32_t
#include "env/TRMemory.hpp"                    // for TR_Memory, etc
#include "infra/Annotations.hpp"               // for OMR_EXTENSIBLE
#include "codegen/RegisterConstants.hpp"

namespace TR { class RealRegister; }
namespace TR { class Machine; }

namespace OMR
{

class OMR_EXTENSIBLE Machine
   {
   uint8_t _numberRegisters[NumRegisterKinds];
   TR_GlobalRegisterNumber _firstGlobalRegisterNumber[NumRegisterKinds];
   TR_GlobalRegisterNumber _lastGlobalRegisterNumber[NumRegisterKinds];
   TR_GlobalRegisterNumber _lastRealRegisterGlobalRegisterNumber;
   TR_GlobalRegisterNumber _overallLastGlobalRegisterNumber;

   public:

   TR_ALLOC(TR_Memory::Machine)

   Machine() : _lastRealRegisterGlobalRegisterNumber(-1), _overallLastGlobalRegisterNumber(-1)
      {
       for(uint32_t i=0;i<NumRegisterKinds;i++)
         {
         _numberRegisters[i]=-1;
         _firstGlobalRegisterNumber[i]=0;
         _lastGlobalRegisterNumber[i]=-1;
         }
      }

   // TODO: numVRFRegs should probably be explicitly set to 0 instead of defaulting to 0
   Machine(uint8_t numIntRegs, uint8_t numFPRegs, uint8_t numVRFRegs = 0) : _lastRealRegisterGlobalRegisterNumber(-1), _overallLastGlobalRegisterNumber(-1)
      {
       for(uint32_t i=0;i<NumRegisterKinds;i++)
         {
         _numberRegisters[i]=0;
         _firstGlobalRegisterNumber[i]=0;
         _lastGlobalRegisterNumber[i]=-1;
         }
       _numberRegisters[TR_GPR] = numIntRegs;
       _numberRegisters[TR_FPR] = numFPRegs;
       _numberRegisters[TR_VRF] = numVRFRegs; // TODO vrf gra : needs this but every platform will need to pass numVRFRegs in
      }

   inline TR::Machine * self();

   uint8_t getNumberOfRegisters(TR_RegisterKinds rk) { return _numberRegisters[rk]; }

   // Lets try and use the genericly named method above. These are only for backward compatibility
   uint8_t getNumberOfGPRs();
   uint8_t getNumberOfFPRs();

   // GlobalRegisterNumbers consiste of real registers in the order of assignment preference. All register kinds combined
   TR_GlobalRegisterNumber getFirstGlobalRegisterNumber(TR_RegisterKinds rk) { return _firstGlobalRegisterNumber[rk]; }
   TR_GlobalRegisterNumber getLastGlobalRegisterNumber(TR_RegisterKinds rk) { return _lastGlobalRegisterNumber[rk]; }
   TR_GlobalRegisterNumber getLastRealRegisterGlobalRegisterNumber() { return _lastRealRegisterGlobalRegisterNumber; }
   TR_GlobalRegisterNumber getLastGlobalRegisterNumber() { return _overallLastGlobalRegisterNumber; }
   virtual TR::RealRegister *getRealRegister(TR_GlobalRegisterNumber grn) {return NULL; }
   TR_GlobalRegisterNumber getNextGlobalRegisterNumber() { return (++_overallLastGlobalRegisterNumber); }

   protected:
   // setters should only be used by specific machine class initializers

   uint8_t setNumberOfRegisters(TR_RegisterKinds rk, uint8_t num) { return (_numberRegisters[rk] = num); }
   uint8_t setNumberOfGPRs(uint8_t numIntRegs);
   uint8_t setNumberOfFPRs(uint8_t numFPRegs);

   TR_GlobalRegisterNumber setFirstGlobalRegisterNumber(TR_RegisterKinds rk, TR_GlobalRegisterNumber grn) { return (_firstGlobalRegisterNumber[rk]=grn); }
   TR_GlobalRegisterNumber setLastGlobalRegisterNumber(TR_RegisterKinds rk, TR_GlobalRegisterNumber grn) { return (_lastGlobalRegisterNumber[rk]=grn); }
   TR_GlobalRegisterNumber setLastRealRegisterGlobalRegisterNumber(TR_GlobalRegisterNumber grn) { _overallLastGlobalRegisterNumber=grn; return (_lastRealRegisterGlobalRegisterNumber=grn); }

   };

}

#endif
