# Test suite for image processing
include(CTest)

# Test executable
add_executable(test_image_processing
    test_image_processing.c
    ../src/sequential_image_processing.c
)

target_include_directories(test_image_processing PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_image_processing m)

# Add individual tests
add_test(NAME test_image_load COMMAND test_image_processing load)
add_test(NAME test_grayscale COMMAND test_image_processing grayscale)
add_test(NAME test_blur COMMAND test_image_processing blur)
add_test(NAME test_sharpen COMMAND test_image_processing sharpen)
add_test(NAME test_edge_detection COMMAND test_image_processing edge)
add_test(NAME test_compression COMMAND test_image_processing compress)

# Performance comparison test
if(BUILD_PARALLEL)
    add_executable(test_performance
        test_performance.c
    )
    
    target_link_libraries(test_performance m OpenMP::OpenMP_C)
    
    add_test(NAME test_parallel_performance 
        COMMAND test_performance
    )
endif()

# Create test data directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)

# Set test properties
set_tests_properties(
    test_image_load
    test_grayscale
    test_blur
    test_sharpen
    test_edge_detection
    test_compression
    PROPERTIES
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)