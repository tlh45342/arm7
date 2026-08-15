# ARM7 VM family convenience Makefile

.PHONY: all lib run test clean rebuild

all: run

lib:
	$(MAKE) -C libarm7vm

run: lib
	$(MAKE) -C arm7-run

test:
	$(MAKE) -C libarm7vm test
	$(MAKE) -C arm7-run test

clean:
	$(MAKE) -C arm7-run clean
	$(MAKE) -C libarm7vm clean

rebuild: clean all
