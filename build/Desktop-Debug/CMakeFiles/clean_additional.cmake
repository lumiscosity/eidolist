# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/eidolist_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/eidolist_autogen.dir/ParseCache.txt"
  "eidolist_autogen"
  "third_party/liblcf/CMakeFiles/lcf_autogen.dir/AutogenUsed.txt"
  "third_party/liblcf/CMakeFiles/lcf_autogen.dir/ParseCache.txt"
  "third_party/liblcf/lcf_autogen"
  )
endif()
