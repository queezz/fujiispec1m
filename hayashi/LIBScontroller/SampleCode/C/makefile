SHELL  = sh
CC     = gcc
CFLAGS = -Wall -Wno-unused-result
ifeq ($(OS),Windows_NT)
LIBS   = -l pthread -lm -L.
else
LIBS   = -l pthread -L. -lm -ldx2lib
endif

SRCDIR = .
INCDIR = .

SRCS    = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*.cpp)
TARGETS = $(notdir $(SRCS))

define MAKEALL
$(basename $(1)):$(1)
ifeq ($(OS),Windows_NT)
ifeq ($(ENV_WINX64),1)
	$(CC) -I$(INCDIR) $(CFLAGS) dx2lib_x64.dll $(SRCDIR)/$(1) $(LIBS) -o $(basename $(1)).exe
else
	$(CC) -I$(INCDIR) $(CFLAGS) dx2lib_x32.dll $(SRCDIR)/$(1) $(LIBS) -o $(basename $(1)).exe
endif
else
	$(CC) -I$(INCDIR) $(CFLAGS) $(SRCDIR)/$(1) $(LIBS) -o $(basename $(1)).out
endif
endef

.PHONY: all
all: $(basename $(TARGETS))
$(foreach var,$(TARGETS),$(eval $(call MAKEALL,$(var))))

#make clean
.PHONY: clean
clean: 
	$(RM) *.exe *.out *.bak *.BAK
