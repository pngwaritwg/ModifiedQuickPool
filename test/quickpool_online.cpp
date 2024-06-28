#define BOOST_TEST_MODULE Quickpool_online

#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/included/unit_test.hpp>

#include "ED_offline_eval.h"
#include "ED_online_eval.h"

using namespace quickpool;
using namespace common::utils;
namespace bdata = boost::unit_test::data;

constexpr int TEST_DATA_MAX_VAL = 1000;
constexpr int SECURITY_PARAM = 128;

struct GlobalFixture {
  GlobalFixture() {
    NTL::ZZ_p::init(NTL::conv<NTL::ZZ>("17816577890427308801"));
  }
};

BOOST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_AUTO_TEST_SUITE(Quickpool_online)

BOOST_AUTO_TEST_CASE(mult) {
  NTL::ZZ_pContext ZZ_p_ctx;
  ZZ_p_ctx.save();
  int rider_count = 1;
  int driver_count = 1;
  int nP = rider_count + driver_count;
  int rider_index = 1;
  int driver_index = 2; 
  auto seed_block = emp::makeBlock(0, 200);
  emp::PRG prg(&seed_block);
  std::mt19937 gen(200);
  std::uniform_int_distribution<uint> distrib(0, TEST_DATA_MAX_VAL);
  Circuit<Field> circ;
  std::vector<wire_t> input_wires;
  std::unordered_map<wire_t, int> input_pid_map;
  std::unordered_map<wire_t, Field> inputs;

  for (size_t i = 0; i < 2; ++i) {
    auto winp = circ.newInputWire(rider_index, driver_index);
    input_wires.push_back(winp);
    input_pid_map[winp] = 1;
    circ.setAsOutput(winp, rider_index, driver_index);
    inputs[winp] = Field(distrib(gen));
  }
  auto w_amb =
     circ.addGate(GateType::kMul, input_wires[0], input_wires[1], rider_index, driver_index);

  circ.setAsOutput(w_amb, rider_index, driver_index);
  auto level_circ = circ.orderGatesByLevel();
  auto exp_output = circ.evaluate(inputs);
  std::vector<std::future<std::vector<Field>>> parties;
  parties.reserve(nP+1);
  for (int i = 0; i <= nP; ++i) {
      parties.push_back(std::async(std::launch::async, [&, i, input_pid_map, inputs]() {
      ZZ_p_ctx.restore();
      // auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true);
      
      OfflineEvaluator eval(i, rider_count, driver_count, network, level_circ, SECURITY_PARAM, nP);
      auto preproc = eval.run(input_pid_map);
      
      OnlineEvaluator online_eval(i, rider_count, driver_count, std::move(network), std::move(preproc), level_circ, SECURITY_PARAM, nP);
      
      auto res = online_eval.evaluateCircuit(inputs);
      return res;
    }));
  }
  auto output = parties[0].get();
  BOOST_TEST(exp_output == output);
}


BOOST_AUTO_TEST_CASE(EDS) {
  NTL::ZZ_pContext ZZ_p_ctx;
  ZZ_p_ctx.save();
  int rider_count = 1;
  int driver_count = 1;
  int nP = rider_count + driver_count;
  int rider_index = 1;
  int driver_index = 2;
  auto seed_block = emp::makeBlock(0, 200);
  emp::PRG prg(&seed_block);
  std::mt19937 gen(200);
  std::uniform_int_distribution<uint> distrib(0, TEST_DATA_MAX_VAL);
  //Circuit<Field> circ;
  std::vector<wire_t> input_wires;
  std::unordered_map<wire_t, int> input_pid_map;
  std::unordered_map<wire_t, Field> inputs;
  for (wire_t i = 0; i < 8; ++i) {
    if(i < 4) {
      input_pid_map[i] = rider_index;
    }
    else {
      input_pid_map[i] = driver_index;
    }
      inputs[i] = 1;
  }
 
  
  auto circ = Circuit<Field>::generateEDSCircuit(rider_count,driver_count);
  auto level_circ = circ.orderGatesByLevel();
  auto exp_output = circ.evaluate(inputs);
  std::vector<std::future<std::vector<Field>>> parties;
  parties.reserve(nP+1);
  for (int i = 0; i <= nP; ++i) {
      parties.push_back(std::async(std::launch::async, [&, i, input_pid_map, inputs]() {
      ZZ_p_ctx.restore();
      // auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true);
      
      OfflineEvaluator eval(i, rider_count, driver_count, network, 
                            level_circ, SECURITY_PARAM, 4);
      auto preproc = eval.run(input_pid_map);
      
      OnlineEvaluator online_eval(i, rider_count, driver_count, std::move(network), std::move(preproc),
                                  level_circ, SECURITY_PARAM, 1);
      
      auto res = online_eval.evaluateCircuit(inputs);
      return res;
    }));
  }
  auto output = parties[0].get();
  BOOST_TEST(exp_output == output);
}

BOOST_AUTO_TEST_SUITE_END()
