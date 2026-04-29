set(ENGINE_DIR ${PROJECT_SOURCE_DIR}/engine)
set(ENGINE_INCLUDE_DIR ${ENGINE_DIR}/include)
set(THIRD_PARTY_DIR ${ENGINE_DIR}/thirdparty)
set(ENGINE_RESOURCE_DIR ${ENGINE_DIR}/resource)
set(ENGINE_ASSET_DIR ${ENGINE_DIR}/asset)
set(ENGINE_SRC_DIR ${ENGINE_DIR}/src)

add_subdirectory(thirdparty)
add_subdirectory(src)