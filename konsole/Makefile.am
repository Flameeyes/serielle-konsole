# This file is part of Konsole - an X terminal for KDE

# set the include path for X, qt and KDE
INCLUDES = -I. $(all_includes)

bin_PROGRAMS =
noinst_PROGRAMS = fontembedder
lib_LTLIBRARIES = 
kdeinit_LTLIBRARIES = serielle_konsole.la

fontembedder_SOURCES = fontembedder.cpp
fontembedder_LDFLAGS = $(all_libraries)
fontembedder_LDADD = $(LIB_QT)

# konsole kdeinit module
serielle_konsole_la_SOURCES = TETty.cpp BlockArray.cpp main.cpp konsole.cpp schema.cpp session.cpp TEWidget.cpp TEmuVt102.cpp \
     TEScreen.cpp TEmulation.cpp TEHistory.cpp keytrans.cpp konsoleiface.skel sessioniface.skel \
     konsole_wcwidth.cpp konsolebookmarkhandler.cpp konsolebookmarkmenu.cpp \
     zmodem_dialog.cpp printsettings.cpp
serielle_konsole_la_LDFLAGS = $(all_libraries) -module -avoid-version
serielle_konsole_la_LIBADD = $(LIB_KDEUI) $(LIB_KIO) $(LIB_KDEPRINT) $(LIBUTIL) $(XTESTLIB) $(LIB_XRENDER)

noinst_HEADERS = TEWidget.h TETty.h TEmulation.h TEmuVt102.h \
	TECommon.h TEScreen.h konsole.h schema.h session.h konsole_wcwidth.h \
	TEHistory.h keytrans.h default.keytab.h BlockArray.h \
        konsolebookmarkhandler.h konsolebookmarkmenu.h zmodem_dialog.h \
        printsettings.h linefont.h

METASOURCES = AUTO

messages: rc.cpp
	rm -f schemas.cpp
	rm -f tips.cpp
	(cd .. && $(PREPARETIPS) > konsole/tips.cpp)
	for i in ../other/*.schema; do \
	  grep "^title" $$i | cut -d':' -f2- | sed -e 's#^title \(.*\)$$#i18n(\"\1\")#' >> schemas.cpp ;\
	done
	for i in ../other/*.Keytab ../other/*.keytab; do \
	  grep "^keyboard" $$i | sed -e 's#^keyboard \"\(.*\)\"$$#i18n(\"\1\")#' >> schemas.cpp ;\
	done
	$(XGETTEXT) *.cpp -o $(podir)/serielle-konsole.pot
	rm -f schemas.cpp
	rm -f tips.cpp

fonts: fontembedder
	./fontembedder $(srcdir)/linefont.src > linefont.h
