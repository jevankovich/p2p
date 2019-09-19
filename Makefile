SOURCEDIR = src
SOURCES = main.c

INCLUDE_DIRS = include

OBJDIR = build
OBJS := $(addprefix $(OBJDIR)/,$(SOURCES:%.c=%.o))

CFLAGS += -MMD -Wall -std=c11 -ggdb $(addprefix -I,$(INCLUDE_DIRS))
LDFLAGS +=

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