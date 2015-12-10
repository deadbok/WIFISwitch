DIR := user/handlers/http/deny/
SOURCES := http-deny.c

HANDLERS_DENY_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(HANDLERS_DENY_TEMP)
