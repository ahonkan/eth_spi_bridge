#
# OpenSSL/crypto/arc4/Makefile
#

DIR=	arc4
TOP=	../..
CC=	cc
CPP=    $(CC) -E
INCLUDES=
CFLAG=-g
AR=		ar r

ARC4_ENC=arc4_enc.o arc4_skey.o

CFLAGS= $(INCLUDES) $(CFLAG)
ASFLAGS= $(INCLUDES) $(ASFLAG)
AFLAGS= $(ASFLAGS)

GENERAL=Makefile
TEST=arc4test.c
APPS=

LIB=$(TOP)/libcrypto.a
LIBSRC=arc4_skey.c arc4_enc.c
LIBOBJ=$(ARC4_ENC)

SRC= $(LIBSRC)

EXHEADER= arc4.h
HEADER=	$(EXHEADER) arc4_locl.h

ALL=    $(GENERAL) $(SRC) $(HEADER)

top:
	(cd ../..; $(MAKE) DIRS=crypto SDIRS=$(DIR) sub_all)

all:	lib

lib:	$(LIBOBJ)
	$(AR) $(LIB) $(LIBOBJ)
	$(RANLIB) $(LIB) || echo Never mind.
	@touch lib

arc4-586.s:	asm/arc4-586.pl ../perlasm/x86asm.pl
	$(PERL) asm/arc4-586.pl $(PERLASM_SCHEME) $(CFLAGS) > $@

arc4-x86_64.s: asm/arc4-x86_64.pl
	$(PERL) asm/arc4-x86_64.pl $(PERLASM_SCHEME) > $@

arc4-ia64.S: asm/arc4-ia64.pl
	$(PERL) asm/arc4-ia64.pl $(CFLAGS) > $@

arc4-s390x.s:	asm/arc4-s390x.pl
	$(PERL) asm/arc4-s390x.pl > $@

arc4-ia64.s: arc4-ia64.S
	@case `awk '/^#define ARC4_INT/{print$$NF}' $(TOP)/include/openssl/opensslconf.h` in \
	int)	set -x; $(CC) $(CFLAGS) -DSZ=4 -E arc4-ia64.S > $@ ;; \
	char)	set -x; $(CC) $(CFLAGS) -DSZ=1 -E arc4-ia64.S > $@ ;; \
	*)	exit 1 ;; \
	esac

files:
	$(PERL) $(TOP)/util/files.pl Makefile >> $(TOP)/MINFO

links:
	@$(PERL) $(TOP)/util/mklink.pl ../../include/openssl $(EXHEADER)
	@$(PERL) $(TOP)/util/mklink.pl ../../test $(TEST)
	@$(PERL) $(TOP)/util/mklink.pl ../../apps $(APPS)

install:
	@[ -n "$(INSTALLTOP)" ] # should be set by top Makefile...
	@headerlist="$(EXHEADER)"; for i in $$headerlist ; \
	do  \
	(cp $$i $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i; \
	chmod 644 $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i ); \
	done;

tags:
	ctags $(SRC)

tests:

lint:
	lint -DLINT $(INCLUDES) $(SRC)>fluff

depend:
	@[ -n "$(MAKEDEPEND)" ] # should be set by upper Makefile...
	$(MAKEDEPEND) -- $(CFLAG) $(INCLUDES) $(DEPFLAG) -- $(PROGS) $(LIBSRC)

dclean:
	$(PERL) -pe 'if (/^# DO NOT DELETE THIS LINE/) {print; exit(0);}' $(MAKEFILE) >Makefile.new
	mv -f Makefile.new $(MAKEFILE)

clean:
	rm -f *.s *.o *.obj lib tags core .pure .nfs* *.old *.bak fluff

# DO NOT DELETE THIS LINE -- make depend depends on it.

arc4_enc.o: ../../e_os.h ../../include/openssl/bio.h
arc4_enc.o: ../../include/openssl/buffer.h ../../include/openssl/crypto.h
arc4_enc.o: ../../include/openssl/e_os2.h ../../include/openssl/err.h
arc4_enc.o: ../../include/openssl/lhash.h ../../include/openssl/opensslconf.h
arc4_enc.o: ../../include/openssl/opensslv.h ../../include/openssl/ossl_typ.h
arc4_enc.o: ../../include/openssl/arc4.h ../../include/openssl/safestack.h
arc4_enc.o: ../../include/openssl/stack.h ../../include/openssl/symhacks.h
arc4_enc.o: ../cryptlib.h arc4_enc.c arc4_locl.h
arc4_skey.o: ../../e_os.h ../../include/openssl/bio.h
arc4_skey.o: ../../include/openssl/buffer.h ../../include/openssl/crypto.h
arc4_skey.o: ../../include/openssl/e_os2.h ../../include/openssl/err.h
arc4_skey.o: ../../include/openssl/lhash.h ../../include/openssl/opensslconf.h
arc4_skey.o: ../../include/openssl/opensslv.h ../../include/openssl/ossl_typ.h
arc4_skey.o: ../../include/openssl/arc4.h ../../include/openssl/safestack.h
arc4_skey.o: ../../include/openssl/stack.h ../../include/openssl/symhacks.h
arc4_skey.o: ../cryptlib.h arc4_locl.h arc4_skey.c
