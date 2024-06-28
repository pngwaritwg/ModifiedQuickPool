#pragma once

#include "netmp.h"
#include "helpers.h"
#include "preproc.h"
#include "circuit.h"
#include "rand_gen_pool.h"

using namespace common::utils;

namespace quickpool
{

class OnlineEvaluator
{
  int id_;
  int rider_count;
  int driver_count;
  int security_param_;
  RandGenPool rgen_;
  std::shared_ptr<io::NetIOMP> network_;
  PreprocCircuit<Field> preproc_;
  LevelOrderedCircuit circ_;
  std::vector<Field> wires_;
  std::shared_ptr<ThreadPool> tpool_;

  // write reconstruction function
public:
  OnlineEvaluator(int id, int rider_count, int driver_count, 
                  std::shared_ptr<io::NetIOMP> network,
                  PreprocCircuit<Field> preproc, LevelOrderedCircuit circ,
                  int security_param, int threads, int seed = 200);

  OnlineEvaluator(int id, int rider_count, int driver_count, 
                  std::shared_ptr<io::NetIOMP> network,
                  PreprocCircuit<Field> preproc, LevelOrderedCircuit circ,
                  int security_param, std::shared_ptr<ThreadPool> tpool, int seed = 200);

  bool amIRider();

  bool amIDriver();

                  
  std::vector<Field> getwires();

  void setInputs(const std::unordered_map<wire_t, Field> &inputs);

  void setRandomInputs();

  void evaluateGatesAtDepthPartySend(size_t depth,
                                      std::vector<std::vector<Field>> &mult_nonTP, std::vector<std::vector<Field>> &dotprod_nonTP);

  void evaluateGatesAtDepthPartyRecv(size_t depth,
                                      std::vector<std::vector<Field>> mult_all,
                                      std::vector<std::vector<Field>> dotprod_all);

  void evaluateGatesAtDepth(size_t depth);

  std::vector<Field> getOutputs();

  // Evaluate online phase for circuit
  std::vector<Field> evaluateCircuit(const std::unordered_map<wire_t, Field> &inputs);
};

}; // namespace quickpool
