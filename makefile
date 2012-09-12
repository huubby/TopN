default: all

all depend ::
	$(MAKE) -C src $@

debug depend ::
	$(MAKE) -C src -f debug.mk $@
release depend ::
	$(MAKE) -C src -f release.mk $@

test::
	$(MAKE) -C test $@

clean::
	$(MAKE) -C src $@
	$(MAKE) -C test $@
