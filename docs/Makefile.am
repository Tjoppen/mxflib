EXTRA_DIST = doxyfile.cfg mxflib.dox dox.bat

CLEANFILES = html/*

htmldir = $(datadir)/doc/mxflib/html

html_DATA = html/index.html

html/index.html: doxyfile.cfg
	doxygen doxyfile.cfg

install-data-hook:
	cp -dpR html/* $(DESTDIR)$(htmldir)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldir)
