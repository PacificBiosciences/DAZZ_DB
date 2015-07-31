THISDIR:=$(abspath $(dir $(lastword ${MAKEFILE_LIST})))
CFLAGS+= -O3 -Wall -Wextra -fno-strict-aliasing -Wno-unused-result
CPPFLAGS+= -MMD -MP
LDLIBS+= -lm
LDFLAGS+=
ALL = fasta2DB DB2fasta quiva2DB DB2quiva DBsplit DBdust Catrack DBshow DBstats DBrm simulator \
      fasta2DAM DAM2fasta

all: ${ALL}
${ALL}: -ldazzdb

%: %.o
	${CC} -o $@ $^ ${LDLIBS} ${LDFLAGS}

libdazzdb.so: DB.o QV.o
	${LINK.o} -o $@ $^ -shared ${LDFLAGS}
libdazzdb.a: DB.o QV.o
	${AR} -rcv $@ $^

clean:
	rm -f ${ALL} *.o
	rm -f ${DEPS}
	rm -fr *.dSYM
	rm -f DBupgrade.Sep.25.2014 DBupgrade.Dec.31.2014 DUSTupgrade.Jan.1.2015
	rm -f dazz.db.tar.gz

SRCS:=$(wildcard ${THISDIR}/*.c)
DEPS:=${SRCS:.c=.d}
-include ${DEPS}
