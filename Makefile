CXX = g++
KLVINST = /usr/local
CXXFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -I.. -I$(KLVINST)/include -g -Wall
KLVLIB = -L$(KLVINST)/lib -lklv
INSTALL_PREFIX = /usr/local

all: mxfwrap/mxfwrap test/mxftest

libmxf.a: deftypes.o esp_dvdif.o esp_mpeg2ves.o esp_wavepcm.o essence.o helper.o index.o klvobject.o mdobject.o mdtraits.o mdtype.o metadata.o mxffile.o partition.o primer.o rip.o
	rm -f libmxf.a
	ar rv libmxf.a deftypes.o esp_dvdif.o esp_mpeg2ves.o esp_wavepcm.o essence.o helper.o index.o klvobject.o mdobject.o mdtraits.o mdtype.o metadata.o mxffile.o partition.o primer.o rip.o

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

mxfwrap/mxfwrap: libmxf.a mxfwrap/mxfwrap.cpp
	$(CXX) $(CXXFLAGS) mxfwrap/mxfwrap.cpp -o mxfwrap/mxfwrap -L. -lmxf $(KLVLIB) -luuid

test/mxftest: libmxf.a test/test.cpp
	$(CXX) $(CXXFLAGS) test/test.cpp -o test/mxftest -L. -lmxf $(KLVLIB) -luuid


install: libmxf.a mxfwrap/mxfwrap test/mxftest
	install -c libmxf.a $(INSTALL_PREFIX)/lib/libmxf.a
	mkdir -p $(INSTALL_PREFIX)/include/mxflib
	for header in mxflib.h datachunk.h debug.h deftypes.h esp_dvdif.h esp_mpeg2ves.h esp_wavepcm.h essence.h forward.h helper.h index.h klvobject.h mdobject.h mdtraits.h mdtype.h metadata.h endian.h mxffile.h system.h types.h primer.h rip.h smartptr.h ; do install -c -m 0644 $$header $(INSTALL_PREFIX)/include/mxflib/$$header ; done
	install -c mxfwrap/mxfwrap $(INSTALL_PREFIX)/bin/mxfwrap
	install -c test/mxftest $(INSTALL_PREFIX)/bin/mxftest

clean:
	rm -f *.o *.a mxfwrap/mxfwrap test/mxftest core

docs:
	doxygen doxyfile.cfg
