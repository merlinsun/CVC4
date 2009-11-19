/*********************                                           -*- C++ -*-  */
/** attr_type.h
 ** This file is part of the CVC4 prototype.
 ** Copyright (c) 2009 The Analysis of Computer Systems Group (ACSys)
 ** Courant Institute of Mathematical Sciences
 ** New York University
 ** See the file COPYING in the top-level source directory for licensing
 ** information.
 **
 **/

#ifndef __CVC4__EXPR__ATTR_TYPE_H
#define __CVC4__EXPR__ATTR_TYPE_H

#include "expr_attribute.h"

namespace CVC4 {
namespace expr {

class Type;

// an "attribute type" for types
// this is essentially a traits structure
class Type_attr {
public:
  enum { hash_value = 11 }; // could use typeid but then different on different machines/compiles
  typedef Type value_type;//Expr?
  static const Type_attr marker;
};

extern AttrTable<Type_attr> type_table;

}/* CVC4::expr namespace */
}/* CVC4 namespace */

#endif /* __CVC4__EXPR__ATTR_TYPE_H */
