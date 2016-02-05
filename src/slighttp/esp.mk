DIR := user/slighttp/
SOURCES := http.c http-common.c http-handler.c http-mime.c http-request.c \
		   http-response.c http-tcp.c

SLIGHTTP_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(SLIGHTTP_TEMP)
