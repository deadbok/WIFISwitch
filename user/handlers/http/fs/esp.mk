DIR := user/handlers/http/fs/
SOURCES := http-fs.c

HANDLERS_FS_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(HANDLERS_FS_TEMP)