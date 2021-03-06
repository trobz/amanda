#
# Copyright (c) 1998,1999,2000
#	Traakan, Inc., Los Altos, CA
#	All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice unmodified, this list of conditions, and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

#
# Project:  NDMJOB
# Ident:    $Id: $
#
# Description
#
#	This illustrates the strata (layers) of the
#	NDMJOB/NDMJOBLIB software, the scope of key
#	header (.h) files, and the source files
#	constituting each layer.
#
#  -  -  -  -  -	+---------------------------------------+
#  ^  ^  ^  ^  ndmjob.h	| NDMJOB Command	ndmjob_*.c	|
#  |  |  |  |  -	+---------------------------------------+
#  |  |  |  |		          NDMJOBLIB API "job"
#  |  |  |  |		+---------------------------------------+  \
#  |  |  |  |		| Rules       (NDMJLR)  ndmjr_*.[ch]	|   \
#  |  |  |  ndmagents.h	| Agents      (NDMJLA)  ndma_*.c	|    |
#  |  |  |  -		+---------------------------------------+    |
#  |  |  ndmlib.h	| Library     (NDMJLL)  ndml_*.c	|    |
#  |  |  -		+---------------------------------------+    |
#  |  ndmprotocol.h	| Protocol    (NDMJLP)  ndmp*.[chx]	| NDMJOBLIB
#  |  -			+---------------------------------------+    |
#  |			| SMC         (NDMJLS)  smc*.[ch]	|    |
#  |			| MD5         (NDMJL5)  md5*.[ch]	|    |
#  |			+---------------------------------------+    |
#  ndmos.h		| OS intf     (NDMJLO)  ndmos*.[ch]	|   /
#  -			+---------------------------------------+  /
#


#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_CONTROL_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_DATA_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_TAPE_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_ROBOT_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_NDMP2
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_NDMP3
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_NDMP4

include $(top_srcdir)/config/automake/vars.am
include $(top_srcdir)/config/automake/installperms.am
include $(top_srcdir)/config/automake/precompile.am

INCLUDES =	-I$(top_builddir)/common-src \
		-I$(top_srcdir)/common-src   \
		-I$(top_srcdir)/gnulib

# TODO: use existing md5 facility? (openssl?)

# TODO: tests!

# add the Amanda version to the ndmjoblib version info
AM_CFLAGS = -DNDMOS_CONST_NDMJOBLIB_REVISION='"amanda-$(VERSION)"' $(AMANDA_FILE_CFLAGS)
# note that this directory is compiled *without* the usual Amanda warnings,
# because the rpcgen-generated code (including header files) generates copious
# warnings and cannot be changed, and cannot be ignored in older versions of
# gcc.

##
## libndmjob.la and libndmlib.la
##

# Divide the library up into the library side and down (libndmlib.la),
# and the full set of rules, agents, and so on required to build ndmjob
# (libndmjob.la).

amlib_LTLIBRARIES = \
	libndmlib.la \
	libndmjob.la

libndmjla = \
	ndma_comm_job.c \
	ndma_comm_session.c \
	ndma_comm_dispatch.c \
	ndma_comm_subr.c \
	ndma_control.c \
	ndma_cops_backreco.c \
	ndma_cops_labels.c \
	ndma_cops_query.c \
	ndma_cops_robot.c \
	ndma_ctrl_calls.c \
	ndma_ctrl_conn.c \
	ndma_ctrl_media.c \
	ndma_ctrl_robot.c \
	ndma_ctst_tape.c \
	ndma_ctst_mover.c \
	ndma_ctst_data.c \
	ndma_ctst_subr.c \
	ndma_data.c \
	ndma_data_fh.c \
	ndma_data_pfe.c \
	ndma_image_stream.c \
	ndma_noti_calls.c \
	ndma_tape.c \
	ndma_robot.c \
	ndma_robot_simulator.c \
	wraplib.c

libndmjlr = \
	ndmjr_none.c

libndmjll = \
	ndml_agent.c \
	ndml_bstf.c \
	ndml_chan.c \
	ndml_config.c \
	ndml_conn.c \
	ndml_cstr.c \
	ndml_log.c \
	ndml_md5.c \
	ndml_fhdb.c \
	ndml_fhh.c \
	ndml_media.c \
	ndml_nmb.c \
	ndml_scsi.c \
	ndml_stzf.c \
	ndml_util.c

libndmjlp = \
	ndmprotocol.c \
	ndmp_translate.c \
	ndmp2_translate.c ndmp3_translate.c ndmp4_translate.c \
	ndmp0_enum_strs.c ndmp0_pp.c ndmp0_xdr.c ndmp0_xmt.c \
	ndmp2_enum_strs.c ndmp2_pp.c ndmp2_xdr.c ndmp2_xmt.c \
	ndmp3_enum_strs.c ndmp3_pp.c ndmp3_xdr.c ndmp3_xmt.c \
	ndmp4_enum_strs.c ndmp4_pp.c ndmp4_xdr.c ndmp4_xmt.c \
	ndmp9_enum_strs.c            ndmp9_xdr.c ndmp9_xmt.c

libndmjls = \
	smc_api.c \
	smc_parse.c \
	smc_pp.c

libndmjl5 = \
	md5c.c

libndmjlo = \
	ndmos.c \
	ndmos_common.c

libndmlib_la_LDFLAGS = -release $(VERSION) $(AS_NEEDED_FLAGS)
libndmlib_la_SOURCES = \
	$(libndmjll) \
	$(libndmjlp) \
	$(libndmjls) \
	$(libndmjl5) \
	$(libndmjlo) \
	ndmpconnobj.c

libndmjob_la_SOURCES = \
	$(libndmjla) \
	$(libndmjlr)

libndmjob_la_LDFLAGS = -release $(VERSION) $(AS_NEEDED_FLAGS)
libndmjob_la_LIBADD = libndmlib.la

##
## ndmjob -- used for testing and by applications
##

amlibexec_PROGRAMS = amndmjob ndmjob

ndmjob_SOURCES = ndmjob_main.c \
		      ndmjob_args.c \
		      ndmjob_job.c \
		      ndmjob_rules.c \
		      ndmjob_main_util.c \
		      ndma_tape_simulator.c


amndmjob_SOURCES = amndmjob_main.c \
		      ndmjob_args.c \
		      ndmjob_job.c \
		      ndmjob_rules.c \
		      ndmjob_main_util.c \
		      amndma_tape_simulator.c

ndmjob_LDADD = libndmjob.la \
		   ../common-src/libamanda.la

amndmjob_LDADD = libndmjob.la \
		   ../common-src/libamanda.la

ndmp0_xdr.c : ndmp0.x
	rm -f ndmp0_xdr.c
	rm -f ndmp0.h
	$(RPCGEN) -c -o ndmp0_xdr.c $(srcdir)/ndmp0.x
	$(RPCGEN) -h -o ndmp0.h $(srcdir)/ndmp0.x
	sed 's+#include "$(srcdir)\/ndmp0.h"+#include "config.h"\
\#include "ndmp0.h"+' ndmp0_xdr.c > ndmp0_xdr.c.new ; mv ndmp0_xdr.c.new ndmp0_xdr.c

ndmp0.h : ndmp0_xdr.c
	@

ndmp2_xdr.c : ndmp2.x
	rm -f ndmp2_xdr.c
	rm -f ndmp2.h
	$(RPCGEN) -c -o ndmp2_xdr.c $(srcdir)/ndmp2.x
	$(RPCGEN) -h -o ndmp2.h $(srcdir)/ndmp2.x
	sed 's+#include "$(srcdir)\/ndmp2.h"+#include "config.h"\
\#include "ndmp2.h"+' ndmp2_xdr.c > ndmp2_xdr.c.new ; mv ndmp2_xdr.c.new ndmp2_xdr.c

ndmp2.h : ndmp2_xdr.c
	@

ndmp3_xdr.c : ndmp3.x
	rm -f ndmp3_xdr.c
	rm -f ndmp3.h
	$(RPCGEN) -c -o ndmp3_xdr.c $(srcdir)/ndmp3.x
	$(RPCGEN) -h -o ndmp3.h $(srcdir)/ndmp3.x
	sed 's+#include "$(srcdir)\/ndmp3.h"+#include "config.h"\
\#include "ndmp3.h"+' ndmp3_xdr.c > ndmp3_xdr.c.new ; mv ndmp3_xdr.c.new ndmp3_xdr.c

ndmp3.h : ndmp3_xdr.c
	@

ndmp4_xdr.c : ndmp4.x
	rm -f ndmp4_xdr.c
	rm -f ndmp4.h
	$(RPCGEN) -c -o ndmp4_xdr.c $(srcdir)/ndmp4.x
	$(RPCGEN) -h -o ndmp4.h $(srcdir)/ndmp4.x
	sed 's+#include "$(srcdir)\/ndmp4.h"+#include "config.h"\
\#include "ndmp4.h"+' ndmp4_xdr.c > ndmp4_xdr.c.new ; mv ndmp4_xdr.c.new ndmp4_xdr.c

ndmp4.h : ndmp4_xdr.c
	@

ndmp9_xdr.c : ndmp9.x
	rm -f ndmp9_xdr.c
	rm -f ndmp9.h
	$(RPCGEN) -c -o ndmp9_xdr.c $(srcdir)/ndmp9.x
	$(RPCGEN) -h -o ndmp9.h $(srcdir)/ndmp9.x
	sed 's+#include "$(srcdir)\/ndmp9.h"+#include "config.h"\
\#include "ndmp9.h"+' ndmp9_xdr.c > ndmp9_xdr.c.new ; mv ndmp9_xdr.c.new ndmp9_xdr.c

ndmp9.h : ndmp9_xdr.c
	@

# cause these files to be built before those that might depend on them
BUILT_SOURCES += \
	ndmp0.h \
	ndmp0_xdr.c \
	ndmp2.h \
	ndmp2_xdr.c \
	ndmp3.h \
	ndmp3_xdr.c \
	ndmp4.h \
	ndmp4_xdr.c \
	ndmp9.h \
	ndmp9_xdr.c

noinst_HEADERS = \
	md5.h            ndmp0_enum_strs.h  ndmp3.x            ndmprotocol.h \
	ndmagents.h      ndmp0.h            ndmp4_enum_strs.h  ndmp_translate.h \
	ndmjob.h         ndmp0.x            ndmp4.h            scsiconst.h \
	ndmjr_none.h     ndmp2_enum_strs.h  ndmp4_translate.h  smc.h \
	ndmlib.h         ndmp2.h            ndmp4.x            smc_priv.h \
	ndmp2_translate.h  ndmp9_enum_strs.h  smc_raw.h \
	ndmos.h          ndmp2.x            ndmp9.h            \
	ndmp3_enum_strs.h  ndmp9.x            \
	ndmp3.h            ndmp_ammend.h      wraplib.h \
	ndmp3_translate.h  ndmp_msg_buf.h \
	ndmos_glib.h \
	ndmpconnobj.h

EXTRA_DIST += \
	ndmos_glib.c \
	ndmos_common.c \
	README \
	ShakeOut.sh \
	ThingsToTry \
	idx_dump.pl

install-build-dependencies: install-data-am
	${SED} -e 's@ ${amlibdir}/libam@ ${DESTDIR}${amlibdir}/libam@g' -i ${DESTDIR}/${amlibdir}/libndmlib.la

