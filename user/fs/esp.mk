DIR := user/fs/
SOURCES := dbffs.c fs.c int_flash.c

FS_TEMP := $(addprefix $(DIR), $(SOURCES))
C_SRC += $(FS_TEMP)
