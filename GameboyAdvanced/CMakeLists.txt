
cmake_minimum_required(VERSION 3.10)
project(GBA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GBA_CORE_SOURCES
    source/Bus.cpp

    # tdmi7 CPU subsystem
    source/tdmi7/Core.cpp
    source/tdmi7/CpuState.cpp
    source/tdmi7/Addressing.cpp
    source/tdmi7/CpuMemory.cpp
    source/tdmi7/DecoderCore.cpp
    source/tdmi7/LegacyInstructionTests.cpp
    source/tdmi7/arm/ArmDataProcessing.cpp
    source/tdmi7/arm/ArmControl.cpp
    source/tdmi7/arm/ArmMultiply.cpp
    source/tdmi7/arm/ArmMemory.cpp
    source/tdmi7/arm/ArmDecoder.cpp
    source/tdmi7/arm/ArmDisassembly.cpp
    source/tdmi7/thumb/ThumbCore.cpp
    source/tdmi7/thumb/ThumbAlu.cpp
    source/tdmi7/thumb/ThumbControl.cpp
    source/tdmi7/thumb/ThumbMemory.cpp
    source/tdmi7/thumb/ThumbDecoder.cpp
    source/tdmi7/thumb/ThumbDisassembly.cpp
    source/PPU.cpp
)

add_library(gba_core STATIC ${GBA_CORE_SOURCES})
target_include_directories(gba_core PUBLIC include/)

add_library(gba_legacy_diagnostics STATIC tests/legacy/ThumbInstructionTests.cpp)
target_link_libraries(gba_legacy_diagnostics PRIVATE gba_core)

add_executable(GBA source/GBA.cpp source/Main.cpp)
target_link_libraries(GBA PRIVATE gba_core gba_legacy_diagnostics)
set_target_properties(GBA PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gba_core PRIVATE -Wall -Wextra)
    target_compile_options(gba_legacy_diagnostics PRIVATE -Wall -Wextra)
    target_compile_options(GBA PRIVATE -Wall -Wextra)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(GBA_regression_tests source/RegressionTests.cpp)
    target_link_libraries(GBA_regression_tests PRIVATE gba_core)
    add_executable(GBA_timing_trace_tests tests/CpuTimingTraceTests.cpp)
    target_link_libraries(GBA_timing_trace_tests PRIVATE gba_core)
    add_executable(arm7_differential_iss tests/differential/Arm7DifferentialIss.cpp)
    target_link_libraries(arm7_differential_iss PRIVATE gba_core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(GBA_regression_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_timing_trace_tests PRIVATE -Wall -Wextra)
        target_compile_options(arm7_differential_iss PRIVATE -Wall -Wextra)
    endif()

    add_test(NAME command_help COMMAND GBA --help)
    add_test(NAME instruction_regressions COMMAND GBA_regression_tests)
    add_test(NAME cpu_timing_traces COMMAND GBA_timing_trace_tests)
    set_tests_properties(cpu_timing_traces PROPERTIES LABELS "cpu;timing")

    add_test(
        NAME raw_instruction_test_build
        COMMAND ${CMAKE_SOURCE_DIR}/tests/assembly/build-all.sh
    )
    set_tests_properties(raw_instruction_test_build PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        LABELS "raw-c;raw-thumb-c;raw-assembly"
    )

    foreach(raw_c_test IN ITEMS
        c_add_cmp
        c_loop_sum
        c_bitwise_shift
        c_memory_roundtrip
        c_crc32_zero_block
        c_rotate_mix
        c_multiply_accumulate
        c_halfword_memory
        c_signed_order
        c_byte_weighted_sum
        c_reverse_memory
        c_pointer_walk
        c_bit_count
        c_unsigned_overflow
        c_matrix_diagonal
    )
        add_test(
            NAME ARM::${raw_c_test}
            COMMAND $<TARGET_FILE:GBA> run
                ${CMAKE_SOURCE_DIR}/tests/assembly/build/${raw_c_test}.bin
                --steps 10000 --test-swi --trace
        )
        set_tests_properties(ARM::${raw_c_test} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS raw_instruction_test_build
            LABELS "raw-c;arm"
        )
    endforeach()

    foreach(raw_arm_assembly_test IN ITEMS
        arm_data_processing_flags
        arm_shift_edges
        arm_condition_codes
        arm_multiply_long
        arm_addressing_modes
        arm_halfword_signed
        arm_block_transfer
        arm_branch_link
        arm_psr_transfer
        arm_swap
    )
        add_test(
            NAME ARM::${raw_arm_assembly_test}
            COMMAND $<TARGET_FILE:GBA> run
                ${CMAKE_SOURCE_DIR}/tests/assembly/build/${raw_arm_assembly_test}.bin
                --steps 10000 --test-swi --trace
        )
        set_tests_properties(ARM::${raw_arm_assembly_test} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS raw_instruction_test_build
            LABELS "raw-assembly;arm"
        )
    endforeach()

    foreach(arm_sst_fixture IN ITEMS
        arm_b_bl
        arm_bx
        arm_cdp
        arm_data_proc_immediate
        arm_data_proc_immediate_shift
        arm_data_proc_register_shift
        arm_ldm_stm
        arm_ldr_str_immediate_offset
        arm_ldr_str_register_offset
        arm_ldrh_strh
        arm_ldrsb_ldrsh
        arm_mcr_mrc
        arm_mrs
        arm_msr_imm
        arm_msr_reg
        arm_mul_mla
        arm_mull_mlal
        arm_stc_ldc
        arm_swi
        arm_swp
    )
        set(arm_sst_path "${CMAKE_SOURCE_DIR}/bin/${arm_sst_fixture}.json.bin")
        if(EXISTS "${arm_sst_path}")
            add_test(
                NAME ARM::SST::${arm_sst_fixture}
                COMMAND $<TARGET_FILE:GBA> test arm ${arm_sst_path}
            )
            set_tests_properties(ARM::SST::${arm_sst_fixture} PROPERTIES
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                LABELS "arm;arm-sst"
                TIMEOUT 120
            )
        else()
            message(STATUS "ARM SST fixture not found; skipping ${arm_sst_fixture}")
        endif()
    endforeach()

    foreach(thumb_sst_fixture IN ITEMS
        thumb_add_cmp_mov_hi
        thumb_add_sp_or_pc
        thumb_add_sub
        thumb_add_sub_sp
        thumb_b
        thumb_bcc
        thumb_bl_blx_prefix
        thumb_bl_suffix
        thumb_bx
        thumb_data_proc
        thumb_ldm_stm
        thumb_ldr_pc_rel
        thumb_ldr_str_imm_offset
        thumb_ldr_str_reg_offset
        thumb_ldr_str_sp_rel
        thumb_ldrb_strb_imm_offset
        thumb_ldrh_strh_imm_offset
        thumb_ldrh_strh_reg_offset
        thumb_ldrsb_strb_reg_offset
        thumb_ldrsh_ldrsb_reg_offset
        thumb_lsl_lsr_asr
        thumb_mov_cmp_add_sub
        thumb_push_pop
        thumb_swi
        thumb_undefined_bcc
    )
        set(thumb_sst_path "${CMAKE_SOURCE_DIR}/bin/${thumb_sst_fixture}.json.bin")
        if(EXISTS "${thumb_sst_path}")
            add_test(
                NAME THUMB::SST::${thumb_sst_fixture}
                COMMAND $<TARGET_FILE:GBA> test thumb ${thumb_sst_path}
            )
            set_tests_properties(THUMB::SST::${thumb_sst_fixture} PROPERTIES
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                LABELS "thumb;thumb-sst"
                TIMEOUT 120
            )
        else()
            message(STATUS "Thumb SST fixture not found; skipping ${thumb_sst_fixture}")
        endif()
    endforeach()

    foreach(raw_thumb_c_test IN ITEMS
        c_add_cmp
        c_loop_sum
        c_bitwise_shift
        c_memory_roundtrip
        c_crc32_zero_block
        c_rotate_mix
        c_multiply_accumulate
        c_halfword_memory
        c_signed_order
        c_byte_weighted_sum
        c_reverse_memory
        c_pointer_walk
        c_bit_count
        c_unsigned_overflow
        c_matrix_diagonal
    )
        add_test(
            NAME THUMB::${raw_thumb_c_test}
            COMMAND $<TARGET_FILE:GBA> run
                ${CMAKE_SOURCE_DIR}/tests/assembly/build/thumb_${raw_thumb_c_test}.bin
                --steps 10000 --test-swi --trace
        )
        set_tests_properties(THUMB::${raw_thumb_c_test} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS raw_instruction_test_build
            LABELS "raw-thumb-c;thumb"
        )
    endforeach()

    add_test(NAME legacy_thumb_instruction_tests COMMAND $<TARGET_FILE:GBA> test thumb)
    set_tests_properties(legacy_thumb_instruction_tests PROPERTIES
        LABELS "legacy;thumb"
    )

    find_package(Python3 COMPONENTS Interpreter QUIET)
    find_program(IVERILOG_EXECUTABLE iverilog)
    find_program(VVP_EXECUTABLE vvp)
    if(Python3_Interpreter_FOUND AND IVERILOG_EXECUTABLE AND VVP_EXECUTABLE)
        set(ARM7_DIFFERENTIAL_TIMEOUT 120)
        if(DEFINED ENV{ARM7_DIFFERENTIAL_TIMEOUT})
            set(ARM7_DIFFERENTIAL_TIMEOUT "$ENV{ARM7_DIFFERENTIAL_TIMEOUT}")
        endif()
        add_test(
            NAME ARM::differential_reference
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/differential/run_arm7_differential.py
                --iss $<TARGET_FILE:arm7_differential_iss>
                --reference-dir ${CMAKE_SOURCE_DIR}/third_party/arm7-reference
                --testbench ${CMAKE_SOURCE_DIR}/tests/differential/arm7_reference_tb.v
                --iverilog ${IVERILOG_EXECUTABLE}
                --vvp ${VVP_EXECUTABLE}
        )
        set_tests_properties(ARM::differential_reference PROPERTIES
            LABELS "arm;differential;rtl"
            TIMEOUT ${ARM7_DIFFERENTIAL_TIMEOUT}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    else()
        message(STATUS "ARM7 differential test disabled: requires Python 3, iverilog, and vvp")
    endif()
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS GBA
        USES_TERMINAL
    )
endif()
