BOARDS_TXT = npboard.txt
BOARD_TAG    = nixiepipe
ISP_PROG = usbasp

#MCU = atmega328p
# F_CPU = 16000000
# MONITOR_PORT = /dev/tty.SLAB*

ARDUINO_LIBS = Time DS3232RTC Wire FastLED NixiePipe
AVRDUDE_OPTS = -V

# compiler flags for FastLED lib
CXXFLAGS_STD = -std=gnu++11
CXXFLAGS += -fno-threadsafe-statics


# ARDUINO_QUIET = 1

include ${ARDMK_DIR}/Arduino.mk
