EXTRA_DIST = dox.bat \
			build/rpm/mxflib.spec
			build/msvc/mxflib.dsw \
			build/msvc/mxflib.dsp \
			build/msvc/all.dsp \
			build/msvc/mxfdump.dsp \
			build/msvc/mxfwrap.dsp \
			build/msvc/mxfsplit.dsp

if HAVE_DOXYGEN
DOC_DIR = docs
else
DOC_DIR =
endif

SUBDIRS = mxflib mxfsplit mxfwrap mxfdump tests $(DOC_DIR)
