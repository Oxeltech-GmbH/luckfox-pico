#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cppnetlib-uri" for configuration "Release"
set_property(TARGET cppnetlib-uri APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cppnetlib-uri PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "/home/basit/arm_build/usr/local/lib/libcppnetlib-uri.a"
  )

list(APPEND _cmake_import_check_targets cppnetlib-uri )
list(APPEND _cmake_import_check_files_for_cppnetlib-uri "/home/basit/arm_build/usr/local/lib/libcppnetlib-uri.a" )

# Import target "cppnetlib-server-parsers" for configuration "Release"
set_property(TARGET cppnetlib-server-parsers APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cppnetlib-server-parsers PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "/home/basit/arm_build/usr/local/lib/libcppnetlib-server-parsers.a"
  )

list(APPEND _cmake_import_check_targets cppnetlib-server-parsers )
list(APPEND _cmake_import_check_files_for_cppnetlib-server-parsers "/home/basit/arm_build/usr/local/lib/libcppnetlib-server-parsers.a" )

# Import target "cppnetlib-client-connections" for configuration "Release"
set_property(TARGET cppnetlib-client-connections APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cppnetlib-client-connections PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "/home/basit/arm_build/usr/local/lib/libboost_system.so;/home/basit/arm_build/usr/local/lib/libboost_thread.so;/home/basit/arm_build/usr/local/lib/libboost_chrono.so;/home/basit/arm_build/usr/local/lib/libboost_date_time.so;/home/basit/arm_build/usr/local/lib/libboost_atomic.so;/home/basit/arm_build/usr/local/lib/libssl.so;/home/basit/arm_build/usr/local/lib/libcrypto.so"
  IMPORTED_LOCATION_RELEASE "/home/basit/arm_build/usr/local/lib/libcppnetlib-client-connections.a"
  )

list(APPEND _cmake_import_check_targets cppnetlib-client-connections )
list(APPEND _cmake_import_check_files_for_cppnetlib-client-connections "/home/basit/arm_build/usr/local/lib/libcppnetlib-client-connections.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
