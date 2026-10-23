cmake_minimum_required(VERSION 3.22)
project(SpeechShare VERSION 0.1.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SPEECHSHARE_BUILD_PLUGINS "Build VST3 and Standalone using JUCE" ON)
option(SPEECHSHARE_BUILD_TESTS "Build dependency-free DSP tests" ON)

if(SPEECHSHARE_BUILD_TESTS)
  enable_testing()
  add_executable(SpeechShareEngineTests Source/SpeechEngine.cpp Tests/EngineTests.cpp)
  target_compile_features(SpeechShareEngineTests PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(SpeechShareEngineTests PRIVATE /W4)
  else()
    target_compile_options(SpeechShareEngineTests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME EngineTests COMMAND SpeechShareEngineTests)
endif()

if(SPEECHSHARE_BUILD_PLUGINS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
  elseif(DEFINED JUCE_ROOT AND EXISTS "${JUCE_ROOT}/CMakeLists.txt")
    add_subdirectory("${JUCE_ROOT}" "${CMAKE_BINARY_DIR}/JUCE")
  else()
    find_package(JUCE CONFIG QUIET)
    if(NOT JUCE_FOUND)
      message(FATAL_ERROR "JUCE missing. Download JUCE 8 and pass -DJUCE_ROOT=/path/to/JUCE, or place JUCE/ beside CMakeLists.txt. Build tests offline with -DSPEECHSHARE_BUILD_PLUGINS=OFF")
    endif()
  endif()

  set(SPEECHSHARE_FORMATS VST3 Standalone)
  if(APPLE)
    list(APPEND SPEECHSHARE_FORMATS AU)
  endif()

  function(add_speechshare_plugin TARGET PRODUCT PLUGIN_CODE MULTI_FLAG BUNDLE_ID)
    juce_add_plugin(${TARGET}
      PRODUCT_NAME "${PRODUCT}"
      COMPANY_NAME "SpeechShare"
      BUNDLE_ID "${BUNDLE_ID}"
      PLUGIN_MANUFACTURER_CODE SpSh
      PLUGIN_CODE ${PLUGIN_CODE}
      PLUGIN_NAME "${PRODUCT}"
      IS_SYNTH FALSE
      NEEDS_MIDI_INPUT FALSE
      NEEDS_MIDI_OUTPUT FALSE
      IS_MIDI_EFFECT FALSE
      EDITOR_WANTS_KEYBOARD_FOCUS TRUE
      COPY_PLUGIN_AFTER_BUILD FALSE
      FORMATS ${SPEECHSHARE_FORMATS}
    )
    juce_generate_juce_header(${TARGET})
    target_sources(${TARGET} PRIVATE
      Source/PluginProcessor.cpp
      Source/PluginProcessor.h
      Source/PluginEditor.cpp
      Source/PluginEditor.h
      Source/SpeechEngine.cpp
      Source/SpeechEngine.h
    )
    target_compile_definitions(${TARGET} PUBLIC
      SPEECHSHARE_MULTI=${MULTI_FLAG}
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
    )
    target_link_libraries(${TARGET} PRIVATE
      juce::juce_audio_utils
      juce::juce_audio_processors
      juce::juce_gui_extra
      juce::juce_recommended_config_flags
      juce::juce_recommended_warning_flags
      juce::juce_recommended_lto_flags
    )
  endfunction()

  add_speechshare_plugin(SpeechShareMultitrack "SpeechShare Multitrack" Ssmu 1 org.speechshare.multitrack)
  add_speechshare_plugin(SpeechShareStereo "SpeechShare Stereo" Ssst 0 org.speechshare.stereo)
endif()

# Experimental DAW-only INSERT branch. Separate plugin ID, does not change Multitrack/Stereo.
option(SPEECHSHARE_BUILD_INSERT "Build experimental per-track Insert VST3" ON)

if(SPEECHSHARE_BUILD_TESTS)
  add_executable(SpeechShareInsertTests
    Insert/SharedPool.cpp Insert/InsertDSP.cpp Tests/InsertTests.cpp)
  target_compile_features(SpeechShareInsertTests PRIVATE cxx_std_17)
  if(NOT MSVC)
    target_compile_options(SpeechShareInsertTests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME InsertSharedPoolTests COMMAND SpeechShareInsertTests)
  add_executable(SpeechSharePresentationTests Tests/PresentationPolicyTests.cpp)
  target_compile_features(SpeechSharePresentationTests PRIVATE cxx_std_17)
  add_test(NAME PresentationPolicyTests COMMAND SpeechSharePresentationTests)
  add_executable(SpeechShareSessionStressTests
    Insert/SharedPool.cpp Tests/SessionStressTests.cpp)
  target_compile_features(SpeechShareSessionStressTests PRIVATE cxx_std_17)
  add_test(NAME SessionStressTests COMMAND SpeechShareSessionStressTests)
endif()

if(SPEECHSHARE_BUILD_PLUGINS AND SPEECHSHARE_BUILD_INSERT)
  juce_add_plugin(SpeechShareInsert
    PRODUCT_NAME "SpeechShare Insert Prototype"
    COMPANY_NAME "SpeechShare"
    BUNDLE_ID "org.speechshare.insertprototype"
    PLUGIN_MANUFACTURER_CODE SpSh
    PLUGIN_CODE Ssin
    PLUGIN_NAME "SpeechShare Insert Prototype"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3
  )
  juce_generate_juce_header(SpeechShareInsert)
  # Backstage signature is compiled into the VST3: no external PNG files are
  # needed when the plugin is installed on the host computer.
  juce_add_binary_data(SpeechShareInsertBranding
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/backstage_logo.png"
  )
  target_link_libraries(SpeechShareInsert PRIVATE SpeechShareInsertBranding)
  target_sources(SpeechShareInsert PRIVATE
    Insert/InsertPluginProcessor.h Insert/InsertPluginProcessor.cpp
    Insert/InsertPluginEditor.h Insert/InsertPluginEditor.cpp
    Insert/SharedPool.h Insert/SharedPool.cpp
    Insert/InsertDSP.h Insert/InsertDSP.cpp
  )
  target_compile_definitions(SpeechShareInsert PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
  )
  target_link_libraries(SpeechShareInsert PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
  )
endif()
