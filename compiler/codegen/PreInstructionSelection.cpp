/*******************************************************************************
 * Copyright (c) 2000, 2017 IBM Corp. and others
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

#include "codegen/OMRCodeGenerator.hpp"

#include <stddef.h>                            // for NULL
#include <stdint.h>                            // for uint16_t
#include "codegen/CodeGenerator.hpp"           // for CodeGenerator, etc
#include "codegen/CodeGenerator_inlines.hpp"
#include "codegen/FrontEnd.hpp"                // for TR_FrontEnd
#include "codegen/Linkage.hpp"                 // for Linkage
#include "codegen/RealRegister.hpp"            // for RealRegister
#include "compile/Compilation.hpp"             // for Compilation
#include "compile/Method.hpp"                  // for TR_Method
#include "compile/ResolvedMethod.hpp"          // for TR_ResolvedMethod
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "control/Recompilation.hpp"
#include "env/CompilerEnv.hpp"
#include "env/TRMemory.hpp"                    // for TR_HeapMemory
#include "il/Block.hpp"                        // for Block
#include "il/DataTypes.hpp"                    // for DataTypes::Address, etc
#include "il/ILOpCodes.hpp"                    // for ILOpCodes::BBStart, etc
#include "il/ILOps.hpp"                        // for TR::ILOpCode, etc
#include "il/Node.hpp"                         // for Node
#include "il/Node_inlines.hpp"                 // for Node::getChild, etc
#include "il/Symbol.hpp"                       // for Symbol
#include "il/SymbolReference.hpp"              // for SymbolReference
#include "il/TreeTop.hpp"                      // for TreeTop
#include "il/TreeTop_inlines.hpp"              // for TreeTop::getNode, etc
#include "il/symbol/AutomaticSymbol.hpp"       // for AutomaticSymbol
#include "il/symbol/LabelSymbol.hpp"           // for LabelSymbol
#include "il/symbol/MethodSymbol.hpp"          // for MethodSymbol
#include "il/symbol/ResolvedMethodSymbol.hpp"
#include "infra/Assert.hpp"                    // for TR_ASSERT
#include "infra/BitVector.hpp"                 // for TR_BitVector
#include "infra/List.hpp"                      // for ListIterator, List
#include "infra/CfgEdge.hpp"                   // for CFGEdge

#define OPT_DETAILS "O^O PRE-INSTRUCTION SELECTION: "


void
OMR::CodeGenerator::setUpStackSizeForCallNode(TR::Node *node)
   {
   uint32_t currentArgSize = 0;

   for (int32_t i = node->getFirstArgumentIndex(); i < node->getNumChildren(); ++i)
      {
      int32_t roundedSize = node->getChild(i)->getRoundedSize();

      if (TR::Compiler->target.is64Bit() && node->getChild(i)->getDataType() != TR::Address)
         {
         currentArgSize += roundedSize * 2;
         }
      else
         {
         currentArgSize += roundedSize;
         }
      }

   if (currentArgSize > self()->getLargestOutgoingArgSize())
      {
      self()->setLargestOutgoingArgSize(currentArgSize);
      }
   }


void
OMR::CodeGenerator::eliminateLoadsOfLocalsThatAreNotStored(
      TR::Node *node,
      int32_t childNum)
   {
   if (node->getVisitCount() == self()->comp()->getVisitCount())
      {
      return;
      }

   node->setVisitCount(self()->comp()->getVisitCount());

   if (node->getOpCode().isLoadVarDirect() &&
       node->getSymbolReference()->getSymbol()->isAuto() &&
       (node->getSymbolReference()->getReferenceNumber() < _numLocalsWhenStoreAnalysisWasDone) &&
       !node->getSymbol()->castToAutoSymbol()->isLiveLocalIndexUninitialized() &&
       (!_liveButMaybeUnreferencedLocals ||
        !_liveButMaybeUnreferencedLocals->get(node->getSymbol()->castToAutoSymbol()->getLiveLocalIndex())) &&
       !_localsThatAreStored->get(node->getSymbolReference()->getReferenceNumber()) &&
       performTransformation(self()->comp(), "%sRemoving dead load of sym ref %d at %p\n", OPT_DETAILS, node->getSymbolReference()->getReferenceNumber(), node))

      {
      TR::Node::recreate(node, self()->comp()->il.opCodeForConst(node->getSymbolReference()->getSymbol()->getDataType()));
      node->setLongInt(0);
      return;
      }

   int32_t i;
   for (i=0; i < node->getNumChildren(); i++)
      {
      self()->eliminateLoadsOfLocalsThatAreNotStored(node->getChild(i), i);
      }
   }


void
OMR::CodeGenerator::prepareNodeForInstructionSelection(TR::Node *node)
   {
   if (node->getVisitCount() == self()->comp()->getVisitCount())
      {
      if (node->getOpCode().hasSymbolReference() && node->getSymbolReference()->isTempVariableSizeSymRef())
         {
         // bcd loads and loadaddr's do not get put into registers so must increment the symbol reference count for commoned nodes too
         TR::AutomaticSymbol *local = node->getSymbol()->getAutoSymbol();
         TR_ASSERT(local,"a tempMemSlot should have an auto symbol\n");
         local->incReferenceCount();
         }

      return;
      }

   if (node->getOpCode().hasSymbolReference())
      {
      TR::AutomaticSymbol *local = node->getSymbol()->getAutoSymbol();
      if (local)
         {
         local->incReferenceCount();
         }
      }

   node->setVisitCount(self()->comp()->getVisitCount());
   node->setRegister(NULL);
   node->setHasBeenVisitedForHints(false); // clear this flag for addStorageReferenceHints pass

   for (int32_t childCount = node->getNumChildren() - 1; childCount >= 0; childCount--)
      {
      self()->prepareNodeForInstructionSelection(node->getChild(childCount));
      }
   }
