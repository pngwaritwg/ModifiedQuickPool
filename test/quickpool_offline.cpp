#define BOOST_TEST_MODULE Quickpool_offline

#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/included/unit_test.hpp>

#include "ED_offline_eval.h"

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

BOOST_AUTO_TEST_SUITE(Quickpool_offline)

BOOST_AUTO_TEST_CASE(random_share) {
  NTL::ZZ_pContext ZZ_p_ctx;
  ZZ_p_ctx.save();
  int rider_count = 1;
  int driver_count = 1;
  int nP = rider_count + driver_count;
  int rider_index = 1;
  int driver_index = 2;  
  
  std::vector<std::future<AddShare<Field>>> parties;
  
  TPShare<Field> tpshares;
  for (int i = 0; i <= nP; i++) {    
    parties.push_back(std::async(std::launch::async, [&, i]() { 
      ZZ_p_ctx.restore();
      AddShare<Field> shares;
      RandGenPool vrgen(i, nP);
      // auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true);
      LevelOrderedCircuit circ;
      OfflineEvaluator eval(i, rider_count, driver_count, std::move(network), circ, SECURITY_PARAM, nP);
      if(i == 0) {
        eval.randomShare(rider_index, driver_index, vrgen, *network, shares, tpshares);
      }
      else if (i == rider_index) {
         eval.randomShare(rider_index, driver_index, vrgen, *network, shares, tpshares);
      }
      else if (i == driver_index) {
         eval.randomShare(rider_index, driver_index, vrgen, *network, shares, tpshares);
      }
      else {
        shares.pushValue((Field)0);
      }
      return shares;
    }));
    
  }
  int i = 0;
  for (auto& p : parties) { 
    auto res = p.get();
    if(i == rider_index) 
      { BOOST_TEST(res.valueAt() == tpshares.commonValueWithParty(1));}
    else if(i == driver_index) 
      { BOOST_TEST(res.valueAt() == tpshares.commonValueWithParty(2));}
    i++;
  }
}

BOOST_AUTO_TEST_CASE(depth_2_circuit) {
	NTL::ZZ_pContext ZZ_p_ctx;
	ZZ_p_ctx.save();
  int rider_count = 1;
  int driver_count = 1;
  int nP = rider_count + driver_count;
  int rider_index = 1;
  int driver_index = 2;
	Circuit<Field> circ;
	std::vector<wire_t> input_wires;
	std::unordered_map<wire_t, int> input_pid_map;

	for (size_t i = 0; i < 4; ++i) {
		auto winp = circ.newInputWire(rider_index, driver_index);
		input_wires.push_back(winp);
		input_pid_map[winp] = 1;
	}
	
	auto w_aab = circ.addGate(GateType::kAdd, input_wires[0], input_wires[1], rider_index, driver_index);
	auto w_cmd = circ.addGate(GateType::kMul, input_wires[2], input_wires[3], rider_index, driver_index);
	auto w_cons = circ.addConstOpGate(GateType::kConstAdd, w_aab, Field(2), rider_index, driver_index);
	auto w_cons_m = circ.addConstOpGate(GateType::kConstMul, w_cmd, Field(2), rider_index, driver_index);
	auto w_mout = circ.addGate(GateType::kMul, w_aab, w_cons, rider_index, driver_index);
	auto w_aout = circ.addGate(GateType::kAdd, w_aab, w_cmd, rider_index, driver_index);
	circ.setAsOutput(w_mout, rider_index, driver_index);
	circ.setAsOutput(w_aout, rider_index, driver_index);
	auto level_circ = circ.orderGatesByLevel();
	std::vector<std::future<PreprocCircuit<Field>>> parties;
	parties.reserve(nP+1);
	for (int i = 0; i <= nP; ++i) {
		parties.push_back(std::async(std::launch::async, [&, i, input_pid_map]() {
			ZZ_p_ctx.restore();
			// auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true);
			RandGenPool vrgen(i, nP);
      OfflineEvaluator eval(i, rider_count, driver_count, std::move(network), level_circ, SECURITY_PARAM, nP);
			return eval.run(input_pid_map);
		}));
	}
	std::vector<PreprocCircuit<Field>> v_preproc;
	v_preproc.reserve(parties.size());
	for (auto& f : parties) {
		v_preproc.push_back(f.get());
	}
	
	BOOST_TEST(v_preproc[0].gates.size() == level_circ.num_gates);
	const auto& preproc_0 = v_preproc[0];
	for (int i = 1; i <= nP; ++i) {
    if(i == rider_index) {
      BOOST_TEST(v_preproc[i].gates.size() == level_circ.num_gates);
      const auto& preproc_i = v_preproc[i];
      for(int j = 0; j < 4; j++) {
        auto tpmask = preproc_0.gates[j]->tpmask;
        auto mask_i = preproc_i.gates[j]->mask;
        BOOST_TEST(mask_i.valueAt() == tpmask.commonValueWithParty(1));
      }	
    }
    else if(i == driver_index) {
      BOOST_TEST(v_preproc[i].gates.size() == level_circ.num_gates);
      const auto& preproc_i = v_preproc[i];
      for(int j = 0; j < 4; j++) {
        auto tpmask = preproc_0.gates[j]->tpmask;
        auto mask_i = preproc_i.gates[j]->mask;
        BOOST_TEST(mask_i.valueAt() == tpmask.commonValueWithParty(2));
      }	
    }
	} 
	
}
BOOST_AUTO_TEST_SUITE_END()
