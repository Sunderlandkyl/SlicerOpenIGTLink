set(proj OpenIGTLink)

# Set dependency list
if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(OpenIGTLink_DIR CACHE)
  find_package(OpenIGTLink REQUIRED NO_MODULE)
endif()

# Sanity checks
if(DEFINED OpenIGTLink_DIR AND NOT EXISTS ${OpenIGTLink_DIR})
  message(FATAL_ERROR "OpenIGTLink_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED OpenIGTLink_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG)
  if(CTEST_USE_LAUNCHERS)
    set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG
      "-DCMAKE_PROJECT_OpenIGTLink_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake")
  endif()

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "https://github.com/openigtlink/OpenIGTLink.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    master
    QUIET
    )
  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/Deps/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/Deps/${proj}-build)
  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    CMAKE_CACHE_ARGS
      -DITK_DIR:PATH=${ITK_DIR}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
      -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}
      ${CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG}
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DOpenIGTLink_SUPERBUILD:BOOL=OFF
      -DOpenIGTLink_PROTOCOL_VERSION_2:BOOL=OFF
      -DOpenIGTLink_PROTOCOL_VERSION_3:BOOL=ON
      -DVP9_INCLUDE_DIR:PATH=${VP9_INCLUDE_DIR}
      -DVP9_LIBRARY_DIR:PATH=${VP9_LIBRARY_DIR}
      -DOpenIGTLink_USE_VP9:BOOL=${Slicer_USE_VP9}
      -DYASM_PYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
      # macOS
      -DCMAKE_MACOSX_RPATH:BOOL=0
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  ExternalProject_GenerateProjectDescription_Step(${proj})

  set(OpenIGTLink_DIR ${EP_BINARY_DIR})

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    ${OpenIGTLink_DIR}
    ${OpenIGTLink_DIR}/bin/<CMAKE_CFG_INTDIR>
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to install tree

  if(UNIX AND NOT APPLE)
    set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED <APPLAUNCHER_DIR>/lib/igtl)
    mark_as_superbuild(
      VARS ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
      LABELS "LIBRARY_PATHS_LAUNCHER_INSTALLED"
      )
  endif()

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

ExternalProject_Message(${proj} "OpenIGTLink_DIR:${OpenIGTLink_DIR}")
mark_as_superbuild(OpenIGTLink_DIR:PATH)
