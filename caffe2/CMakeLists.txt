# ---[ Declare source file lists

# Caffe2_{CPU,GPU}_SRCS is the list that will have all the related source
# files for CPU and GPU respectively. They will be filled with the
# CMakeLists.txt files under each folder respectively.
set(Caffe2_CPU_SRCS)
set(Caffe2_GPU_SRCS)
set(Caffe2_HIP_SRCS)

# Caffe2_{CPU,GPU}_TEST_SRCS is the list that will have all the related source
# files for CPU and GPU tests respectively.
set(Caffe2_CPU_TEST_SRCS)
set(Caffe2_GPU_TEST_SRCS)

# ---[ Add respective subdirectories
# Note: the folders that are being commented out have not been properly
# addressed yet.

add_subdirectory(proto)

add_subdirectory(contrib)
add_subdirectory(core)
add_subdirectory(cuda_rtc)
add_subdirectory(db)
add_subdirectory(distributed)
# add_subdirectory(experiments) # note, we may remove this folder at some point
add_subdirectory(image)
add_subdirectory(video)
add_subdirectory(mkl)
add_subdirectory(mobile)
add_subdirectory(mpi)
add_subdirectory(observers)
add_subdirectory(operators)
add_subdirectory(perfkernels)
add_subdirectory(python)
add_subdirectory(queue)
add_subdirectory(sgd)
if (BUILD_SHARE_DIR)
  add_subdirectory(share)
endif()
# add_subdirectory(test) # todo: use caffe2_gtest_main instead of gtest_main because we will need to call GlobalInit
add_subdirectory(transforms)
add_subdirectory(utils)

# Advanced: if we have white list specified, we will do intersections for all
# main lib srcs.
if (CAFFE2_WHITELISTED_FILES)
  caffe2_do_whitelist(Caffe2_CPU_SRCS CAFFE2_WHITELISTED_FILES)
  caffe2_do_whitelist(Caffe2_GPU_SRCS CAFFE2_WHITELISTED_FILES)
  caffe2_do_whitelist(Caffe2_HIP_SRCS CAFFE2_WHITELISTED_FILES)
endif()

# Debug messages - if you want to get a list of source files, enable the
# following.
if (FALSE)
  message(STATUS "CPU sources: ")
  foreach(tmp ${Caffe2_CPU_SRCS})
    message(STATUS "  " ${tmp})
  endforeach()

  message(STATUS "GPU sources: ")
  foreach(tmp ${Caffe2_GPU_SRCS})
    message(STATUS "  " ${tmp})
  endforeach()

  message(STATUS "HIP sources: ")
  foreach(tmp ${Caffe2_HIP_SRCS})
    message(STATUS "  " ${tmp})
  endforeach()

  message(STATUS "CPU test sources: ")
  foreach(tmp ${Caffe2_CPU_TEST_SRCS})
    message(STATUS "  " ${tmp})
  endforeach()

  message(STATUS "GPU test sources: ")
  foreach(tmp ${Caffe2_GPU_TEST_SRCS})
    message(STATUS "  " ${tmp})
  endforeach()

  message(STATUS "HIP test sources: ")
  foreach(tmp ${Caffe2_HIP_TEST_SRCS})
    message(STATUS "  " ${tmp})
  endforeach()
endif()

# ---[ Generate and install header files.

# Write the macros file.
configure_file(
    ${PROJECT_SOURCE_DIR}/caffe2/core/macros.h.in
    ${PROJECT_BINARY_DIR}/caffe2/core/macros.h)

# Installing the header files
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
install(FILES ${PROJECT_BINARY_DIR}/caffe2/core/macros.h
        DESTINATION include/caffe2/core)


# ---[ List of libraries to link with

# In the static linking + clang mode, cmake will fail to identify the build
# order because the lib becomes one single string -Wl,-force-load,libCaffe2_CPU.so
# As a result, we will create a Caffe2_MAIN_LIBS_ORDER variable simply to
# enforce the dependency.
set(Caffe2_MAIN_LIBS_ORDER)
set(Caffe2_MAIN_LIBS)

# Compile exposed libraries.
add_library(caffe2 ${Caffe2_CPU_SRCS} $<TARGET_OBJECTS:Caffe_PROTO> $<TARGET_OBJECTS:Caffe2_PROTO>)
target_link_libraries(caffe2 PRIVATE ${Caffe2_DEPENDENCY_LIBS})
target_include_directories(caffe2 INTERFACE $<INSTALL_INTERFACE:include>)
target_compile_options(caffe2 INTERFACE "-std=c++11")
install(TARGETS caffe2 EXPORT Caffe2Targets DESTINATION lib)
link_directories(${CMAKE_INSTALL_PREFIX}/lib)
caffe_add_linker_flag(caffe2 Caffe2_CPU_LINK)
list(APPEND Caffe2_MAIN_LIBS_ORDER caffe2 Caffe2_PROTO)
list(APPEND Caffe2_MAIN_LIBS ${Caffe2_CPU_LINK})
if (Caffe2_EXTERNAL_DEPENDENCIES)
  add_dependencies(caffe2 ${Caffe2_EXTERNAL_DEPENDENCIES})
endif()

# ---[ On Android, Caffe2 uses cpufeatures library in the thread pool
if (ANDROID)
  # ---[ Check if cpufeatures was already imported by NNPACK
  if (NOT TARGET cpufeatures)
    include(${CMAKE_SOURCE_DIR}/third_party/android-cmake/AndroidNdkModules.cmake)
    android_ndk_import_module_cpufeatures()
  endif()
  target_link_libraries(caffe2 PRIVATE cpufeatures)
endif()

# ---[ CUDA library.
if(USE_CUDA)
  # A hack to deal with cuda library dependencies and modern CMake: the
  # CUDA_ADD_LIBRARY includes a target_link_libraries, and as a result,
  # one cannot use PUBLIC/PRIVATE/INTERFACE for the target anymore. This
  # hack adds the PRIVATE keywords to CUDA_LIBRARIES so we can deal with
  # it.
  set(__tmp ${CUDA_LIBRARIES})
  set(CUDA_LIBRARIES PRIVATE ${CUDA_LIBRARIES})
  CUDA_ADD_LIBRARY(caffe2_gpu ${Caffe2_GPU_SRCS})
  set(CUDA_LIBRARIES ${__tmp})
  target_include_directories(
      caffe2_gpu INTERFACE $<INSTALL_INTERFACE:include>)
  target_compile_options(caffe2 INTERFACE "-std=c++11")
  list(APPEND Caffe2_MAIN_LIBS_ORDER caffe2_gpu)
  add_dependencies(caffe2_gpu Caffe2_PROTO)
  if (BUILD_SHARED_LIBS)
    target_link_libraries(
        caffe2_gpu
        PRIVATE
        caffe2
        ${Caffe2_DEPENDENCY_LIBS}
        ${Caffe2_CUDA_DEPENDENCY_LIBS})
  else()
    target_link_libraries(
        caffe2_gpu
        PRIVATE
        ${Caffe2_DEPENDENCY_LIBS}
        ${Caffe2_CUDA_DEPENDENCY_LIBS})
  endif()
  caffe_add_linker_flag(caffe2_gpu Caffe2_GPU_LINK)
  list(APPEND Caffe2_MAIN_LIBS_ORDER caffe2_gpu)
  list(APPEND Caffe2_MAIN_LIBS ${Caffe2_GPU_LINK})
  list(APPEND Caffe2_MAIN_LIBS_ORDER caffe2_gpu)
  if (Caffe2_EXTERNAL_DEPENDENCIES)
    add_dependencies(caffe2_gpu ${Caffe2_EXTERNAL_DEPENDENCIES})
  endif()
  install(TARGETS caffe2_gpu EXPORT Caffe2Targets DESTINATION lib)
endif()

# ---[ HIP library.
if(USE_HIP)
  set(Caffe2_HIP_GCC_COMPILE_FLAGS "-D__HIP_PLATFORM_HCC__=1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Caffe2_HIP_GCC_COMPILE_FLAGS}")
  set(HIP_HIPCC_FLAGS "${HIP_HIPCC_FLAGS} -fPIC")
  set(HIP_HIPCC_FLAGS "${HIP_HIPCC_FLAGS} -Wno-inconsistent-missing-override")
  set(HIP_HIPCC_FLAGS "${HIP_HIPCC_FLAGS} -Wno-exceptions")
  set(HIP_HIPCC_FLAGS "${HIP_HIPCC_FLAGS} -Wno-shift-count-negative")
  set(HIP_HIPCC_FLAGS "${HIP_HIPCC_FLAGS} -Wno-unused-command-line-argument")
  INCLUDE_DIRECTORIES(/opt/rocm/include /opt/rocm/hiprand/include /opt/rocm/rocrand/include /opt/rocm/miopen/include)
  LINK_DIRECTORIES(/opt/rocm/lib /opt/rocm/hiprand/lib /opt/rocm/rocrand/lib /opt/rocm/miopen/lib)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC -Wl,-Bsymbolic")
  set_source_files_properties(${Caffe2_HIP_SRCS} PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)
  HIP_ADD_LIBRARY(caffe2_hip ${Caffe2_HIP_SRCS})
  set( CMAKE_HIP_CREATE_SHARED_LIBRARY "${HIP_HIPCC_EXECUTABLE} <CMAKE_SHARED_LIBRARY_CXX_FLAGS> ${CMAKE_CXX_FLAGS} <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>" )
  set_target_properties(caffe2_hip PROPERTIES LINKER_LANGUAGE HIP)

  target_include_directories(
      caffe2_hip INTERFACE $<INSTALL_INTERFACE:include>)
  target_compile_options(caffe2 INTERFACE "-std=c++11")
  list(APPEND Caffe2_MAIN_LIBS_ORDER caffe2_hip)
  add_dependencies(caffe2_hip Caffe2_PROTO)
  list(APPEND Caffe2_HIP_DEPENDENCY_LIBS rocrand hiprand rocblas MIOpen)
  if (BUILD_SHARED_LIBS)
    target_link_libraries(
        caffe2_hip
        PRIVATE
        caffe2
        ${Caffe2_DEPENDENCY_LIBS}
        ${Caffe2_HIP_DEPENDENCY_LIBS})
  else()
    target_link_libraries(
        caffe2_hip
        PRIVATE
        ${Caffe2_DEPENDENCY_LIBS}
        ${Caffe2_HIP_DEPENDENCY_LIBS})
  endif()
  caffe_add_linker_flag(caffe2_hip Caffe2_HIP_LINK)
  list(APPEND Caffe2_MAIN_LIBS_ORDER caffe2_hip)
  list(APPEND Caffe2_MAIN_LIBS ${Caffe2_HIP_LINK})
  list(APPEND Caffe2_MAIN_LIBS_ORDER caffe2_hip)
  if (Caffe2_EXTERNAL_DEPENDENCIES)
    add_dependencies(caffe2_hip ${Caffe2_EXTERNAL_DEPENDENCIES})
  endif()
  install(TARGETS caffe2_hip EXPORT Caffe2Targets DESTINATION lib)
endif()

# ---[ Test binaries.
if (BUILD_TEST)
  set(Caffe2_ALL_TEST_SRCS ${Caffe2_CPU_TEST_SRCS})
  if (USE_CUDA)
    list(APPEND Caffe2_ALL_TEST_SRCS ${Caffe2_GPU_TEST_SRCS})
  endif()

  if (USE_HIP)
    set(Caffe2_HIP_GCC_COMPILE_FLAGS "-D__HIP_PLATFORM_HCC__=1")
    set(Caffe2_HIP_GCC_COMPILE_FLAGS "${Caffe2_HIP_GCC_COMPILE_FLAGS} -I${Caffe2_ROCBLAS_DIR}/include")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Caffe2_HIP_GCC_COMPILE_FLAGS}")
    INCLUDE_DIRECTORIES(${HIP_PATH}/include)
    foreach(test_src ${Caffe2_ALL_TEST_SRCS})
      get_filename_component(test_name ${test_src} NAME_WE)
      add_executable(${test_name} "${test_src}")
      add_dependencies(${test_name} ${Caffe2_MAIN_LIBS_ORDER})
      target_link_libraries(${test_name} ${Caffe2_MAIN_LIBS} ${Caffe2_DEPENDENCY_LIBS} gtest_main)
      if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 3.0)
        target_compile_features(${test_name} PRIVATE cxx_range_for)
      endif()
      add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
      install(TARGETS ${test_name} DESTINATION ${CMAKE_INSTALL_PREFIX}/test)
    endforeach()
    foreach(test_src ${Caffe2_HIP_TEST_SRCS})
      get_filename_component(test_name ${test_src} NAME_WE)
      add_executable(${test_name} "${test_src}")
      add_dependencies(${test_name} ${Caffe2_MAIN_LIBS_ORDER})
      target_link_libraries(${test_name} ${Caffe2_MAIN_LIBS} ${Caffe2_DEPENDENCY_LIBS} ${Caffe2_HIP_DEPENDENCY_LIBS} gtest_main)
      if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 3.0)
        target_compile_features(${test_name} PRIVATE cxx_range_for)
      endif()
      add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
      install(TARGETS ${test_name} DESTINATION ${CMAKE_INSTALL_PREFIX}/test)
    endforeach()
  else()
  foreach(test_src ${Caffe2_ALL_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} "${test_src}")
    add_dependencies(${test_name} ${Caffe2_MAIN_LIBS_ORDER})
    if (USE_CUDA)
      target_link_libraries(
          ${test_name} ${Caffe2_MAIN_LIBS} ${Caffe2_DEPENDENCY_LIBS}
          ${Caffe2_CUDA_DEPENDENCY_LIBS} gtest_main)
    else()
      target_link_libraries(
          ${test_name} ${Caffe2_MAIN_LIBS} ${Caffe2_DEPENDENCY_LIBS}
          gtest_main)
    endif()
    if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 3.0)
      target_compile_features(${test_name} PRIVATE cxx_range_for)
    endif()
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    install(TARGETS ${test_name} DESTINATION test)
  endforeach()
  endif()
endif()


if (BUILD_PYTHON)
  # ---[ Python.
  add_library(caffe2_pybind11_state MODULE ${Caffe2_CPU_PYTHON_SRCS})
  set_target_properties(caffe2_pybind11_state PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
  add_dependencies(caffe2_pybind11_state ${Caffe2_MAIN_LIBS_ORDER})
  set_target_properties(caffe2_pybind11_state PROPERTIES PREFIX "")
  if (APPLE)
    set_target_properties(caffe2_pybind11_state PROPERTIES SUFFIX ".so")
    set_target_properties(caffe2_pybind11_state PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  elseif (MSVC)
    set_target_properties(caffe2_pybind11_state PROPERTIES SUFFIX ".pyd")
  endif()
  set_target_properties(
      caffe2_pybind11_state PROPERTIES LIBRARY_OUTPUT_DIRECTORY
      ${CMAKE_BINARY_DIR}/caffe2/python)
  target_link_libraries(
      caffe2_pybind11_state ${Caffe2_CPU_LINK} ${Caffe2_DEPENDENCY_LIBS}
      ${Caffe2_PYTHON_DEPENDENCY_LIBS})
  install(TARGETS caffe2_pybind11_state DESTINATION caffe2/python)

  if(USE_CUDA)
    add_library(caffe2_pybind11_state_gpu MODULE ${Caffe2_GPU_PYTHON_SRCS})
    set_target_properties(caffe2_pybind11_state_gpu PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
    add_dependencies(caffe2_pybind11_state_gpu ${Caffe2_MAIN_LIBS_ORDER})
    set_target_properties(caffe2_pybind11_state_gpu PROPERTIES PREFIX "")
    if (APPLE)
      set_target_properties(caffe2_pybind11_state_gpu PROPERTIES SUFFIX ".so")
      set_target_properties(caffe2_pybind11_state_gpu PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    elseif (MSVC)
      set_target_properties(caffe2_pybind11_state_gpu PROPERTIES SUFFIX ".pyd")
    endif()
    set_target_properties(
        caffe2_pybind11_state_gpu PROPERTIES LIBRARY_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/caffe2/python)
    target_link_libraries(
        caffe2_pybind11_state_gpu ${Caffe2_CPU_LINK} ${Caffe2_GPU_LINK} ${Caffe2_DEPENDENCY_LIBS}
        ${Caffe2_CUDA_DEPENDENCY_LIBS} ${Caffe2_PYTHON_DEPENDENCY_LIBS})
    install(TARGETS caffe2_pybind11_state_gpu DESTINATION caffe2/python)
  endif()

  if(USE_HIP)
    add_library(caffe2_pybind11_state_hip MODULE ${Caffe2_HIP_PYTHON_SRCS})
    set_target_properties(caffe2_pybind11_state_hip PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
    add_dependencies(caffe2_pybind11_state_hip ${Caffe2_MAIN_LIBS_ORDER})
    set_target_properties(caffe2_pybind11_state_hip PROPERTIES PREFIX "")
    if (APPLE)
      set_target_properties(caffe2_pybind11_state_hip PROPERTIES SUFFIX ".so")
      set_target_properties(caffe2_pybind11_state_hip PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    elseif (MSVC)
      set_target_properties(caffe2_pybind11_state_hip PROPERTIES SUFFIX ".pyd")
    endif()
    set_target_properties(
        caffe2_pybind11_state_hip PROPERTIES LIBRARY_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/caffe2/python)
    target_link_libraries(
        caffe2_pybind11_state_hip ${Caffe2_CPU_LINK} ${Caffe2_HIP_LINK} ${Caffe2_DEPENDENCY_LIBS}
        ${Caffe2_HIP_DEPENDENCY_LIBS} ${Caffe2_PYTHON_DEPENDENCY_LIBS})
    install(TARGETS caffe2_pybind11_state_hip DESTINATION caffe2/python)
  endif()

  if (MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio")
    # If we are building under windows, we will copy the file from
    # build/caffe2/python/{Debug,Release}/caffe2_pybind11_state.pyd
    # to its parent folder so that we can do in-build execution.
    add_custom_target(windows_python_copy_lib ALL)
    add_dependencies(windows_python_copy_lib caffe2_pybind11_state)
    add_custom_command(
        TARGET windows_python_copy_lib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:caffe2_pybind11_state>
        ${CMAKE_BINARY_DIR}/caffe2/python)
    if (USE_CUDA)
      add_dependencies(windows_python_copy_lib caffe2_pybind11_state_gpu)
      add_custom_command(
          TARGET windows_python_copy_lib POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
          $<TARGET_FILE:caffe2_pybind11_state_gpu>
          ${CMAKE_BINARY_DIR}/caffe2/python)
    endif()
  endif()

  # Finally, Copy all python files to build directory
  # Generate and create all needed __init__.py files, if they aren't already
  # present in the current source tree.
  message(STATUS "Automatically generating missing __init__.py files.")
  caffe_autogen_init_py_files()

  # Create a custom target that copies all python files.
  file(GLOB_RECURSE PYTHON_SRCS RELATIVE ${PROJECT_SOURCE_DIR}
       "${PROJECT_SOURCE_DIR}/caffe2/*.py")
  add_custom_target(python_copy_files ALL)
  if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
    # ninja fails when the command line is too long so we split
    # the target into several. This would be beneficial for VS also
    # since it build targets in parallel but not custom commands
    foreach(python_src ${PYTHON_SRCS})
      get_filename_component(dir ${python_src} DIRECTORY)
      string(SHA1 name_hash "${python_src}")
      # get_filename_component(name_we ${python_src} NAME_WE)
      add_custom_target(python_copy_files_${name_hash}
          COMMAND ${CMAKE_COMMAND} -E copy
          ${PROJECT_SOURCE_DIR}/${python_src} ${CMAKE_BINARY_DIR}/${dir})
      add_dependencies(python_copy_files python_copy_files_${name_hash})
    endforeach()
  else()
    foreach(python_src ${PYTHON_SRCS})
      get_filename_component(dir ${python_src} DIRECTORY)
      add_custom_command(
          TARGET python_copy_files PRE_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
          ${PROJECT_SOURCE_DIR}/${python_src} ${CMAKE_BINARY_DIR}/${dir})
    endforeach()
  endif()

  # Install commands
  # Pick up static python files
  install(DIRECTORY ${CMAKE_BINARY_DIR}/caffe2 DESTINATION .
          FILES_MATCHING PATTERN "*.py")
  # Caffe proto files
  install(DIRECTORY ${CMAKE_BINARY_DIR}/caffe DESTINATION .
          FILES_MATCHING PATTERN "*.py")
  # Caffe2 proto files
  install(DIRECTORY ${CMAKE_BINARY_DIR}/caffe2 DESTINATION .
          FILES_MATCHING PATTERN "*.py")
endif()


# ---[ Any builds that should happen after the main targets should be added here.
# Binaries
if (BUILD_BINARY)
  add_subdirectory(binaries)
  # There is a linking issue that happens in some of the Windows builds.
  # TODO(Yangqing): after the module redesing, enable this back.
  if (NOT MSVC AND BUILD_SHARE_DIR)
    add_subdirectory(share/contrib/binaries)
  endif()
endif()
