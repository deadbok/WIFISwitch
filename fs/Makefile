#Make file for esp8266 file system preparation 2015-10-13 by Martin Grønholdt.

ROOT_DIR := root_src
ROOT_FILES := $(shell find $(ROOT_DIR) -type f -name '*')
ROOT_FILES_MINIFY := $(filter %.css %.html,$(ROOT_FILES))

TARGET_DIR := root_out
TARGET_FILES := $(patsubst $(ROOT_DIR)/%,$(TARGET_DIR)/%,$(ROOT_FILES))
TARGET_FILES_MINIFY := $(filter %.css %.html,$(TARGET_FILES))

TOOLS_DIR := tools

CSS_HTML_MINIFY_PY := $(TOOLS_DIR)/css-html-js-minify.py

all: $(TARGET_FILES_MINIFY)
	rsync -rulv $(ROOT_DIR)/ $(TARGET_DIR)/

$(TARGET_DIR) $(TOOLS_DIR):
	mkdir -p $@
	
$(CSS_HTML_MINIFY_PY): $(TOOLS_DIR)
	wget -O $(CSS_HTML_MINIFY_PY) https://raw.githubusercontent.com/deadbok/css-html-js-minify/master/css-html-js-minify.py
	chmod +x $(CSS_HTML_MINIFY_PY)
	
$(TARGET_FILES_MINIFY): $(ROOT_FILES_MINIFY) $(CSS_HTML_MINIFY_PY) | $(TARGET_DIR)
	mkdir -p $(dir $@)
ifdef DEBUGFS
	cp -Rv $(patsubst $(TARGET_DIR)/%,$(ROOT_DIR)/%,$@) $@
else
	$(CSS_HTML_MINIFY_PY) --quiet $(patsubst $(TARGET_DIR)/%,$(ROOT_DIR)/%,$@) $@
endif

.PHONY: clean
clean:
	-rm -fR $(TARGET_DIR)
	
.PHONY: distclean
distclean:
	-rm -fR $(TARGET_DIR)
	-rm -fR $(TOOLS_DIR)
