# Generated automatically from Makefile.in by configure.
# CircleMUD 3.0 Makefile.in - Makefile template used by 'configure'
#

# C compiler to use
CC = g++

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -Wno-unused -Wno-sign-compare -Wno-switch -Wno-write-strings
# -Weffc++
#-fno-inline

#flags for profiling (see hacker.doc for more information)
PROFILE = 
#PROFILE = -pg

LIBS = -lz -lpcre -lpcrecpp 


##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin
DEPDIR = .deps
OBJDIR = o

CFLAGS = -g3 $(MYFLAGS) $(PROFILE)
OPTCFLAGS = -O3 $(MYFLAGS) $(PROFILE)

ACTFILES = act.comm.cpp act.informative.cpp act.movement.cpp act.item.cpp \
	act.offensive.cpp act.other.cpp act.social.cpp act.wizard.cpp
CUSTOMFILES = elevators.cpp newfight.cpp vehicles.cpp events.cpp ident.cpp
OLC = aedit.cpp cedit.cpp hedit.cpp medit.cpp oedit.cpp redit.cpp sedit.cpp zedit.cpp \
	scriptedit.cpp olc.cpp bedit.cpp
SCRIPTENGINE = scriptengine.cpp triggers.cpp scriptcmd.cpp scriptcompiler.cpp
CPPFILES = affects.cpp characters.cpp clans.cpp descriptors.cpp event.cpp objects.cpp \
	files.cpp index.cpp rooms.cpp scripts.cpp sstring.cpp parser.cpp help.cpp opinion.cpp \
	queue.cpp entity.cpp zones.cpp idmap.cpp enum.cpp virtualid.cpp
IMC = imc/imc.cpp imc/sha256.cpp
LEXI = lexifile.cpp lexiparser.cpp
#OPTIMIZEDCPP = utils.cpp buffer.cpp lexistring.cpp
OPTIMIZEDCPP = utils.cpp buffer.cpp lexistring.cpp
C_FILES = comm.cpp $(ACTFILES) ban.cpp boards.cpp config.cpp constants.cpp db.cpp \
	fight.cpp find.cpp graph.cpp handler.cpp house.cpp interpreter.cpp limits.cpp mail.cpp \
	mobact.cpp modify.cpp objsave.cpp shop.cpp spec_assign.cpp spec_procs.cpp \
	weather.cpp races.cpp random.cpp log.cpp track.cpp \
	$(OPTIMIZEDCPP) $(OLC) $(CUSTOMFILES) $(SCRIPTENGINE) \
	$(CPPFILES) $(IMC) $(LEXI) network/protocols/TelnetProtocol.cpp 

#O_FILES := $(patsubst %.cpp,$(OBJDIR)/%.o,$(C_FILES))
O_FILES := $(C_FILES:%.cpp=$(OBJDIR)/%.o)
D_FILES := $(C_FILES:%.cpp=$(DEPDIR)/%.d)

default: 
	$(MAKE) ../bin/circle

all: 
	$(MAKE) ../bin/circle
	
circle:
	$(MAKE) $(BINDIR)/circle

../bin/circle : $(O_FILES)
	$(CC) -o $(BINDIR)/circle $(PROFILE) $(O_FILES) $(LIBS)

# remove all the compiled objects for a clean compile
clean:
	rm -rf $(OBJDIR)

# default rule to make all of the .c files
$(OBJDIR)/%.opt.o: %.cpp
	@echo $<
	@$(CC) -c $(OPTCFLAGS) $< -o $@

# default rule to make all of the .c files
#	Uncomment the mkdir and dependency generation lines
#	to use dependency generation on obj file generation
#	sed:	append directory to object target
#	then remove file if it is empty
$(OBJDIR)/%.o: %.cpp
	@echo $<
	@mkdir -p $(OBJDIR)/$(patsubst ./,,$(dir $<))
	@$(CC) -c $(CFLAGS) $< -o $@
	@mkdir -p $(DEPDIR)/$(patsubst ./,,$(dir $<))
	@$(CC) -MM $(CPPFLAGS) $< | \
	  sed 's|.*:|$@:|' > $(DEPDIR)/$*.d; \
	  [ -s $(DEPDIR)/$*.d ] || rm -f $(DEPDIR)/$*.d

# Dependencies for the object files (automagically generated with
# gcc -MM)

#$(DEPDIR)/%.opt.d $(DEPDIR)/%.d: %.cpp
#	@echo Generating dependencies for $<
#	@mkdir -p $(@D)
#	@mkdir -p $(OBJDIR)/$(patsubst ./,,$(dir $<))
#	@$(CC) -MM $(CPPFLAGS) $< | \
#	  sed 's@\(.*\).o:@$(OBJDIR)/$(patsubst ./,,$(dir $<))\1.o $@:@' > $@; \
#	  [ -s $@ ] || rm -f $@

-include $(D_FILES)
