# Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(ACCEL_FILES "avx2.cpp" "avx512.cpp")
else()
  unset(ACCEL_FILES)
endif()

vespa_add_library(vespa_hwaccelerated
    SOURCES
    iaccelerated.cpp
    generic.cpp
    ${ACCEL_FILES}
    DEPENDS
    INSTALL lib64
)
set_source_files_properties(avx2.cpp PROPERTIES COMPILE_FLAGS "-O3 -march=haswell")
set_source_files_properties(avx512.cpp PROPERTIES COMPILE_FLAGS "-O3 -march=skylake-avx512 -mprefer-vector-width=512")
set(BLA_VENDOR OpenBLAS)
vespa_add_target_package_dependency(vespa_hwaccelerated BLAS)
