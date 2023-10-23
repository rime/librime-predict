set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

aux_source_directory(. build_predict_src)
include_directories(../src)

add_executable(build_predict
  ${build_predict_src}
  $<TARGET_OBJECTS:rime-predict-objs>)
target_link_libraries(build_predict
  ${rime_library}
  ${rime_dict_library})
