#Make file for esp-config-tools 2015-10-08 by Martin Grønholdt.

TARGET := bin/gen_config
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

#Add project name
ifndef PROJECT_NAME
$(error No project name set (PROJECT_NAME));
else
CFLAGS += -DPROJECT_NAME=\"$(PROJECT_NAME)\"
endif

#Configuration signature.
CFLAGS += -DESP_CONFIG_SIG=$(ESP_CONFIG_SIG)

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
