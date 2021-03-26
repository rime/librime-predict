project(rime-predict)
cmake_minimum_required(VERSION 3.10)

aux_source_directory(src predict_src)

add_library(rime-predict-objs OBJECT ${predict_src})
if(BUILD_SHARED_LIBS)
  set_target_properties(rime-predict-objs
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
endif()

set(plugin_name rime-predict PARENT_SCOPE)
set(plugin_objs $<TARGET_OBJECTS:rime-predict-objs> PARENT_SCOPE)
set(plugin_deps ${rime_library} PARENT_SCOPE)
set(plugin_modules "predict" PARENT_SCOPE)

add_subdirectory(tools)
