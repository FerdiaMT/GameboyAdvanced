
cmake_minimum_required(VERSION 3.10)
project(GBA LANGUAGES CXX)

# Cross-translation-unit inlining matters for an interpreter: instruction and
# pixel paths cross several deliberately small abstraction boundaries.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GBA_ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(GBA_ENABLE_COVERAGE "Build with GCC gcov coverage instrumentation" OFF)

if(GBA_ENABLE_SANITIZERS AND GBA_ENABLE_COVERAGE)
    message(FATAL_ERROR "Sanitizers and gcov coverage cannot be enabled in the same build.")
endif()

if(GBA_ENABLE_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "GBA_ENABLE_SANITIZERS requires GCC or Clang.")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

if(GBA_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "GBA_ENABLE_COVERAGE currently requires GCC/gcov.")
    endif()
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

set(GBA_CORE_SOURCES
    source/Bus.cpp
    source/GBA.cpp
    source/SystemControl.cpp

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

if(NOT EMSCRIPTEN)
    add_executable(GBA source/Main.cpp)
    target_link_libraries(GBA PRIVATE gba_core gba_legacy_diagnostics)
    set_target_properties(GBA PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

# Browser frontend.  It deliberately has its own entry point: the native
# executable is a CLI/SDL application, whereas browsers need a callback-based
# main loop and receive input through the page.  Emscripten's virtual file
# system contains the bundled BIOS and selectable cartridge library.
if(EMSCRIPTEN)
    add_executable(GBA_web source/WebMain.cpp)
    target_link_libraries(GBA_web PRIVATE gba_core)
    set(GBA_WEB_CONTROLLER_SOURCE "${CMAKE_SOURCE_DIR}/web/assets/nes-controller.png")
    set(GBA_WEB_CONTROLLER_OUTPUT "${CMAKE_BINARY_DIR}/assets/nes-controller.png")
    add_custom_command(
        OUTPUT "${GBA_WEB_CONTROLLER_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GBA_WEB_CONTROLLER_SOURCE}" "${GBA_WEB_CONTROLLER_OUTPUT}"
        DEPENDS "${GBA_WEB_CONTROLLER_SOURCE}"
    )
    add_custom_target(gba_web_assets DEPENDS "${GBA_WEB_CONTROLLER_OUTPUT}")
    add_dependencies(GBA_web gba_web_assets)
    target_link_options(GBA_web PRIVATE
        "--shell-file" "${CMAKE_SOURCE_DIR}/web/shell.html"
        "--preload-file=${CMAKE_SOURCE_DIR}/bin/bios/gba_bios.bin@/assets/gba_bios.bin"
        "--preload-file=${CMAKE_SOURCE_DIR}/bin/games/sma.gba@/assets/sma.gba"
        "--preload-file=${CMAKE_SOURCE_DIR}/bin/games/sonic.gba@/assets/sonic.gba"
        "--preload-file=${CMAKE_SOURCE_DIR}/bin/games/Metroid Fusion (USA).gba@/assets/metroid-fusion.gba"
        "--preload-file=${CMAKE_SOURCE_DIR}/bin/games/Mother 3 (Tr).gba@/assets/mother-3.gba"
        "--preload-file=${CMAKE_SOURCE_DIR}/bin/games/Pokemon - Emerald Version (USA, Europe).gba@/assets/pokemon-emerald.gba"
        "--preload-file=${CMAKE_SOURCE_DIR}/bin/games/Super Mario Advance 4 - Super Mario Bros. 3 (USA, Australia) (Rev 1).gba@/assets/super-mario-advance-4.gba"
        "-sENVIRONMENT=web"
        "-sFILESYSTEM=1"
        "-sINITIAL_MEMORY=134217728"
        "-sASSERTIONS=1"
    )
    set_target_properties(GBA_web PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html"
    )
    set_property(TARGET GBA_web APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/web/shell.html"
    )
    add_custom_command(
        TARGET GBA_web
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DINPUT=$<TARGET_FILE_DIR:GBA_web>/index.js
            -P "${CMAKE_SOURCE_DIR}/cmake/PatchEmscriptenRuntime.cmake"
        VERBATIM
    )
endif()

if(NOT EMSCRIPTEN)
    # The emulator remains usable for headless testing without SDL.  When the
    # development package is present, the command-line runner can show the PPU's
    # 240x160 ARGB framebuffer in a simple scaled window.
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        target_link_libraries(GBA PRIVATE SDL2::SDL2)
        target_compile_definitions(GBA PRIVATE GBA_HAS_SDL2=1)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gba_core PRIVATE -Wall -Wextra)
    target_compile_options(gba_legacy_diagnostics PRIVATE -Wall -Wextra)
    if(TARGET GBA)
        target_compile_options(GBA PRIVATE -Wall -Wextra)
    endif()
    if(TARGET GBA_web)
        target_compile_options(GBA_web PRIVATE -Wall -Wextra)
    endif()
endif()

include(CTest)
if(BUILD_TESTING AND NOT EMSCRIPTEN)
    add_executable(GBA_regression_tests source/RegressionTests.cpp)
    target_link_libraries(GBA_regression_tests PRIVATE gba_core)
    add_executable(GBA_timing_trace_tests tests/CpuTimingTraceTests.cpp)
    target_link_libraries(GBA_timing_trace_tests PRIVATE gba_core)
    add_executable(GBA_cpu_architectural_tests tests/CpuArchitecturalTests.cpp)
    target_link_libraries(GBA_cpu_architectural_tests PRIVATE gba_core)
    add_executable(GBA_bios_bootstrap_tests tests/BiosBootstrapTests.cpp)
    target_link_libraries(GBA_bios_bootstrap_tests PRIVATE gba_core)
    add_executable(GBA_bus_mapping_tests tests/BusMappingTests.cpp)
    target_link_libraries(GBA_bus_mapping_tests PRIVATE gba_core)
    add_executable(GBA_master_scheduler_tests tests/MasterSchedulerTests.cpp)
    target_link_libraries(GBA_master_scheduler_tests PRIVATE gba_core)
    add_executable(GBA_system_control_tests tests/SystemControlTests.cpp)
    target_link_libraries(GBA_system_control_tests PRIVATE gba_core)
    add_executable(GBA_video_program_tests tests/VideoProgramTests.cpp)
    target_link_libraries(GBA_video_program_tests PRIVATE gba_core)
    add_executable(arm7_differential_iss tests/differential/Arm7DifferentialIss.cpp)
    target_link_libraries(arm7_differential_iss PRIVATE gba_core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(GBA_regression_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_timing_trace_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_cpu_architectural_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_bios_bootstrap_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_bus_mapping_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_master_scheduler_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_system_control_tests PRIVATE -Wall -Wextra)
        target_compile_options(GBA_video_program_tests PRIVATE -Wall -Wextra)
        target_compile_options(arm7_differential_iss PRIVATE -Wall -Wextra)
    endif()

    add_test(NAME command_help COMMAND GBA --help)
    if(SDL2_FOUND)
        add_test(NAME sdl_framebuffer_smoke COMMAND GBA --screen-smoke)
        set_tests_properties(sdl_framebuffer_smoke PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy"
            LABELS "gba;video;sdl"
        )
        add_test(NAME sdl_input_mapping_smoke COMMAND GBA --input-smoke)
        set_tests_properties(sdl_input_mapping_smoke PROPERTIES LABELS "gba;input;sdl")
    endif()
    add_test(NAME instruction_regressions COMMAND GBA_regression_tests)
    add_test(NAME cpu_timing_traces COMMAND GBA_timing_trace_tests)
    set_tests_properties(cpu_timing_traces PROPERTIES LABELS "cpu;timing")
    add_test(NAME cpu_architectural_state COMMAND GBA_cpu_architectural_tests)
    set_tests_properties(cpu_architectural_state PROPERTIES LABELS "cpu;architecture;exceptions")
    add_test(NAME bios_bootstrap_control_flow COMMAND GBA_bios_bootstrap_tests)
    set_tests_properties(bios_bootstrap_control_flow PROPERTIES LABELS "bios;cpu;boot")
    add_test(NAME gba_bus_mapping COMMAND GBA_bus_mapping_tests)
    set_tests_properties(gba_bus_mapping PROPERTIES LABELS "bus;gba;memory")
    add_test(NAME gba_master_scheduler COMMAND GBA_master_scheduler_tests)
    set_tests_properties(gba_master_scheduler PROPERTIES LABELS "gba;scheduler;timing")
    add_test(NAME gba_system_control COMMAND GBA_system_control_tests)
    set_tests_properties(gba_system_control PROPERTIES LABELS "gba;system;interrupts;timers;dma")
    add_test(NAME gba_mode3_gradient_program COMMAND GBA_video_program_tests)
    set_tests_properties(gba_mode3_gradient_program PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS raw_instruction_test_build
        LABELS "gba;video;integration"
    )

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
