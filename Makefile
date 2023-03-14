# Makefile
include ../Makefile.common
include Makefile.common

DIRS 	=  filter_wheel detector nudgematic c java 
# usb_pio 

top:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

depend:
	@for i in $(DIRS); \
	do \
		(echo depend in $$i...; cd $$i; $(MAKE) depend);\
	done;

clean:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; cd $$i; $(MAKE) clean); \
	done;

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; cd $$i; $(MAKE) tidy); \
	done;
