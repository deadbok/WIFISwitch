DIR := user/tools/
SOURCES := base64.c ftoa.c itoa.c jsmn.c json-gen.c ring.c sha1.c strxtra.c

TOOLS_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(TOOLS_TEMP)
