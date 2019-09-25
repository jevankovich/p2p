SOURCEDIR = src
SOURCES = main.c net.c p2p.c

INCLUDE_DIRS = include

OBJDIR = build
OBJS := $(addprefix $(OBJDIR)/,$(SOURCES:%.c=%.o))

CFLAGS += -MMD -Wall -Wpedantic -std=c11 -ggdb $(addprefix -I,$(INCLUDE_DIRS)) -D_XOPEN_SOURCE=600
LDFLAGS +=

.PHONY: debug-sanitize
debug-sanitize: CFLAGS += -fsanitize=undefined -fsanitize=address
debug-sanitize: LDFLAGS += -fsanitize=undefined -fsanitize=address
debug-sanitize: debug

.PHONY: debug
debug: CFLAGS += -Og -DDEBUG
debug: $(OBJDIR)/main

.PHONY: release
release: CFLAGS += -O3 -DNDEBUG
release: $(OBJDIR)/main


$(OBJDIR)/main: $(OBJS) Makefile
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SOURCEDIR)/%.c Makefile | dirs
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJDIR)

.PHONY: dirs
dirs:
	mkdir -p $(sort $(dir $(OBJS)))

-include $(OBJS:.o=.d)