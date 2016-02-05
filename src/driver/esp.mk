DIR := user/driver/
SOURCES := button.c uart.c

DRIVER_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(DRIVER_TEMP)
