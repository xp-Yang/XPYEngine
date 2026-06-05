set(LIB_NAME EngineRuntime)

set(BASE_INCLUDE_DIR ${ENGINE_SRC_DIR}/Base/include)
set(ASSET_INCLUDE_DIR ${ENGINE_SRC_DIR}/AssetManager/include)
set(LOGICAL_INCLUDE_DIR ${ENGINE_SRC_DIR}/Logical/include)
set(GUI_INCLUDE_DIR ${ENGINE_SRC_DIR}/GUI/include)

file(GLOB_RECURSE SRC_FILES "${ENGINE_SRC_DIR}/*.cpp" "${ENGINE_SRC_DIR}/*.c")
if(APPLE)
	file(GLOB_RECURSE OBJCXX_SRC_FILES "${ENGINE_SRC_DIR}/*.mm")
	list(APPEND SRC_FILES ${OBJCXX_SRC_FILES})
endif()
file(GLOB_RECURSE HEAD_FILES "${ENGINE_SRC_DIR}/*.hpp" "${ENGINE_SRC_DIR}/*.h" "${ENGINE_INCLUDE_DIR}/*.hpp" "${ENGINE_INCLUDE_DIR}/*.h")
file(GLOB_RECURSE ASSET_FILES "${ENGINE_ASSET_DIR}/*.*")

if(NOT WIN32)
	list(FILTER SRC_FILES EXCLUDE REGEX ".*/Platform/Windows/.*")
endif()
if(NOT APPLE)
	list(FILTER SRC_FILES EXCLUDE REGEX ".*/Platform/Mac/.*")
endif()

#使ide呈现文件夹层级结构
#Note: 只有add_library/add_executable中的文件才会被source_group分组
source_group(TREE "${ENGINE_DIR}" FILES ${SRC_FILES} ${HEAD_FILES} ${ASSET_FILES})
#下面这种方式不会形成树形结构
#source_group("" FILES ${SRC_FILES} ${HEAD_FILES})

#增加所有文件以便ide可以显示这些文件
add_library(${LIB_NAME} ${SRC_FILES} ${HEAD_FILES})
#hack 通过这种方式使ide显示asset文件，否则asset中的.obj文件被错误地当成目标文件链接
set_source_files_properties(${ASSET_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
target_sources(${LIB_NAME} PRIVATE ${ASSET_FILES})

target_include_directories(${LIB_NAME}
	PUBLIC ${ENGINE_INCLUDE_DIR}
	PUBLIC ${BASE_INCLUDE_DIR}
	PUBLIC ${ASSET_INCLUDE_DIR}
	PUBLIC ${LOGICAL_INCLUDE_DIR}
	PUBLIC ${GUI_INCLUDE_DIR}
	PRIVATE ${ENGINE_SRC_DIR})

target_link_libraries(${LIB_NAME} 
	PRIVATE glad
	PRIVATE stb
	PUBLIC imgui
	PUBLIC spdlog
	PUBLIC json11
	PUBLIC glm
	PUBLIC TinyReflection
	PRIVATE glfw
	PRIVATE assimp)

if(APPLE)
	target_link_libraries(${LIB_NAME} PRIVATE "-framework AppKit")
	target_link_libraries(${LIB_NAME} PRIVATE "-framework UniformTypeIdentifiers")
endif()

#通过宏定义告诉程序asset目录
target_compile_definitions(EngineRuntime PUBLIC ASSET_DIR="${ENGINE_ASSET_DIR}")
target_compile_definitions(EngineRuntime PUBLIC ENABLE_ECS=false)
target_compile_definitions(EngineRuntime PUBLIC NO_TITLE_BAR=false)
target_compile_features(${LIB_NAME} PRIVATE cxx_std_17)

if(MSVC)
	target_compile_options(${LIB_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()
