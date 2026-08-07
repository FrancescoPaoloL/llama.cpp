cmake_minimum_required(VERSION 3.10)
project(owasp-llm-tool CXX)


include_directories(${CMAKE_SOURCE_DIR}/external)

add_executable(owasp-llm-tool
    main.cpp
    category.cpp
    json_utils.cpp
    llama_runner.cpp
    perplexity_utils.cpp
    llm09_misinformation.cpp
)

target_link_libraries(owasp-llm-tool
    PRIVATE
        llama
        llama-common
)

target_compile_features(owasp-llm-tool PRIVATE cxx_std_11)

install(TARGETS owasp-llm-tool RUNTIME DESTINATION bin)

