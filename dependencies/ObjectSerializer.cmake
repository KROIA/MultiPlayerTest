## description: Library to save and load objects to/from a binary file
include(FetchContent)

function(dep LIBRARY_MACRO_NAME SHARED_LIB STATIC_LIB STATIC_PROFILE_LIB INCLUDE_PATHS)
    # Define the git repository and tag to download from
    set(LIB_NAME ObjectSerializer)								# Change this line
    set(LIB_MACRO_NAME OBJECT_SERIALIZER_LIBRARY_AVAILABLE)		# Change this line
    set(GIT_REPO https://github.com/KROIA/ObjectSerializer.git)	# Change this line
    set(GIT_TAG main)										# Change this line

    FetchContent_Declare(
        ${LIB_NAME}
        GIT_REPOSITORY ${GIT_REPO}
        GIT_TAG        ${GIT_TAG}
    )

    set(${LIB_NAME}_NO_EXAMPLES True)						# Disables the examlpes of the library
    set(${LIB_NAME}_NO_UNITTESTS True)						# Disables the unittests of the library
    
    message("Downloading dependency: ${LIB_NAME} from: ${GIT_REPO} tag: ${GIT_TAG}")
    FetchContent_MakeAvailable(${LIB_NAME})

    # Add this library to the specific profiles of this project
    list(APPEND DEPS_FOR_SHARED_LIB ${LIB_NAME}_shared)
    list(APPEND DEPS_FOR_STATIC_LIB ${LIB_NAME}_static)
    list(APPEND DEPS_FOR_STATIC_PROFILE_LIB ${LIB_NAME}_static_profile) # only use for static profiling profile

    # Add additional include paths
    #  string(TOLOWER ${LIB_NAME} LIB_NAME_LOWER)
    #  list(APPEND INCLUDES ${${LIB_NAME_LOWER}_SOURCE_DIR}/include)
    list(APPEND INCLUDES )

    set(${LIBRARY_MACRO_NAME} "${${LIBRARY_MACRO_NAME}};${LIB_MACRO_NAME}" PARENT_SCOPE)
    set(${SHARED_LIB} "${${SHARED_LIB}};${DEPS_FOR_SHARED_LIB}" PARENT_SCOPE)
    set(${STATIC_LIB} "${${STATIC_LIB}};${DEPS_FOR_STATIC_LIB}" PARENT_SCOPE)
    set(${STATIC_PROFILE_LIB} "${${STATIC_PROFILE_LIB}};${DEPS_FOR_STATIC_PROFILE_LIB}" PARENT_SCOPE)
    set(${INCLUDE_PATHS} "${${INCLUDE_PATHS}};${INCLUDES}" PARENT_SCOPE)
endfunction()

dep(DEPENDENCY_NAME_MACRO
    DEPENDENCIES_FOR_SHARED_LIB 
    DEPENDENCIES_FOR_STATIC_LIB 
    DEPENDENCIES_FOR_STATIC_PROFILE_LIB 
    DEPENDENCIES_INCLUDE_PATHS)
