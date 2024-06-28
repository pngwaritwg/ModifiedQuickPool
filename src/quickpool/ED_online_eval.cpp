#include "ED_online_eval.h"

namespace quickpool
{
OnlineEvaluator::OnlineEvaluator(int id, int rider_count, int driver_count, 
                std::shared_ptr<io::NetIOMP> network,
                PreprocCircuit<Field> preproc, LevelOrderedCircuit circ,
                int security_param, int threads, int seed)
    : id_(id),
        rider_count(rider_count),
        driver_count(driver_count),
        security_param_(security_param),
        rgen_(id, seed),
        network_(std::move(network)),
        preproc_(std::move(preproc)),
        circ_(std::move(circ)),
        wires_(circ.num_gates) 
        {
            tpool_ = std::make_shared<ThreadPool>(threads);
        }

OnlineEvaluator::OnlineEvaluator(int id, int rider_count, int driver_count, 
                std::shared_ptr<io::NetIOMP> network,
                PreprocCircuit<Field> preproc, LevelOrderedCircuit circ,
                int security_param, std::shared_ptr<ThreadPool> tpool, int seed)
    : 
        id_(id),
        rider_count(rider_count),
        driver_count(driver_count),
        security_param_(security_param),
        rgen_(id, seed),
        network_(std::move(network)),
        preproc_(std::move(preproc)),
        circ_(std::move(circ)),
        tpool_(std::move(tpool)),
        wires_(circ.num_gates) {}

// checking if the current party is a rider or not
bool OnlineEvaluator::amIRider() {
  return (id_!=0 && id_<=rider_count);
}

// checking if the current party is a driver or not
bool OnlineEvaluator::amIDriver() {
  return (id_!=0 && id_>rider_count);
}

std::vector<Field> OnlineEvaluator::getwires() {
    return wires_;
}

// perform online phase for the Input gates    
void OnlineEvaluator::setInputs(const std::unordered_map<wire_t, Field> &inputs) {
    std::vector<Field> masked_values;
    // Input gates have depth 0
    for (auto &g : circ_.gates_by_level[0]) {
        if (g->type == GateType::kInp) {
            auto *pre_input = static_cast<PreprocInput<Field> *>(preproc_.gates[g->out].get());
            auto pid = pre_input->pid;
            auto rider_id = g->rider_id;
            auto driver_id = g->driver_id;
            if (id_ == pid || id_==rider_id || id_==driver_id) {
                if (id_ == pid) {
                    wires_[g->out] = pre_input->mask_value + inputs.at(g->out);
                    if(pid == rider_id) {                        
                        network_->send(driver_id, &wires_[g->out], sizeof(Field));
                    }
                    else if (pid == driver_id) {
                        network_->send(rider_id, &wires_[g->out], sizeof(Field));
                    }
                }
                else {
                    if (pid == rider_id && id_ == driver_id) {
                        network_->recv(pid, &wires_[g->out], sizeof(Field));
                    }
                    else if (pid == driver_id && id_ == rider_id) {
                        network_->recv(pid, &wires_[g->out], sizeof(Field));
                    }
                }
            }
        }
    }
}

void OnlineEvaluator::setRandomInputs() { // Incomplete
    // Input gates have depth 0.
    for (auto &g : circ_.gates_by_level[0]) {
        if (g->type == GateType::kInp) {
            // randomizeZZp(rgen_.all(), wires_[g->out], sizeof(Field));
            randomize(rgen_.all(), wires_[g->out], sizeof(Field));
        }
    }
}

void OnlineEvaluator::evaluateGatesAtDepthPartySend(size_t depth, std::vector<std::vector<Field>> &mult_nonTP, std::vector<std::vector<Field>> &dotprod_nonTP) {
    for (auto &gate : circ_.gates_by_level[depth]) {
        switch (gate->type) {

            case GateType::kMul: {
                auto *g = static_cast<FIn2Gate *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id || id_ == driver_id) {
                    auto &m_in1 = preproc_.gates[g->in1]->mask;
                    auto &m_in2 = preproc_.gates[g->in2]->mask;
                    auto *pre_out =
                        static_cast<PreprocMultGate<Field> *>(preproc_.gates[g->out].get());
                    auto q_share = pre_out->mask + pre_out->mask_prod -
                                    m_in1 * wires_[g->in2] - m_in2 * wires_[g->in1];
                    q_share.addWithAdder((wires_[g->in1] * wires_[g->in2]), id_, rider_id);
                    if (id_ == rider_id)
                        mult_nonTP[driver_id-1].push_back(q_share.valueAt());
                    else if (id_ == driver_id)
                        mult_nonTP[rider_id-1].push_back(q_share.valueAt());
                }
                break;
            }

            case ::GateType::kDotprod: {
                auto *g = static_cast<SIMDGate *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id || id_ == driver_id) {
                    auto *pre_out =
                    static_cast<PreprocDotpGate<Field> *>(preproc_.gates[g->out].get());
                    auto q_share = pre_out->mask + pre_out->mask_prod;
                    for (size_t i = 0; i < g->in1.size(); ++i) {
                        auto win1 = g->in1[i];                    // index for masked value for left input wires
                        auto win2 = g->in2[i];                    // index for masked value for right input wires
                        auto &m_in1 = preproc_.gates[win1]->mask; // masks for left wires
                        auto &m_in2 = preproc_.gates[win2]->mask; // masks for right wires
                        q_share -= (m_in1 * wires_[win2] + m_in2 * wires_[win1]);
                        q_share.addWithAdder((wires_[win1] * wires_[win2]), id_, rider_id);
                    }
                    if (id_ == rider_id)
                        dotprod_nonTP[driver_id-1].push_back(q_share.valueAt());
                    else if (id_ == driver_id)
                        dotprod_nonTP[rider_id-1].push_back(q_share.valueAt());
                }
                break;
            }
            case ::GateType::kAdd:
            case ::GateType::kSub:
            case ::GateType::kConstAdd:
            case ::GateType::kConstMul:
            {
                break;
            }

            default:
                break;
        }
    }
}

void OnlineEvaluator::evaluateGatesAtDepthPartyRecv(size_t depth, std::vector<std::vector<Field>> mult_all, std::vector<std::vector<Field>> dotprod_all) {
    std::vector<size_t> idx_mult(rider_count+driver_count, 0);
    std::vector<size_t> idx_dotprod(rider_count+driver_count, 0);

    for (auto &gate : circ_.gates_by_level[depth]) {
        switch (gate->type) {

            case GateType::kAdd: {
                auto *g = static_cast<FIn2Gate *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id || id_ == driver_id)
                    wires_[g->out] = wires_[g->in1] + wires_[g->in2];
                break;
            }

            case GateType::kSub: {
                auto *g = static_cast<FIn2Gate *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id || id_ == driver_id)
                    wires_[g->out] = wires_[g->in1] - wires_[g->in2];
                break;
            }

            case GateType::kConstAdd: {
                auto *g = static_cast<ConstOpGate<Field> *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id || id_ == driver_id)
                    wires_[g->out] = wires_[g->in] + g->cval;
                break;
            }

            case GateType::kConstMul: {
                auto *g = static_cast<ConstOpGate<Field> *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id || id_ == driver_id)
                    wires_[g->out] = wires_[g->in] * g->cval;
                break;
            }

            case GateType::kMul: {
                auto *g = static_cast<FIn2Gate *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id) {
                    wires_[g->out] = mult_all[driver_id-1][idx_mult[driver_id-1]];
                    idx_mult[driver_id-1]++;
                }
                else if (id_ == driver_id) {
                    wires_[g->out] = mult_all[rider_id-1][idx_mult[rider_id-1]];
                    idx_mult[rider_id-1]++;
                }
                break;
            }

            case GateType::kDotprod: {
                auto *g = static_cast<SIMDGate *>(gate.get());
                auto rider_id = g->rider_id;
                auto driver_id = g->driver_id;
                if (id_ == rider_id) {
                    wires_[g->out] = dotprod_all[driver_id-1][idx_dotprod[driver_id-1]];
                    idx_dotprod[driver_id-1]++;
                }
                else if (id_ == driver_id) {
                    wires_[g->out] = dotprod_all[rider_id-1][idx_dotprod[rider_id-1]];
                    idx_dotprod[rider_id-1]++;
                }
                break;
            }

            default:
                break;
        }
    }
}

void OnlineEvaluator::evaluateGatesAtDepth(size_t depth) {
    std::vector<size_t> mult_num (rider_count + driver_count, 0);
    std::vector<size_t> dotprod_num (rider_count + driver_count, 0);

    std::vector<std::vector<Field>> mult_nonTP(rider_count + driver_count);
    std::vector<std::vector<Field>> dotprod_nonTP(rider_count + driver_count);

    for (auto &gate : circ_.gates_by_level[depth]) {
        switch (gate->type) {
            case GateType::kInp:
            case GateType::kAdd:
            case GateType::kSub: {
                break;
            }
            case GateType::kMul: {
                if (id_ == gate->rider_id)
                    mult_num[gate->driver_id-1]++;
                else if (id_ == gate->driver_id)
                    mult_num[gate->rider_id-1]++;
                break;
            }

            case GateType::kDotprod: {
                if (id_ == gate->rider_id)
                    dotprod_num[gate->driver_id-1]++;
                else if (id_ == gate->driver_id)
                    dotprod_num[gate->rider_id-1]++;
                break;
            }
        }
    }

    std::vector<std::vector<Field>> mult_all(rider_count + driver_count);
    std::vector<std::vector<Field>> dotprod_all(rider_count + driver_count);
    for (size_t j=0; j<rider_count+driver_count; j++) {
        mult_all[j].resize(mult_num[j]);
        dotprod_all[j].resize(dotprod_num[j]);
    }

    if (id_ != 0) {
        evaluateGatesAtDepthPartySend(depth, mult_nonTP, dotprod_nonTP);

        for (size_t j=0; j<rider_count+driver_count; j++) {
            size_t total_comm = mult_num[j] + dotprod_num[j];
            if (total_comm!=0) {
                std::vector<Field> online_send(total_comm);
                for (size_t i = 0; i < mult_num[j]; i++)
                {
                    online_send[i] = mult_nonTP[j][i];
                }
                for (size_t i = 0; i < dotprod_num[j]; i++)
                {
                    online_send[i + mult_num[j]] = dotprod_nonTP[j][i];
                }
                std::vector<Field> online_recv(total_comm);
                std::vector<Field> agg_values(total_comm);
                network_->send(j+1, online_send.data(), sizeof(Field) * total_comm);
                // network_->flush();
                network_->flush(rider_count, driver_count);
                network_->recv(j+1, online_recv.data(), sizeof(Field) * total_comm);
                for(size_t i = 0; i < total_comm; ++i) {
                    agg_values[i] = online_send[i] + online_recv[i];
                }
                for (size_t i = 0; i < mult_num[j]; i++) {
                    mult_all[j][i] = agg_values[i];
                }
                for (size_t i = 0; i < dotprod_num[j]; i++) {
                    dotprod_all[j][i] = agg_values[mult_num[j] + i];
                }
            }
        }

        evaluateGatesAtDepthPartyRecv(depth, mult_all, dotprod_all);
    }
}

// output reconstruction
std::vector<Field> OnlineEvaluator::getOutputs() {
    std::vector<Field> outvals(circ_.outputs.size());
    if (circ_.outputs.empty()) {
        return outvals;
    }

    for (size_t i = 0; i < circ_.outputs.size(); ++i) {
        auto wout = circ_.outputs[i];
        int rider_id = circ_.output_owners[wout][0];
        if (id_ !=0) {
            Field maksed_val = wires_[wout];
            if (id_==rider_id){
                network_->send(0, &maksed_val, sizeof(Field));
            }                
        }
        else {
            // network_->flush();
            network_->flush(rider_count, driver_count);
            network_->recv(rider_id, &outvals[i], sizeof(Field));
            Field outmask = preproc_.gates[wout]->tpmask.secret();
            outvals[i] -=  outmask;
        }
    }

    return outvals;
}


// run the online phase
std::vector<Field> OnlineEvaluator::evaluateCircuit(const std::unordered_map<wire_t, Field> &inputs) {
    setInputs(inputs);
    for (size_t i = 0; i < circ_.gates_by_level.size(); ++i) {
        evaluateGatesAtDepth(i);
    }
    return getOutputs();
}

};  // namespace quickpool
