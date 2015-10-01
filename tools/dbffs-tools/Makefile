#Make file for dbffs-tools 2015-09-14 by Martin Grønholdt.

TARGET := bin/dbffs-image
SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)
DEPS := $(SOURCES:.c=.d)

INSTALL_DIR ?= ..
INSTALL_TARGET := $(INSTALL_DIR)/$(notdir $(TARGET))

ifdef DEBUG
CFLAGS := -Wall -g -MD -std=c99
else
CFLAGS := -Wall -MD -std=c99
endif

all: $(TARGET)

bin:
	mkdir bin

$(TARGET): bin $(OBJECTS) 
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(INSTALL_TARGET): $(TARGET)
	cp $(TARGET) $(INSTALL_TARGET)

.PHONY: docs
docs:
	mkdir -p docs
	doxygen .doxyfile

.PHONY: install
install: $(INSTALL_TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET)
	-rm -f $(OBJECTS)
	-rm -f $(DEPS)

-include $(DEPS)
