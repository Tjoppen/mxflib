# $Id: Makefile,v 1.6 2004/03/28 18:32:58 matt-beard Exp $
#
# Default values assume you have g++ installed in /usr/local/* and
# a uuid library called libuuid.a (E.g. GNU/Linux with e2fsprogs)
#
CXX = g++
CXXFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -I.. -g -Wall
UUIDLIB = -luuid
INSTALL_PREFIX = /usr/local

all: mxfwrap/mxfwrap mxfsplit/mxfsplit test/mxftest

libmxf.a: deftypes.o esp_dvdif.o esp_mpeg2ves.o esp_wavepcm.o essence.o helper.o index.o klvobject.o mdobject.o mdtraits.o mdtype.o metadata.o mxffile.o partition.o primer.o rip.o sopsax.o
	rm -f libmxf.a
	ar -rvs libmxf.a deftypes.o esp_dvdif.o esp_mpeg2ves.o \
 esp_wavepcm.o essence.o helper.o index.o klvobject.o mdobject.o mdtraits.o mdtype.o \
 metadata.o mxffile.o partition.o primer.o rip.o sopsax.o

deftypes.o: deftypes.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h 
	$(CXX) $(CXXFLAGS) -c $<

esp_dvdif.o: esp_dvdif.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

esp_mpeg2ves.o: esp_mpeg2ves.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

esp_wavepcm.o: esp_wavepcm.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

essence.o: essence.cpp mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

helper.o: helper.cpp mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

index.o: index.cpp mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

klvobject.o: klvobject.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mdobject.o: mdobject.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mdtraits.o: mdtraits.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mdtype.o: mdtype.cpp mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

metadata.o: metadata.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mxffile.o: mxffile.cpp mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

partition.o: partition.cpp mxflib.h system.h debug.h forward.h \
  smartptr.h endian.h types.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

primer.o: primer.cpp mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

rip.o: rip.cpp mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

sopsax.o: sopsax.cpp sopsax.h mxflib.h system.h debug.h forward.h smartptr.h \
  endian.h types.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mxfwrap/mxfwrap: libmxf.a mxfwrap/mxfwrap.cpp
	$(CXX) $(CXXFLAGS) mxfwrap/mxfwrap.cpp -o mxfwrap/mxfwrap -L. -lmxf $(UUIDLIB)

mxfsplit/mxfsplit: libmxf.a waveheader.h mxfsplit/mxfsplit.cpp
	$(CXX) $(CXXFLAGS) mxfsplit/mxfsplit.cpp -o mxfsplit/mxfsplit -L. -lmxf $(UUIDLIB)

test/mxftest: libmxf.a test/test.cpp
	$(CXX) $(CXXFLAGS) test/test.cpp -o test/mxftest -L. -lmxf $(UUIDLIB)


install: libmxf.a mxfwrap/mxfwrap test/mxftest
	install -c libmxf.a $(INSTALL_PREFIX)/lib/libmxf.a
	mkdir -p $(INSTALL_PREFIX)/include/mxflib
	for header in mxflib.h datachunk.h debug.h deftypes.h esp_dvdif.h esp_mpeg2ves.h esp_wavepcm.h essence.h forward.h helper.h index.h klvobject.h mdobject.h mdtraits.h mdtype.h metadata.h endian.h mxffile.h system.h types.h primer.h rip.h smartptr.h ; do install -c -m 0644 $$header $(INSTALL_PREFIX)/include/mxflib/$$header ; done
	install -c mxfwrap/mxfwrap $(INSTALL_PREFIX)/bin/mxfwrap
	install -c mxfsplit/mxfsplit $(INSTALL_PREFIX)/bin/mxfsplit
	install -c test/mxftest $(INSTALL_PREFIX)/bin/mxftest

clean:
	rm -f *.o *.a mxfwrap/mxfwrap mxfsplit/mxfsplit test/mxftest core

docs:
	doxygen doxyfile.cfg
