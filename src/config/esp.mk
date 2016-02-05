DIR := user/config/
SOURCES := config.c

TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(TEMP)
