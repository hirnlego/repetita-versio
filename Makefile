# Project Name
TARGET ?= RepetitaVersio

DEBUG = 1
OPT = -O0
#OPT = -O3

# Sources
CPP_SOURCES = wreath/looper.cpp repetita.cpp
C_INCLUDES = -Iwreath/DaisySP/Source -Iwreath/libDaisy/src -Iwreath

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

USE_FATFS = 1

# Library Locations
LIBDAISY_DIR = wreath/libDaisy
DAISYSP_DIR = wreath/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
