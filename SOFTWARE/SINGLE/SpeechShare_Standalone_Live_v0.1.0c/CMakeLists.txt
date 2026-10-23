cmake_minimum_required(VERSION 3.22)
project(SpeechShareStandaloneLive VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(SPEECHSHARE_LIVE_BUILD_APP "Build the JUCE Windows standalone" ON)
option(SPEECHSHARE_LIVE_BUILD_TESTS "Build source-level routing + DSP tests" ON)

if(SPEECHSHARE_LIVE_BUILD_TESTS)
  enable_testing()
  add_executable(SpeechSharePatchTests Tests/PatchRoutingTests.cpp App/PatchRouting.h)
  add_test(NAME PatchRoutingTests COMMAND SpeechSharePatchTests)
  add_executable(SpeechShareInsertEngineTests Insert/SharedPool.cpp Insert/InsertDSP.cpp Tests/InsertTests.cpp)
  target_compile_features(SpeechShareInsertEngineTests PRIVATE cxx_std_17)
  add_test(NAME StandaloneDSPTests COMMAND SpeechShareInsertEngineTests)
endif()

if(SPEECHSHARE_LIVE_BUILD_APP)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
  elseif(DEFINED JUCE_ROOT AND EXISTS "${JUCE_ROOT}/CMakeLists.txt")
    add_subdirectory("${JUCE_ROOT}" "${CMAKE_BINARY_DIR}/JUCE")
  else()
    find_package(JUCE CONFIG QUIET)
    if(NOT JUCE_FOUND)
      message(FATAL_ERROR "JUCE not found. Supply -DJUCE_ROOT=C:/JUCE or build tests only using -DSPEECHSHARE_LIVE_BUILD_APP=OFF")
    endif()
  endif()

  juce_add_gui_app(SpeechShareStandaloneLive
    PRODUCT_NAME "SpeechShare Standalone Live"
    COMPANY_NAME "Backstage"
    BUNDLE_ID "it.backstage.speechshare.live"
    NEEDS_CURL FALSE
  )
  juce_generate_juce_header(SpeechShareStandaloneLive)
  juce_add_binary_data(SpeechShareLiveBranding SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/backstage_logo.png")
  target_sources(SpeechShareStandaloneLive PRIVATE
    App/Main.cpp App/StandaloneAudio.h App/StandaloneAudio.cpp App/PatchRouting.h
    Insert/InsertPluginProcessor.h Insert/InsertPluginProcessor.cpp
    Insert/InsertPluginEditor.h Insert/InsertPluginEditor.cpp
    Insert/InsertDSP.h Insert/InsertDSP.cpp
    Insert/SharedPool.h Insert/SharedPool.cpp Insert/PresentationPolicy.h
  )
  target_link_libraries(SpeechShareStandaloneLive PRIVATE
    SpeechShareLiveBranding
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
  )
  target_compile_definitions(SpeechShareStandaloneLive PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_ASIO=1
    SPEECHSHARE_STANDALONE_APP=1
  )
endif()
