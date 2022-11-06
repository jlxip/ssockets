PROJNAME := echo
RESULT := echoserver



SHELL := /bin/bash
SRCPATH := src
OBJPATH := obj

CC := gcc
INCLUDES := -I../../pub/

CFLAGS_BASE := -O2
ifdef DEBUG
CFLAGS_BASE += -g
endif
CFLAGS_WARN := -Wall -Wextra -Werror
CFLAGS := $(INCLUDES) $(CFLAGS_BASE) $(CFLAGS_WARN)

LINKER := $(CC)
LINKER_FLAGS := -Wl,-z,relro,-z,now -L../../
LINKER_FLAGS_END := -lssockets

# --- OBJS ---
OBJPATHS := $(shell cd src && find . -type d ! -path "$(EXCLUDE)" | \
	xargs -I {} echo "$(OBJPATH)/"{})
OBJS := $(shell cd src && find . -type f -iname '*.c' ! -path "$(EXCLUDE)" | \
	sed 's/\.\///g' | sed 's/\.c/\.o/g' | xargs -I {} echo "$(OBJPATH)/"{})

.PHONY: all run clean
all: $(RESULT)
	@

run: all
	@LD_LIBRARY_PATH=../../:$LD_LIBRARY_PATH ./$(RESULT)

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

clean:
	@rm -rf $(RESULT) $(OBJPATH)/
