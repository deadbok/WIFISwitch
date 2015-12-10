DIR := user/net/
SOURCES := capport.c net.c tcp.c udp.c websocket.c wifi.c

NET_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(NET_TEMP)
