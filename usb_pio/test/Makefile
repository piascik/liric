# Makefile
include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(USB_PIO_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(USB_PIO_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(USB_PIO_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) $(LOG_UDP_CFLAGS)
#CONFIG_LDFLAGS	= -L$(eSTAR_LIB_HOME) -lestar_config

DOCFLAGS 	= -static

SRCS 		= usb_pio_test_get_inputs.c usb_pio_test_get_outputs.c usb_pio_test_set_outputs.c \
		usb_pio_test_get_input.c usb_pio_test_get_output.c usb_pio_test_set_output.c 
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)
SCRIPT_SRCS	= 
SCRIPT_BINS	= $(SCRIPT_SRCS:%=$(BINDIR)/%)

top: $(PROGS) scripts docs

$(BINDIR)/%: $(BINDIR)/%.o
	$(CC) -o $@ $< -L$(LT_LIB_HOME) $(TIMELIB) $(SOCKETLIB) -lm -lc -l$(USB_PIO_LIBNAME)

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

scripts: $(SCRIPT_BINS)

$(BINDIR)/%.csh:%.csh
	$(CP) $< $@

$(BINDIR)/%:%
	$(CP) $< $@

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(PROGS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
