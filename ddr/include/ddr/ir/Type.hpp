/*******************************************************************************
 * Copyright (c) 2016, 2017 IBM Corp. and others
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

#ifndef TYPE_HPP
#define TYPE_HPP

#include "ddr/config.hpp"

#include <set>
#include "ddr/std/string.hpp"

#include "ddr/blobgen/genBlob.hpp"
#include "ddr/ir/Symbol_IR.hpp"
#include "ddr/ir/TypeVisitor.hpp"
#include "ddr/scanner/Scanner.hpp"

using std::set;
using std::string;
using std::vector;

class ClassUDT;
class ClassType;
class EnumUDT;
class Macro;
class NamespaceUDT;
class TypedefUDT;
class UDT;
class UnionUDT;
struct FieldOverride;

class Type
{
public:
	string _name;
	size_t _sizeOf; /* Size of type in bytes */
	bool _isDuplicate;

	Type(size_t size);
	virtual ~Type();

	virtual bool isAnonymousType();

	virtual string getFullName();
	virtual string getSymbolKindName();

	/* Visitor pattern function to allow the scanner/generator/IR to dispatch functionality based on type. */
	virtual DDR_RC acceptVisitor(TypeVisitor const &visitor);

	virtual void checkDuplicate(Symbol_IR *ir);
	virtual NamespaceUDT *getNamespace();
	virtual size_t getPointerCount();
	virtual size_t getArrayDimensions();
	virtual void computeFieldOffsets();
	virtual void addMacro(Macro *macro);
	virtual vector<UDT *> *getSubUDTS();
	virtual void renameFieldsAndMacros(FieldOverride fieldOverride, Type *replacementType);
	virtual Type *getBaseType();

	bool operator==(Type const & rhs) const;
	virtual bool compareToClass(ClassUDT const &) const;
	virtual bool compareToClasstype(ClassType const &) const;
	virtual bool compareToEnum(EnumUDT const &) const;
	virtual bool compareToNamespace(NamespaceUDT const &) const;
	virtual bool compareToType(Type const &) const;
	virtual bool compareToTypedef(TypedefUDT const &) const;
	virtual bool compareToUDT(UDT const &) const;
	virtual bool compareToUnion(UnionUDT const &) const;
};

#endif /* TYPE_HPP */
