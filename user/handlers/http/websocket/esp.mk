DIR := user/handlers/http/websocket/
SOURCES := websocket.c

HANDLERS_WS_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(HANDLERS_WS_TEMP)
