PROJECT(rvv_bench_${RVV_BENCH_BENCHMARK} C CXX ASM)

COMMON_ADD_LIBRARY(${PROJECT_NAME} STATIC bench.c ${RVV_BENCH_BENCHMARK}.c template.S)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC . ../common)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE support)
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE -DINC=${RVV_BENCH_BENCHMARK}.S)
# SET_SOURCE_FILES_PROPERTIES(template.S PROPERTIES COMPILE_DEFINITIONS INC=${RVV_BENCH_BENCHMARK}.S)
# TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE INC="\"${RVV_BENCH_BENCHMARK}.S\"")
# TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE INC="\\\"${RVV_BENCH_BENCHMARK}.S\\\"")
# TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE INC=${RVV_BENCH_BENCHMARK}.S)

IF(${GLOBAL_ISEL})
target_compile_options(${PROJECT_NAME} PRIVATE "SHELL:$<$<COMPILE_LANGUAGE:C>:-mllvm -global-isel=1>")
target_compile_options(${PROJECT_NAME} PRIVATE "SHELL:$<$<COMPILE_LANGUAGE:CXX>:-mllvm -global-isel=1>")
SET(GLOBAL_ISEL_ABORT 2)
target_compile_options(${PROJECT_NAME} PRIVATE "SHELL:$<$<COMPILE_LANGUAGE:C>:-mllvm -global-isel-abort=${GLOBAL_ISEL_ABORT}>")
target_compile_options(${PROJECT_NAME} PRIVATE "SHELL:$<$<COMPILE_LANGUAGE:CXX>:-mllvm -global-isel-abort=${GLOBAL_ISEL_ABORT}>")
ENDIF()
