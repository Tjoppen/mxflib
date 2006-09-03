EXTRA_DIST = dox.bat \
			build/rpm/mxflib.spec \
			build/msvc/mxflib.dsw \
			build/msvc/mxflib.dsp \
			build/msvc/all.dsp \
			build/msvc/all+crypto.dsp \
			build/msvc/mxfcrypt.dsp \
			build/msvc/mxfdump.dsp \
			build/msvc/mxfwrap.dsp \
			build/msvc/simplewrap.dsp \
			build/msvc/mxfsplit.dsp

if HAVE_DOXYGEN
DOC_DIR = docs
else
DOC_DIR =
endif

SUBDIRS = mxflib mxfsplit mxfwrap mxfdump mxfcrypt simplewrap tests $(DOC_DIR)