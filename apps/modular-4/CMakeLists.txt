cmake_minimum_required(VERSION 3.22)
project(SpeechSharePHONModular VERSION 0.3.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(SPEECHSHARE_LIVE_BUILD_APP "Build the JUCE Windows standalone" ON)
option(SPEECHSHARE_LIVE_BUILD_TESTS "Build source-level routing + DSP tests" ON)

if(SPEECHSHARE_LIVE_BUILD_TESTS)
  enable_testing()
  add_executable(SpeechShareChannelFormatTests Tests/ChannelFormatTests.cpp App/ChannelFormat.h)
  add_test(NAME ChannelFormatTests COMMAND SpeechShareChannelFormatTests)
  add_executable(SpeechShareModularPatchTests Tests/ModularPatchTests.cpp App/ModularPatch.h)
  add_test(NAME ModularPatchTests COMMAND SpeechShareModularPatchTests)
  add_executable(SpeechShareFourSessionTests Tests/FourSessionTests.cpp App/ModularPatch.h)
  add_test(NAME FourSessionTests COMMAND SpeechShareFourSessionTests)
  add_executable(SpeechShareSleepTests Tests/SessionActivityTests.cpp App/SessionActivity.h App/ModularPatch.h)
  add_test(NAME SessionSleepTests COMMAND SpeechShareSleepTests)
  add_executable(SpeechSharePHONMappingTests Tests/PhonMacroTests.cpp Insert/PhonMacro.h)
  add_test(NAME PhonMappingTests COMMAND SpeechSharePHONMappingTests)
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

  juce_add_gui_app(SpeechSharePHONModular
    PRODUCT_NAME "SpeechShare PHON Modular 4 Sleep"
    COMPANY_NAME "Backstage"
    BUNDLE_ID "it.backstage.speechshare.phonmodular4sleep"
    NEEDS_CURL FALSE
  )
  juce_generate_juce_header(SpeechSharePHONModular)
  juce_add_binary_data(SpeechShareLiveBranding SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/backstage_logo.png")
  target_sources(SpeechSharePHONModular PRIVATE
    App/Main.cpp App/StandaloneAudio.h App/StandaloneAudio.cpp App/ModularPatch.h App/SessionActivity.h App/ChannelFormat.h
    Insert/InsertPluginProcessor.h Insert/InsertPluginProcessor.cpp
    Insert/InsertPluginEditor.h Insert/InsertPluginEditor.cpp
    Insert/InsertDSP.h Insert/InsertDSP.cpp Insert/PhonMacro.h
    Insert/SharedPool.h Insert/SharedPool.cpp Insert/PresentationPolicy.h
  )
  target_link_libraries(SpeechSharePHONModular PRIVATE
    SpeechShareLiveBranding
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
  )
  target_compile_definitions(SpeechSharePHONModular PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_ASIO=1
    SPEECHSHARE_STANDALONE_APP=1
    SPEECHSHARE_PHON_STANDALONE=1
    SPEECHSHARE_PHON_MODULAR=1
  )
endif()
