set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/thirdparty)
set(third_party_folder "ThirdParty")

#glad
file(GLOB_RECURSE GLAD_HEAD_FILES "${THIRD_PARTY_DIR}/glad/*.h" "${THIRD_PARTY_DIR}/glad/*.hpp")
file(GLOB_RECURSE GLAD_SRC_FILES "${THIRD_PARTY_DIR}/glad/*.c" "${THIRD_PARTY_DIR}/glad/*.cpp")
add_library(glad ${GLAD_HEAD_FILES} ${GLAD_SRC_FILES})
set_target_properties(glad PROPERTIES FOLDER ${third_party_folder}/glad)
target_include_directories(glad PUBLIC ${THIRD_PARTY_DIR}/glad/include)
	
#imgui
file(GLOB_RECURSE IMGUI_HEAD_FILES "${THIRD_PARTY_DIR}/imgui/*.h" "${THIRD_PARTY_DIR}/imgui/*.hpp")
file(GLOB_RECURSE IMGUI_SRC_FILES "${THIRD_PARTY_DIR}/imgui/*.c" "${THIRD_PARTY_DIR}/imgui/*.cpp")
add_library(imgui ${IMGUI_HEAD_FILES} ${IMGUI_SRC_FILES})
set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
target_include_directories(imgui PUBLIC ${THIRD_PARTY_DIR}/imgui)
target_include_directories(imgui PRIVATE ${THIRD_PARTY_DIR}/glfw/include)

#stb
file(GLOB_RECURSE STB_HEAD_FILES "${THIRD_PARTY_DIR}/stb/*.h" "${THIRD_PARTY_DIR}/stb/*.hpp")
file(GLOB_RECURSE STB_SRC_FILES "${THIRD_PARTY_DIR}/stb/*.c" "${THIRD_PARTY_DIR}/stb/*.cpp")
add_library(stb ${STB_HEAD_FILES} ${STB_SRC_FILES})
set_target_properties(stb PROPERTIES FOLDER ${third_party_folder}/stb)
target_include_directories(stb PUBLIC ${THIRD_PARTY_DIR}/stb)

#json11
add_library(json11 ${THIRD_PARTY_DIR}/json11/json11.hpp ${THIRD_PARTY_DIR}/json11/json11.cpp)
set_target_properties(json11 PROPERTIES FOLDER ${third_party_folder}/json11)
target_include_directories(json11 PUBLIC ${THIRD_PARTY_DIR}/json11)

#TinyReflection
if(NOT TARGET TinyReflection)
	add_subdirectory(TinyReflection)
	set_target_properties(TinyReflection PROPERTIES FOLDER ${third_party_folder}/TinyReflection)
endif()

#spdlog
if(NOT TARGET spdlog)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER ${third_party_folder}/spdlog)
endif()

#glm
if(NOT TARGET glm)
	option(BUILD_STATIC_LIBS "" ON)
	add_subdirectory(glm)
	set_target_properties(glm_static PROPERTIES FOLDER ${third_party_folder}/glm)
endif()

#glfw
if(NOT TARGET glfw)
	option(GLFW_BUILD_EXAMPLES "" OFF)
	option(GLFW_BUILD_TESTS "" OFF)
	option(GLFW_BUILD_DOCS "" OFF)
	option(GLFW_INSTALL "" OFF)
	add_subdirectory(glfw)
	set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder}/glfw)
	set_target_properties(update_mappings PROPERTIES FOLDER ${third_party_folder}/glfw)
endif()

#assimp
if(NOT TARGET assimp)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
	set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
	add_subdirectory(assimp)
	if(TARGET assimp)
		set_target_properties(assimp PROPERTIES DEBUG_POSTFIX "d")
		set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder}/assimp)
		target_include_directories(assimp PUBLIC ${THIRD_PARTY_DIR}/assimp/include)
	endif()
	if(TARGET uninstall)
		set_target_properties(uninstall PROPERTIES FOLDER ${third_party_folder}/assimp)
	endif()
	if(MSVC AND TARGET UpdateAssimpLibsDebugSymbolsAndDLLs)
		set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ${third_party_folder}/assimp)
	endif()
	#set_target_properties(zlibstatic PROPERTIES FOLDER ${third_party_folder}/assimp)
endif()
