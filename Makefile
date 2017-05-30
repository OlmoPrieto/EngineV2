# GNU Make solution makefile autogenerated by GENie
# Type "make help" for usage help

ifndef config
  config=debug
endif
export config

PROJECTS := Engine

.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

Engine: 
	@echo "==== Building Engine ($(config)) ===="
	@${MAKE} --no-print-directory -C Engine -f Makefile

clean:
	@${MAKE} --no-print-directory -C Engine -f Makefile clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   release"
	@echo "   debug64"
	@echo "   release64"
	@echo "   debug32"
	@echo "   release32"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   Engine"
	@echo ""
	@echo "For more information, see https://github.com/bkaradzic/genie"