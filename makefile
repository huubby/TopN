default: all

all depend ::
	$(MAKE) -C src $@

clean::
	$(MAKE) -C src $@
