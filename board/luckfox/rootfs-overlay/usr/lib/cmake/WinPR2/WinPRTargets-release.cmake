#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "winpr" for configuration "Release"
set_property(TARGET winpr APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(winpr PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "rt;/home/basit/arm_build/usr/local/lib/libssl.a;/home/basit/arm_build/usr/local/lib/libcrypto.a;m;dl"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libwinpr2.a"
  )

list(APPEND _cmake_import_check_targets winpr )
list(APPEND _cmake_import_check_files_for_winpr "${_IMPORT_PREFIX}/lib/libwinpr2.a" )

# Import target "winpr-tools" for configuration "Release"
set_property(TARGET winpr-tools APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(winpr-tools PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "/home/basit/arm_build/usr/local/lib/libcrypto.a;winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libwinpr-tools2.a"
  )

list(APPEND _cmake_import_check_targets winpr-tools )
list(APPEND _cmake_import_check_files_for_winpr-tools "${_IMPORT_PREFIX}/lib/libwinpr-tools2.a" )

# Import target "winpr-makecert" for configuration "Release"
set_property(TARGET winpr-makecert APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(winpr-makecert PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/winpr-makecert"
  )

list(APPEND _cmake_import_check_targets winpr-makecert )
list(APPEND _cmake_import_check_files_for_winpr-makecert "${_IMPORT_PREFIX}/bin/winpr-makecert" )

# Import target "winpr-hash" for configuration "Release"
set_property(TARGET winpr-hash APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(winpr-hash PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/winpr-hash"
  )

list(APPEND _cmake_import_check_targets winpr-hash )
list(APPEND _cmake_import_check_files_for_winpr-hash "${_IMPORT_PREFIX}/bin/winpr-hash" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
