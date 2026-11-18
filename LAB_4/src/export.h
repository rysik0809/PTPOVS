#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(LAB3_BUILD_SHARED)
#    define LAB3_API __declspec(dllexport)
#  elif defined(LAB3_USE_SHARED)
#    define LAB3_API __declspec(dllimport)
#  else
#    define LAB3_API
#  endif
#else
#  if defined(LAB3_BUILD_SHARED)
#    define LAB3_API __attribute__((visibility("default")))
#  else
#    define LAB3_API
#  endif
#endif