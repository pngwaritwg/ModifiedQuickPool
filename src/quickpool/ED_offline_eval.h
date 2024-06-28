#pragma once

#include "netmp.h"
#include "helpers.h"
#include "preproc.h"
#include "circuit.h"
#include "rand_gen_pool.h"

using namespace common::utils;

namespace quickpool {

class OfflineEvaluator {  
  int id_;
  int rider_count;
  int driver_count;
  int security_param_;
  RandGenPool rgen_;
  std::shared_ptr<io::NetIOMP> network_;
  LevelOrderedCircuit circ_;
  std::shared_ptr<ThreadPool> tpool_;
  PreprocCircuit<Field> preproc_;

 public:
  
  OfflineEvaluator(int my_id, int rider_count, int driver_count, std::shared_ptr<io::NetIOMP> network,
                   LevelOrderedCircuit circ, int security_param,
                   int threads, int seed = 200); 

  OfflineEvaluator(int my_id, int rider_count, int driver_count, std::shared_ptr<io::NetIOMP> network,
                   LevelOrderedCircuit circ, int security_param,
                   std::shared_ptr<ThreadPool> tpool, int seed = 200); 

  bool amIRider();

  bool amIDriver();

  bool isDealerRider(int dealer);

  bool isDealerDriver(int dealer);

  // Generate sharing of a random unknown value.
  void randomShare(int rider_id, int driver_id,
                  RandGenPool& rgen, io::NetIOMP& network,
                  AddShare<Field>& share, TPShare<Field>& tpShare);

  // Generate sharing of a random value known to dealer (called by all parties
  // except the dealer).
  // Generate sharing of a random value known to party. Should be called by
  // dealer when other parties call other variant.
  void randomShareSecret(int rider_id, int driver_id,
                        RandGenPool& rgen, io::NetIOMP& network,
                        AddShare<Field>& share, TPShare<Field>& tpShare,
                        Field secret, std::vector<std::vector<Field>>& rand_sh_sec, size_t& idx_rand_sh_sec);


  void randomShareWithParty(int dealer, int rider_id, int driver_id,
                                  RandGenPool& rgen, io::NetIOMP& network, AddShare<Field>& share,
                                  TPShare<Field>& tpShare, Field& secret, std::vector<std::vector<Field>>& rand_sh_party,          
                                  size_t& idx_rand_sh_party);
                                          
                                           

  // Following methods implement various preprocessing subprotocols.

  // Set masks for each wire. Should be called before running any of the other
  // subprotocols.
  void setWireMasksParty(const std::unordered_map<wire_t, int>& input_pid_map, std::vector<std::vector<Field>>& rand_sh_sec, std::vector<std::vector<Field>>& rand_sh_party);

  void setWireMasks(const std::unordered_map<wire_t, int>& input_pid_map);
  
  // void getOutputMasks(int pid, std::vector<Field>& output_mask);

  PreprocCircuit<Field> getPreproc();

  // Efficiently runs above subprotocols.
  PreprocCircuit<Field> run(
      const std::unordered_map<wire_t, int>& input_pid_map);

  
};

};  // namespace quickpool

