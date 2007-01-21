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
			build/msvc/mxf2dot.dsp \
			build/msvc/mxfsplit.dsp \
			build/msvc/mxflib.sln \
			build/msvc/mxflib.vcproj \
			build/msvc/all.vcproj \
			build/msvc/all+crypto.vcproj \
			build/msvc/mxfcrypt.vcproj \
			build/msvc/mxfdump.vcproj \
			build/msvc/mxfwrap.vcproj \
			build/msvc/simplewrap.vcproj \
			build/msvc/mxf2dot.vcproj \
			build/msvc/mxfsplit.vcproj

if HAVE_DOXYGEN
DOC_DIR = docs
else
DOC_DIR =
endif

SUBDIRS = mxflib mxfsplit mxfwrap mxfdump mxfcrypt mxf2dot simplewrap tests $(DOC_DIR)