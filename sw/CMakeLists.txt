cmake_minimum_required(VERSION 2.8.8)
set(PROJECT_NAME_STR "pumpensteuerung")
project(${PROJECT_NAME_STR})

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Project settings
set(MCU "atmega8")
# Clock frequency.
# Possible values are 8000000UL, 4000000UL, 2000000UL, 1000000UL
set(F_CPU "8000000UL")
set(BAURAT "9600")

set(COMMON_FLAGS
  "-Wall -Werror -Wno-unused-variable -Wno-unused-function -mmcu=${MCU} -Os -ffunction-sections -fdata-sections"
  )

set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=c11 -Wfatal-errors" )
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++17 -Wfatal-errors")

set(CMAKE_C_COMPILER "avr-gcc")
set(CMAKE_CXX_COMPILER "avr-g++")
# --gc-sections will allow the linker to remove unused sections
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--cref,--gc-sections -static")
set(OBJCOPY avr-objcopy)
set(SIZEPROG avr-size)
set(TERMPROG picocom)
set(TERMPORT /dev/ttyUSB0)

set(AVRDUDE avrdude)
set(AVRDUDE_PROGRAMMER usbasp)
set(AVRDUDE_FLAGS -p ${MCU} -c ${AVRDUDE_PROGRAMMER})
set(AVRDUDE_WRITE_FLASH -U flash:w:${PROJECT_NAME_STR}.hex)
# configure fuses for ATMEGA8/ATMEGA32 internal oscillator:
if("${F_CPU}" EQUAL "8000000UL")
  set(FUSE_LOW 0b11100010:m)
elseif ("${F_CPU}" EQUAL "1000000UL")
  set(FUSE_LOW 0b11100010:m)
else()
  message("error invalid F_CPU settings")
endif()
set(AVRDUDE_WRITE_FUSES -U lfuse:w:${FUSE_LOW})

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBRARIES OFF)
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

add_definitions(-DF_CPU=${F_CPU} )

find_path(GENERATOR_PATH gen.pl
  HINTS bspgen
  NO_DEFAULT_PATH
  )
message("GENERATOR PATH is ${GENERATOR_PATH}")


add_custom_target(build-gen-directory ${CMAKE_COMMAND} -E make_directory gen
    VERBATIM
  )

add_custom_command(
  OUTPUT gen/bsp_io.c
  COMMAND perl ${GENERATOR_PATH}/gen.pl ${PROJECT_SOURCE_DIR}/bsp_io_pins.txt bsp_io
  COMMENT "Running generator"
  DEPENDS build-gen-directory
  WORKING_DIRECTORY gen
  VERBATIM
)

include_directories(${CMAKE_BINARY_DIR}/gen)

add_library(
  ${PROJECT_NAME_STR}_generated STATIC
  gen/bsp_io.c
  )

set(FORMAT ihex)
file(GLOB ${PROJECT_NAME_STR}_C_SOURCE *.c)
file(GLOB ${PROJECT_NAME_STR}_CXX_SOURCE *.cpp)
message("SOURCE is ${${PROJECT_NAME_STR}_C_SOURCE}
  ${${PROJECT_NAME_STR}_CXX_SOURCE}
  ")
add_executable(
  ${PROJECT_NAME_STR}.elf
  ${${PROJECT_NAME_STR}_C_SOURCE}
  ${${PROJECT_NAME_STR}_CXX_SOURCE}
  )
if(GEN_MAP)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map,${PROJECT_NAME_STR}.map")
endif(GEN_MAP)
#add_dependencies(${PROJECT_NAME_STR}.elf bsp)

target_link_libraries(${PROJECT_NAME_STR}.elf ${PROJECT_NAME_STR}_generated)

add_custom_target(elf2hex ALL ${OBJCOPY} -O ${FORMAT} -R .eeprom ${PROJECT_NAME_STR}.elf ${PROJECT_NAME_STR}.hex
  DEPENDS  ${PROJECT_NAME_STR}.elf
  VERBATIM 
  )
add_custom_target(upload ${AVRDUDE} ${AVRDUDE_FLAGS} ${AVRDUDE_WRITE_FLASH}
  VERBATIM 
  )
add_custom_target(fuse 
	${AVRDUDE} ${AVRDUDE_FLAGS} ${AVRDUDE_WRITE_FUSES}
    COMMENT "lfuse: ${FUSE_LOW}"
    VERBATIM
  )
add_custom_target(size ${SIZEPROG} ${PROJECT_NAME_STR}.elf
    VERBATIM
)
add_custom_target(term ${TERMPROG} -b ${BAURAT} ${TERMPORT}
  VERBATIM
)
