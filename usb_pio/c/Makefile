# Makefile

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

# estar_config
#CONFIG_CFLAGS	= -I$(eSTAR_SRC_HOME)/config/include/
#CONFIG_LDFLAGS	= -L$(eSTAR_LIB_HOME) -lestar_config

INCDIR 		= $(USB_PIO_SRC_HOME)/include
BINDIR		= $(USB_PIO_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(USB_PIO_DOC_HOME)/cdocs

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) $(LOG_UDP_CFLAGS) $(CONFIG_CFLAGS) \
		$(SHARED_LIB_CFLAGS) 
LDFLAGS		= $(CONFIG_LDFLAGS)
DOCFLAGS 	= -static

SRCS 		= usb_pio_general.c usb_pio_connection.c usb_pio_command.c
#usb_pio_config.c
HEADERS		= $(SRCS:%.c=%.h)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: shared docs

shared: $(LT_LIB_HOME)/lib$(USB_PIO_LIBNAME).so
$(LT_LIB_HOME)/lib$(USB_PIO_LIBNAME).so: $(OBJS)
	$(CC) $(CCSHAREDFLAG) $(LDFLAGS) $(OBJS) -o $@

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/lib$(USB_PIO_LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

# DO NOT DELETE
