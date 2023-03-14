# Makefile

include ../../../Makefile.common
include ../../Makefile.common
include ../../usb_pio/Makefile.common
include ../Makefile.common

INCDIR 		= $(NUDGEMATIC_SRC_HOME)/include
BINDIR		= $(NUDGEMATIC_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(NUDGEMATIC_DOC_HOME)/cdocs

# usb_pio
#USB_PIO_CFLAGS	= -I$(USB_PIO_SRC_HOME)/include
#USB_PIO_LDFLAGS = -L$(LT_LIB_HOME) -l$(USB_PIO_LIBNAME)

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) $(USB_PIO_CFLAGS) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) $(LOG_UDP_CFLAGS) \
		$(SHARED_LIB_CFLAGS) 
LDFLAGS		= #$(USB_PIO_LDFLAGS)
DOCFLAGS 	= -static

SRCS 		= nudgematic_general.c nudgematic_connection.c nudgematic_command.c

HEADERS		= $(SRCS:%.c=%.h)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: shared docs

shared: $(LT_LIB_HOME)/lib$(NUDGEMATIC_LIBNAME).so
$(LT_LIB_HOME)/lib$(NUDGEMATIC_LIBNAME).so: $(OBJS)
	$(CC) $(CCSHAREDFLAG) $(LDFLAGS) $(OBJS) -o $@

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/lib$(NUDGEMATIC_LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

# DO NOT DELETE
