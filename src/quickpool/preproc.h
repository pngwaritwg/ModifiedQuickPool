#pragma once

#include "sharing.h"

namespace quickpool {

// Preprocessed data for a gate.
template <class R>
struct PreprocGate {
  // Secret shared mask for the output wire of the gate.
  AddShare<R> mask{};
  TPShare<R> tpmask{};

  PreprocGate() = default;
  PreprocGate(const AddShare<R>& mask, const TPShare<R>& tpmask)
      : mask(mask), tpmask(tpmask) {}

  virtual ~PreprocGate() = default;
};

template <class R>
using preprocg_ptr_t = std::unique_ptr<PreprocGate<R>>;

template <class R>
struct PreprocInput : public PreprocGate<R> {
  // ID of party providing input on wire.
  int pid{};
  // Plaintext value of mask on input wire. Non-zero for all parties except
  // party with id 'pid'.
  R mask_value{};

  PreprocInput() = default;
  PreprocInput(const AddShare<R>& mask, const TPShare<R>& tpmask, int pid, R mask_value = 0) 
      : PreprocGate<R>(mask, tpmask), pid(pid), mask_value(mask_value) {}
  PreprocInput(const PreprocInput<R>& pregate)
      : PreprocGate<R>(pregate.mask, pregate.tpmask), pid(pregate.pid), mask_value(pregate.mask_value) {}
};

template <class R>
struct PreprocMultGate : public PreprocGate<R> {
  // Secret shared product of inputs masks.
  AddShare<R> mask_prod{};
  TPShare<R> tpmask_prod{};

  PreprocMultGate() = default;
  PreprocMultGate(const AddShare<R>& mask, const TPShare<R>& tpmask, const AddShare<R>& mask_prod, const TPShare<R>& tpmask_prod)
    : PreprocGate<R>(mask, tpmask), mask_prod(mask_prod), tpmask_prod(tpmask_prod) {}
};

template <class R>
struct PreprocDotpGate : public PreprocGate<R> {
  AddShare<R> mask_prod{};
  TPShare<R> tpmask_prod{};

  PreprocDotpGate() = default;
  PreprocDotpGate(const AddShare<R>& mask, const TPShare<R>& tpmask, const AddShare<R>& mask_prod, const TPShare<R>& tpmask_prod)
      : PreprocGate<R>(mask, tpmask), mask_prod(mask_prod), tpmask_prod(tpmask_prod) {}
};

// Preprocessed data for the circuit.
template <class R>
struct PreprocCircuit {
  std::vector<preprocg_ptr_t<R>> gates;

  PreprocCircuit() = default;
  PreprocCircuit(size_t num_gates) : gates(num_gates) {}      
};

};  // namespace quickpool

