#include "Intersection_eval_4.h"

using namespace quickpool;

// Define a constant for the security parameter
constexpr int SECURITY_PARAM = 128; // Example value, adjust as needed

int main() {
    // Initialize ZZ_p context
    NTL::ZZ_pContext ZZ_p_ctx;
    ZZ_p_ctx.save();

    // Define the number of riders and drivers
    int rider_count = 3;
    int driver_count = 3;
    int nP = rider_count + driver_count;

    // Allocate routes based on your chosen implementation
    emp::block **all_routes = new emp::block*[nP];  

    // Prepare futures for party processing
    std::vector<std::future<std::vector<std::vector<bool>>>> parties;
    parties.reserve(nP + 1);

    // Launch asynchronous tasks for each party
    for (int i = 0; i <= nP; ++i) {
        std::cout << "Creating async task for party id: " << i << std::endl;
        parties.push_back(std::async(std::launch::async, [&, i]() -> std::vector<std::vector<bool>> {
            // Restore the ZZ_p context for this thread
            ZZ_p_ctx.restore();

            // Initialize the network for this party
            auto network = std::make_shared<io::NetIOMP>(i, rider_count, driver_count, 10000, nullptr, true);
            
            // Create an instance of the Intersection_eval class
            Intersection_eval iter_eval(i, rider_count, driver_count, network, SECURITY_PARAM, nP);

            // Set inputs for non-zero parties
            if (0 < i && i <= rider_count) {
                std::cout << "Setting inputs for rider id: " << i << std::endl;
                // Fetch rider routes
                all_routes[i - 1] = new emp::block[VERTEX_NUM];
                memcpy(all_routes[i - 1], iter_eval.getRiderInputs(i), VERTEX_NUM * sizeof(emp::block));
            } else if (rider_count < i && i <= nP) {
                std::cout << "Setting inputs for driver id: " << i << std::endl;
                // Fetch driver routes
                all_routes[i - 1] = new emp::block[VERTEX_NUM];
                memcpy(all_routes[i - 1], iter_eval.getDriverInputs(i-rider_count), VERTEX_NUM * sizeof(emp::block));
            } else {
                std::cout << "No inputs for SP id: " << i << std::endl;
            }
        
            // Process intersection matching
            auto res = iter_eval.processIntersectionMatching();
            return res; // Return results
        }));
    }

    // Retrieve results from the main party (party 0)
    std::vector<std::vector<bool>> match = parties[0].get();

    // Print the match matrix
    std::cout << "Match matrix:\n";
    for (size_t i = 0; i < match.size(); ++i) {
        for (size_t j = 0; j < match[i].size(); ++j) {
            std::cout << match[i][j] << " ";
        }
        std::cout << "\n";
    }

    // Clean up any dynamically allocated memory if applicable
    for (int i = 0; i < nP; ++i) {
        delete[] all_routes[i];
    }
    delete[] all_routes;

    return 0;
}
