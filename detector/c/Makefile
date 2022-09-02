# Makefile

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(DETECTOR_SRC_HOME)/include
BINDIR		= $(DETECTOR_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(DETECTOR_DOC_HOME)/cdocs

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) $(LOG_UDP_CFLAGS) $(FITSCFLAGS) \
		$(XCLIB_CFLAGS) $(MJDCFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= $(XCLIB_LDFLAGS) $(MJDLIB) $(CFITSIOLIB)
DOCFLAGS 	= -static

SRCS 		= detector_buffer.c detector_exposure.c detector_fits_filename.c detector_fits_header.c \
		detector_general.c detector_setup.c 
HEADERS		= $(SRCS:%.c=%.h)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: shared docs

shared: $(LT_LIB_HOME)/lib$(DETECTOR_LIBNAME).so
$(LT_LIB_HOME)/lib$(DETECTOR_LIBNAME).so: $(OBJS)
	$(CC) $(CCSHAREDFLAG) $(LDFLAGS) $(OBJS) -o $@

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/lib$(DETECTOR_LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

# DO NOT DELETE
