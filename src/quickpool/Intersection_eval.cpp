#include "Intersection_eval.h"

namespace quickpool {

Intersection_eval::Intersection_eval(int id, int rider_count, int driver_count, std::shared_ptr<io::NetIOMP> network, int security_param, int threads, int seed) : 
    id_(id), 
    rider_count_(rider_count),
    driver_count_(driver_count),
    network_(network),
    security_param_(security_param),
    rgen_(id, seed)
    {tpool_ = std::make_shared<ThreadPool>(threads);}

void Intersection_eval::setInputs() {
    if (id_!=0) {
        // sample the route vertices
        // int max_val = INT32_MAX;
        int max_val = 5000;
        int count = VERTEX_NUM; // number of route vertices

        std::vector<uint32_t> sample_space(max_val);
        std::iota(sample_space.begin(), sample_space.end(), 1);
        std::random_shuffle(sample_space.begin(),sample_space.end());

        for (size_t i=0; i<count; i++) {
            uint64_t val = uint64_t(sample_space[i])<<32 ^ sample_space[i+1];
            routes.push_back(val);
        }

        // std::cout << "ID: " << id_ << std::endl;
        // for (auto route : routes) {
        //     std::cout << route << " " ;
        // }
        // std::cout << "\n==========================" << std::endl;
    }    
}

std::vector<uint64_t> Intersection_eval::getInputs() {
    return routes;
}

std::vector<bool> Intersection_eval::processIntersectionMatching(){
    std::vector<bool> output;
    if (id_!=0) { // I am not SP
        // random key sampling
        uint64_t key_own;
        rgen_.self().random_data(&key_own, sizeof(uint64_t));
        std::vector<uint64_t> keys;
        // exchange keys and prepare the keys to be used in PRF evaluation
        if (id_<=rider_count_) { // I am a rider
            for (size_t i=rider_count_+1; i<=rider_count_+driver_count_; i++) {
                // send the sampled key to each driver
                network_->send(i, &key_own, sizeof(uint64_t));
                // receive key from each driver
                uint64_t key_other;
                network_->recv(i, &key_other, sizeof(uint64_t));
                keys.push_back(key_own^key_other);
            }
        }
        else { // I am a driver
            for (size_t i=1; i<=rider_count_; i++) {
                // send the sampled key to each rider
                network_->send(i, &key_own, sizeof(uint64_t));
                // receive key from each rider
                uint64_t key_other;
                network_->recv(i, &key_other, sizeof(uint64_t));
                keys.push_back(key_own^key_other);
            }
        }
        
        // PRF evaluation of route vertices
        if (id_<=rider_count_) { // I am a rider
            // run PRF on routes vertices for each driver
            emp::block prf_result[driver_count_*VERTEX_NUM];
            for (size_t i=0, k=0; i<driver_count_; i++) {
                // form the input message of the following form: key||vertex, total 128 bits
                uint8_t in[16] = {}, out[16] = {}; // G_tiny, AES implementation from funshade supports only these formats for input and output
                memcpy(in, &keys[i], sizeof(uint64_t));
                // for each of the vertices, repeat
                for (size_t j=0; j<VERTEX_NUM; j++) {
                    memcpy(in+8, &routes[j], sizeof(uint64_t));
                    // run aes on in
                    G_tiny(in, out, sizeof(in), sizeof(out));
                    // store it in a buffer to send it to SP
                    uint64_t out1 = *((uint64_t *)out);
                    uint64_t out2 = *((uint64_t *)(out+8));
                    prf_result[k++] = emp::makeBlock(out1,out2);
                }                
            }
            // send prf results to SP
            network_->send(0, prf_result, sizeof(prf_result));
        }
        else { // I am a driver
            // run PRF on routes vertices for each rider
            emp::block prf_result[rider_count_*VERTEX_NUM];
            for (size_t i=0, k=0; i<rider_count_; i++) {
                // form the input message of the following form: key||vertex, total 128 bits
                uint8_t in[16] = {}, out[16] = {}; // G_tiny, AES implementation from funshade supports only these formats for input and output
                memcpy(in, &keys[i], sizeof(uint64_t));
                // for each of the vertices, repeat
                for (size_t j=0; j<VERTEX_NUM; j++) {
                    memcpy(in+8, &routes[j], sizeof(uint64_t));
                    // run aes on in
                    G_tiny(in, out, sizeof(in), sizeof(out));
                    // store it in a buffer to send it to SP
                    uint64_t out1 = *((uint64_t *)out);
                    uint64_t out2 = *((uint64_t *)(out+8));
                    prf_result[k++] = emp::makeBlock(out1,out2);
                }
            }
            // send prf results to SP
            network_->send(0, prf_result, sizeof(prf_result));
        }
    }
    else { // I am SP
        std::vector<std::vector<bool>> match(rider_count_, std::vector<bool>(driver_count_));
        emp::block *prf_results_riders = new emp::block[rider_count_*driver_count_*VERTEX_NUM];
        emp::block *prf_results_drivers = new emp::block[rider_count_*driver_count_*VERTEX_NUM];
        for (size_t i=1; i<=rider_count_; i++) { // receive from riders
            network_->recv(i, prf_results_riders+(i-1)*driver_count_*VERTEX_NUM, driver_count_*VERTEX_NUM*sizeof(emp::block));
        }
        for (size_t i=1; i<=driver_count_; i++) { // receive from drivers
            network_->recv(i+rider_count_, prf_results_drivers+(i-1)*rider_count_*VERTEX_NUM, rider_count_*VERTEX_NUM*sizeof(emp::block));
        }
        // for each pair of rider and driver check if the match_count is greater than threshold or not
        for (size_t i=0; i<rider_count_; i++) {
            for (size_t j=0; j<driver_count_; j++) {
                // count number of common values
                int match_count=0;
                size_t r_index = i*driver_count_*VERTEX_NUM+j*VERTEX_NUM;
                size_t d_index = j*rider_count_*VERTEX_NUM+i*VERTEX_NUM;
                for (size_t k=0; k<VERTEX_NUM; k++) {
                    emp::block r_elem = prf_results_riders[r_index+k];
                    for (size_t l=0; l<VERTEX_NUM; l++) {
                        emp::block d_elem = prf_results_drivers[d_index+l];
                        if (emp::cmpBlock(&r_elem, &d_elem, 1))
                            match_count++; 
                    }
                }
                // check if the count is higher than threshold or not
                if (match_count>=DRIVER_RIDER_MATCH_THRESHOLD) {
                    output.push_back(1);
                    match[i][j] = true;
                }
                else {
                    output.push_back(0);
                    match[i][j] = false;
                }
            }
        }
        int maxBPM_res = maxBPM(match); // just for benchmarking
        delete[] prf_results_riders;
        delete[] prf_results_drivers;    
    }
    
    return output;
}

};