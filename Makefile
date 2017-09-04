BUILDFLAGS :=

ifeq ($(DEBUG), 1)
	BUILDFLAGS += -g -DDEBUG
endif

# in logger.c, flockfile() is used to support
# multithread
ifeq ($(MULTITHREAD), 1)
	BUILDFLAGS += -DMULTITHREAD_LOGGING
endif

# $(MEMTOOLSDIR)
MEMTOOLSDIR   := memtools
MEMTOOLSHEADS := $(MEMTOOLSDIR)/memcheck.h $(MEMTOOLSDIR)/hashtable_memcheck.h $(MEMTOOLSDIR)/fatalerror.h
MEMTOOLSOBJS  := $(MEMTOOLSDIR)/memcheck.o $(MEMTOOLSDIR)/hashtable_memcheck.o

$(MEMTOOLSDIR)/%.o: $(MEMTOOLSDIR)/%.c $(MEMTOOLSHEADS)
	gcc -o $@ -c $(BUILDFLAGS) $<
# end $(MEMTOOLSDIR)

TARGETS := test 
HEADERS := logger.h fatalerror.h hashtable_logfile.h
OBJS := logger.o hashtable_logfile.o test.o	

all: $(TARGETS)

%.o: %.c $(HEADERS) $(MEMTOOLSHEADS)
	gcc -o $@ -c $(BUILDFLAGS) $<

test: $(OBJS) $(MEMTOOLSOBJS)
	gcc -o $@ $^

clean:
	rm -rf *~ $(TARGETS) $(OBJS) $(MEMTOOLSOBJS) *.log



