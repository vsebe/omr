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

#include "codegen/IA32LinkageUtils.hpp"

#include <stddef.h>                          // for NULL
#include <stdint.h>                          // for int32_t
#include "codegen/CodeGenerator.hpp"         // for CodeGenerator
#include "codegen/Machine.hpp"               // for Machine
#include "codegen/MemoryReference.hpp"       // for generateX86MemoryReference, etc
#include "codegen/RealRegister.hpp"          // for RealRegister
#include "codegen/Register.hpp"              // for Register
#include "codegen/RegisterConstants.hpp"     // for TR_RegisterKinds::TR_FPR
#include "compile/Compilation.hpp"           // for Compilation
#include "compile/SymbolReferenceTable.hpp"  // for SymbolReferenceTable
#include "env/jittypes.h"                    // for uintptrj_t
#include "il/ILOpCodes.hpp"                  // for ILOpCodes::dbits2l, etc
#include "il/ILOps.hpp"                      // for ILOpCode
#include "il/Node.hpp"                       // for Node
#include "il/Node_inlines.hpp"               // for Node::getReferenceCount, etc
#include "il/Symbol.hpp"                     // for Symbol
#include "il/SymbolReference.hpp"            // for SymbolReference
#include "il/symbol/StaticSymbol.hpp"        // for StaticSymbol
#include "infra/Assert.hpp"                  // for TR_ASSERT
#include "x/codegen/X86Instruction.hpp"      // for generateImmInstruction, etc
#include "x/codegen/X86Ops.hpp"              // for ::PUSHImm4, ::PUSHImms, etc

namespace TR
{

TR::Register *IA32LinkageUtils::pushIntegerWordArg(
      TR::Node *child,
      TR::CodeGenerator *cg)
   {
   TR::Register *pushRegister;
   if (child->getRegister() == NULL)
      {
      if (child->getOpCode().isLoadConst())
         {
         int32_t value = child->getInt();
         TR_X86OpCodes pushOp;
         if (value >= -128 && value <= 127)
            {
            pushOp = PUSHImms;
            }
         else
            {
            pushOp = PUSHImm4;
            }

         generateImmInstruction(pushOp, child, value, cg);
         cg->decReferenceCount(child);
         return NULL;
         }
      else if (child->getOpCodeValue() == TR::loadaddr)
         {
         TR::SymbolReference * symRef = child->getSymbolReference();
         TR::StaticSymbol *sym = symRef->getSymbol()->getStaticSymbol();
         if (sym)
            {
            TR_ASSERT(!symRef->isUnresolved(), "pushIntegerWordArg loadaddr expecting resolved symbol");
            generateImmSymInstruction(PUSHImm4, child, (uintptrj_t)sym->getStaticAddress(), symRef, cg);
            cg->decReferenceCount(child);
            return NULL;
            }
         }
      else if (child->getOpCodeValue() == TR::fbits2i &&
               !child->normalizeNanValues() &&
               child->getReferenceCount() == 1)
         {
         pushRegister = pushFloatArg(child->getFirstChild(), cg);
         cg->decReferenceCount(child);
         return pushRegister;
         }
      else if (child->getOpCode().isMemoryReference() &&
               (child->getReferenceCount() == 1) &&
               (child->getSymbolReference() != cg->comp()->getSymRefTab()->findVftSymbolRef()))
         {
         TR::MemoryReference  *tempMR = generateX86MemoryReference(child, cg);
         generateMemInstruction(PUSHMem, child, tempMR, cg);
         tempMR->decNodeReferenceCounts(cg);
         cg->decReferenceCount(child);
         return NULL;
         }
      }

   pushRegister = cg->evaluate(child);
   generateRegInstruction(PUSHReg, child, pushRegister, cg);
   cg->decReferenceCount(child);
   return pushRegister;
   }


TR::Register *IA32LinkageUtils::pushLongArg(
      TR::Node *child,
      TR::CodeGenerator *cg)
   {
   TR::Register *pushRegister;
   if (child->getRegister() == NULL)
      {
      if (child->getOpCode().isLoadConst())
         {
         TR_X86OpCodes pushOp;

         int32_t highValue = child->getLongIntHigh();
         if (highValue >= -128 && highValue <= 127)
            {
            pushOp = PUSHImms;
            }
         else
            {
            pushOp = PUSHImm4;
            }
         generateImmInstruction(pushOp, child, highValue, cg);

         int32_t lowValue = child->getLongIntLow();
         if (lowValue >= -128 && lowValue <= 127)
            {
            pushOp = PUSHImms;
            }
         else
            {
            pushOp = PUSHImm4;
            }
         generateImmInstruction(pushOp, child, lowValue, cg);
         cg->decReferenceCount(child);
         return NULL;
         }
      else if (child->getOpCodeValue() == TR::dbits2l &&
               !child->normalizeNanValues() &&
               child->getReferenceCount() == 1)
         {
         pushRegister = pushDoubleArg(child->getFirstChild(), cg);
         cg->decReferenceCount(child);
         return pushRegister;
         }
      else if (child->getOpCode().isMemoryReference() &&
                child->getReferenceCount() == 1)
         {
         TR::MemoryReference  *lowMR = generateX86MemoryReference(child, cg);
         generateMemInstruction(PUSHMem, child, generateX86MemoryReference(*lowMR,4, cg), cg);
         generateMemInstruction(PUSHMem, child, lowMR, cg);
         lowMR->decNodeReferenceCounts(cg);
         return NULL;
         }
      }

   pushRegister = cg->evaluate(child);
   generateRegInstruction(PUSHReg, child, pushRegister->getHighOrder(), cg);
   generateRegInstruction(PUSHReg, child, pushRegister->getLowOrder(), cg);
   cg->decReferenceCount(child);
   return pushRegister;
   }


TR::Register *IA32LinkageUtils::pushFloatArg(
      TR::Node *child,
      TR::CodeGenerator *cg)
   {
   TR::Register *pushRegister;
   if (child->getRegister() == NULL)
      {
      if (child->getOpCodeValue() == TR::fconst)
         {
         int32_t value = child->getFloatBits();
         TR_X86OpCodes pushOp;
         if (value >= -128 && value <= 127)
            {
            pushOp = PUSHImms;
            }
         else
            {
            pushOp = PUSHImm4;
            }
         generateImmInstruction(pushOp, child, value, cg);
         cg->decReferenceCount(child);
         return NULL;
         }
      else if (child->getReferenceCount() == 1)
         {
         if (child->getOpCode().isLoad())
            {
            TR::MemoryReference  *tempMR = generateX86MemoryReference(child, cg);
            generateMemInstruction(PUSHMem, child, tempMR, cg);
            tempMR->decNodeReferenceCounts(cg);
            cg->decReferenceCount(child);
            return NULL;
            }
      else if (child->getOpCodeValue() == TR::ibits2f)
         {
         pushRegister = pushIntegerWordArg(child->getFirstChild(), cg);
         cg->decReferenceCount(child);
         return pushRegister;
         }
      }
   }

   pushRegister = cg->evaluate(child);
   TR::RealRegister *espReal = cg->machine()->getX86RealRegister(TR::RealRegister::esp);
   generateRegImmInstruction(SUB4RegImms, child, espReal, 4, cg);

   if (cg->useSSEForSinglePrecision() && pushRegister->getKind() == TR_FPR)
      generateMemRegInstruction(MOVSSMemReg, child, generateX86MemoryReference(espReal, 0, cg), pushRegister, cg);
   else
      generateFPMemRegInstruction(FSTMemReg, child, generateX86MemoryReference(espReal, 0, cg), pushRegister, cg);

   cg->decReferenceCount(child);
   return pushRegister;
   }


TR::Register *IA32LinkageUtils::pushDoubleArg(
      TR::Node *child,
      TR::CodeGenerator *cg)
   {
   TR::Register *pushRegister;
   if (child->getRegister() == NULL)
      {
      if (child->getOpCodeValue() == TR::dconst)
         {
         TR_X86OpCodes pushOp;

         int32_t highValue = child->getLongIntHigh();
         if (highValue >= -128 && highValue <= 127)
            {
            pushOp = PUSHImms;
            }
         else
            {
            pushOp = PUSHImm4;
            }
         generateImmInstruction(pushOp, child, highValue, cg);

         int32_t lowValue = child->getLongIntLow();
         if (lowValue >= -128 && lowValue <= 127)
            {
            pushOp = PUSHImms;
            }
         else
            {
            pushOp = PUSHImm4;
            }
         generateImmInstruction(pushOp, child, lowValue, cg);
         cg->decReferenceCount(child);
         return NULL;
         }
      else if (child->getReferenceCount() == 1)
         {
         if (child->getOpCode().isLoad())
            {
            TR::MemoryReference  *lowMR = generateX86MemoryReference(child, cg);
            generateMemInstruction(PUSHMem, child, generateX86MemoryReference(*lowMR, 4, cg), cg);
            generateMemInstruction(PUSHMem, child, lowMR, cg);
            lowMR->decNodeReferenceCounts(cg);
            cg->decReferenceCount(child);
            return NULL;
            }
         else if (child->getOpCodeValue() == TR::lbits2d)
            {
            pushRegister = pushLongArg(child->getFirstChild(), cg);
            cg->decReferenceCount(child);
            return pushRegister;
            }
         }
      }

   pushRegister = cg->evaluate(child);
   TR::RealRegister *espReal = cg->machine()->getX86RealRegister(TR::RealRegister::esp);
   generateRegImmInstruction(SUB4RegImms, child, espReal, 8, cg);

   if (cg->useSSEForSinglePrecision() && pushRegister->getKind() == TR_FPR)
      generateMemRegInstruction(MOVSDMemReg, child, generateX86MemoryReference(espReal, 0, cg), pushRegister, cg);
   else
      generateFPMemRegInstruction(DSTMemReg, child, generateX86MemoryReference(espReal, 0, cg), pushRegister, cg);

   cg->decReferenceCount(child);
   return pushRegister;
   }

}
