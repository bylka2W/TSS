cmake_minimum_required(VERSION 3.16)
project(TSSGenerator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /Ot /GL /arch:AVX2 /fp:fast")
set(CMAKE_C_FLAGS_RELEASE "/O2")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tss_api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tss_backend/dx12)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tss_fg/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tss_fg/internal)

add_library(TSSPerfectInterpolation src/TSSPerfectInterpolation.c)
add_library(TSSTransform3D src/TSSTransform3D.c)
add_library(TSSCollision3D src/TSSCollision3D.c)
add_library(TSSSOAEntities src/TSSSOAEntities.c)
add_library(TSSSIMDInterpolation src/TSSSIMDInterpolation.c)
add_library(TSSInputLatency src/TSSInputLatency.c)
add_library(TSSLowLatency src/TSSLowLatency.c)
add_library(TSSMotionVectors src/TSSMotionVectors.c)
add_library(TSSAntiAliasing src/TSSAntiAliasing.c)
add_library(TSSMetrics src/TSSMetrics.c)
add_library(TSSTemporalReconstruct src/TSSTemporalReconstruct.c)
add_library(TSSRCASDX12 src/TSSRCASDX12.c)
add_library(TSSVarianceClipping src/TSSVarianceClipping.c)
add_library(TSSJitter src/TSSJitter.c)
add_library(TSSSmartTSS src/TSSSmartTSS.c)
add_library(TSSGLMWrapper src/TSSGLMWrapper.c)

add_library(TSS SHARED src/tss.c)
target_compile_definitions(TSS PRIVATE TSS_EXPORTS)
set_target_properties(TSS PROPERTIES
    OUTPUT_NAME "TSS"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(TSSSmartDLL SHARED src/TSSSmartDLL.c)
target_compile_definitions(TSSSmartDLL PRIVATE TSS_EXPORTS)
target_link_libraries(TSSSmartDLL TSSSmartTSS TSSGLMWrapper)
set_target_properties(TSSSmartDLL PROPERTIES
    OUTPUT_NAME "TSSSmart"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(TSSHybridDLL SHARED src/TSSHybridWrapper.c)
target_compile_definitions(TSSHybridDLL PRIVATE TSS_EXPORTS)
target_link_libraries(TSSHybridDLL TSSSmartTSS)
set_target_properties(TSSHybridDLL PROPERTIES
    OUTPUT_NAME "TSSHybrid"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_executable(demo_perfect WIN32 src/main_perfect_fg.cpp)
add_executable(demo_velocity WIN32 src/main_velocity_fg.cpp)
add_executable(demo_3d WIN32 src/main_3d_fg.cpp)
add_executable(demo_latency WIN32 src/main_latency_fg.cpp)
add_executable(demo_metrics WIN32 src/main_metrics_fg.cpp)

target_link_libraries(demo_perfect TSSPerfectInterpolation)
target_link_libraries(demo_velocity TSSPerfectInterpolation)
target_link_libraries(demo_3d TSSTransform3D)
target_link_libraries(demo_latency TSSInputLatency TSSLowLatency TSSTransform3D)
target_link_libraries(demo_metrics TSSMetrics TSSAntiAliasing TSSTransform3D)
