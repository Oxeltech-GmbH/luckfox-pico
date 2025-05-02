#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "drdynvc-client" for configuration "Release"
set_property(TARGET drdynvc-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(drdynvc-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libdrdynvc-client.a"
  )

list(APPEND _cmake_import_check_targets drdynvc-client )
list(APPEND _cmake_import_check_files_for_drdynvc-client "${_IMPORT_PREFIX}/lib/freerdp2/libdrdynvc-client.a" )

# Import target "video-client" for configuration "Release"
set_property(TARGET video-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(video-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libvideo-client.a"
  )

list(APPEND _cmake_import_check_targets video-client )
list(APPEND _cmake_import_check_files_for_video-client "${_IMPORT_PREFIX}/lib/freerdp2/libvideo-client.a" )

# Import target "urbdrc-client" for configuration "Release"
set_property(TARGET urbdrc-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(urbdrc-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/liburbdrc-client.a"
  )

list(APPEND _cmake_import_check_targets urbdrc-client )
list(APPEND _cmake_import_check_files_for_urbdrc-client "${_IMPORT_PREFIX}/lib/freerdp2/liburbdrc-client.a" )

# Import target "urbdrc-client-libusb" for configuration "Release"
set_property(TARGET urbdrc-client-libusb APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(urbdrc-client-libusb PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "/home/basit/libsub_build/usr/lib/libusb-1.0.a;winpr;urbdrc-client"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/liburbdrc-client-libusb.a"
  )

list(APPEND _cmake_import_check_targets urbdrc-client-libusb )
list(APPEND _cmake_import_check_files_for_urbdrc-client-libusb "${_IMPORT_PREFIX}/lib/freerdp2/liburbdrc-client-libusb.a" )

# Import target "smartcard-client" for configuration "Release"
set_property(TARGET smartcard-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(smartcard-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libsmartcard-client.a"
  )

list(APPEND _cmake_import_check_targets smartcard-client )
list(APPEND _cmake_import_check_files_for_smartcard-client "${_IMPORT_PREFIX}/lib/freerdp2/libsmartcard-client.a" )

# Import target "serial-client" for configuration "Release"
set_property(TARGET serial-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(serial-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libserial-client.a"
  )

list(APPEND _cmake_import_check_targets serial-client )
list(APPEND _cmake_import_check_files_for_serial-client "${_IMPORT_PREFIX}/lib/freerdp2/libserial-client.a" )

# Import target "remdesk-client" for configuration "Release"
set_property(TARGET remdesk-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(remdesk-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libremdesk-client.a"
  )

list(APPEND _cmake_import_check_targets remdesk-client )
list(APPEND _cmake_import_check_files_for_remdesk-client "${_IMPORT_PREFIX}/lib/freerdp2/libremdesk-client.a" )

# Import target "rdpsnd-client" for configuration "Release"
set_property(TARGET rdpsnd-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rdpsnd-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librdpsnd-client.a"
  )

list(APPEND _cmake_import_check_targets rdpsnd-client )
list(APPEND _cmake_import_check_files_for_rdpsnd-client "${_IMPORT_PREFIX}/lib/freerdp2/librdpsnd-client.a" )

# Import target "rdpsnd-client-oss" for configuration "Release"
set_property(TARGET rdpsnd-client-oss APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rdpsnd-client-oss PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librdpsnd-client-oss.a"
  )

list(APPEND _cmake_import_check_targets rdpsnd-client-oss )
list(APPEND _cmake_import_check_files_for_rdpsnd-client-oss "${_IMPORT_PREFIX}/lib/freerdp2/librdpsnd-client-oss.a" )

# Import target "rdpsnd-client-fake" for configuration "Release"
set_property(TARGET rdpsnd-client-fake APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rdpsnd-client-fake PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "freerdp;winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librdpsnd-client-fake.a"
  )

list(APPEND _cmake_import_check_targets rdpsnd-client-fake )
list(APPEND _cmake_import_check_files_for_rdpsnd-client-fake "${_IMPORT_PREFIX}/lib/freerdp2/librdpsnd-client-fake.a" )

# Import target "rdpgfx-client" for configuration "Release"
set_property(TARGET rdpgfx-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rdpgfx-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librdpgfx-client.a"
  )

list(APPEND _cmake_import_check_targets rdpgfx-client )
list(APPEND _cmake_import_check_files_for_rdpgfx-client "${_IMPORT_PREFIX}/lib/freerdp2/librdpgfx-client.a" )

# Import target "rdpei-client" for configuration "Release"
set_property(TARGET rdpei-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rdpei-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librdpei-client.a"
  )

list(APPEND _cmake_import_check_targets rdpei-client )
list(APPEND _cmake_import_check_files_for_rdpei-client "${_IMPORT_PREFIX}/lib/freerdp2/librdpei-client.a" )

# Import target "rdpdr-client" for configuration "Release"
set_property(TARGET rdpdr-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rdpdr-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librdpdr-client.a"
  )

list(APPEND _cmake_import_check_targets rdpdr-client )
list(APPEND _cmake_import_check_files_for_rdpdr-client "${_IMPORT_PREFIX}/lib/freerdp2/librdpdr-client.a" )

# Import target "rdp2tcp-client" for configuration "Release"
set_property(TARGET rdp2tcp-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rdp2tcp-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librdp2tcp-client.a"
  )

list(APPEND _cmake_import_check_targets rdp2tcp-client )
list(APPEND _cmake_import_check_files_for_rdp2tcp-client "${_IMPORT_PREFIX}/lib/freerdp2/librdp2tcp-client.a" )

# Import target "rail-client" for configuration "Release"
set_property(TARGET rail-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(rail-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/librail-client.a"
  )

list(APPEND _cmake_import_check_targets rail-client )
list(APPEND _cmake_import_check_files_for_rail-client "${_IMPORT_PREFIX}/lib/freerdp2/librail-client.a" )

# Import target "parallel-client" for configuration "Release"
set_property(TARGET parallel-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(parallel-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "freerdp;winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libparallel-client.a"
  )

list(APPEND _cmake_import_check_targets parallel-client )
list(APPEND _cmake_import_check_files_for_parallel-client "${_IMPORT_PREFIX}/lib/freerdp2/libparallel-client.a" )

# Import target "geometry-client" for configuration "Release"
set_property(TARGET geometry-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(geometry-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp-client"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libgeometry-client.a"
  )

list(APPEND _cmake_import_check_targets geometry-client )
list(APPEND _cmake_import_check_files_for_geometry-client "${_IMPORT_PREFIX}/lib/freerdp2/libgeometry-client.a" )

# Import target "encomsp-client" for configuration "Release"
set_property(TARGET encomsp-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(encomsp-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libencomsp-client.a"
  )

list(APPEND _cmake_import_check_targets encomsp-client )
list(APPEND _cmake_import_check_files_for_encomsp-client "${_IMPORT_PREFIX}/lib/freerdp2/libencomsp-client.a" )

# Import target "echo-client" for configuration "Release"
set_property(TARGET echo-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(echo-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libecho-client.a"
  )

list(APPEND _cmake_import_check_targets echo-client )
list(APPEND _cmake_import_check_files_for_echo-client "${_IMPORT_PREFIX}/lib/freerdp2/libecho-client.a" )

# Import target "drive-client" for configuration "Release"
set_property(TARGET drive-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(drive-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr;freerdp"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libdrive-client.a"
  )

list(APPEND _cmake_import_check_targets drive-client )
list(APPEND _cmake_import_check_files_for_drive-client "${_IMPORT_PREFIX}/lib/freerdp2/libdrive-client.a" )

# Import target "disp-client" for configuration "Release"
set_property(TARGET disp-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(disp-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libdisp-client.a"
  )

list(APPEND _cmake_import_check_targets disp-client )
list(APPEND _cmake_import_check_files_for_disp-client "${_IMPORT_PREFIX}/lib/freerdp2/libdisp-client.a" )

# Import target "cliprdr-client" for configuration "Release"
set_property(TARGET cliprdr-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cliprdr-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libcliprdr-client.a"
  )

list(APPEND _cmake_import_check_targets cliprdr-client )
list(APPEND _cmake_import_check_files_for_cliprdr-client "${_IMPORT_PREFIX}/lib/freerdp2/libcliprdr-client.a" )

# Import target "audin-client" for configuration "Release"
set_property(TARGET audin-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(audin-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "freerdp;winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libaudin-client.a"
  )

list(APPEND _cmake_import_check_targets audin-client )
list(APPEND _cmake_import_check_files_for_audin-client "${_IMPORT_PREFIX}/lib/freerdp2/libaudin-client.a" )

# Import target "audin-client-oss" for configuration "Release"
set_property(TARGET audin-client-oss APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(audin-client-oss PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "freerdp;winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libaudin-client-oss.a"
  )

list(APPEND _cmake_import_check_targets audin-client-oss )
list(APPEND _cmake_import_check_files_for_audin-client-oss "${_IMPORT_PREFIX}/lib/freerdp2/libaudin-client-oss.a" )

# Import target "ainput-client" for configuration "Release"
set_property(TARGET ainput-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ainput-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "winpr"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/freerdp2/libainput-client.a"
  )

list(APPEND _cmake_import_check_targets ainput-client )
list(APPEND _cmake_import_check_files_for_ainput-client "${_IMPORT_PREFIX}/lib/freerdp2/libainput-client.a" )

# Import target "freerdp-client" for configuration "Release"
set_property(TARGET freerdp-client APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(freerdp-client PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libfreerdp-client2.a"
  )

list(APPEND _cmake_import_check_targets freerdp-client )
list(APPEND _cmake_import_check_files_for_freerdp-client "${_IMPORT_PREFIX}/lib/libfreerdp-client2.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
