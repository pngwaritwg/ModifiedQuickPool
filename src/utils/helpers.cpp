#include "helpers.h"

namespace common::utils {

void print128_num(__m128i var) {
    uint8_t val[16];
    memcpy(val, &var, sizeof(val));
    printf("Numerical: %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n",  val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8], val[9], val[10], val[11], val[12], val[13], val[14], val[15]);
}

std::vector<uint64_t> packBool(const bool* data, size_t len) {
  std::vector<uint64_t> res;
  for (size_t i = 0; i < len;) {
    uint64_t temp = 0;
    for (size_t j = 0; j < 64 && i < len; ++j, ++i) {
      if (data[i]) {
        temp |= (0x1ULL << j);
      }
    }
    res.push_back(temp);
  }
  return res;
}

void unpackBool(const std::vector<uint64_t>& packed, bool* data, size_t len) {
  for (size_t i = 0, count = 0; i < len; count++) {
    uint64_t temp = packed[count];
    for (int j = 63; j >= 0 && i < len; ++i, --j) {
      data[i] = (temp & 0x1) == 0x1;
      temp >>= 1;
    }
  }
}

void randomize(emp::PRG& prg, Field& val, int nbytes) {
    uint64_t var;
    prg.random_data(&var, nbytes);
    val = Field(var);
}

void randomizeZZp(emp::PRG& prg, NTL::ZZ_p& val, int nbytes) {
    uint64_t var;
    prg.random_data(&var, nbytes);
    val = NTL::ZZ_p(var);
}

void randomizeZZpE(emp::PRG& prg, NTL::ZZ_pE& val) {
  std::vector<Ring> coeff(NTL::ZZ_pE::degree());
  prg.random_data(coeff.data(), sizeof(Ring) * coeff.size());
  NTL::ZZ_pX temp;
  temp.SetLength(NTL::ZZ_pE::degree());
  for (size_t i = 0; i < coeff.size(); ++i) {
    temp[i] = coeff[i];
  }
  NTL::conv(val, temp);
}

void randomizeZZpE(emp::PRG& prg, NTL::ZZ_pE& val, Ring rval) {
  std::vector<Ring> coeff(NTL::ZZ_pE::degree() - 1);
  prg.random_data(coeff.data(), sizeof(Ring) * coeff.size());
  NTL::ZZ_pX temp;
  temp.SetLength(NTL::ZZ_pE::degree());
  temp[0] = rval;
  for (size_t i = 1; i < coeff.size(); ++i) {
    temp[i] = coeff[i];
  }
  NTL::conv(val, temp);
}

void sendZZpE(emp::NetIO* ios, const NTL::ZZ_pE* data, size_t length) {
  auto degree = NTL::ZZ_pE::degree();
  // Assumes that every co-efficient of ZZ_pE is same range as Ring.
  std::vector<uint8_t> serialized(sizeof(Ring));
  for (size_t i = 0; i < length; ++i) {
    const auto& poly = NTL::rep(data[i]);
    for (size_t d = 0; d < degree; ++d) {
      const auto& coeff = NTL::rep(NTL::coeff(poly, d));
      NTL::BytesFromZZ(serialized.data(), coeff, serialized.size());
      ios->send_data(serialized.data(), serialized.size());
    }
  }
  ios->flush();
}

void receiveZZpE(emp::NetIO* ios, NTL::ZZ_pE* data, size_t length) {
  auto degree = NTL::ZZ_pE::degree();
  // Assumes that every co-efficient of ZZ_pE is same range as Ring.
  std::vector<uint8_t> serialized(sizeof(Ring));
  NTL::ZZ_pX poly;
  poly.SetLength(degree);
  for (size_t i = 0; i < length; ++i) {
    for (size_t d = 0; d < degree; ++d) {
      ios->recv_data(serialized.data(), serialized.size());
      auto coeff = NTL::conv<NTL::ZZ_p>(
          NTL::ZZFromBytes(serialized.data(), serialized.size()));
      poly[d] = coeff;
    }
    NTL::conv(data[i], poly);
  }
}
};  // namespace common::utils 
