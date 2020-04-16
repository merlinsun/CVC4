/*********************                                                        */
/*! \file inference_manager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Customized inference manager for the theory of strings
 **/

#include "cvc4_private.h"

#ifndef CVC4__THEORY__STRINGS__INFERENCE_MANAGER_H
#define CVC4__THEORY__STRINGS__INFERENCE_MANAGER_H

#include <map>
#include <vector>

#include "context/cdhashset.h"
#include "context/context.h"
#include "expr/node.h"
#include "theory/ext_theory.h"
#include "theory/proof_output_channel.h"
#include "theory/strings/infer_info.h"
#include "theory/strings/sequences_stats.h"
#include "theory/strings/solver_state.h"
#include "theory/strings/term_registry.h"
#include "theory/uf/equality_engine.h"

namespace CVC4 {
namespace theory {
namespace strings {

/** Inference Manager
 *
 * The purpose of this class is to process inference steps for strategies
 * in the theory of strings.
 *
 * In particular, inferences are given to this class via calls to functions:
 *
 * sendInternalInference, sendInference, sendSplit
 *
 * This class decides how the conclusion of these calls will be processed.
 * It primarily has to decide whether the conclusions will be processed:
 *
 * (1) Internally in the strings solver, via calls to equality engine's
 * assertLiteral and assertPredicate commands. We refer to these literals as
 * "facts",
 * (2) Externally on the output channel of theory of strings as "lemmas",
 * (3) External on the output channel as "conflicts" (when a conclusion of an
 * inference is false).
 *
 * It buffers facts and lemmas in vectors d_pending and d_pending_lem
 * respectively.
 *
 * When applicable, facts can be flushed to the equality engine via a call to
 * doPendingFacts, and lemmas can be flushed to the output channel via a call
 * to doPendingLemmas.
 *
 * It also manages other kinds of interaction with the output channel of the
 * theory of strings, e.g. sendPhaseRequirement, setIncomplete, and
 * with the extended theory object e.g. markCongruent.
 */
class InferenceManager
{
  typedef context::CDHashSet<Node, NodeHashFunction> NodeSet;
  typedef context::CDHashMap<Node, Node, NodeHashFunction> NodeNodeMap;

 public:
  InferenceManager(context::Context* c,
                   context::UserContext* u,
                   SolverState& s,
                   TermRegistry& tr,
                   ExtTheory& e,
                   ProofOutputChannel& poc,
                   SequencesStatistics& statistics,
                   bool pfEnabled = false);
  ~InferenceManager() {}

  /** send assumption
   *
   * This is called when a fact is asserted to TheoryStrings. It adds lit
   * to the equality engine maintained by this class immediately.
   */
  void sendAssumption(TNode lit);

  /** send internal inferences
   *
   * This is called when we have inferred exp => conc, where exp is a set
   * of equalities and disequalities that hold in the current equality engine.
   * This method adds equalities and disequalities ~( s = t ) via
   * sendInference such that both s and t are either constants or terms
   * that already occur in the equality engine, and ~( s = t ) is a consequence
   * of conc. This function can be seen as a "conservative" version of
   * sendInference below in that it does not introduce any new non-constant
   * terms to the state.
   *
   * The argument infer identifies the reason for the inference.
   * This is used for debugging and statistics purposes.
   *
   * Return true if the inference is complete, in the sense that we infer
   * inferences that are equivalent to conc. This returns false e.g. if conc
   * (or one of its conjuncts if it is a conjunction) was not inferred due
   * to the criteria mentioned above.
   */
  bool sendInternalInference(std::vector<Node>& exp,
                             Node conc,
                             Inference infer);
  /** send inference
   *
   * This function should be called when ( exp ^ exp_n ) => eq. The set exp
   * contains literals that are explainable, i.e. those that hold in the
   * equality engine of the theory of strings. On the other hand, the set
   * exp_n ("explanations new") contain nodes that are not explainable by the
   * theory of strings. This method may call sendInfer or sendLemma. Overall,
   * the result of this method is one of the following:
   *
   * [1] (No-op) Do nothing if eq is true,
   *
   * [2] (Infer) Indicate that eq should be added to the equality engine of this
   * class with explanation EXPLAIN(exp), where EXPLAIN returns the
   * explanation of the node in exp in terms of the literals asserted to the
   * theory of strings,
   *
   * [3] (Lemma) Indicate that the lemma ( EXPLAIN(exp) ^ exp_n ) => eq should
   * be sent on the output channel of the theory of strings, or
   *
   * [4] (Conflict) Immediately report a conflict EXPLAIN(exp) on the output
   * channel of the theory of strings.
   *
   * Determining which case to apply depends on the form of eq and whether
   * exp_n is empty. In particular, lemmas must be used whenever exp_n is
   * non-empty, conflicts are used when exp_n is empty and eq is false.
   *
   * The argument infer identifies the reason for inference, used for
   * debugging. This updates the statistics about the number of inferences made
   * of each type.
   *
   * If the flag asLemma is true, then this method will send a lemma instead
   * of an inference whenever applicable.
   */
  void sendInference(const std::vector<Node>& exp,
                     const std::vector<Node>& exp_n,
                     Node eq,
                     Inference infer,
                     bool asLemma = false);
  /** same as above, but where exp_n is empty */
  void sendInference(const std::vector<Node>& exp,
                     Node eq,
                     Inference infer,
                     bool asLemma = false);

  /** Send inference
   *
   * Makes the appropriate call to send inference based on the infer info
   * data structure (see sendInference documentation above).
   */
  void sendInference(const InferInfo& i);
  /** Send split
   *
   * This requests that ( a = b V a != b ) is sent on the output channel as a
   * lemma. We additionally request a phase requirement for the equality a=b
   * with polarity preq.
   *
   * The argument infer identifies the reason for inference, used for
   * debugging. This updates the statistics about the number of
   * inferences made of each type.
   *
   * This method returns true if the split was non-trivial, and false
   * otherwise. A split is trivial if a=b rewrites to a constant.
   */
  bool sendSplit(Node a, Node b, Inference infer, bool preq = true);

  /** Send phase requirement
   *
   * This method is called to indicate this class should send a phase
   * requirement request to the output channel for literal lit to be
   * decided with polarity pol.
   */
  void sendPhaseRequirement(Node lit, bool pol);
  /**
   * Set that we are incomplete for the current set of assertions (in other
   * words, we must answer "unknown" instead of "sat"); this calls the output
   * channel's setIncomplete method.
   */
  void setIncomplete();

  //----------------------------constructing antecedants
  /**
   * Adds equality a = b to the vector exp if a and b are distinct terms. It
   * must be the case that areEqual( a, b ) holds in this context.
   */
  void addToExplanation(Node a, Node b, std::vector<Node>& exp) const;
  /** Adds lit to the vector exp if it is non-null */
  void addToExplanation(Node lit, std::vector<Node>& exp) const;
  //----------------------------end constructing antecedants
  /** Do pending facts
   *
   * This method asserts pending facts (d_pending) with explanations
   * (d_pendingExp) to the equality engine of the theory of strings via calls
   * to assertPendingFact.
   *
   * It terminates early if a conflict is encountered, for instance, by
   * equality reasoning within the equality engine.
   *
   * Regardless of whether a conflict is encountered, the vector d_pending
   * and map d_pendingExp are cleared.
   */
  void doPendingFacts();
  /** Do pending lemmas
   *
   * This method flushes all pending lemmas (d_pending_lem) to the output
   * channel of theory of strings.
   *
   * Like doPendingFacts, this function will terminate early if a conflict
   * has already been encountered by the theory of strings. The vector
   * d_pending_lem is cleared regardless of whether a conflict is discovered.
   *
   * Notice that as a result of the above design, some lemmas may be "dropped"
   * if a conflict is discovered in between when a lemma is added to the
   * pending vector of this class (via a sendInference call). Lemmas
   * e.g. those that are required for initialization should not be sent via
   * this class, since they should never be dropped.
   */
  void doPendingLemmas();
  /**
   * Have we processed an inference during this call to check? In particular,
   * this returns true if we have a pending fact or lemma, or have encountered
   * a conflict.
   */
  bool hasProcessed() const;
  /** Do we have a pending fact to add to the equality engine? */
  bool hasPendingFact() const { return !d_pending.empty(); }
  /** Do we have a pending lemma to send on the output channel? */
  bool hasPendingLemma() const { return !d_pendingLem.empty(); }

  /** make explanation
   *
   * This returns a node corresponding to the explanation of formulas in a,
   * interpreted conjunctively. The returned node is a conjunction of literals
   * that have been asserted to the equality engine.
   */
  Node mkExplain(const std::vector<Node>& a) const;
  /** Same as above, but the new literals an are append to the result */
  Node mkExplain(const std::vector<Node>& a, const std::vector<Node>& an) const;
  /**
   * Explain literal l, add conjuncts to assumptions vector instead of making
   * the node corresponding to their conjunction.
   */
  void explain(TNode literal, std::vector<TNode>& assumptions) const;
  // ------------------------------------------------- extended theory
  /**
   * Mark that terms a and b are congruent in the current context.
   * This makes a call to markCongruent in the extended theory object of
   * the parent theory if the kind of a (and b) is owned by the extended
   * theory.
   */
  void markCongruent(Node a, Node b);
  /**
   * Mark that extended function is reduced. If contextDepend is true,
   * then this mark is SAT-context dependent, otherwise it is user-context
   * dependent (see ExtTheory::markReduced).
   */
  void markReduced(Node n, bool contextDepend = true);
  // ------------------------------------------------- end extended theory

 private:
  /** assert pending fact
   *
   * This asserts atom with polarity to the equality engine of this class,
   * where exp is the explanation of why (~) atom holds.
   *
   * This call may trigger further initialization steps involving the terms
   * of atom, including calls to registerTerm.
   */
  void assertPendingFact(Node atom, bool polarity, Node exp);
  /**
   * Indicates that ant => conc should be sent on the output channel of this
   * class. This will either trigger an immediate call to the conflict
   * method of the output channel of this class of conc is false, or adds the
   * above lemma to the lemma cache d_pending_lem, which may be flushed
   * later within the current call to TheoryStrings::check.
   *
   * The argument infer identifies the reason for inference, used for
   * debugging.
   */
  void sendLemma(Node ant, Node conc, Inference infer);
  /**
   * Indicates that conc should be added to the equality engine of this class
   * with explanation eq_exp. It must be the case that eq_exp is a (conjunction
   * of) literals that each are explainable, i.e. they already hold in the
   * equality engine of this class.
   */
  void sendInfer(Node eq_exp, Node eq, Inference infer);
  /** Reference to the solver state of the theory of strings. */
  SolverState& d_state;
  /** Reference to the term registry of theory of strings */
  TermRegistry& d_termReg;
  /** the extended theory object for the theory of strings */
  ExtTheory& d_extt;
  /** Reference to the output channel of the theory of strings. */
  ProofOutputChannel& d_poc;
  /** Reference to the statistics for the theory of strings/sequences. */
  SequencesStatistics& d_statistics;
  /** The proof equality engine */

  /** Common constants */
  Node d_true;
  Node d_false;
  Node d_zero;
  Node d_one;
  /** The list of pending literals to assert to the equality engine */
  std::vector<Node> d_pending;
  /** A map from the literals in the above vector to their explanation */
  std::map<Node, Node> d_pendingExp;
  /** A map from literals to their pending phase requirement */
  std::map<Node, bool> d_pendingReqPhase;
  /** A list of pending lemmas to be sent on the output channel. */
  std::vector<Node> d_pendingLem;
  /**
   * The keep set of this class. This set is maintained to ensure that
   * facts and their explanations are ref-counted. Since facts and their
   * explanations are SAT-context-dependent, this set is also
   * SAT-context-dependent.
   */
  NodeSet d_keep;
  /** Whether proofs are enabled */
  bool d_pfEnabled;
};

}  // namespace strings
}  // namespace theory
}  // namespace CVC4

#endif
