#
# neon source directory Makefile
# 
# Use the NEON_NORMAL_BUILD or NEON_LIBTOOL_BUILD autoconf 
# macros to set up this Makefile correctly.
#

SHELL = /bin/sh

# Installation paths
prefix = /usr
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib

# Build paths

top_builddir = ../../
top_srcdir = ../..

# Toolchain settings.
CC = gcc
AR = /usr/bin/ar
RANLIB = /usr/bin/ranlib
LIBTOOL = @LIBTOOL@

# Flags
CPPFLAGS = -DHAVE_CONFIG_H  -D_LARGEFILE64_SOURCE -DNE_LFS  -I$(top_builddir)
CFLAGS = -g -O2 -I$(top_srcdir)/lib/neon
LDFLAGS = 
NEON_LINK_FLAGS = 
# Note: don't substitute  in here; during a bundled
# build of this directory,  may include -lneon.
LIBS = -L$(top_builddir)/lib/neon -lneon  -L/usr/lib -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err -lexpat 

COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS)
LINK = $(LIBTOOL) --quiet --mode=link $(CC) -no-undefined $(LDFLAGS)

NEON_BASEOBJS = ne_request.o ne_session.o 	    \
	ne_basic.o  ne_string.o 		    \
	ne_uri.o ne_dates.o ne_alloc.o  \
	ne_md5.o ne_utils.o    \
	ne_socket.o ne_auth.o 			    \
	ne_redirect.o ne_compress.o		    \
	ne_i18n.o

NEON_DAVOBJS = $(NEON_BASEOBJS) \
	ne_207.o ne_xml.o \
	ne_props.o ne_locks.o \
	ne_xmlreq.o

OBJECTS =  $(NEON_DAVOBJS)  ne_stubssl.o

.SUFFIXES:
.SUFFIXES: .c .lo .o

NEON_TARGET = libneon.a

# Thanks to gettext for this neat trick.
all: all-yes

all-yes: $(NEON_TARGET)
all-no:
	@echo "Bundled neon build not being used."

.c.lo:
	$(LIBTOOL) --quiet --mode=compile $(COMPILE) -c $< -o $@
.c.o:
	$(COMPILE) -c $< -o $@

libneon.la: $(OBJECTS)
	$(LINK) -rpath $(libdir) $(NEON_LINK_FLAGS) -o $@ $(OBJECTS) $(LIBS)

libneon.a: $(OBJECTS)
	@rm -f $@
	$(AR) cru $@ $(OBJECTS)
	$(RANLIB) $@

clean:
	rm -f $(NEON_TARGET) *.o *.lo *.bbg *.bb *.gc*
	rm -rf .libs

c++.c:
	find . -name ne_\*.h -print | sed 's/.*/#include "&"/;/ne_priv/d' > $@
	echo "int main(void) {}" >> $@

check-c++: c++.c
	c++ -I. c++.c

check-incl:
	@for f in ne_*.h; do \
	 echo Checking $$f...; \
	 echo "#include \"$$f\"" > checkincl.c; \
	 $(COMPILE) -c checkincl.c -o checkincl.o || exit 1; done

neonreq = ne_request.h ne_session.h ne_utils.h ne_string.h ne_socket.h \
	ne_alloc.h $(top_builddir)/config.h ne_internal.h ne_uri.h

ne_request.o: ne_request.c $(neonreq) ne_private.h ne_uri.h

ne_session.o: ne_session.c ne_session.h ne_alloc.h \
	ne_utils.h ne_private.h ne_uri.h $(top_builddir)/config.h

ne_openssl.o: ne_openssl.c ne_session.h ne_ssl.h ne_privssl.h \
	ne_private.h $(top_builddir)/config.h

ne_gnutls.o: ne_gnutls.c ne_session.h ne_ssl.h ne_privssl.h \
	ne_private.h $(top_builddir)/config.h

ne_socket.o: ne_socket.c ne_socket.h $(top_builddir)/config.h \
	ne_privssl.h ne_string.h

ne_auth.o: ne_auth.c ne_auth.h $(neonreq) ne_private.h \
	ne_dates.h ne_md5.h ne_uri.h 

ne_basic.o: ne_basic.c ne_basic.h $(neonreq)

ne_utils.o: ne_utils.c $(top_builddir)/config.h \
	ne_utils.h ne_dates.h

ne_xml.o: ne_xml.c ne_xml.h ne_string.h ne_utils.h \
	$(top_builddir)/config.h

ne_207.o: ne_207.c ne_207.h ne_xml.h ne_uri.h \
	$(top_builddir)/config.h ne_utils.h ne_internal.h

ne_string.o: ne_string.c ne_string.h ne_alloc.h \
	$(top_builddir)/config.h

ne_alloc.o: ne_alloc.c ne_alloc.h $(top_builddir)/config.h

ne_dates.o: ne_dates.c ne_dates.h $(top_builddir)/config.h

ne_uri.o: ne_uri.c ne_uri.h ne_utils.h ne_string.h ne_alloc.h \
	$(top_builddir)/config.h

ne_md5.o: ne_md5.c ne_md5.h $(top_builddir)/config.h

ne_props.o: ne_props.c $(top_builddir)/config.h \
	ne_props.h ne_207.h ne_xml.h $(neonreq)

ne_locks.o: ne_locks.c $(neonreq) ne_locks.h ne_207.h ne_xml.h

ne_redirect.o: ne_redirect.c $(neonreq) ne_redirect.h \
	ne_uri.h ne_private.h

ne_compress.o: ne_compress.c $(neonreq) ne_compress.h

ne_acl.o: ne_acl.c ne_acl.h $(neonreq)

ne_xmlreq.o: ne_xmlreq.c ne_xmlreq.h ne_xml.h $(neonreq)

ne_stubssl.o: ne_stubssl.c $(neonreq)

ne_i18n.o: ne_i18n.c ne_i18n.h $(top_builddir)/config.h
