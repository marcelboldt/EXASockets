//
// Created by mbo on 28.09.2016.
//

#ifndef EXASOCKETS_OSNAME_H
#define EXASOCKETS_OSNAME_H

#ifdef __linux__
  #define OS_NAME "Linux"
#elif defined __APPLE__
  #define OS_NAME "MaxOS X"
#elif defined _WIN32_
  #ifdef _WIN64
    #define OS_NAME "WIN64"
  #else
    #define OS_NAME "WIN32"
  #endif
#else
#define OS_NAME "unknown"
#endif

#endif //EXASOCKETS_OSNAME_H
