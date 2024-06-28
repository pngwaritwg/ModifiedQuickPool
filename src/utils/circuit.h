#pragma once

#include <array>
#include <boost/format.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace common::utils {

using wire_t = size_t;

enum GateType {
  kInp,
  kAdd,
  kMul,
  kSub,
  kConstAdd,
  kConstMul,
  kDotprod,
  kInvalid,
  NumGates
};

std::ostream& operator<<(std::ostream& os, GateType type);

// Gates represent primitive operations.
// All gates have one output.
struct Gate {
  GateType type{GateType::kInvalid};
  wire_t out;
  int rider_id = -1;
  int driver_id = -1;

  Gate() = default;
  Gate(GateType type, wire_t out);
  Gate(GateType type, wire_t out, int rider_id, int driver_id);

  virtual ~Gate() = default;
};

// Represents a gate with fan-in 2.
struct FIn2Gate : public Gate {
  wire_t in1{0};
  wire_t in2{0};

  FIn2Gate() = default;
  FIn2Gate(GateType type, wire_t in1, wire_t in2, wire_t out);
  FIn2Gate(GateType type, wire_t in1, wire_t in2, wire_t out, int rider_id, int driver_id);
};

// Represents a gate used to denote SIMD operations.
// These type is used to represent operations that take vectors of inputs but
// might not necessarily be SIMD e.g., dot product.
struct SIMDGate : public Gate {
  std::vector<wire_t> in1{0};
  std::vector<wire_t> in2{0};

  SIMDGate() = default;
  SIMDGate(GateType type, std::vector<wire_t> in1, std::vector<wire_t> in2, wire_t out);
  SIMDGate(GateType type, std::vector<wire_t> in1, std::vector<wire_t> in2, wire_t out, int rider_id, int driver_id);
};

// Represents gates where one input is a constant.
template <class R>
struct ConstOpGate : public Gate {
  wire_t in{0};
  R cval;

  ConstOpGate() = default;
  ConstOpGate(GateType type, wire_t in, R cval, wire_t out) : Gate(type, out), in(in), cval(std::move(cval)) {}
  ConstOpGate(GateType type, wire_t in, R cval, wire_t out, int rider_id, int driver_id) : Gate(type, out, rider_id, driver_id), in(in), cval(std::move(cval)) {}
};

using gate_ptr_t = std::shared_ptr<Gate>;

// Gates ordered by multiplicative depth.
//
// Addition gates are not considered to increase the depth.
// Moreover, if gates_by_level[l][i]'s output is input to gates_by_level[l][j]
// then i < j.
struct LevelOrderedCircuit {
  size_t num_gates;
  std::array<uint64_t, GateType::NumGates> count;
  std::vector<wire_t> outputs;
  std::unordered_map<wire_t, std::vector<int>> output_owners;
  std::vector<std::vector<gate_ptr_t>> gates_by_level;

  friend std::ostream& operator<<(std::ostream& os,
                                  const LevelOrderedCircuit& circ);
};

// Represents an arithmetic circuit.
template <class R>
class Circuit {
  std::vector<wire_t> outputs_;
  std::unordered_map<wire_t, std::vector<int>> output_owners_;
  std::vector<gate_ptr_t> gates_;

  bool isWireValid(wire_t wid) { return wid < gates_.size(); }

 public:
  Circuit() = default;

  // Methods to manually build a circuit.

  wire_t newInputWire() {
    wire_t wid = gates_.size();
    gates_.push_back(std::make_shared<Gate>(GateType::kInp, wid));
    return wid;
  }

  wire_t newInputWire(int rider_id, int driver_id) {
    wire_t wid = gates_.size();
    gates_.push_back(std::make_shared<Gate>(GateType::kInp, wid, rider_id, driver_id));
    return wid;
  }

  void setAsOutput(wire_t wid) {
    if (!isWireValid(wid)) {
      throw std::invalid_argument("Invalid wire ID.");
    }
    outputs_.push_back(wid);
  }

  void setAsOutput(wire_t wid, int rider_id, int driver_id) {
    if (!isWireValid(wid)) {
      throw std::invalid_argument("Invalid wire ID.");
    }
    outputs_.push_back(wid);
    std::vector<int> owners{rider_id, driver_id};
    output_owners_[wid] = owners;
  }

  // Function to add a gate with fan-in 2.
  wire_t addGate(GateType type, wire_t input1, wire_t input2) {
    if (type != GateType::kAdd && type != GateType::kMul &&
        type != GateType::kSub) {
      throw std::invalid_argument("Invalid gate type.");
    }

    if (!isWireValid(input1) || !isWireValid(input2)) {
      throw std::invalid_argument("Invalid wire ID.");
    }

    wire_t output = gates_.size();
    gates_.push_back(std::make_shared<FIn2Gate>(type, input1, input2, output));

    return output;
  }

  wire_t addGate(GateType type, wire_t input1, wire_t input2, int rider_id, int driver_id) {
    if (type != GateType::kAdd && type != GateType::kMul &&
        type != GateType::kSub) {
      throw std::invalid_argument("Invalid gate type.");
    }

    if (!isWireValid(input1) || !isWireValid(input2)) {
      throw std::invalid_argument("Invalid wire ID.");
    }

    wire_t output = gates_.size();
    gates_.push_back(std::make_shared<FIn2Gate>(type, input1, input2, output, rider_id, driver_id));

    return output;
  }    

  // Function to add a gate with one input from a wire and a second constant input.
  wire_t addConstOpGate(GateType type, wire_t wid, R cval) {
    if (type != kConstAdd && type != kConstMul) {
      throw std::invalid_argument("Invalid gate type.");
    }

    if (!isWireValid(wid)) {
      throw std::invalid_argument("Invalid wire ID.");
    }

    wire_t output = gates_.size();
    gates_.push_back(std::make_shared<ConstOpGate<R>>(type, wid, cval, output));

    return output;
  }

  wire_t addConstOpGate(GateType type, wire_t wid, R cval, int rider_id, int driver_id) {
    if (type != kConstAdd && type != kConstMul) {
      throw std::invalid_argument("Invalid gate type.");
    }

    if (!isWireValid(wid)) {
      throw std::invalid_argument("Invalid wire ID.");
    }

    wire_t output = gates_.size();
    gates_.push_back(std::make_shared<ConstOpGate<R>>(type, wid, cval, output, rider_id, driver_id));

    return output;
  }

  // Function to add a multiple fan-in gate.
  wire_t addGate(GateType type, const std::vector<wire_t>& input1, const std::vector<wire_t>& input2) {
    if (type != GateType::kDotprod) {
      throw std::invalid_argument("Invalid gate type.");
    }

    if (input1.size() != input2.size()) {
      throw std::invalid_argument("Expected same length inputs.");
    }

    for (size_t i = 0; i < input1.size(); ++i) {
      if (!isWireValid(input1[i]) || !isWireValid(input2[i])) {
        throw std::invalid_argument("Invalid wire ID.");
      }
    }

    wire_t output = gates_.size();
    gates_.push_back(std::make_shared<SIMDGate>(type, input1, input2, output));
    return output;
  }

  wire_t addGate(GateType type, const std::vector<wire_t>& input1, const std::vector<wire_t>& input2, int rider_id, int driver_id) {
    if (type != GateType::kDotprod) {
      throw std::invalid_argument("Invalid gate type.");
    }

    if (input1.size() != input2.size()) {
      throw std::invalid_argument("Expected same length inputs.");
    }

    for (size_t i = 0; i < input1.size(); ++i) {
      if (!isWireValid(input1[i]) || !isWireValid(input2[i])) {
        throw std::invalid_argument("Invalid wire ID.");
      }
    }

    wire_t output = gates_.size();
    gates_.push_back(std::make_shared<SIMDGate>(type, input1, input2, output, rider_id, driver_id));
    return output;
  }

  // Level ordered gates are helpful for evaluation.
  [[nodiscard]] LevelOrderedCircuit orderGatesByLevel() const {
    LevelOrderedCircuit res;
    res.outputs = outputs_;
    res.output_owners = output_owners_;
    res.num_gates = gates_.size();

    // Map from output wire id to multiplicative depth/level.
    // Input gates have a depth of 0.
    std::vector<size_t> gate_level(res.num_gates, 0);
    size_t depth = 0;

    // This assumes that if gates_[i]'s output is input to gates_[j] then
    // i < j.
    for (const auto& gate : gates_) {
      switch (gate->type) {
        case GateType::kAdd:
        case GateType::kSub: {
          const auto* g = static_cast<FIn2Gate*>(gate.get());
          gate_level[g->out] = std::max(gate_level[g->in1], gate_level[g->in2]);
          break;
        }

        case GateType::kMul: {
          const auto* g = static_cast<FIn2Gate*>(gate.get());
          gate_level[g->out] =
              std::max(gate_level[g->in1], gate_level[g->in2]) + 1;
          break;
        }

        case GateType::kConstAdd:
        case GateType::kConstMul: {
          const auto* g = static_cast<ConstOpGate<R>*>(gate.get());
          gate_level[g->out] = gate_level[g->in];
          break;
        }

        case GateType::kDotprod: {
          const auto* g = static_cast<SIMDGate*>(gate.get());
          size_t gate_depth = 0;
          for (size_t i = 0; i < g->in1.size(); ++i) {
            gate_depth = std::max(
                {gate_level[g->in1[i]], gate_level[g->in2[i]], gate_depth});
          }
          gate_level[g->out] = gate_depth + 1;
          break;
        }

        default:
          break;
      }

      depth = std::max(depth, gate_level[gate->out]);
    }

    std::fill(res.count.begin(), res.count.end(), 0);

    std::vector<std::vector<gate_ptr_t>> gates_by_level(depth + 1);
    for (const auto& gate : gates_) {
      res.count[gate->type]++;
      gates_by_level[gate_level[gate->out]].push_back(gate);
    }

    res.gates_by_level = std::move(gates_by_level);

    return res;
  }

  // Evaluate circuit on plaintext inputs.
  [[nodiscard]] std::vector<R> evaluate(
      const std::unordered_map<wire_t, R>& inputs) const {
    auto level_circ = orderGatesByLevel();
    std::vector<R> wires(level_circ.num_gates);

    auto num_inp_gates = level_circ.count[GateType::kInp];
    if (inputs.size() != num_inp_gates) {
      throw std::invalid_argument(boost::str(
          boost::format("Expected %1% inputs but received %2% inputs.") %
          num_inp_gates % inputs.size()));
    }

    for (const auto& level : level_circ.gates_by_level) {
      for (const auto& gate : level) {
        switch (gate->type) {
          case GateType::kInp: {
            wires[gate->out] = inputs.at(gate->out);
            break;
          }

          case GateType::kMul: {
            auto* g = static_cast<FIn2Gate*>(gate.get());
            wires[g->out] = wires[g->in1] * wires[g->in2];
            break;
          }

          case GateType::kAdd: {
            auto* g = static_cast<FIn2Gate*>(gate.get());
            wires[g->out] = wires[g->in1] + wires[g->in2];
            break;
          }

          case GateType::kSub: {
            auto* g = static_cast<FIn2Gate*>(gate.get());
            wires[g->out] = wires[g->in1] - wires[g->in2];
            break;
          }

          case GateType::kConstAdd: {
            auto* g = static_cast<ConstOpGate<R>*>(gate.get());
            wires[g->out] = wires[g->in] + g->cval;
            break;
          }

          case GateType::kConstMul: {
            auto* g = static_cast<ConstOpGate<R>*>(gate.get());
            wires[g->out] = wires[g->in] * g->cval;
            break;
          }

          case GateType::kDotprod: {
            auto* g = static_cast<SIMDGate*>(gate.get());
            for (size_t i = 0; i < g->in1.size(); i++) {
              wires[g->out] += wires[g->in1.at(i)] * wires[g->in2.at(i)];
            }
            break;
          }

          default: {
            throw std::runtime_error("Invalid gate type.");
          }
        }
      }
    }

    std::vector<R> outputs;
    for (auto i : level_circ.outputs) {
      outputs.push_back(wires[i]);
    }

    return outputs;
  }
     
  static Circuit generateEDSCircuit(int rider_count, int driver_count) {
    Circuit circ;    
    
    std::vector<wire_t> rider_start_loc(2);
    std::vector<wire_t> rider_end_loc(2);    
    std::vector<wire_t> driver_start_loc(2);
    std::vector<wire_t> driver_end_loc(2);

    std::vector<wire_t> start_diff(2);
    std::vector<wire_t> end_diff(2);

    for (int rider=0; rider<rider_count; rider++) {
      int rider_id = rider+1;
      for (int driver=0; driver<driver_count; driver++) {
        int driver_id = driver+rider_count+1;
        for(int i = 0; i < 2; ++i) {
          rider_start_loc[i] = circ.newInputWire(rider_id, driver_id);
          rider_end_loc[i] = circ.newInputWire(rider_id, driver_id);
          driver_start_loc[i] = circ.newInputWire(rider_id, driver_id);
          driver_end_loc[i] = circ.newInputWire(rider_id, driver_id);
        }
        for (int i=0; i<2; i++) {
          start_diff[i] = circ.addGate(GateType::kSub, rider_start_loc[i], driver_start_loc[i], rider_id, driver_id);
          end_diff[i] = circ.addGate(GateType::kSub, rider_end_loc[i], driver_end_loc[i], rider_id, driver_id);
        }
        auto ED_sq_start = circ.addGate(GateType::kDotprod, start_diff, start_diff, rider_id, driver_id);
        auto ED_sq_end = circ.addGate(GateType::kDotprod, end_diff, end_diff, rider_id, driver_id);        
        circ.setAsOutput(ED_sq_start, rider_id, driver_id);
        circ.setAsOutput(ED_sq_end, rider_id, driver_id);
      }
    }

    return circ;
  }
};

};  // namespace common::utils
