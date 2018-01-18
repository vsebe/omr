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

#include "p/codegen/PPCHelperCallSnippet.hpp"

#include <stddef.h>                            // for NULL
#include <stdint.h>                            // for uint8_t, int32_t, etc
#include "codegen/CodeGenerator.hpp"           // for CodeGenerator
#include "codegen/FrontEnd.hpp"                // for TR_FrontEnd
#include "codegen/InstOpCode.hpp"              // for InstOpCode, etc
#include "codegen/Machine.hpp"                 // for Machine, UPPER_IMMED
#include "codegen/RealRegister.hpp"            // for RealRegister
#include "codegen/SnippetGCMap.hpp"
#include "compile/Compilation.hpp"             // for Compilation, comp
#include "env/IO.hpp"                          // for POINTER_PRINTF_FORMAT
#include "env/jittypes.h"                      // for intptrj_t
#include "il/DataTypes.hpp"                    // for TR::DataType
#include "il/ILOpCodes.hpp"                    // for ILOpCodes::arraycopy
#include "il/ILOps.hpp"                        // for ILOpCode
#include "il/Node.hpp"                         // for Node
#include "il/Node_inlines.hpp"                 // for Node::getChild, etc
#include "il/Symbol.hpp"                       // for Symbol
#include "il/SymbolReference.hpp"              // for SymbolReference
#include "il/symbol/LabelSymbol.hpp"           // for LabelSymbol
#include "il/symbol/MethodSymbol.hpp"          // for MethodSymbol
#include "infra/Assert.hpp"                    // for TR_ASSERT
#include "ras/Debug.hpp"                       // for TR_Debug
#include "runtime/Runtime.hpp"                 // for BRANCH_BACKWARD_LIMIT, etc

uint8_t *TR::PPCHelperCallSnippet::emitSnippetBody()
   {
   uint8_t             *buffer = cg()->getBinaryBufferCursor();
   uint8_t             *gtrmpln, *trmpln;

   getSnippetLabel()->setCodeLocation(buffer);

   return genHelperCall(buffer);
   }


void
TR_Debug::print(TR::FILE *pOutFile, TR::PPCHelperCallSnippet * snippet)
   {
   uint8_t *cursor = snippet->getSnippetLabel()->getCodeLocation();
   TR::LabelSymbol *restartLabel = snippet->getRestartLabel();

   if (snippet->getKind() == TR::Snippet::IsArrayCopyCall)
      {
      cursor = print(pOutFile, (TR::PPCArrayCopyCallSnippet *)snippet, cursor);
      }
   else
      {
      printSnippetLabel(pOutFile, snippet->getSnippetLabel(), cursor, "Helper Call Snippet");
      }

   char    *info = "";
   int32_t  distance;
   if (isBranchToTrampoline(snippet->getDestination(), cursor, distance))
      info = " Through trampoline";

   printPrefix(pOutFile, NULL, cursor, 4);
   distance = *((int32_t *) cursor) & 0x03fffffc;
   distance = (distance << 6) >> 6;   // sign extend
   trfprintf(pOutFile, "%s \t" POINTER_PRINTF_FORMAT "\t\t; %s %s",
      restartLabel ? "bl" : "b", (intptrj_t)cursor + distance, getName(snippet->getDestination()), info);

   if (restartLabel)
      {
      cursor += 4;
      printPrefix(pOutFile, NULL, cursor, 4);
      distance = *((int32_t *) cursor) & 0x03fffffc;
      distance = (distance << 6) >> 6;   // sign extend
      trfprintf(pOutFile, "b \t" POINTER_PRINTF_FORMAT "\t\t; Restart", (intptrj_t)cursor + distance);
      }
   }

uint32_t TR::PPCHelperCallSnippet::getLength(int32_t estimatedSnippetStart)
   {
   return getHelperCallLength();
   }

uint8_t *TR::PPCHelperCallSnippet::genHelperCall(uint8_t *buffer)
   {
   intptrj_t distance = (intptrj_t)getDestination()->getSymbol()->castToMethodSymbol()->getMethodAddress() - (intptrj_t)buffer;

   if (!(distance>=BRANCH_BACKWARD_LIMIT && distance<=BRANCH_FORWARD_LIMIT))
      {
      distance = cg()->comp()->fe()->indexedTrampolineLookup(getDestination()->getReferenceNumber(), (void *)buffer) - (intptrj_t)buffer;
      TR_ASSERT(distance>=BRANCH_BACKWARD_LIMIT && distance<=BRANCH_FORWARD_LIMIT,
             "CodeCache is more than 32MB.\n");
      }

   // b|bl distance
   *(int32_t *)buffer = 0x48000000 | (distance & 0x03fffffc);
   if (_restartLabel != NULL)
      {
      *(int32_t *)buffer |= 0x00000001;
      }

   cg()->addProjectSpecializedRelocation(buffer,(uint8_t *)getDestination(), NULL, TR_HelperAddress,
                          __FILE__, __LINE__, getNode());
   buffer += 4;

   gcMap().registerStackMap(buffer, cg());

   if (_restartLabel != NULL)
      {
      int32_t returnDistance = _restartLabel->getCodeLocation() - buffer;
      *(int32_t *)buffer = 0x48000000 | (returnDistance & 0x03fffffc);
      buffer += 4;
      }

   return buffer;
   }

uint32_t TR::PPCHelperCallSnippet::getHelperCallLength()
   {
   return ((_restartLabel==NULL)?4:8);
   }

uint8_t *TR::PPCArrayCopyCallSnippet::emitSnippetBody()
   {
   TR::Node *node = getNode();
   TR_ASSERT(node->getOpCodeValue() == TR::arraycopy &&
          node->getChild(2)->getOpCode().isLoadConst(), "only valid for arraycopies with a constant length\n");

   uint8_t *buffer = cg()->getBinaryBufferCursor();
   getSnippetLabel()->setCodeLocation(buffer);
   TR::RealRegister *lengthReg = cg()->machine()->getPPCRealRegister(_lengthRegNum);
   TR::Node *lengthNode = node->getChild(2);
   int64_t byteLen = (lengthNode->getType().isInt32() ?
                      lengthNode->getInt() : lengthNode->getLongInt());
   TR::InstOpCode opcode;

   // li lengthReg, #byteLen
   opcode.setOpCodeValue(TR::InstOpCode::li);
   buffer = opcode.copyBinaryToBuffer(buffer);
   lengthReg->setRegisterFieldRT((uint32_t *)buffer);
   TR_ASSERT(byteLen <= UPPER_IMMED,"byteLen too big to encode\n");
   *(int32_t *)buffer |= byteLen;
   buffer += 4;

   return TR::PPCHelperCallSnippet::genHelperCall(buffer);
   }

uint8_t*
TR_Debug::print(TR::FILE *pOutFile, TR::PPCArrayCopyCallSnippet *snippet, uint8_t *cursor)
   {
   printSnippetLabel(pOutFile, snippet->getSnippetLabel(), cursor, "ArrayCopy Helper Call Snippet");

   TR::RealRegister *lengthReg = _cg->machine()->getPPCRealRegister(snippet->getLengthRegNum());

   printPrefix(pOutFile, NULL, cursor, 4);
   trfprintf(pOutFile, "li \t%s, %d", getName(lengthReg), *((int32_t *) cursor) & 0x0000ffff);
   cursor += 4;

   return cursor;
   }

uint32_t TR::PPCArrayCopyCallSnippet::getLength(int32_t estimatedSnippetStart)
   {
   return TR::PPCHelperCallSnippet::getHelperCallLength() + 4;
   }
