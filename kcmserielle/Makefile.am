kde_module_LTLIBRARIES = kcm_serielle.la

kcm_serielle_la_SOURCES =  schemaeditor.cpp sessioneditor.cpp kcmkonsole.cpp \
	schemadialog.ui sessiondialog.ui kcmkonsoledialog.ui

kcm_serielle_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_serielle_la_LIBADD = -lkdeui $(LIB_KIO)
AM_CPPFLAGS = $(all_includes) 

METASOURCES = AUTO 

noinst_HEADERS = kcmkonsole.h schemaeditor.h sessioneditor.h schemadialog.h sessiondialog.h kcmkonsoledialog.h

kcmserielle_data_DATA = kcmserielle.desktop
kcmserielle_datadir = $(kde_appsdir)/.hidden

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmserielle.pot
