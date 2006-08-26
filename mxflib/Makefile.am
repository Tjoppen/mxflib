EXTRA_DIST = $(rsrc_DATA)

rsrcdir = $(datadir)/mxflib
pkgincludedir = $(includedir)/mxflib

lib_LIBRARIES = libmxf.a
libmxf_a_SOURCES = crypto.cpp deftypes.cpp esp_dvdif.cpp esp_mpeg2ves.cpp esp_jp2k.cpp esp_wavepcm.cpp essence.cpp helper.cpp index.cpp klvobject.cpp mdobject.cpp mdtraits.cpp mdtype.cpp metadata.cpp mxffile.cpp partition.cpp primer.cpp rip.cpp sopsax.cpp datachunk.cpp xmlparser.cpp esp.cpp vbi.cpp

INCLUDES = -I$(top_builddir)
pkginclude_HEADERS = \
			mxflib.h \
			constants.h \
			crypto.h \
			datachunk.h \
			debug.h \
			deftypes.h \
			esp_dvdif.h \
			esp_mpeg2ves.h \
			esp_jp2k.h \
			esp_wavepcm.h \
			essence.h \
			features.h \
			forward.h \
			helper.h \
			index.h \
			klvobject.h \
			mdobject.h \
			mdtraits.h \
			mdtype.h \
			metadata.h \
			endian.h \
			mxffile.h \
			partition.h \
			system.h \
			types.h \
			primer.h \
			rip.h \
			smartptr.h \
			waveheader.h \
			xmlparser.h \
			sopsax.h \
			ulmap.h \
			vbi.h

rsrc_DATA = \
			$(top_builddir)/dict.xml \
			$(top_builddir)/DMS_Crypto.xml
