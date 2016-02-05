DIR := user/handlers/websocket/
SOURCES := wifiswitch.c

C_SRC := $(C_SRC) $(addprefix $(DIR), $(SOURCES))
