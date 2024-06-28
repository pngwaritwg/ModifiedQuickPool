#define BOOST_TEST_MODULE Quickpool_sharing

#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/included/unit_test.hpp>

#include "sharing.h"
#include "types.h"

using namespace quickpool;
using namespace common::utils;
namespace bdata = boost::unit_test::data;

struct GlobalFixture {
  GlobalFixture() {
    NTL::ZZ_p::init(NTL::conv<NTL::ZZ>("17816577890427308801"));
  }
};

BOOST_GLOBAL_FIXTURE(GlobalFixture);

constexpr int TEST_DATA_MAX_VAL = 1000;
constexpr int NUM_SAMPLES = 1;
std::random_device rd;
std::mt19937 engine(rd());
std::uniform_int_distribution<uint64_t> distrib;

// Utility function to generate replicated secret sharing of 3 parties.
std::vector<AddShare<Field>> generateAddShares(Field secret) {
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<uint64_t> distrib;
  int nP = 2;

  std::vector<Field> values(nP);

  Field sum = Field(0);
  for (int i = 0; i < nP-1; ++i) {
    values[i] = Field(distrib(engine));
    sum += values[i];
  }
  values[nP-1] = secret - sum;

  std::vector<AddShare<Field>> AAS;
  for(size_t i = 0; i < nP ; i++) {
    AddShare<Field> temp(values[i]);
    AAS.push_back(temp);
  }

  return AAS;

}

// Utility function to reconstruct secret from shares as generated by
// generateReplicatedShares function.

Field reconstructAddShares(
    const std::vector<AddShare<Field>>& v_aas) {
  int nP = 2;
  Field secret = Field(0);
      for(size_t i = 1; i <= nP; i++) {
        secret += v_aas[i-1].valueAt();
      }
  return secret;
  }

BOOST_AUTO_TEST_SUITE(additive_sharing)

BOOST_DATA_TEST_CASE(reconstruction,
                     bdata::random(0, TEST_DATA_MAX_VAL) ^
                         bdata::xrange(NUM_SAMPLES),
                     secret_val, idx) {
  Field secret = Field(secret_val);

  auto v_aas = generateAddShares(secret);
  
  auto recon_value = reconstructAddShares(v_aas);
    
  BOOST_TEST(recon_value == secret);
}

BOOST_DATA_TEST_CASE(share_arithmetic,
                     bdata::random(0, TEST_DATA_MAX_VAL) ^
                         bdata::random(0, TEST_DATA_MAX_VAL) ^
                         bdata::xrange(NUM_SAMPLES),
                     vala, valb, idx) {
  Field a = Field(vala);
  Field b = Field(valb);
  auto v_aas_a = generateAddShares(a);
  auto v_aas_b = generateAddShares(b);

  int nP = 2;
  std::vector<AddShare<Field>> v_aas_c(nP);

  for (size_t i = 0; i < nP; ++i) {
    // This implicitly checks compound assignment operators too.
    v_aas_c[i] = v_aas_a[i] + v_aas_b[i];
  }

  auto sum = reconstructAddShares(v_aas_c);

  // std::cout << sum <<"\t" << a + b <<"\n";
  BOOST_TEST(sum == a + b);
  
  for (size_t i = 0; i < nP; ++i) {
    // This implicitly checks compound assignment operators too.
    v_aas_c[i] = v_aas_a[i] - v_aas_b[i];
  }

  auto difference = reconstructAddShares(v_aas_c);
  BOOST_TEST(difference == a - b);
  
}

BOOST_DATA_TEST_CASE(share_const_arithmetic,
                     bdata::random(0, TEST_DATA_MAX_VAL) ^
                         bdata::random(0, TEST_DATA_MAX_VAL) ^
                         bdata::xrange(NUM_SAMPLES),
                     secret_val, const_val, idx) {
  size_t nP = 2;
  // Field secret = secret_val;
  // Field constant = const_val;
  Field secret = Field(100);
  Field constant = Field(200);
  auto v_aas = generateAddShares(secret);

  std::vector<AddShare<Field>> v_aas_res(nP);
  for (size_t i = 0; i < nP; ++i) {
    // This implicitly checks compound assignment operators too.
    v_aas_res[i] = v_aas[i].add(constant, i);
  }

  auto sum = reconstructAddShares(v_aas_res);
  BOOST_TEST(sum == secret + constant);

}


BOOST_DATA_TEST_CASE(const_addition,
                     bdata::random(0, TEST_DATA_MAX_VAL) ^
                         bdata::random(0, TEST_DATA_MAX_VAL) ^
                         bdata::xrange(NUM_SAMPLES),
                     secret_val, const_val, idx) {
  size_t nP = 2;
  // Field secret = secret_val;
  // Field constant = const_val;
  Field secret = Field(100);
  Field constant = Field(200);
  auto v_aas = generateAddShares(secret);

  std::vector<AddShare<Field>> v_aas_res(nP);
  for (size_t i = 0; i < nP; ++i) {
    // This implicitly checks compound assignment operators too.
    v_aas_res[i] = v_aas[i] * constant;
  }

  auto product = reconstructAddShares(v_aas_res);
  BOOST_TEST(product == secret * constant);
  
  
}

BOOST_AUTO_TEST_SUITE_END()
