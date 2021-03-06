include(BerkeleyDB)
include(Linking)
include(Prebuilt)

set(APR_FIND_QUIETLY ON)
set(APR_FIND_REQUIRED ON)

set(APRUTIL_FIND_QUIETLY ON)
set(APRUTIL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindAPR)
else (STANDALONE)
  use_prebuilt_binary(apr_suite)
  if (WINDOWS)
    if (LLCOMMON_LINK_SHARED)
      set(APR_selector "lib")
    else (LLCOMMON_LINK_SHARED)
      set(APR_selector "")
    endif (LLCOMMON_LINK_SHARED)
    set(APR_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/${APR_selector}apr-1.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/${APR_selector}apr-1.lib
      )
    set(APRICONV_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/${APR_selector}apriconv-1.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/${APR_selector}apriconv-1.lib
      )
    set(APRUTIL_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/${APR_selector}aprutil-1.lib ${APRICONV_LIBRARIES}
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/${APR_selector}aprutil-1.lib ${APRICONV_LIBRARIES}
      )
  elseif (DARWIN)
    if (LLCOMMON_LINK_SHARED)
      set(APR_selector     "0.3.7.dylib")
      set(APRUTIL_selector "0.3.8.dylib")
    else (LLCOMMON_LINK_SHARED)
      set(APR_selector     "a")
      set(APRUTIL_selector "a")
    endif (LLCOMMON_LINK_SHARED)
    set(APR_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libapr-1.${APR_selector}
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libapr-1.${APR_selector}
      )
    set(APRUTIL_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libaprutil-1.${APRUTIL_selector}
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libaprutil-1.${APRUTIL_selector}
      )
    set(APRICONV_LIBRARIES iconv)
  else (WINDOWS)
    set(APR_LIBRARIES apr-1)
    set(APRUTIL_LIBRARIES aprutil-1)
    set(APRICONV_LIBRARIES iconv)
  endif (WINDOWS)
  set(APR_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/apr-1)

  if (LINUX)
    list(APPEND APRUTIL_LIBRARIES ${DB_LIBRARIES} rt pthread)
  endif (LINUX)
endif (STANDALONE)
