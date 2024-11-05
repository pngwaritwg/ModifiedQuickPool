#include "Intersection_eval_4.h"

// #include <mongocxx/client.hpp>
// #include <mongocxx/instance.hpp>
// #include <mongocxx/uri.hpp>
// #include <bsoncxx/builder/stream/document.hpp> // For building BSON documents
// #include <bsoncxx/json.hpp> // For converting to/from JSON (optional)
// #include <bsoncxx/builder/stream/array.hpp> // For building BSON arrays (optional)
#include <iostream>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/options/find.hpp>
#include <bsoncxx/types.hpp>

namespace quickpool {

void printEmpBlock(const emp::block& block) {
    const uint64_t* data = reinterpret_cast<const uint64_t*>(&block);
    std::cout << "emp::block: high = 0x" << std::hex << std::setw(16) << std::setfill('0') << data[1]
              << ", low = 0x" << std::setw(16) << std::setfill('0') << data[0] << std::dec << std::endl;
}

Intersection_eval::Intersection_eval(int id, int rider_count, int driver_count, std::shared_ptr<io::NetIOMP> network, int security_param, int threads, int seed) : 
    id_(id), 
    rider_count_(rider_count),
    driver_count_(driver_count),
    network_(network),
    security_param_(security_param),
    rgen_(id, seed)
    {tpool_ = std::make_shared<ThreadPool>(threads);}

// Function to convert bsoncxx type to string for better logging
std::string bson_type_to_string(bsoncxx::type type) {
    switch (type) {
        case bsoncxx::type::k_int64: return "int64";
        case bsoncxx::type::k_int32: return "int32";
        case bsoncxx::type::k_double: return "double";
        case bsoncxx::type::k_utf8: return "string";
        case bsoncxx::type::k_bool: return "bool";
        // Add other types as needed
        default: return "unknown";
    }
}

// Utility function to fetch routes based on the given ID and role (rider or driver)
emp::block* Intersection_eval::fetchRoutesFromDB(const std::string& role, int id_) {
    mongocxx::uri uri{"mongodb://localhost:27017/?maxPoolSize=10"};
    static mongocxx::instance instance{}; // MongoDB instance, initialized only once
    mongocxx::client client{uri};
    
    auto db = client["rider_driver_pooling"];
    auto collections = db.list_collection_names();

    // std::cout << "Collections in the database:" << std::endl;
    // for (const auto& collection_name : collections) {
    //     std::cout << " - " << collection_name << std::endl;
    // }

    auto collection = db["route_triples"];

    // Ping to check the connection to MongoDB
    bsoncxx::builder::stream::document ping_cmd;
    ping_cmd << "ping" << 1;
    
    auto result = db.run_command(ping_cmd.view());
    std::cout << "Ping successful: " << bsoncxx::to_json(result) << " for role " << role << " id " << id_ << std::endl;

    // Create filter for MongoDB query based on role
    auto filter_builder = bsoncxx::builder::stream::document{};
    bsoncxx::document::value filter = filter_builder << "user_type" << role << bsoncxx::builder::stream::finalize;
    std::cout << "Created filter for user_type: " << role << " id " << id_ << std::endl;
    // std::cout << "Filter: " << bsoncxx::to_json(filter) << std::endl;

    // Sort by 'created_at' in ascending order
    mongocxx::options::find options;
    options.sort(bsoncxx::builder::stream::document{} << "created_at" << 1 << bsoncxx::builder::stream::finalize);

    // Execute the query without a limit to get all sorted documents
    auto cursor = collection.find(filter.view(), options);

    // Check if the cursor is empty
    if (cursor.begin() == cursor.end()) {
        std::cerr << "No documents found for role: " << role << " id " << id_ << std::endl;
        return nullptr;
    }

    // Convert cursor to a vector to allow indexed access
    std::vector<bsoncxx::document::value> documents;
    for (auto&& doc : cursor) {
        documents.push_back(bsoncxx::document::value{doc}); // Convert view to value
    }

    // // Convert cursor to a vector to allow indexed access
    // std::vector<bsoncxx::document::value> documents;
    // for (auto&& doc : cursor) {
    //     documents.push_back(doc);
    // }

    // Check if id_ is within the bounds of the document list
    if (documents.size() < id_) {
        std::cerr << "Not enough documents for role: " << role << ", requested id: " << id_ 
                << " but only " << documents.size() << " documents available." << std::endl;
        return nullptr;
    }

    // Access the (id_ - 1)th document after sorting
    auto selected_document = documents[id_ - 1];
     
    // // Sort by 'created_at' in ascending order and limit to the desired entry
    // mongocxx::options::find options;
    // options.sort(bsoncxx::builder::stream::document{} << "created_at" << 1 << bsoncxx::builder::stream::finalize);
    // options.limit(id_); // Fetch up to 'id_' documents to ensure we can skip to the (id_-1)th

    // // Execute the query
    // auto cursor = collection.find(filter.view(), options);
    // if (cursor.begin() == cursor.end()) {
    //     std::cerr << "No documents found for role: " << role << " id " << id_ << std::endl;
    //     return nullptr;
    // }

    // // Check if the cursor is empty before advancing
    // auto it = cursor.begin();
    // if (it == cursor.end()) {
    //     std::cerr << "No data found for " << role << " with index: " << id_ << std::endl;
    //     return nullptr;
    // }

    // // Advance to the (id_ - 1)th document (0-based index)
    // std::advance(it, id_ - 1);
    // if (it == cursor.end()) {
    //     std::cerr << "Index out of range for " << role << " with index: " << id_ << std::endl;
    //     return nullptr;
    // }

    // // Process the document as needed
    // bsoncxx::document::view doc = *it;
    // std::cout << "Found document: " << bsoncxx::to_json(doc) << std::endl;
    
    // // Allocate memory for emp::block array
    // emp::block* routes = new emp::block[VERTEX_NUM]; 

    // Convert MongoDB route data to emp::block array format
    auto pre_routes = selected_document.view()["triples"].get_array().value;

    size_t length = std::distance(pre_routes.begin(), pre_routes.end());

    std::cout << "The length of the routes array is: " << length << " for " << role << " with id: " << id_ << std::endl;
    // auto routes = (*it)["triples"].get_array().value;

    // for (size_t i = 0; i < routes.size() && i < VERTEX_NUM; ++i) {
    //     auto triple = routes[i].get_array().value;
    //     uint64_t v_i = triple[0].get_int64();
    //     uint64_t v_j = triple[1].get_int64();
    //     uint64_t t_k = triple[2].get_int64();

    //     // Ensure values fit within 42 bits
    //     if ((v_i >= (1ULL << 42)) || (v_j >= (1ULL << 42)) || (t_k >= (1ULL << 42))) {
    //         std::cerr << "Values exceed 42-bit limit." << std::endl;
    //         delete[] route_array; // Ensure we free allocated memory
    //         return nullptr;
    //     }

    //     // Pack the three 42-bit values into two 64-bit integers (using two uint64_t values for high and low)
    //     uint64_t high = (v_i << 22) | (v_j >> 20);     // Top 64 bits
    //     uint64_t low = ((v_j & 0xFFFFF) << 42) | t_k;  // Bottom 64 bits
    //     route_array[i] = emp::makeBlock(high, low);
    // }

    // Iterate over the elements of the BSON array
    std::cout << "Start converting to empblock in role: " << role << " id: " << id_ << std::endl;
    size_t i = 0;
    for (const auto& elem : pre_routes) {
        if (i >= VERTEX_NUM) break;

        // std::cout << "Mid converting to empblock, before elem.get_array().value: " << role << " id: " << id_ << std::endl;
        auto triple = elem.get_array().value;

       // Using an iterator to check the number of elements in the array
        size_t triple_count = 0;
        for (const auto& t : triple) {
            ++triple_count;
        }

        // Check if the number of elements is less than 3
        if (triple_count < 3) {
            std::cerr << "Insufficient elements in triple. Expected 3, found: " << triple_count << std::endl;
            continue; // Skip to the next element
        }

        // Retrieve values with checks
        auto v_i_elem = triple[0];
        auto v_j_elem = triple[1];
        auto t_k_elem = triple[2];

        // Check types and get values
        // std::cout << "Mid converting to empblock, v_i type: " << bson_type_to_string(v_i_elem.type()) << " in role " << role << " id: " << id_ << std::endl;
        // std::cout << "Mid converting to empblock, v_i type: " << bson_type_to_string(v_j_elem.type()) << " in role " << role << " id: " << id_ << std::endl;
        // std::cout << "Mid converting to empblock, v_i type: " << bson_type_to_string(t_k_elem.type()) << " in role " << role << " id: " << id_ << std::endl;

        // std::cout << "Mid converting to empblock, before uint64_t v_i = triple[0].get_int64(); " << role << " id: " << id_ << std::endl;
        // uint64_t v_i = triple[0].get_int64();
        // std::cout << "Mid converting to empblock, before uint64_t v_j = triple[1].get_int64(); " << role << " id: " << id_ << std::endl;
        // uint64_t v_j = triple[1].get_int64();
        // std::cout << "Mid converting to empblock, before uint64_t t_k = triple[2].get_int64(); " << role << " id: " << id_ << std::endl;
        // uint64_t t_k = triple[2].get_int64();

        int64_t v_i, v_j, t_k;

        // std::cout << "Mid converting to empblock, before uint64_t v_i " << role << " id: " << id_ << std::endl;
        if (v_i_elem.type() == bsoncxx::type::k_int32) {
            v_i = static_cast<uint64_t>(v_i_elem.get_int32().value);
        } else if (v_i_elem.type() == bsoncxx::type::k_int64) {
            v_i = v_i_elem.get_int64().value;
        } else {
            throw std::runtime_error("Unexpected BSON type for triple[0]");
        }
        // std::cout << "Mid converting to empblock, before uint64_t v_j " << role << " id: " << id_ << std::endl;
        if (v_j_elem.type() == bsoncxx::type::k_int32) {
            v_j = static_cast<uint64_t>(v_j_elem.get_int32().value);
        } else if (v_j_elem.type() == bsoncxx::type::k_int64) {
            v_j = v_j_elem.get_int64().value;
        } else {
            throw std::runtime_error("Unexpected BSON type for triple[1]");
        }
        // std::cout << "Mid converting to empblock, before uint64_t t_k " << role << " id: " << id_ << std::endl;
        if (t_k_elem.type() == bsoncxx::type::k_int32) {
            t_k = static_cast<uint64_t>(t_k_elem.get_int32().value);
        } else if (t_k_elem.type() == bsoncxx::type::k_int64) {
            t_k = t_k_elem.get_int64().value;
        } else {
            throw std::runtime_error("Unexpected BSON type for triple[2]");
        }

        // Ensure values fit within 42 bits
        if ((v_i >= (1ULL << 42)) || (v_j >= (1ULL << 42)) || (t_k >= (1ULL << 42))) {
            std::cerr << "Values exceed 42-bit limit." << std::endl;
            // delete[] route_array; // Ensure we free allocated memory
            // return nullptr;
        }

        // std::cout << "Mid converting to empblock, before  uint64_t high = (v_i << 22) | (v_j >> 20); " << role << " id: " << id_ << std::endl;
        // Pack the three 42-bit values into two 64-bit integers (using two uint64_t values for high and low)
        uint64_t high = (v_i << 22) | (v_j >> 20);     // Top 64 bits
        // std::cout << "Mid converting to empblock, before  int64_t low = ((v_j & 0xFFFFF) << 42) | t_k; " << role << " id: " << id_ << std::endl;
        uint64_t low = ((v_j & 0xFFFFF) << 42) | t_k;  // Bottom 64 bits
        // std::cout << "Mid converting to empblock, before  route_array[i] = emp::makeBlock(high, low); " << role << " id: " << id_ << std::endl;
        // routes[i] = emp::makeBlock(v_i,0) ^ emp::makeBlock(0,v_j);
        routes[i] = emp::makeBlock(high, low);

        i++;
    }
    // for (size_t i = 0; i < VERTEX_NUM; ++i) {
    //     std::cout << "Route [" << i << "]: ";
    //     printEmpBlock(routes[i]); // Print each route using the helper function
    // }
    std::cout << "Finish converting to empblock in role: " << role << " id: " << id_ << std::endl;

    return routes;
}

emp::block* Intersection_eval::getRiderInputs(int rider_id) {
    // Fetch routes for the rider using the rider_id identifier
    return fetchRoutesFromDB("rider", rider_id);
}

emp::block* Intersection_eval::getDriverInputs(int driver_id) {
    // Fetch routes for the driver using the driver_id identifier
    return fetchRoutesFromDB("driver", driver_id);
}

std::vector<std::vector<bool>> Intersection_eval::processIntersectionMatching(){
    std::vector<bool> output;
    std::vector<std::vector<bool>> match(rider_count_, std::vector<bool>(driver_count_));
    if (id_!=0) { // I am not SP
        // random key sampling
        emp::block key_own;
        rgen_.self().random_data(&key_own, sizeof(emp::block));
        emp::block *keys;
        // exchange keys and prepare the keys to be used in PRF evaluation
        if (id_<=rider_count_) { // I am a rider
            std::cout << "start key exchange for rider id: " << id_ << std::endl;
            keys = new emp::block[driver_count_];
            for (size_t i=rider_count_+1; i<=rider_count_+driver_count_; i++) {
                // send the sampled key to each driver
                network_->send(i, &key_own, sizeof(emp::block));
                // receive key from each driver
                emp::block key_other;
                network_->recv(i, &key_other, sizeof(emp::block));
                keys[i-rider_count_-1] = _mm_xor_si128(key_own,key_other);
            }
            std::cout << "finish key exchange for rider id: " << id_ << std::endl;
        }
        else { // I am a driver
            std::cout << "start key exchange for driver id: " << id_ << std::endl;
            keys = new emp::block[rider_count_];
            for (size_t i=1; i<=rider_count_; i++) {
                // send the sampled key to each rider
                network_->send(i, &key_own, sizeof(emp::block));
                // receive key from each rider
                emp::block key_other;
                network_->recv(i, &key_other, sizeof(emp::block));
                keys[i-1] = _mm_xor_si128(key_own,key_other);
            }
            std::cout << "finish key exchange for driver id: " << id_ << std::endl;
        }
        
        // PRF evaluation of route vertices
        if (id_<=rider_count_) { // I am a rider
            // run PRF on routes vertices for each driver
            // for (size_t i = 0; i < VERTEX_NUM; ++i) {
            //     std::cout << "og route vertex" << i <<  " role: rider" << " id: " << id_ << " ";
            //     printEmpBlock(routes[i]); // Print each route using the helper function
            // }
            emp::block prf_result[driver_count_*VERTEX_NUM];
            for (size_t i=0; i<driver_count_; i++) {
                emp::PRP aes(keys[i]);
                memcpy(prf_result+(i*VERTEX_NUM), routes, sizeof(routes));
                aes.permute_block(prf_result+(i*VERTEX_NUM), VERTEX_NUM);
            }
            // send prf results to SP
            network_->send(0, prf_result, sizeof(prf_result));
            // for (size_t i = 0; i < driver_count_*VERTEX_NUM ; ++i) {\
            //     int prf_route = i / driver_count_;
            //     int prf_driver= i % driver_count_ + 1;
            //     std::cout << "prf_result for route vertex" << prf_route <<  " role: rider" << " id: " << id_ << "for driver " << prf_driver << " ";
            //     printEmpBlock(prf_result[i]); // Print each route using the helper function
            //     std::cout << " " << std::endl;
            // }
        }
        else { // I am a driver
            // run PRF on routes vertices for each rider
            // for (size_t i = 0; i < VERTEX_NUM; ++i) {
            //     std::cout << "og route vertex" << i <<  " role: driver" << " id: " << id_ << " ";
            //     printEmpBlock(routes[i]); // Print each route using the helper function
            // }
            emp::block prf_result[rider_count_*VERTEX_NUM];
            for (size_t i=0, k=0; i<rider_count_; i++) {
                emp::PRP aes(keys[i]);
                memcpy(prf_result+(i*VERTEX_NUM), routes, sizeof(routes));
                aes.permute_block(prf_result+(i*VERTEX_NUM), VERTEX_NUM);
            }
            // send prf results to SP
            network_->send(0, prf_result, sizeof(prf_result));
            // for (size_t i = 0; i < rider_count_*VERTEX_NUM ; ++i) {
            //     int prf_route = i / rider_count_;
            //     int prf_rider = i % rider_count_ + 1;
            //     std::cout << "prf_result for route vertex" << prf_route <<  " role: driver" << " id: " << id_ - driver_count_ << "for rider " << prf_rider << " ";
            //     printEmpBlock(prf_result[i]); // Print each route using the helper function
            //     std::cout << " " << std::endl;
            // }
        }
        delete[] keys;
    }
    else { // I am SP
        std::cout << "start intersection for SP id: " << id_ << std::endl;
        // std::vector<std::vector<bool>> match(rider_count_, std::vector<bool>(driver_count_));
        emp::block *prf_results_riders = new emp::block[rider_count_*driver_count_*VERTEX_NUM];
        emp::block *prf_results_drivers = new emp::block[rider_count_*driver_count_*VERTEX_NUM];
        for (size_t i=1; i<=rider_count_; i++) { // receive from riders
            network_->recv(i, prf_results_riders+(i-1)*driver_count_*VERTEX_NUM, driver_count_*VERTEX_NUM*sizeof(emp::block));
        }
        for (size_t i=1; i<=driver_count_; i++) { // receive from drivers
            network_->recv(i+rider_count_, prf_results_drivers+(i-1)*rider_count_*VERTEX_NUM, rider_count_*VERTEX_NUM*sizeof(emp::block));
        }
        std::cout << "mid intersection for SP id: " << id_ << " recieve all data for intersection" << std::endl;
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
                std::cout << "match count for rider id: " << i << " and driver id " << j << ": " << match_count << std::endl;
                // check if the count is higher than threshold or not
                if (match_count>DRIVER_RIDER_MATCH_THRESHOLD) {
                    output.push_back(1);
                    match[i][j] = true;
                }
                else {
                    output.push_back(0);
                    match[i][j] = false;
                }
            }
        }
        std::cout << "mid intersection for SP id: " << id_ << " get all possible matches" << std::endl;
        delete[] prf_results_riders;
        delete[] prf_results_drivers;
        std::cout << "finish intersection for SP id: " << id_ << std::endl; 
    }
    
    return match;
}
}