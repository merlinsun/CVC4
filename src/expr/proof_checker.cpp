/*********************                                                        */
/*! \file proof_checker.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Implementation of proof checker
 **/

#include "expr/proof_checker.h"

namespace CVC4 {

Node ProofChecker::check(ProofNode* pn, Node expected)
{
  return check(pn->getRule(), pn->getChildren(), pn->getArguments(), expected);
}

Node ProofChecker::check(
    PfRule id,
    const std::vector<std::shared_ptr<ProofNode>>& children,
    const std::vector<Node>& args,
    Node expected)
{
  /*
  // optimization?
  if (id==PfRule::ASSUME)
  {
    return expected;
  }
  */
  Trace("pfcheck") << "ProofChecker::check: " << id << std::endl;
  std::vector<Node> cchildren;
  for (const std::shared_ptr<ProofNode>& pc : children)
  {
    Assert(pc != nullptr);
    Node cres = pc->getResult();
    if (cres.isNull())
    {
      Trace("pfcheck") << "ProofChecker::check: failed child" << std::endl;
      Unreachable()
          << "ProofChecker::check: child proof was invalid (null conclusion)"
          << std::endl;
      // should not have been able to create such a proof node
      return Node::null();
    }
    cchildren.push_back(cres);
    if (Trace.isOn("pfcheck"))
    {
      std::stringstream ssc;
      pc->printDebug(ssc);
      Trace("pfcheck") << "     child: " << ssc.str() << " : " << cres
                       << std::endl;
    }
  }
  Trace("pfcheck") << "      args: " << args << std::endl;
  Trace("pfcheck") << "  expected: " << expected << std::endl;
  std::stringstream out;
  // we use trusted (null) checkers here, since we want the proof generation to
  // proceed without failing here.
  Node res = checkInternal(id, cchildren, args, expected, out, true);
  if (res.isNull())
  {
    Trace("pfcheck") << "ProofChecker::check: failed" << std::endl;
    Unreachable() << "ProofChecker::check: failed, " << out.str() << std::endl;
    // it did not match the given expectation, fail
    return Node::null();
  }
  Trace("pfcheck") << "ProofChecker::check: success!" << std::endl;
  return res;
}

Node ProofChecker::checkDebug(PfRule id,
                              const std::vector<Node>& cchildren,
                              const std::vector<Node>& args,
                              Node expected,
                              const char* traceTag)
{
  std::stringstream out;
  // since we are debugging, we want to treat trusted (null) checkers as
  // a failure.
  Node res = checkInternal(id, cchildren, args, expected, out, false);
  Trace(traceTag) << "ProofChecker::checkDebug: " << id;
  if (res.isNull())
  {
    Trace(traceTag) << " failed, " << out.str() << std::endl;
  }
  else
  {
    Trace(traceTag) << " success" << std::endl;
  }
  return res;
}

Node ProofChecker::checkInternal(PfRule id,
                                 const std::vector<Node>& cchildren,
                                 const std::vector<Node>& args,
                                 Node expected,
                                 std::stringstream& out,
                                 bool useTrustedChecker)
{
  std::map<PfRule, ProofRuleChecker*>::iterator it = d_checker.find(id);
  if (it == d_checker.end())
  {
    // no checker for the rule
    out << "no checker for rule " << id << std::endl;
    return Node::null();
  }
  else if (it->second == nullptr)
  {
    if (useTrustedChecker)
    {
      Notice() << "ProofChecker::check: trusting PfRule " << id << std::endl;
      // trusted checker
      return expected;
    }
    else
    {
      out << "trusted checker for rule " << id << std::endl;
      return Node::null();
    }
  }
  // check it with the corresponding checker
  Node res = it->second->check(id, cchildren, args);
  if (!expected.isNull() && res != expected)
  {
    out << "result does not match expected value." << std::endl
        << "    result: " << res << std::endl
        << "  expected: " << expected << std::endl;
    // it did not match the given expectation, fail
    return Node::null();
  }
  return res;
}

void ProofChecker::registerChecker(PfRule id, ProofRuleChecker* psc)
{
  std::map<PfRule, ProofRuleChecker*>::iterator it = d_checker.find(id);
  if (it != d_checker.end())
  {
    // checker is already provided
    Notice() << "ProofChecker::registerChecker: checker already exists for "
             << id << std::endl;
    return;
  }
  d_checker[id] = psc;
}

}  // namespace CVC4
