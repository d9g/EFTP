cmake_minimum_required(VERSION 3.5)
project(EFTP-Server)

set(SOURCE_FILES main.c eftp-server.c eftp-server.h ../EFTP-Lib/eftplib.c ../EFTP-Lib/eftplib.h)
include_directories(../EFTP-Lib)
add_executable(EFTP-Server ${SOURCE_FILES})
