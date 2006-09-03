TESTSUITE_AT = testsuite.at types.at mxfdump.at mxfwrap.at simplewrap.at
TESTSUITE = $(srcdir)/testsuite

EXTRA_DIST = $(TESTSUITE_AT) testsuite package.m4 \
	small.wav small_wav.mxf

DISTCLEANFILES = atconfig
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE)

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	{					\
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >$(srcdir)/package.m4

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) testsuite.at -o $@.tmp
	mv $@.tmp $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status tests/$@

clean-local:
	if test -f $(TESTSUITE); then \
		$(SHELL) $(TESTSUITE) --clean; \
	fi

check-local: atconfig $(TESTSUITE)
	$(SHELL) $(TESTSUITE);

installcheck-local:
	if test -f $(TESTSUITE); then \
		$(SHELL) $(TESTSUITE) AUTOTEST_PATH=$(exec_prefix)/bin; \
	fi
