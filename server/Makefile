
SUBDIRS =
PROJ = iotcpd

### Machine flags
#
#CC_CMACH	= 
#CC_DMACH	= 
### Build flags
#
# -fdata-sections, -ffunction-sections and -Wl,--gc-sections -Wl,-s
# are used for dead code elimination, see:
# http://gcc.gnu.org/ml/gcc-help/2003-08/msg00128.html
#
#CFLAGS		+= $(CC_CMACH) $(CC_DMACH) -Wall -Wno-switch
#CFLAGS		+= -fno-force-addr -finline-limit=1 -fno-schedule-insns
#CFLAGS		+= -fshort-enums -Wl,-Map=output.map
CFLAGS		+= $(CC_CMACH) $(CC_DMACH) -Wall
LDFLAGS		=

CFLAGS_REL	+= -O2 -pipe
#LDFLAGS_REL	+= -Wl,--gc-sections -Wl,-s

CFLAGS_DBG	+= -O1 -g
#LDFLAGS_DBG	+= -Wl,--gc-sections

# linker flags and include directories
INCLUDES	+= -I./ -Igcc/
### Build tools
# 
CC		= gcc
LD		= ld
AS		= as
AR		= ar

BASH := $(shell which bash || which bash)
COVERITY_SCAN_TOKEN := $(shell cat ../.secrets.COVERITY_SCAN_TOKEN)

.PHONY: all
.PHONY: clean
.PHONY: install
.PHONY: config
.PHONY: depend
.PHONY: doc
.PHONY: tags
.PHONY: force

all: depend config.h version.h tags $(PROJ)

#
# Build list of sources and objects to build
SRCS := $(wildcard *.c)
$(foreach subdir,$(SUBDIRS), \
	$(eval SRCS := $(SRCS) $(wildcard $(subdir)/*.c)) \
)
OBJS := $(patsubst %.c,%.o,$(SRCS))

#
# Dependencies rules
depend: $(PROJ).dep

$(PROJ).dep: $(SRCS)
	@echo "Generating dependencies.."
	@touch $@
	@makedepend $(INCLUDES) -Y -f $@ $^ &> /dev/null
	@rm -f $@.bak

#
# Append specific CFLAGS/LDFLAGS
DEBUG := $(shell grep "^\#define CONFIG_DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
CFLAGS	+= $(CFLAGS_REL)
LDFLAGS	+= $(LDFLAGS_REL)
else
TARGET	:= DEBUG
CFLAGS	+= $(CFLAGS_DBG)
LDFLAGS	+= $(LDFLAGS_DBG)
endif

# rebuild if CFLAGS changed, as suggested in:
# http://stackoverflow.com/questions/3236145/force-gnu-make-to-rebuild-objects-affected-by-compiler-definition/3237349#3237349
$(PROJ).cflags: force
	@echo "$(CFLAGS)" | cmp -s - $@ || echo "$(CFLAGS)" > $@

$(OBJS): $(PROJ).cflags
#
# Top rules

$(PROJ): $(OBJS)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $@ $+ && size $@ 
#ifeq ($(DEBUG),)
#	@strip $@
#endif

modinit.o: modinit.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -Wno-implicit-function-declaration \
		$(INCLUDES) -c $< -o $@

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

modinit.c:
	@echo "Please do a 'make config' first!" && false

config.h:
	@echo "Please do a 'make config' first!" && false

version.h: $(SRCS)
	@./tools/setver.sh

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

#install: $(PROJ).elf
	@#bash ~/bin/burn.sh
	@#mspdebug olimex-iso-mk2 "prog $<"

clean: $(SUBDIRS)
	@for subdir in $(SUBDIRS); do \
		echo "Cleaning $$subdir .."; rm -f $$subdir/*.o; \
	done
	@rm -f *.o $(PROJ) $(PROJ).{txt,cflags,dep} output.map tags log
	@rm -rf cov-int

doc:
	rm -rf doc/*
	doxygen Doxyfile

cppcheck: $(PROJ)
	@cppcheck --quiet --force --enable=warning --enable=performance --enable=information --enable=missingInclude --enable=performance --enable=portability -I ./ ./

scan-build: clean
	@scan-build make

valgrind: $(PROJ)
	@valgrind --trace-children=no --child-silent-after-fork=yes --leak-check=full --show-leak-kinds=all ./$(PROJ) --num-daemons 8 

coverity: clean
	@cov-build --dir cov-int make
	@tar czvf /tmp/$(PROJ).tgz cov-int
	@curl --form token=$(COVERITY_SCAN_TOKEN) \
	  --form email=2b4eda@subdimension.ro \
	  --form file=@/tmp/$(PROJ).tgz \
	  --form version="Version" \
	  --form description="Description" \
	  https://scan.coverity.com/builds?project=rodan%2Fiotcpd

-include $(PROJ).dep
