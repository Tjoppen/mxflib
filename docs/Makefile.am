# doxyfile.cfg is distributed in addition to doxyfile.cfg.in since in
# the MSVC environment ./configure is not used and a hardcoded doxyfile.cfg
# must be provided.
EXTRA_DIST = doxyfile.cfg doxyfile.cfg.in mxflib.dox

CLEANFILES = html/*

htmldir = $(datadir)/doc/mxflib/html

html_DATA = html/index.html

html/index.html: doxyfile.cfg
	doxygen doxyfile.cfg

install-data-hook:
	cp -dpR html/* $(DESTDIR)$(htmldir)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldir)
