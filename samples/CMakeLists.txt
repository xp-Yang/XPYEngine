set(SAMPLES_EXEC_NAME SAMPLES)

file(GLOB_RECURSE HEAD_FILES "*.hpp" "*.h")
file(GLOB_RECURSE SRC_FILES "*.cpp" "*.c")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} ${HEAD_FILES})
#增加HEAD_FILES以便ide可以显示这些头文件
add_executable(${SAMPLES_EXEC_NAME}
    ${HEAD_FILES}
    ${SRC_FILES})

#设为vs默认启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${SAMPLES_EXEC_NAME})
#设为作为窗口启动，能去掉控制台
#set_target_properties(${SAMPLES_EXEC_NAME} PROPERTIES
#    WIN32_EXECUTABLE ON
#    MACOSX_BUNDLE ON
#)

target_link_libraries(${SAMPLES_EXEC_NAME} PUBLIC EngineRuntime)

target_compile_features(${SAMPLES_EXEC_NAME} PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(${SAMPLES_EXEC_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()
