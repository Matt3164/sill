set(TCMALLOC_NAMES ${TCMALLOC_NAMES} tcmalloc)
find_library(TCMALLOC_LIBRARY NAMES ${TCMALLOC_NAMES})

if(TCMALLOC_FOUND)
  set(TCMALLOC_LIBRARIES ${TCMALLOC_LIBRARY})
endif()

mark_as_advanced(TCMALLOC_LIBRARY)