DIR := user/net/
SOURCES := capport.c net-task.c net.c tcp.c udp.c websocket.c wifi.c

C_SRC := $(C_SRC) $(addprefix $(DIR), $(SOURCES))
