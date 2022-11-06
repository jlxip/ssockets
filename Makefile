PROJNAME := ssockets
RESULT := libssockets.so



SHELL := /bin/bash
SRCPATH := src
OBJPATH := obj

LIBWHERE := /usr/lib
INCWHERE   := /usr/include

CC := gcc
INCLUDES := -Isrc -Ipub

CFLAGS_BASE := -O2 -fPIC
ifdef DEBUG
CFLAGS_BASE += -g
endif
CFLAGS_WARN := -Wall -Wextra -Werror
CFLAGS := $(INCLUDES) $(CFLAGS_BASE) $(CFLAGS_WARN)

LINKER := $(CC)
LINKER_FLAGS := -Wl,-z,relro,-z,now -shared
LINKER_FLAGS_END := -pthread

# --- OBJS ---
OBJPATHS := $(shell cd src && find . -type d ! -path "$(EXCLUDE)" | \
	xargs -I {} echo "$(OBJPATH)/"{})
OBJS := $(shell cd src && find . -type f -iname '*.c' ! -path "$(EXCLUDE)" | \
	sed 's/\.\///g' | sed 's/\.c/\.o/g' | xargs -I {} echo "$(OBJPATH)/"{})

.PHONY: all install uninstall clean echo echorun
all: $(RESULT)
	@

$(RESULT): $(OBJS)
	@echo "[$(PROJNAME)] Linking..."
	@$(LINKER) $(LINKER_FLAGS) $(OBJS) $(LINKER_FLAGS_END) -o $@
	@if [[ ! -v DEBUG ]]; then \
		echo "[$(PROJNAME)] Stripping..."; \
		strip $(RESULT); \
	fi

-include $(OBJS:.o=.o.d)

$(OBJS): $(OBJPATH)/%.o: $(SRCPATH)/%.c | $(OBJPATHS)
	@echo "[$(PROJNAME)] ===> $<"
	@$(CC) -c -o $@ $< $(CFLAGS)
	@$(CC) -MM $< -o $@.d.tmp $(CFLAGS)
	@sed -e 's|.*:|$@:|' < $@.d.tmp > $@.d
	@rm -f $@.d.tmp

$(OBJPATHS): $(OBJPATH)/%: $(SRCPATH)/%
	@mkdir -p $(OBJPATH)/$*

install: all
	@install -D -m755 -v $(RESULT) $(LIBWHERE)/$(RESULT)
	@install -D -m744 -v pub/ssockets.h $(INCWHERE)/ssockets.h

uninstall:
	@rm -v /usr/lib/$(RESULT)
	@rm -v /usr/include/ssockets.h

clean:
	@rm -rf $(RESULT) $(OBJPATH)/
	@make -C examples/echo clean

echo: all
	@make -C examples/echo
echorun: all
	@make -C examples/echo run
