cmake_minimum_required(VERSION 3.24)
project(nosfix VERSION 0.4.0 LANGUAGES C CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "nosfix is a Windows x64 DLL")
endif()

include(FetchContent)
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        9fbd087432700d73fc571118d6a9697a36443d88 # v1.3.3
    GIT_SHALLOW    FALSE
)
FetchContent_GetProperties(minhook)
if(NOT minhook_POPULATED)
    FetchContent_Populate(minhook)
endif()

# Upstream v1.3.3 provides Visual Studio project files but no CMake project.
# Build only the x64 decoder and implementation as a private static library.
add_library(minhook STATIC
    "${minhook_SOURCE_DIR}/src/buffer.c"
    "${minhook_SOURCE_DIR}/src/hde/hde64.c"
    "${minhook_SOURCE_DIR}/src/hook.c"
    "${minhook_SOURCE_DIR}/src/trampoline.c"
)
target_include_directories(minhook PUBLIC "${minhook_SOURCE_DIR}/include")
target_compile_definitions(minhook PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

add_library(nosfix SHARED
    src/asio_output.cpp
    src/audio_clock_patch.cpp
    src/audio_timeline.cpp
    src/bootstrap.cpp
    src/call_patch.cpp
    src/chart_event_cursor.cpp
    src/config.cpp
    src/game_hooks.cpp
    src/gameplay_clock.cpp
    src/hooks.cpp
    src/input_decoder.cpp
    src/input_normalizer.cpp
    src/input_pipeline.cpp
    src/keysound_dispatch.cpp
    src/log.cpp
    src/module_guard.cpp
    src/motion.cpp
    src/motion_hooks.cpp
    src/monitor_check.cpp
    src/native_gameplay.cpp
    src/native_note_state.cpp
    src/normal_judgment.cpp
    src/primitive_motion.cpp
    src/presentation.cpp
    src/render_tail.cpp
    src/raw_input_capture.cpp
    src/scheduler.cpp
    src/song_clock.cpp
    src/song_clock_publication.cpp
    src/spice_binding_adapter.cpp
    src/timed_normal.cpp
    src/timed_note_geometry.cpp
    src/timed_rendering.cpp
    src/timed_ui_timeline.cpp
    src/ui_motion.cpp
    src/ui_hooks.cpp
)
target_compile_features(nosfix PRIVATE cxx_std_20)
target_compile_definitions(nosfix PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_definitions(nosfix PRIVATE NOSFIX_VERSION_STRING="${PROJECT_VERSION}")
target_include_directories(nosfix PRIVATE include)
target_link_libraries(nosfix PRIVATE minhook bcrypt hid ole32 advapi32 user32 winmm)
set_target_properties(nosfix PROPERTIES
    OUTPUT_NAME "nosfix"
    PREFIX ""
    CXX_EXTENSIONS OFF
)
if(MSVC)
    target_compile_options(nosfix PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_link_options(nosfix PRIVATE /DYNAMICBASE /NXCOMPAT /MANIFEST:NO)
endif()

include(CTest)
if(BUILD_TESTING)
    enable_language(ASM_MASM)
    add_executable(nosfix_song_clock_tests tests/song_clock_tests.cpp src/song_clock.cpp)
    add_executable(nosfix_input_normalizer_tests tests/input_normalizer_tests.cpp src/input_normalizer.cpp)
    add_test(NAME input_normalizer COMMAND nosfix_input_normalizer_tests)
    add_executable(nosfix_normal_judgment_tests tests/normal_judgment_tests.cpp src/normal_judgment.cpp)
    add_test(NAME normal_judgment COMMAND nosfix_normal_judgment_tests)
    add_executable(nosfix_chart_event_cursor_tests tests/chart_event_cursor_tests.cpp src/chart_event_cursor.cpp)
    add_test(NAME chart_event_cursor COMMAND nosfix_chart_event_cursor_tests)
    add_executable(nosfix_timing_pipeline_tests tests/timing_pipeline_tests.cpp
        src/input_normalizer.cpp src/normal_judgment.cpp src/chart_event_cursor.cpp src/song_clock.cpp)
    add_test(NAME timing_pipeline COMMAND nosfix_timing_pipeline_tests)
    add_executable(nosfix_gameplay_clock_tests
        tests/gameplay_clock_tests.cpp src/gameplay_clock.cpp src/song_clock.cpp)
    add_test(NAME gameplay_clock COMMAND nosfix_gameplay_clock_tests)
    add_executable(nosfix_native_note_state_tests
        tests/native_note_state_tests.cpp src/native_note_state.cpp
        src/normal_judgment.cpp src/song_clock.cpp)
    add_test(NAME native_note_state COMMAND nosfix_native_note_state_tests)
    add_executable(nosfix_native_gameplay_tests
        tests/native_gameplay_tests.cpp src/native_gameplay.cpp
        src/native_note_state.cpp src/gameplay_clock.cpp
        src/chart_event_cursor.cpp src/normal_judgment.cpp src/song_clock.cpp
        src/input_normalizer.cpp)
    add_test(NAME native_gameplay COMMAND nosfix_native_gameplay_tests)
    add_executable(nosfix_raw_input_tests tests/raw_input_capture_tests.cpp
        src/raw_input_capture.cpp src/song_clock_publication.cpp)
    target_compile_definitions(nosfix_raw_input_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_raw_input_tests PRIVATE user32)
    add_test(NAME raw_input_capture COMMAND nosfix_raw_input_tests)
    add_executable(nosfix_raw_input_windows_tests tests/raw_input_windows_tests.cpp
        src/raw_input_capture.cpp src/song_clock_publication.cpp)
    target_compile_definitions(nosfix_raw_input_windows_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_raw_input_windows_tests PRIVATE user32)
    add_test(NAME raw_input_windows COMMAND nosfix_raw_input_windows_tests)
    add_executable(nosfix_input_decoder_tests tests/input_decoder_tests.cpp src/input_decoder.cpp)
    target_compile_definitions(nosfix_input_decoder_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_input_decoder_tests PRIVATE hid user32)
    add_test(NAME input_decoder COMMAND nosfix_input_decoder_tests)
    add_executable(nosfix_input_pipeline_tests tests/input_pipeline_tests.cpp
        src/input_pipeline.cpp src/input_decoder.cpp src/input_normalizer.cpp
        src/raw_input_capture.cpp src/song_clock.cpp src/song_clock_publication.cpp)
    target_compile_definitions(nosfix_input_pipeline_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_input_pipeline_tests PRIVATE hid user32)
    add_test(NAME input_pipeline COMMAND nosfix_input_pipeline_tests)
    add_executable(nosfix_spice_binding_adapter_tests tests/spice_binding_adapter_tests.cpp
        src/spice_binding_adapter.cpp src/input_decoder.cpp)
    target_compile_definitions(nosfix_spice_binding_adapter_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_spice_binding_adapter_tests PRIVATE hid user32)
    add_test(NAME spice_binding_adapter COMMAND nosfix_spice_binding_adapter_tests)
    add_executable(nosfix_monitor_check_tests tests/monitor_check_tests.cpp
        src/monitor_check.cpp src/hooks.cpp src/module_guard.cpp)
    target_compile_definitions(nosfix_monitor_check_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_monitor_check_tests PRIVATE minhook bcrypt)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll")
        add_test(NAME monitor_check COMMAND nosfix_monitor_check_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll")
    endif()
    add_executable(nosfix_timed_normal_tests tests/timed_normal_tests.cpp src/timed_normal.cpp)
    add_test(NAME timed_normal COMMAND nosfix_timed_normal_tests)
    add_executable(nosfix_timed_note_geometry_tests
        tests/timed_note_geometry_tests.cpp src/timed_note_geometry.cpp)
    add_test(NAME timed_note_geometry COMMAND nosfix_timed_note_geometry_tests)
    add_executable(nosfix_timed_ui_timeline_tests
        tests/timed_ui_timeline_tests.cpp src/timed_ui_timeline.cpp)
    add_test(NAME timed_ui_timeline COMMAND nosfix_timed_ui_timeline_tests)
    add_executable(nosfix_timed_rendering_tests tests/timed_rendering_tests.cpp
        src/timed_rendering.cpp src/timed_normal.cpp src/timed_note_geometry.cpp
        src/module_guard.cpp)
    target_compile_definitions(nosfix_timed_rendering_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_timed_rendering_tests PRIVATE bcrypt)
    add_executable(nosfix_timed_note_geometry_native_tests
        tests/timed_note_geometry_native_tests.cpp
        src/timed_rendering.cpp src/timed_normal.cpp src/timed_note_geometry.cpp
        src/module_guard.cpp)
    target_compile_definitions(nosfix_timed_note_geometry_native_tests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_timed_note_geometry_native_tests PRIVATE bcrypt)
    add_executable(nosfix_motion_hook_timed_tests tests/motion_hook_timed_tests.cpp
        src/motion_hooks.cpp src/primitive_motion.cpp src/motion.cpp src/timed_normal.cpp
        src/timed_rendering.cpp src/timed_note_geometry.cpp src/module_guard.cpp)
    target_compile_definitions(nosfix_motion_hook_timed_tests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_motion_hook_timed_tests PRIVATE bcrypt)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll" AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/libxact.dll")
        add_test(NAME timed_rendering COMMAND nosfix_timed_rendering_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll"
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/libxact.dll")
        add_test(NAME timed_note_geometry_native
            COMMAND nosfix_timed_note_geometry_native_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll"
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/libxact.dll")
        add_test(NAME motion_hook_timed COMMAND nosfix_motion_hook_timed_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll"
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/libxact.dll")
    endif()
    add_executable(nosfix_audio_clock_tests tests/audio_clock_patch_tests.cpp
        tests/qpc_patch_runner.asm src/audio_clock_patch.cpp src/module_guard.cpp)
    target_compile_definitions(nosfix_audio_clock_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(nosfix_audio_clock_tests PRIVATE bcrypt)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/libxact.dll")
        add_test(NAME audio_clock_patch COMMAND nosfix_audio_clock_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/analyze/libxact.dll")
    endif()
    add_executable(nosfix_audio_timeline_tests
        tests/audio_timeline_tests.cpp src/audio_timeline.cpp)
    add_executable(nosfix_keysound_dispatch_tests
        tests/keysound_dispatch_tests.cpp src/keysound_dispatch.cpp)
    add_executable(nosfix_audio_native_abi_tests tests/asio_native_abi_tests.cpp)
    add_executable(nosfix_asio_output_tests
        tests/asio_output_tests.cpp src/asio_output.cpp src/audio_timeline.cpp)
    add_executable(nosfix_asio_registry_smoke
        tests/asio_registry_smoke.cpp src/asio_output.cpp src/audio_timeline.cpp)
    foreach(audio_test_target
            nosfix_audio_timeline_tests
            nosfix_keysound_dispatch_tests
            nosfix_audio_native_abi_tests
            nosfix_asio_output_tests
            nosfix_asio_registry_smoke)
        target_compile_features(${audio_test_target} PRIVATE cxx_std_20)
        target_include_directories(${audio_test_target} PRIVATE include tests src)
        target_compile_definitions(${audio_test_target} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
        if(MSVC)
            target_compile_options(${audio_test_target} PRIVATE /W4 /permissive- /EHsc /utf-8)
            target_link_options(${audio_test_target} PRIVATE /MANIFEST:NO)
        endif()
    endforeach()
    target_link_libraries(nosfix_asio_output_tests PRIVATE ole32 advapi32)
    target_link_libraries(nosfix_asio_registry_smoke PRIVATE ole32 advapi32)
    add_test(NAME audio_timeline_core COMMAND nosfix_audio_timeline_tests)
    add_test(NAME keysound_dispatch_core COMMAND nosfix_keysound_dispatch_tests)
    add_test(NAME asio_native_abi COMMAND nosfix_audio_native_abi_tests)
    add_test(NAME asio_output_core COMMAND nosfix_asio_output_tests)
    add_test(NAME asio_registry_smoke COMMAND nosfix_asio_registry_smoke)
    set(NOSFIX_ASIO_SDK_INCLUDE
        "D:/Share/chart-requester/reference/spice2x/src/spice2x/external/asio"
        CACHE PATH "Official ASIO SDK include directory (not vendored)")
    if(EXISTS "${NOSFIX_ASIO_SDK_INCLUDE}/asio.h")
        add_executable(nosfix_asio_sdk_abi_compare tests/asio_sdk_abi_compare.cpp)
        target_compile_features(nosfix_asio_sdk_abi_compare PRIVATE cxx_std_20)
        target_include_directories(nosfix_asio_sdk_abi_compare PRIVATE
            include tests src "${NOSFIX_ASIO_SDK_INCLUDE}")
        target_compile_definitions(nosfix_asio_sdk_abi_compare PRIVATE
            WIN32_LEAN_AND_MEAN NOMINMAX)
        if(MSVC)
            target_compile_options(nosfix_asio_sdk_abi_compare PRIVATE
                /W4 /permissive- /EHsc /utf-8)
            target_link_options(nosfix_asio_sdk_abi_compare PRIVATE /MANIFEST:NO)
        endif()
        add_test(NAME asio_sdk_abi_compare COMMAND nosfix_asio_sdk_abi_compare)
    endif()
    add_test(NAME song_clock COMMAND nosfix_song_clock_tests)
    add_executable(nosfix_motion_tests tests/motion_tests.cpp src/motion.cpp)
    add_executable(nosfix_ui_motion_tests tests/ui_motion_tests.cpp src/ui_motion.cpp)
    add_executable(nosfix_ui_hook_tests tests/ui_hook_tests.cpp src/ui_hooks.cpp src/ui_motion.cpp)
    target_compile_definitions(nosfix_ui_hook_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    add_executable(nosfix_primitive_motion_tests
        tests/primitive_motion_tests.cpp src/primitive_motion.cpp src/motion.cpp)
    add_executable(nosfix_motion_hook_tests tests/motion_hook_tests.cpp
        src/motion_hooks.cpp src/primitive_motion.cpp src/motion.cpp src/timed_normal.cpp
        src/timed_rendering.cpp src/timed_note_geometry.cpp src/module_guard.cpp)
    target_link_libraries(nosfix_motion_hook_tests PRIVATE bcrypt)
    target_compile_definitions(nosfix_motion_hook_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    foreach(target nosfix_motion_tests nosfix_primitive_motion_tests nosfix_motion_hook_tests nosfix_ui_motion_tests nosfix_ui_hook_tests nosfix_song_clock_tests nosfix_audio_clock_tests nosfix_timed_normal_tests nosfix_timed_rendering_tests nosfix_timed_note_geometry_tests nosfix_timed_ui_timeline_tests nosfix_timed_note_geometry_native_tests nosfix_motion_hook_timed_tests nosfix_monitor_check_tests nosfix_raw_input_tests nosfix_raw_input_windows_tests nosfix_input_decoder_tests nosfix_input_pipeline_tests nosfix_spice_binding_adapter_tests nosfix_gameplay_clock_tests nosfix_native_note_state_tests nosfix_native_gameplay_tests nosfix_input_normalizer_tests nosfix_normal_judgment_tests nosfix_chart_event_cursor_tests nosfix_timing_pipeline_tests)
        target_compile_features(${target} PRIVATE cxx_std_20)
        target_include_directories(${target} PRIVATE include)
        if(MSVC)
            target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/EHsc;/utf-8>)
            target_link_options(${target} PRIVATE /MANIFEST:NO)
        endif()
    endforeach()
    add_test(NAME motion COMMAND nosfix_motion_tests)
    add_test(NAME ui_motion COMMAND nosfix_ui_motion_tests)
    add_test(NAME ui_hooks COMMAND nosfix_ui_hook_tests)
    add_test(NAME primitive_motion COMMAND nosfix_primitive_motion_tests)
    add_test(NAME motion_hooks COMMAND nosfix_motion_hook_tests)
    add_executable(nosfix_scheduler_tests
        tests/scheduler_tests.cpp
        src/render_tail.cpp
        src/scheduler.cpp
    )
    target_compile_features(nosfix_scheduler_tests PRIVATE cxx_std_20)
    target_include_directories(nosfix_scheduler_tests PRIVATE include)
    if(MSVC)
        target_compile_options(nosfix_scheduler_tests PRIVATE /W4 /permissive- /EHsc)
        target_link_options(nosfix_scheduler_tests PRIVATE /MANIFEST:NO)
    endif()
    add_test(NAME scheduler COMMAND nosfix_scheduler_tests)

    add_executable(nosfix_plugin_load_tests tests/plugin_load_tests.cpp)
    target_compile_features(nosfix_plugin_load_tests PRIVATE cxx_std_20)
    target_compile_definitions(nosfix_plugin_load_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    add_test(NAME plugin_missing_game
        COMMAND nosfix_plugin_load_tests "$<TARGET_FILE:nosfix>")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll")
        add_test(NAME plugin_missing_afp
            COMMAND nosfix_plugin_load_tests "$<TARGET_FILE:nosfix>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/afp-core.dll")
            add_test(NAME plugin_missing_libxact
                COMMAND nosfix_plugin_load_tests "$<TARGET_FILE:nosfix>"
                        "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll"
                        "${CMAKE_CURRENT_SOURCE_DIR}/analyze/afp-core.dll")
            add_test(NAME plugin_inert_image
                COMMAND nosfix_plugin_load_tests "$<TARGET_FILE:nosfix>"
                        "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll"
                        "${CMAKE_CURRENT_SOURCE_DIR}/analyze/afp-core.dll"
                        "${CMAKE_CURRENT_SOURCE_DIR}/analyze/libxact.dll")
        endif()
    endif()

    add_executable(nosfix_hook_install_tests
        tests/hook_install_tests.cpp
        src/hooks.cpp
        src/module_guard.cpp
    )
    target_compile_features(nosfix_hook_install_tests PRIVATE cxx_std_20)
    target_compile_definitions(nosfix_hook_install_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    target_include_directories(nosfix_hook_install_tests PRIVATE include)
    target_link_libraries(nosfix_hook_install_tests PRIVATE minhook bcrypt)
    if(MSVC)
        target_compile_options(nosfix_hook_install_tests PRIVATE /W4 /permissive- /EHsc /utf-8)
        target_link_options(nosfix_hook_install_tests PRIVATE /MANIFEST:NO)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll")
        add_test(NAME real_image_hook_install
            COMMAND nosfix_hook_install_tests
                    "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll")
    endif()

    add_executable(nosfix_call_patch_tests
        tests/call_patch_tests.cpp
        src/call_patch.cpp
        src/module_guard.cpp
    )
    target_compile_features(nosfix_call_patch_tests PRIVATE cxx_std_20)
    target_compile_definitions(nosfix_call_patch_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    target_include_directories(nosfix_call_patch_tests PRIVATE include)
    target_link_libraries(nosfix_call_patch_tests PRIVATE bcrypt)
    if(MSVC)
        target_compile_options(nosfix_call_patch_tests PRIVATE /W4 /permissive- /EHsc /utf-8)
        target_link_options(nosfix_call_patch_tests PRIVATE /MANIFEST:NO)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll")
        add_test(NAME real_image_call_patch
            COMMAND nosfix_call_patch_tests
                    "${CMAKE_CURRENT_SOURCE_DIR}/analyze/nostalgia.dll"
                    "${CMAKE_CURRENT_SOURCE_DIR}/analyze/afp-core.dll")
    endif()
endif()

add_custom_command(TARGET nosfix POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${minhook_SOURCE_DIR}/LICENSE.txt"
        "$<TARGET_FILE_DIR:nosfix>/MinHook-LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/nosfix.ini"
        "$<TARGET_FILE_DIR:nosfix>/nosfix.ini"
    COMMENT "Copying runtime configuration and third-party license"
)
