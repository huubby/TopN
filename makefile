default: all

all depend ::
	$(MAKE) -C src $@

analyzerd depend ::
	$(MAKE) -C src -f debug.mk $@
analyzer depend ::
	$(MAKE) -C src -f release.mk $@
combinerd depend ::
	$(MAKE) -C src -f debug.mk $@
combiner depend ::
	$(MAKE) -C src -f release.mk $@

.PHONY: release

release:
	make analyzer combiner

test::
	$(MAKE) -C test $@

clean::
	$(MAKE) -C src $@
	$(MAKE) -C test $@
