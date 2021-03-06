/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2000, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/

#ifndef TR_REGISTER_DEPENDENCY_INCL
#define TR_REGISTER_DEPENDENCY_INCL

#include "codegen/OMRRegisterDependency.hpp"

#include "codegen/RegisterDependencyStruct.hpp"

class TR_Memory;
namespace TR { class CodeGenerator; }
namespace TR { class Node; }
namespace TR { class Register; }
template <typename ListKind> class List;

namespace TR
{

class RegisterDependencyConditions : public OMR::RegisterDependencyConditionsConnector
   {
   public:

   RegisterDependencyConditions() : OMR::RegisterDependencyConditionsConnector () {}

   RegisterDependencyConditions(TR_X86RegisterDependencyIndex numPreConds, TR_X86RegisterDependencyIndex numPostConds, TR_Memory * m) :
      OMR::RegisterDependencyConditionsConnector(numPreConds, numPostConds, m) {}

   RegisterDependencyConditions(TR::Node *node, TR::CodeGenerator *cg, TR_X86RegisterDependencyIndex additionalRegDeps = 0, List<TR::Register> *reglist = 0) :
      OMR::RegisterDependencyConditionsConnector(node, cg, additionalRegDeps, reglist) {}

   };
}

#endif
