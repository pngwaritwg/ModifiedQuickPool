option(Inter_v1 "Use Intersection_v1" OFF)
option(Inter_v2 "Use Intersection_v2" OFF)
option(Inter_v3 "Use Intersection_v3" OFF)

add_library(Quickpool
            utils/circuit.cpp 
            utils/types.cpp
            utils/helpers.cpp
            io/netmp.cpp
            funshade/aes.cpp
            funshade/fss.cpp
            quickpool/rand_gen_pool.cpp
            quickpool/ED_offline_eval.cpp
            quickpool/ED_online_eval.cpp
            quickpool/ED_eval.cpp
            )
            
if (Inter_v1) # This is when the tiny AES (G_tiny) from funshade is being used
    target_compile_definitions(Quickpool PUBLIC "Inter_v1")
    target_sources(Quickpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/quickpool/Intersection_eval.cpp)
elseif (Inter_v2) # This is when the full AES from funshade is being used
    target_compile_definitions(Quickpool PUBLIC "Inter_v2")
    target_sources(Quickpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/quickpool/Intersection_eval_2.cpp)
else() # This is when the full AES from emptool is being used
    target_compile_definitions(Quickpool PUBLIC "Inter_v3")
    target_sources(Quickpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/quickpool/Intersection_eval_3.cpp)
endif()

target_include_directories(Quickpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/io ${CMAKE_CURRENT_SOURCE_DIR}/utils ${CMAKE_CURRENT_SOURCE_DIR}/funshade ${CMAKE_CURRENT_SOURCE_DIR}/quickpool)
target_link_libraries(Quickpool PUBLIC Boost::system EMPTool NTL GMP)