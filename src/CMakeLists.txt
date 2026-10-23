
# build physics lib 
add_library(gas_core STATIC
    ${CMAKE_SOURCE_DIR}/src/gas_core/simulator.cpp
    
)
target_include_directories(gas_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)


# build visual lib:
add_library(gas_visual STATIC
    ${CMAKE_SOURCE_DIR}/src/visual/bridge_v2.cpp
    ${CMAKE_SOURCE_DIR}/src/visual/rendering_engine.cpp
    
)

target_include_directories(gas_visual PUBLIC
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(gas_visual  
    glfw
    glad
    glm::glm
    OpenGL::GL
    gas_core
)



# Main executable
add_executable(gas
  main.cpp
  app.cpp
)

# Include paths
# -------------------------
# GLAD
target_include_directories(gas PRIVATE
    ${CMAKE_SOURCE_DIR}/external/glad/include
)


target_include_directories(gas
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    )

# FINAL LINK 
# --------------------------
# Find OpenGL (must come before target_link_libraries)
find_package(OpenGL REQUIRED)

# Link libraries
target_link_libraries(gas PRIVATE
    gas_visual # from my custom visualization
    gas_core   # from my custom core physics
)


# Headless driver (no window, no OpenGL): parameter sweeps and README figures
add_executable(gas_headless
  headless/main.cpp
)
target_include_directories(gas_headless
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(gas_headless PRIVATE gas_core)
