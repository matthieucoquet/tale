# Find physx lib
# -------
# Import physx::physx

set(physx_root ${PROJECT_SOURCE_DIR}/externals/PhysX/physx/install/vc17win64/PhysX)
get_filename_component(physx_bin_path "${physx_root}/bin/win.x86_64.vc143.md" ABSOLUTE)
set(physx_debug_path "${physx_bin_path}/debug")
set(physx_release_path "${physx_bin_path}/release")

function(find_physx_static_library lib_name lib_target lib_filename)
    find_library(${lib_name}_LIBRARY_RELEASE
        NAMES ${lib_filename}
        PATHS ${physx_release_path}
        NO_CMAKE_PATH
    )    
    find_library(${lib_name}_LIBRARY_DEBUG
        NAMES ${lib_filename}
        PATHS ${physx_debug_path}
        NO_CMAKE_PATH
    )

    add_library(${lib_target} STATIC IMPORTED)
    set_target_properties(${lib_target}
        PROPERTIES
            IMPORTED_CONFIGURATIONS "RELEASE"
            IMPORTED_LOCATION_RELEASE "${${lib_name}_LIBRARY_RELEASE}"         
            IMPORTED_LOCATION_DEBUG "${${lib_name}_LIBRARY_DEBUG}"
            INTERFACE_INCLUDE_DIRECTORIES "${physx_INCLUDE_DIR}"
            MAP_IMPORTED_CONFIG_MINSIZEREL Release
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    )

    if(NOT ${lib_target} STREQUAL "physx::physx")
        set_property(TARGET physx::physx APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES ${lib_target}
        )
    endif()
endfunction()

find_path(physx_INCLUDE_DIR
    NAMES PxPhysics.h
    PATHS ${physx_root}/include
    NO_CMAKE_PATH
)

find_physx_static_library("physx" "physx::physx" "PhysX_static_64")
find_physx_static_library("physx_common" "physx::common" "PhysXCommon_static_64")
find_physx_static_library("physx_foundation" "physx::foundation" "PhysXFoundation_static_64")
find_physx_static_library("physx_extension" "physx::extension" "PhysXExtensions_static_64")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(physx
    FOUND_VAR physx_FOUND
    REQUIRED_VARS
    physx_LIBRARY_RELEASE
    physx_LIBRARY_DEBUG
    physx_INCLUDE_DIR
)
