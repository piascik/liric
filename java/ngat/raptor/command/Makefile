# Makefile
include ../Makefile.common
include ../../../../../Makefile.common
include	../../../../Makefile.common

PACKAGEDIR	= ngat/raptor/command
BINDIR 		= $(RAPTOR_BIN_HOME)/java/$(PACKAGEDIR)
SRCS 		= Command.java BooleanReplyCommand.java DoubleReplyCommand.java IntegerReplyCommand.java \
		AbortCommand.java ConfigFilterCommand.java \
		FitsHeaderAddCommand.java FitsHeaderClearCommand.java FitsHeaderDeleteCommand.java \
		MultrunCommand.java \
		ShutdownCommand.java \
		StatusExposureCountCommand.java StatusExposureIndexCommand.java \
		StatusExposureLengthCommand.java StatusExposureStartTimeCommand.java\
		StatusExposureMultrunCommand.java StatusExposureRunCommand.java StatusExposureStatusCommand.java \
		StatusFilterWheelFilterCommand.java StatusFilterWheelPositionCommand.java \
		StatusFilterWheelStatusCommand.java \
		StatusTemperatureGetCommand.java 
#		MultbiasCommand.java MultdarkCommand.java

OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)

top: $(OBJS)

$(BINDIR)/%.class:%.java
	$(JAVAC) -sourcepath .:$(RAPTOR_SRC_HOME)/java \
		-classpath $(RAPTOR_BIN_HOME)/java:$(CLASSPATH) $(JAVAC_OPTIONS) $(JAVAC_VERSION_OPTIONS) -d $(RAPTOR_BIN_HOME)/java $<

clean: tidy
	-$(RM) $(RM_OPTIONS) $(OBJS)

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
