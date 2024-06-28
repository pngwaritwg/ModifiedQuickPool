#include "netmp.h"

namespace io {
    
NetIOMP::NetIOMP(int party, int nP, int port, char* IP[], bool localhost)
      : ios(nP), ios2(nP), party(party), nP(nP), sent(nP, false) {
    for (int i = 0; i < nP; ++i) {
      for (int j = i + 1; j < nP; ++j) {
        if (i == party) {
          usleep(1000);
          if (localhost) {
            ios[j] = std::make_unique<NetIO>("127.0.0.1", port + 2 * (i * nP + j), true);
          } else {
            ios[j] = std::make_unique<NetIO>(IP[j], port + 2 * (i * nP +j), true);
          }
          ios[j]->set_nodelay();

          usleep(1000);
          if (localhost) {
            ios2[j] = std::make_unique<NetIO>(nullptr, port + 2 * (i * nP + j) + 1, true);
          } else {
            ios2[j] = std::make_unique<NetIO>(nullptr, port + 2 * (i * nP +j) + 1, true);
          }
          ios2[j]->set_nodelay();
        } else if (j == party) {
          usleep(1000);
          if (localhost) {
            ios[i] = std::make_unique<NetIO>(nullptr, port + 2 * (i * nP + j), true);
          } else {
            ios[i] = std::make_unique<NetIO>(nullptr, port + 2 * (i * nP +j), true);
          }
          ios[i]->set_nodelay();

          usleep(1000);
          if (localhost) {
            ios2[i] = std::make_unique<NetIO>("127.0.0.1", port + 2 * (i * nP + j) + 1, true);
          } else {
            ios2[i] = std::make_unique<NetIO>(IP[i], port + 2 * (i * nP +j) + 1, true);
          }
          ios2[i]->set_nodelay();
        }
      }
    }
  }

  NetIOMP::NetIOMP(int party, int rider_count, int driver_count, int port, char* IP[], bool localhost)
      : party(party), nP(rider_count+driver_count+1), ios(rider_count+driver_count+1), ios2(rider_count+driver_count+1), sent(rider_count+driver_count+1, false) {
    if (party == 0) {
      for (int k = 1; k < nP; ++k) {
        usleep(1000);
        if (localhost) {
          ios[k] = std::make_unique<NetIO>("127.0.0.1", port + 2*k, true);
        } else {
          ios[k] = std::make_unique<NetIO>(IP[k], port + 2*k, true);
        }
        ios[k]->set_nodelay();

        usleep(1000);
        if (localhost) {
          ios2[k] = std::make_unique<NetIO>(nullptr, port + 2*k + 1, true);
        } else {
          ios2[k] = std::make_unique<NetIO>(nullptr, port + 2*k + 1, true);
        }
        ios2[k]->set_nodelay();
      }
    }
    else {
      // for ID 0
      usleep(1000);
      if (localhost) {
        ios[0] = std::make_unique<NetIO>(nullptr, port + 2*party, true);
      } else {
        ios[0] = std::make_unique<NetIO>(nullptr, port + 2*party, true);
      }
      ios[0]->set_nodelay();

      usleep(1000);
      if (localhost) {
        ios2[0] = std::make_unique<NetIO>("127.0.0.1", port + 2*party + 1, true);
      } else {
        ios2[0] = std::make_unique<NetIO>(IP[0], port + 2*party + 1, true);
      }
      ios2[0]->set_nodelay();      

      // for others
      int shift_port = 2 * (nP-1);
      for (int i = 1; i <= rider_count; ++i) {
        for (int j = rider_count+1; j <= rider_count+driver_count; ++j) {
          int comp_port = 2*((i-1)*driver_count + j - rider_count);
          if (i == party) {
            usleep(1000);
            if (localhost) {
              ios[j] = std::make_unique<NetIO>("127.0.0.1", port + shift_port + comp_port, true);
            } else {
              ios[j] = std::make_unique<NetIO>(IP[j], port + shift_port + comp_port, true);
            }
            ios[j]->set_nodelay();

            usleep(1000);
            if (localhost) {
              ios2[j] = std::make_unique<NetIO>(nullptr, port + shift_port + comp_port + 1, true);
            } else {
              ios2[j] = std::make_unique<NetIO>(nullptr, port + shift_port + comp_port + 1, true);
            }
            ios2[j]->set_nodelay();
          } else if (j == party) {
            usleep(1000);
            if (localhost) {
              ios[i] = std::make_unique<NetIO>(nullptr, port + shift_port + comp_port, true);
            } else {
              ios[i] = std::make_unique<NetIO>(nullptr, port + shift_port + comp_port, true);
            }
            ios[i]->set_nodelay();

            usleep(1000);
            if (localhost) {
              ios2[i] = std::make_unique<NetIO>("127.0.0.1", port + shift_port + comp_port + 1, true);
            } else {
              ios2[i] = std::make_unique<NetIO>(IP[i], port + shift_port + comp_port + 1, true);
            }
            ios2[i]->set_nodelay();
          }
        }
      }
    }
  }

  int64_t NetIOMP::count() {
    int64_t res = 0;
    for (int i = 0; i < nP; ++i)
      if (i != party) {
        res += ios[i]->counter;
        res += ios2[i]->counter;
      }
    return res;
  }

  void NetIOMP::resetStats() {
    for (int i = 0; i < nP; ++i) {
      if (i != party) {
        ios[i]->counter = 0;
        ios2[i]->counter = 0;
      }
    }
  }

  void NetIOMP::send(int dst, const void* data, size_t len) {
    if (dst != -1 and dst != party) {
      if (party < dst)
        ios[dst]->send_data(data, len);
      else
        ios2[dst]->send_data(data, len);
      sent[dst] = true;
    }
    #ifdef __clang__
        flush(dst);
    #endif
  }

  void NetIOMP::send(int dst, const NTL::ZZ_p* data, size_t length) {  
    // Assumes that every co-efficient of ZZ_pE is same range as Ring.
    std::vector<uint8_t> serialized(length);
    size_t num = (length + FIELDSIZE - 1) / FIELDSIZE;
    for (size_t i = 0; i < num; ++i) {
        NTL::BytesFromZZ(serialized.data() + i * FIELDSIZE, NTL::conv<NTL::ZZ>(data[i]), FIELDSIZE);
    }
    send(dst, serialized.data(), serialized.size());
  }

  void NetIOMP::sendRelative(int offset, const void* data, size_t len) {
    int dst = (party + offset) % nP;
    if (dst < 0) {
      dst += nP;
    }
    send(dst, data, len);
  }

  void NetIOMP::sendBool(int dst, const bool* data, size_t len) {
    for (int i = 0; i < len;) {
      uint64_t tmp = 0;
      for (int j = 0; j < 64 && i < len; ++i, ++j) {
        if (data[i]) {
          tmp |= (0x1ULL << j);
        }
      }
      send(dst, &tmp, 8);
    }
  }

  void NetIOMP::sendBoolRelative(int offset, const bool* data, size_t len) {
    int dst = (party + offset) % nP;
    if (dst < 0) {
      dst += nP;
    }
    sendBool(dst, data, len);
  }

  void NetIOMP::recv(int src, void* data, size_t len) {
    if (src != -1 && src != party) {
      if (sent[src]) flush(src);
      if (src < party)
        ios[src]->recv_data(data, len);
      else
        ios2[src]->recv_data(data, len);
    }
  }

  void NetIOMP::recv(int dst, NTL::ZZ_p* data, size_t length) {
    std::vector<uint8_t> serialized(length);
    recv(dst, serialized.data(), serialized.size());
    // Assumes that every co-efficient of ZZ_pE is same range as Ring.
    
    size_t num = (length + FIELDSIZE - 1) / FIELDSIZE;
    for (size_t i = 0; i < num; ++i) {
      data[i] = NTL::conv<NTL::ZZ_p>(NTL::ZZFromBytes(serialized.data() + i * FIELDSIZE, FIELDSIZE));
    }
  }

  void NetIOMP::recvRelative(int offset, void* data, size_t len) {
    int src = (party + offset) % nP;
    if (src < 0) {
      src += nP;
    }
    recv(src, data, len);
  }

  void NetIOMP::recvBool(int src, bool* data, size_t len) {
    for (int i = 0; i < len;) {
      uint64_t tmp = 0;
      recv(src, &tmp, 8);
      for (int j = 63; j >= 0 && i < len; ++i, --j) {
        data[i] = (tmp & 0x1) == 0x1;
        tmp >>= 1;
      }
    }
  }

  void NetIOMP::recvRelative(int offset, bool* data, size_t len) {
    int src = (party + offset) % nP;
    if (src < 0) {
      src += nP;
    }
    recvBool(src, data, len);
  }

  NetIO* NetIOMP::get(size_t idx, bool b) {
    if (b)
      return ios[idx].get();
    else
      return ios2[idx].get();
  }

  NetIO* NetIOMP::getSendChannel(size_t idx) {
    if (party < idx) {
      return ios[idx].get();
    }
    return ios2[idx].get();
  }

  NetIO* NetIOMP::getRecvChannel(size_t idx) {
    if (idx < party) {
      return ios[idx].get();
    }
    return ios2[idx].get();
  }  

  void NetIOMP::flush(int idx) {
    if (idx == -1) {
      for (int i = 0; i < nP; ++i) {
        if (i != party) {
          ios[i]->flush();
          ios2[i]->flush();
        }
      }
    } else {
      if (party < idx) {
        ios[idx]->flush();
      } else {
        ios2[idx]->flush();
      }
    }
  }

  void NetIOMP::flush(int rider_count, int driver_count, int idx) {
    if (party == 0) {
      if (idx == -1) {
        for (int i = 1; i < nP; ++i) {
            ios[i]->flush();
            ios2[i]->flush();
        }
      }
      else {
        ios[idx]->flush();
      }
    }
    else if (party <= rider_count) {
      if (idx == -1) {
        ios[0]->flush();
        ios2[0]->flush();
        for (int i = rider_count+1; i < nP; ++i) {
            ios[i]->flush();
            ios2[i]->flush();
        }
      }
      else {
        ios[idx]->flush();
      }
    }
    else {
      if (idx == -1) {
        ios[0]->flush();
        ios2[0]->flush();
        for (int i = 1; i <= rider_count; ++i) {
            ios[i]->flush();
            ios2[i]->flush();
        }
      }
      else {
        ios2[idx]->flush();
      }
    }
  }

  void NetIOMP::sync() {
    for (int i = 0; i < nP; ++i) {
      for (int j = 0; j < nP; ++j) {
        if (i < j) {
          if (i == party) {
            ios[j]->sync();
            ios2[j]->sync();
          } else if (j == party) {
            ios[i]->sync();
            ios2[i]->sync();
          }
        }
      }
    }
  }

};  // namespace io

