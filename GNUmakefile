THISDIR:=$(abspath $(dir $(realpath $(lastword ${MAKEFILE_LIST}))))
CFLAGS+= -O3 -Wall -Wextra -fno-strict-aliasing -Wno-unused-result
CPPFLAGS+= -MMD -MP
LDLIBS+= -lm
LDFLAGS+=
ALL = fasta2DB DB2fasta quiva2DB DB2quiva DBsplit DBdust Catrack DBshow DBstats DBrm simulator \
      fasta2DAM DAM2fasta
vpath %.c ${THISDIR}

all: ${ALL}
${ALL}: libdazzdb.a

libdazzdb.a: DB.o QV.o open_compressed.o
	${AR} -rcv $@ $^

# Shared libs are not used yet, but maybe someday.
%.os: %.c
	${CC} -o $@ -c $< -fPIC ${CFLAGS} ${CPPFLAGS}
libdazzdb.so: DB.os QV.os
	${CC} -o $@ $^ -shared ${LDFLAGS}
install:
	cp -f fasta2DB DBsplit DBshow DBstats DBrm ${PREFIX}/bin
	cp -f libdazzdb.* ${PREFIX}/lib
clean:
	rm -f ${ALL}
	rm -f ${DEPS}
	rm -fr *.dSYM *.o *.a *.so *.os
	rm -f DBupgrade.Sep.25.2014 DBupgrade.Dec.31.2014 DUSTupgrade.Jan.1.2015
	rm -f dazz.db.tar.gz

SRCS:=$(notdir $(wildcard ${THISDIR}/*.c))
DEPS:=$(patsubst %.c,%.d,${SRCS})
-include ${DEPS}
