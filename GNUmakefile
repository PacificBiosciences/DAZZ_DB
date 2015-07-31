THISDIR:=$(abspath $(dir $(lastword ${MAKEFILE_LIST})))
CFLAGS+= -O3 -Wall -Wextra -fno-strict-aliasing -Wno-unused-result
CPPFLAGS+= -MMD -MP
LDLIBS+= -lm
LDFLAGS+= -g -rdynamic
ALL = fasta2DB DB2fasta quiva2DB DB2quiva DBsplit DBdust Catrack DBshow DBstats DBrm simulator \
      fasta2DAM DAM2fasta

all: ${ALL}

%: %.o
	${CC} -o $@ $^ ${LDLIBS} ${LDFLAGS}
fasta2DB: fasta2DB.o DB.o QV.o
DB2fasta: DB2fasta.o DB.o QV.o
quiva2DB: quiva2DB.o DB.o QV.o
DB2quiva: DB2quiva.o DB.o QV.o
DBsplit: DBsplit.o DB.o QV.o
DBdust: DBdust.o DB.o QV.o
Catrack: Catrack.o DB.o QV.o
DBshow: DBshow.o DB.o QV.o
DBstats: DBstats.o DB.o QV.o
DBrm: DBrm.o DB.o QV.o
simulator: simulator.o DB.o QV.o
fasta2DAM: fasta2DAM.o DB.o QV.o
DAM2fasta: DAM2fasta.o DB.o QV.o
DBupgrade.Sep.25.2014: DBupgrade.Sep.25.2014.o DB.o QV.o
DBupgrade.Dec.31.2014: DBupgrade.Dec.31.2014.o DB.o QV.o
DUSTupgrade.Jan.1.2015: DUSTupgrade.Jan.1.2015.o DB.o QV.o

clean:
	rm -f ${ALL} *.o
	rm -f ${DEPS}
	rm -fr *.dSYM
	rm -f DBupgrade.Sep.25.2014 DBupgrade.Dec.31.2014 DUSTupgrade.Jan.1.2015
	rm -f dazz.db.tar.gz

SRCS:=$(wildcard ${THISDIR}/*.c)
DEPS:=${SRCS:.c=.d}
-include ${DEPS}
