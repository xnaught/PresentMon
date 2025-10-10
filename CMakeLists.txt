cmake_minimum_required(VERSION 3.29)

project(PresentData
    VERSION 1.0.0
    LANGUAGES C CXX
)

# C++ standard for the whole project.
set(CMAKE_CXX_STANDARD 23)

## Trying to get a .lib file built w/o having to a declspec() to symbols...
#set_target_properties(presentdata PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(presentdata SHARED
    PresentData/ETW/Intel_PresentMon.h
    PresentData/ETW/Microsoft_Windows_D3D9.h
    PresentData/ETW/Microsoft_Windows_Dwm_Core.h
    PresentData/ETW/Microsoft_Windows_Dwm_Core_Win7.h
    PresentData/ETW/Microsoft_Windows_DXGI.h
    PresentData/ETW/Microsoft_Windows_DxgKrnl.h
    PresentData/ETW/Microsoft_Windows_DxgKrnl_Win7.h
    PresentData/ETW/Microsoft_Windows_EventMetadata.h
    PresentData/ETW/Microsoft_Windows_Kernel_Process.h
    PresentData/ETW/Microsoft_Windows_Win32k.h
    PresentData/ETW/NT_Process.h
    PresentData/ETW/NV_DD.h
    PresentData/ETW/Nvidia_PCL.h

    PresentData/Debug.hpp
    PresentData/Debug.cpp
    PresentData/GpuTrace.hpp
    PresentData/GpuTrace.cpp
    PresentData/NvidiaTraceConsumer.hpp
    PresentData/NvidiaTraceConsumer.cpp
    PresentData/PresentMonTraceConsumer.hpp
    PresentData/PresentMonTraceConsumer.cpp
    PresentData/PresentMonTraceSession.hpp
    PresentData/PresentMonTraceSession.cpp
    PresentData/shims.h
    PresentData/shims.cpp
    PresentData/TraceConsumer.hpp
    PresentData/TraceConsumer.cpp
    PresentData/TraceLogging.h
    PresentData/TraceLogging.cpp
)

#target_include_directories(presentdata PUBLIC PresentData)
target_include_directories(presentdata PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/PresentData>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(presentdata PRIVATE _WIN32_WINNT=0x0601 NTDDI_VERSION=0x06010000 WIN32_LEAN_AND_MEAN)
target_compile_definitions(presentdata PRIVATE UNICODE)

# Install the library itself
install(TARGETS presentdata
    EXPORT presentdataTargets
    RUNTIME DESTINATION bin       # for DLLs (Windows)
    LIBRARY DESTINATION lib       # for shared libs (.so, .dylib)
    ARCHIVE DESTINATION lib       # for import/static libs
)

# Install public headers
install(
    DIRECTORY PresentData/
    DESTINATION include/PresentData
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# Install the CMake package config files (so consumers can use find_package)
include(CMakePackageConfigHelpers)

# Generate a config file
configure_package_config_file(
    presentdataConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/presentdataConfig.cmake
    INSTALL_DESTINATION lib/cmake/presentdata
)

# Generate a version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/presentdataConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)

# Install the config and version file
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/presentdataConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/presentdataConfigVersion.cmake
    DESTINATION lib/cmake/presentdata
)

# Export the targets
install(EXPORT presentdataTargets
    FILE presentdataTargets.cmake
    NAMESPACE presentdata::
    DESTINATION lib/cmake/presentdata
)
