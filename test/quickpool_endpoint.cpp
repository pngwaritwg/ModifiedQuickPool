#define BOOST_TEST_MODULE Endpoint_Matching

#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/included/unit_test.hpp>

#include "ED_offline_eval.h"
#include "ED_online_eval.h"
#include "ED_eval.h"
#include "sharing.h"

#define START_MATCH_THRESHOLD (Field)50
#define END_MATCH_THRESHOLD (Field)50

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

BOOST_AUTO_TEST_SUITE(Endpoint_Matching)

// testing the functionality of computing Euclidean distances between 
// the start points and end points of a single rider and driver
BOOST_AUTO_TEST_CASE(single_pair_matching) {
  NTL::ZZ_pContext ZZ_p_ctx;
  ZZ_p_ctx.save();
  int rider_count = 1;
  int driver_count = 1;
  int nP = rider_count + driver_count;
  int rider_index = 1;
  int driver_index = 2;

  srand(time(0));
  std::mt19937 gen(rand());
  std::uniform_int_distribution<uint> distrib(0, TEST_DATA_MAX_VAL);

  auto circ = Circuit<Field>::generateEDSCircuit(rider_count, driver_count);
  std::unordered_map<wire_t, int> input_pid_map;
  std::unordered_map<wire_t, Field> inputs;
  for (int rider=0, j=0; rider<rider_count; rider++) {
    int rider_id = rider+1;
    for (int driver=0; driver<driver_count; driver++) {
      int driver_id = driver+rider_count+1;
      for(int i = 0; i < 2; ++i) {
        input_pid_map[j] = rider_id;
        inputs[j++] = Field(distrib(gen));
        input_pid_map[j] = rider_id;
        inputs[j++] = Field(distrib(gen));
        input_pid_map[j] = driver_id;
        inputs[j++] = Field(distrib(gen));
        input_pid_map[j] = driver_id;
        inputs[j++] = Field(distrib(gen));
      }
      j+= 6; // to skip subtraction gates and dotproduct gates in the circuit
    }
  }
  
  auto insecure_outputs = circ.evaluate(inputs);

  auto level_circ = circ.orderGatesByLevel();
  std::vector<std::future<std::vector<Field>>> parties;
  parties.reserve(nP+1);
  for (int i = 0; i <= nP; ++i) {      
    parties.push_back(std::async(std::launch::async, [&, i]() {          
      ZZ_p_ctx.restore();       
      // auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);  
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true);  
      ED_eval ed_eval(i, rider_count, driver_count, network, level_circ, SECURITY_PARAM, nP);   
      auto res = ed_eval.pair_matching(input_pid_map, inputs, rider_index, driver_index);
      return res;
    }));
  }

  auto exp_output = parties[0].get();
  BOOST_TEST(exp_output == insecure_outputs);
}

// testing the functionality of computing Euclidean distances between 
// the start points and end points of multiple riders and drivers
BOOST_AUTO_TEST_CASE(multi_pair_matching) {
  NTL::ZZ_pContext ZZ_p_ctx;
  ZZ_p_ctx.save();
  int rider_count = 8;
  int driver_count = 9;
  int nP = rider_count + driver_count;

  srand(time(0));
  std::mt19937 gen(rand());
  std::uniform_int_distribution<uint> distrib(0, TEST_DATA_MAX_VAL);

  auto circ = Circuit<Field>::generateEDSCircuit(rider_count, driver_count);
  std::unordered_map<wire_t, int> input_pid_map;
  std::unordered_map<wire_t, Field> inputs;
  for (int rider=0, j=0; rider<rider_count; rider++) {
    int rider_id = rider+1;
    for (int driver=0; driver<driver_count; driver++) {
      int driver_id = driver+rider_count+1;
      for(int i = 0; i < 2; ++i) {
        input_pid_map[j] = rider_id;
        inputs[j++] = Field(distrib(gen));
        input_pid_map[j] = rider_id;
        inputs[j++] = Field(distrib(gen));
        input_pid_map[j] = driver_id;
        inputs[j++] = Field(distrib(gen));
        input_pid_map[j] = driver_id;
        inputs[j++] = Field(distrib(gen));
      }
      j+= 6; // to skip subtraction gates and dotproduct gates in the circuit
    }
  }   
  
  auto insecure_outputs = circ.evaluate(inputs);

  auto level_circ = circ.orderGatesByLevel();
  std::vector<std::future<std::vector<Field>>> parties;
  parties.reserve(nP+1);
  for (int i = 0; i <= nP; ++i) {      
    parties.push_back(std::async(std::launch::async, [&, i]() {          
      ZZ_p_ctx.restore();  
      // auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);  
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true); 
      ED_eval ed_eval(i, rider_count, driver_count, network, level_circ, SECURITY_PARAM, nP); 
      auto res = ed_eval.pair_matching(input_pid_map, inputs);
      return res;
    }));
  }

  auto exp_output = parties[0].get();
  BOOST_TEST(exp_output == insecure_outputs);
}

// testing the functionality of end-point based matching between a single rider and a single driver
BOOST_AUTO_TEST_CASE(single_pair_ED_Matching) {
  NTL::ZZ_pContext ZZ_p_ctx;
  ZZ_p_ctx.save();
  int rider_count = 1;
  int driver_count = 1;
  int nP = rider_count + driver_count;
  int rider_index = 1;
  int driver_index = 2;

  srand(time(0));
  std::mt19937 gen(rand());
  std::uniform_int_distribution<uint> distrib(0, TEST_DATA_MAX_VAL);

  auto circ = Circuit<Field>::generateEDSCircuit(rider_count, driver_count);
  std::unordered_map<wire_t, int> input_pid_map;
  std::unordered_map<wire_t, Field> inputs;
  for (int rider=0, j=0; rider<rider_count; rider++) {
    int rider_id = rider+1;
    for (int driver=0; driver<driver_count; driver++) {
      int driver_id = driver+rider_count+1;
      for(int i = 0; i < 2; ++i) {
        input_pid_map[j] = rider_id;
        // inputs[j++] = Field(distrib(gen));
        inputs[j++] = Field(distrib(gen))%10;
        input_pid_map[j] = rider_id;
        // inputs[j++] = Field(distrib(gen));
        inputs[j++] = Field(distrib(gen))%10;
        input_pid_map[j] = driver_id;
        // inputs[j++] = Field(distrib(gen));
        inputs[j++] = Field(distrib(gen))%10;
        input_pid_map[j] = driver_id;
        // inputs[j++] = Field(distrib(gen));
        inputs[j++] = Field(distrib(gen))%10;
      }
      j+= 6; // to skip subtraction gates and dotproduct gates in the circuit
    }
  }
  
  auto insecure_outputs = circ.evaluate(inputs);

  auto level_circ = circ.orderGatesByLevel();
  std::vector<std::future<std::vector<Field>>> parties;
  parties.reserve(nP+1);
  for (int i = 0; i <= nP; ++i) {      
    parties.push_back(std::async(std::launch::async, [&, i]() {          
      ZZ_p_ctx.restore();       
      // auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);  
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true); 
      ED_eval ed_eval(i, rider_count, driver_count, network, level_circ, SECURITY_PARAM, nP);   
      auto res = ed_eval.pair_EDMatching(input_pid_map, inputs, rider_index, driver_index);
      return res;
    }));
  }

  int i = 0;
  std::vector<Field> output_rider;
  std::vector<Field> output_driver;
  
  for (auto& p : parties) {
    if (i == rider_index) {
      output_rider = p.get();
    }
    else if(i == driver_index) {
      output_driver = p.get();
    }
    i++;
  }

  Field output = (output_rider[0] + output_driver[0]) * (output_rider[1] + output_driver[1]);

  Field check;
  if(insecure_outputs[0] < START_MATCH_THRESHOLD*START_MATCH_THRESHOLD && insecure_outputs[1] < END_MATCH_THRESHOLD*END_MATCH_THRESHOLD){
    check = 1;
  }
  else {
    check = 0;
  }

  BOOST_TEST(output == check);
}

// testing the functionality of end-point based matching among multiple riders and drivers
BOOST_AUTO_TEST_CASE(all_pair_ED_Matching) {
  NTL::ZZ_pContext ZZ_p_ctx;
  ZZ_p_ctx.save();
  int rider_count = 2;
  int driver_count = 2;
  int nP = rider_count + driver_count;

  srand(time(0));
  std::mt19937 gen(rand());
  std::uniform_int_distribution<uint> distrib(0, TEST_DATA_MAX_VAL);

  auto circ = Circuit<Field>::generateEDSCircuit(rider_count, driver_count);
  std::unordered_map<wire_t, int> input_pid_map;
  std::unordered_map<wire_t, Field> inputs;
  for (int rider=0, j=0; rider<rider_count; rider++) {
    int rider_id = rider+1;
    for (int driver=0; driver<driver_count; driver++) {
      int driver_id = driver+rider_count+1;
      for(int i = 0; i < 2; ++i) {
        input_pid_map[j] = rider_id;
        inputs[j++] = Field(distrib(gen));
        // inputs[j++] = Field(distrib(gen))%10;
        input_pid_map[j] = rider_id;
        inputs[j++] = Field(distrib(gen));
        // inputs[j++] = Field(distrib(gen))%10;
        input_pid_map[j] = driver_id;
        inputs[j++] = Field(distrib(gen));
        // inputs[j++] = Field(distrib(gen))%10;
        input_pid_map[j] = driver_id;
        inputs[j++] = Field(distrib(gen));
        // inputs[j++] = Field(distrib(gen))%10;
      }
      j+= 6; // to skip subtraction gates and dotproduct gates in the circuit
    }
  }
  
  auto insecure_outputs = circ.evaluate(inputs);

  auto level_circ = circ.orderGatesByLevel();
  std::vector<std::future<std::vector<Field>>> parties;
  parties.reserve(nP+1);
  for (int i = 0; i <= nP; ++i) {      
    parties.push_back(std::async(std::launch::async, [&, i]() {          
      ZZ_p_ctx.restore();       
      // auto network = std::make_shared<io::NetIOMP>(i, nP+1, 10000, nullptr, true);  
      auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true); 
      ED_eval ed_eval(i, rider_count, driver_count, network, level_circ, SECURITY_PARAM, nP);   
      auto res = ed_eval.pair_EDMatching(input_pid_map, inputs);
      return res;
    }));
  }
  
  auto output = parties[0].get();

  std::vector<Field> check;
  for (size_t i=0; i<insecure_outputs.size(); i+=2) {
    if(insecure_outputs[i] < START_MATCH_THRESHOLD*START_MATCH_THRESHOLD && insecure_outputs[i+1] < END_MATCH_THRESHOLD*END_MATCH_THRESHOLD){
      check.push_back(1);
    }
    else {
      check.push_back(0);
    }
  }  

  BOOST_TEST(output == check);
}

BOOST_AUTO_TEST_SUITE_END()
