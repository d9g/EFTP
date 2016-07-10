cmake_minimum_required(VERSION 3.5)
project(EFTP-Client)

set(SOURCE_FILES eftp-client.c eftp-client.h main.c ../EFTP-Lib/eftplib.c ../EFTP-Lib/eftplib.h)
include_directories(../EFTP-Lib)
add_executable(EFTP-Client ${SOURCE_FILES})
