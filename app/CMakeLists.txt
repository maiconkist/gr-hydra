# Include libraries and their implementation
include_directories(${HYDRA_SOURCE_DIR}/hydra/include)
link_directories(${HYDRA_SOURCE_DIR}/lib)

# Add the Server executable
add_executable(server server.cc)
target_link_libraries(server gnuradio-hydra)

# Add the Client executable
add_executable(client client.cc)
target_link_libraries(client gnuradio-hydra)
