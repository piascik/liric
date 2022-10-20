# Makefile
# $Header$
include ../Makefile.common
include ../../Makefile.common

MAKE 		= make
DIRS 		= ngat
JAR_FILE       	= ngat_raptor.jar
JARLIBDIR	= $(LT_JAVALIB_HOME)
LIBDIR		= $(RAPTOR_BIN_HOME)/java/
DOCSDIR 	= $(RAPTOR_DOC_HOME)/javadocs/
DOCFLAGS 	= -version -author -private
JAVASRCS 	= ngat/raptor/*.java ngat/raptor/command/*.java
JAVAPKGS	= ngat.raptor ngat.raptor.command

JAVADOCS 	= $(JAVASRCS:%.java=$(DOCSDIR)/%.html)
OBJS 		= $(JAVASRCS:%.java=$(LIBDIR)/%.class)

top: dirs $(JARLIBDIR)/$(JAR_FILE) javadocs

dirs:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

$(JARLIBDIR)/$(JAR_FILE): $(OBJS)
	( cd $(LIBDIR); $(JAR) $(JAR_OPTIONS) $(JARLIBDIR)/$(JAR_FILE) ngat)

$(OBJS): $(JAVASRCS)

#
# Do all javadocs at top level, to ensure javadoc creates package links correctly
#
javadocs: $(JAVADOCS)

$(JAVADOCS): $(JAVASRCS)
	$(JAVADOC) -sourcepath $(LT_SRC_HOME):$(RAPTOR_SRC_HOME)/java -d $(DOCSDIR) $(DOCFLAGS) \
		-link http://java.sun.com/j2se/1.4.2/docs/api/ $(JAVAPKGS)

depend:
	echo no depend in raptor/java directory.

clean:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-$(RM) $(RM_OPTIONS) $(JARLIBDIR)/$(JAR_FILE)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; cd $$i; $(MAKE) clean); \
	done;
	$(RM) $(RM_OPTIONS) $(JAVADOCS)
	$(RM) $(RM_OPTIONS) -r $(DOCSDIR)/*

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; cd $$i; $(MAKE) tidy); \
	done;

backup: checkin
