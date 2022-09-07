# Makefile
include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(DETECTOR_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(DETECTOR_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(DETECTOR_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) $(LOG_UDP_CFLAGS) $(FITSCFLAGS) $(XCLIB_CFLAGS) $(MJDCFLAGS) 

DOCFLAGS 	= -static

SRCS 		= detector_test_exposure.c detector_test_hex_parsing.c
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: $(PROGS) docs

$(BINDIR)/%: $(BINDIR)/%.o
	$(CC) -o $@ $< -L$(LT_LIB_HOME) -l$(DETECTOR_LIBNAME) $(XCLIB_LDFLAGS) $(MJDLIB) $(CFITSIOLIB) $(TIMELIB) $(SOCKETLIB) -lm -lc 

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  
docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(PROGS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
# DO NOT DELETE
