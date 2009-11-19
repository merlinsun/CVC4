/*********************                                           -*- C++ -*-  */
/** unique_id.h
 ** This file is part of the CVC4 prototype.
 ** Copyright (c) 2009 The Analysis of Computer Systems Group (ACSys)
 ** Courant Institute of Mathematical Sciences
 ** New York University
 ** See the file COPYING in the top-level source directory for licensing
 ** information.
 **
 **/

#ifndef __CVC4__UNIQUE_ID_H
#define __CVC4__UNIQUE_ID_H

namespace CVC4 {

// NOTE that UniqueID is intended for startup registration; it
// shouldn't be used in multi-threaded contexts.

template <class T>
class UniqueID {
  static unsigned s_topID;
  const unsigned d_thisID;

public:
  UniqueID() : d_thisID( s_topID++ ) { }
  operator unsigned() const { return d_thisID; }
};

template <class T>
unsigned UniqueID<T>::s_topID = 0;

}/* CVC4 namespace */

#endif /* __CVC4__UNIQUE_ID_H */
