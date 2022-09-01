# Makefile

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

# Do we wish to link with CFITSIO routines?
FITSCFLAGS = -DCFITSIO=1 -I$(CFITSIOINCDIR) 
CFITSIOLIB = -L$(LT_LIB_HOME) -lcfitsio

# estar_config
CONFIG_CFLAGS	= -I$(eSTAR_SRC_HOME)/config/include/
CONFIG_LDFLAGS	= -L$(eSTAR_LIB_HOME) -lestar_config

# raptor xclib
XCLIB_CFLAGS	= -I/usr/local/xclib/inc
XCLIB_LDFLAGS	= /usr/local/xclib/lib/xclib_x86_64.a

INCDIR 		= $(DETECTOR_SRC_HOME)/include
BINDIR		= $(DETECTOR_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(DETECTOR_DOC_HOME)/cdocs

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) $(LOG_UDP_CFLAGS) $(FITSCFLAGS) $(CONFIG_CFLAGS) \
		$(XCLIB_CFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= $(XCLIB_LDFLAGS) $(CONFIG_LDFLAGS) $(CFITSIOLIB)
DOCFLAGS 	= -static

SRCS 		= detector_buffer.c detector_fits_filename.c detector_fits_header.c detector_general.c detector_setup.c 
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
