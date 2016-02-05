DIR := user/handlers/http/rest
SOURCES :=
#SOURCES := gpio.c mem.c net-names.c net-passwd.c network.c rest.c version.c

REST_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(REST_TEMP)
