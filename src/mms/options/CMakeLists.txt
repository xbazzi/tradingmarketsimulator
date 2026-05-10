project(options CXX)


# Glob all source files in src/
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

add_executable(options ${SRC_FILES} main.cc)

set_target_properties(options PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_include_directories(options PRIVATE 
  ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(options PRIVATE cxx_std_23)

target_compile_options(options PRIVATE 
  -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined,leak
)

target_link_options(options PRIVATE
  -fsanitize=address,undefined,leak
)
