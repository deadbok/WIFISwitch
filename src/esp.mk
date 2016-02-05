# All esp.mk files are read be the main Makefile, to gather the
# sources.
# Current directory.
DIR := user/
# Source files.
SOURCES := debug.c task.c user_main.c

# Variable nam must be unique.
ROOT_TEMP := $(addprefix $(DIR), $(SOURCES))
# Add sources to global Makefile variable.
C_SRC += $(ROOT_TEMP)
