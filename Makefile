CXX = g++
CXXFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -I. -I/usr/local/include -g -Wall -Wno-uninitialized
KLVLIB = -L/usr/local/lib -lklv

all: mxfwrap/mxfwrap

libmxf.a: deftypes.o esp_dvdif.o esp_mpeg2ves.o esp_wavepcm.o essence.o helper.o index.o klvobject.o mdobject.o mdtraits.o mdtype.o metadata.o mxffile.o partition.o primer.o rip.o
	rm -f libmxf.a
	ar rv libmxf.a deftypes.o esp_dvdif.o esp_mpeg2ves.o esp_wavepcm.o essence.o helper.o index.o klvobject.o mdobject.o mdtraits.o mdtype.o metadata.o mxffile.o partition.o primer.o rip.o

deftypes.o: deftypes.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h 
	$(CXX) $(CXXFLAGS) -c $<

esp_dvdif.o: esp_dvdif.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

esp_mpeg2ves.o: esp_mpeg2ves.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

esp_wavepcm.o: esp_wavepcm.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

essence.o: essence.cpp mxflib.h mxfsystem.h debug.h forward.h smartptr.h \
  mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

helper.o: helper.cpp mxflib.h mxfsystem.h debug.h forward.h smartptr.h \
  mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

index.o: index.cpp mxflib.h mxfsystem.h debug.h forward.h smartptr.h \
  mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

klvobject.o: klvobject.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mdobject.o: mdobject.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mdtraits.o: mdtraits.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mdtype.o: mdtype.cpp mxflib.h mxfsystem.h debug.h forward.h smartptr.h \
  mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

metadata.o: metadata.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mxffile.o: mxffile.cpp mxflib.h mxfsystem.h debug.h forward.h smartptr.h \
  mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

partition.o: partition.cpp mxflib.h mxfsystem.h debug.h forward.h \
  smartptr.h mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h \
  mdtype.h deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h \
  partition.h mxffile.h index.h essence.h esp_mpeg2ves.h esp_wavepcm.h \
  esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

primer.o: primer.cpp mxflib.h mxfsystem.h debug.h forward.h smartptr.h \
  mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

rip.o: rip.cpp mxflib.h mxfsystem.h debug.h forward.h smartptr.h \
  mxfendian.h mxftypes.h helper.h datachunk.h mdtraits.h mdtype.h \
  deftypes.h klvobject.h mdobject.h \
  \
  primer.h metadata.h rip.h partition.h mxffile.h \
  index.h essence.h esp_mpeg2ves.h esp_wavepcm.h esp_dvdif.h
	$(CXX) $(CXXFLAGS) -c $<

mxfwrap/mxfwrap: libmxf.a mxfwrap/mxfwrap.cpp
	$(CXX) $(CXXFLAGS) mxfwrap/mxfwrap.cpp -o mxfwrap/mxfwrap -L. -lmxf $(KLVLIB) -luuid

opatom/mxfwrapatom: libmxf.a opatom/mxfwrapopatom.cpp
	$(CXX) $(CXXFLAGS) opatom/mxfwrapopatom.cpp -o opatom/mxfwrapopatom -L. -lmxf $(KLVLIB) -luuid

test/mxftest: libmxf.a test/test.cpp
	$(CXX) $(CXXFLAGS) test/test.cpp -o test/mxftest -L. -lmxf $(KLVLIB) -luuid


install: libmxf.a mxfwrap/mxfwrap
	install -c libmxf.a /usr/local/lib/libmxf.a
	install -c mxfwrap/mxfwrap /usr/local/bin/mxfwrap

clean:
	rm -f *.o *.a mxfwrap/mxfwrap test/test

docs:
	doxygen doxyfile.cfg
