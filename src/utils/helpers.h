#pragma once

#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pE.h>
#include <NTL/ZZ_pX.h>
#include <emp-tool/emp-tool.h>
#include <vector>

#include "types.h"

namespace common::utils  {

void print128_num(__m128i var);

template <class R>
std::vector<BoolRing> bitDecompose(R val) {
  auto num_bits = sizeof(val) * 8;
  std::vector<BoolRing> res(num_bits);
  for (size_t i = 0; i < num_bits; ++i) {
    res[i] = ((val >> i) & 1ULL) == 1;
  }
  return res;
}

template <class R>
std::vector<BoolRing> bitDecomposeTwo(R value) {
  uint64_t val = conv<uint64_t>(value);
  return bitDecompose(val);
}

std::vector<uint64_t> packBool(const bool* data, size_t len);
void unpackBool(const std::vector<uint64_t>& packed, bool* data, size_t len);
void randomize(emp::PRG& prg, Field& val, int nbytes);
void randomizeZZp(emp::PRG& prg, NTL::ZZ_p& val, int nbytes);
void randomizeZZpE(emp::PRG& prg, NTL::ZZ_pE& val);
void randomizeZZpE(emp::PRG& prg, NTL::ZZ_pE& val, Ring rval);

void sendZZpE(emp::NetIO* ios, const NTL::ZZ_pE* data, size_t length);
void receiveZZpE(emp::NetIO* ios, NTL::ZZ_pE* data, size_t length);

/*
bool bpm(std::vector<std::vector<bool>> bpGraph, int u,
         std::vector<bool> seen, std::vector<int> matchR)
{
  int M = bpGraph.size();
  int N = bpGraph[0].size();
    for (int v = 0; v < N; v++)
    {
        if (bpGraph[u][v] && !seen[v])
        {
            seen[v] = true; 
            if (matchR[v] < 0 || bpm(bpGraph, matchR[v],
                                     seen, matchR))
            {
                matchR[v] = u;
                return true;
            }
        }
    }
    return false;
}
 

int maxBPM(std::vector<std::vector<bool>> bpGraph)
{
  int M = bpGraph.size();
  int N = bpGraph[0].size();
    
    std::vector<int> matchR(N, -1);
 
    int result = 0; 
    for (int u = 0; u < M; u++)
    {
        std::vector<bool> seen(N,0);
        
        if (bpm(bpGraph, u, seen, matchR))
            result++;
    }
    return result;
}
*/
};  // namespace common::utils 
