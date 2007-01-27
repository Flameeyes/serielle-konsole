/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

#ifndef SESSION_H
#define SESSION_H

#include <kapplication.h>
#include <kmainwindow.h>
#include <qstrlist.h>

#include "TETty.h"
#include "TEWidget.h"
#include "TEmuVt102.h"

#include "sessioniface.h"

class KProcIO;
class KProcess;
class ZModemDialog;

class TESession : public QObject, virtual public SessionIface
{ Q_OBJECT

public:

  TESession(TEWidget* w, const QString &device,
	    ulong winId, const QString &sessionId="session-1");
  void changeWidget(TEWidget* w);
  void setPty( TETty *_sh );
  TEWidget* widget() { return te; }
  ~TESession();

  void        setConnect(bool r);  // calls setListenToKeyPress(r)
  void        setListenToKeyPress(bool l);
  TEmulation* getEmulation();      // to control emulation;
  bool        isSecure();
  bool        isMonitorActivity();
  bool        isMonitorSilence();
  bool        isMasterMode();
  int schemaNo();
  int encodingNo();
  int fontNo();
  const QString& Device();
  const QString& SessionId();
  const QString& Title();
  const QString& IconName();
  const QString& IconText();
  QString fullTitle() const;
  int keymapNo();
  QString keymap();

  void setHistory(const HistoryType&);
  const HistoryType& history();

  void setMonitorActivity(bool);
  void setMonitorSilence(bool);
  void setMonitorSilenceSeconds(int seconds);
  void setMasterMode(bool);
  void setSchemaNo(int sn);
  void setEncodingNo(int index);
  void setKeymapNo(int kn);
  void setKeymap(const QString& _id);
  void setFontNo(int fn);
  void setTitle(const QString& _title);
  void setIconName(const QString& _iconName);
  void setIconText(const QString& _iconText);
  bool testAndSetStateIconName (const QString& newname);

  // Additional functions for DCOP
  bool closeSession();
  void clearHistory();
  void feedSession(const QString &text);
  void sendSession(const QString &text);
  void renameSession(const QString &name);
  QString sessionName() { return title; }

  virtual bool processDynamic(const QCString &fun, const QByteArray &data, QCString& replyType, QByteArray &replyData);
  virtual QCStringList functionsDynamic();
  void enableFullScripting(bool b) { fullScripting = b; }

  bool sendBreak();
  void startZModem(const QString &rz, const QString &dir, const QStringList &list);
  void cancelZModem();
  bool zmodemIsBusy() { return zmodemBusy; }

  void print(QPainter &paint, bool friendly, bool exact);

  QString schema();
  void setSchema(const QString &schema);
  QString encoding();
  void setEncoding(const QString &encoding);
  QString keytab();
  void setKeytab(const QString &keytab);
  QSize size();
  void setSize(QSize size);
  void setFont(const QString &font);
  QString font();

public slots:

  void run();
  void done();
  void terminate();
  void setUserTitle( int, const QString &caption );
  void changeTabTextColor( int );
  void slotZModemDetected();
  void emitZModemDetected();

  void zmodemStatus(KProcess *, char *data, int len);
  void zmodemSendBlock(KProcess *, char *data, int len);
  void zmodemRcvBlock(const char *data, int len);
  void zmodemDone();
  void zmodemContinue();

  void setFlowControl(TETty::FlowControl flow)
  { sh->setFlowControl(flow); }

  void setSpeed(int speed)
  { sh->setSpeed(speed); }

  void setParity(TETty::Parity parity)
  { sh->setParity(parity); }

  void setBits(uint8_t bits)
  { sh->setBits(bits); }

  void setStopBits(uint8_t stopbits)
  { sh->setStopBits(stopbits); }

signals:

  void receivedData( const QString& text );
  void done(TESession*);
  void updateTitle(TESession*);
  void notifySessionState(TESession* session, int state);
  void changeTabTextColor( TESession*, int );

  void disableMasterModeConnections();
  void enableMasterModeConnections();
  void renameSession(TESession* ses, const QString &name);

  void openURLRequest(const QString &cwd);

  void zmodemDetected(TESession *);
  void updateSessionConfig(TESession *);
  void resizeSession(TESession *session, QSize size);
  void setSessionEncoding(TESession *session, const QString &encoding);
  void getSessionSchema(TESession *session, QString &schema);
  void setSessionSchema(TESession *session, const QString &schema);

private slots:
  void onRcvBlock( const char* buf, int len );
  void monitorTimerDone();
  void notifySessionState(int state);
  void onContentSizeChange(int height, int width);
  void onFontMetricChange(int height, int width);

private:

  TETty*         sh;
  TEWidget*      te;
  TEmulation*    em;

  bool           connected;
  bool           monitorActivity;
  bool           monitorSilence;
  bool           notifiedActivity;
  bool           masterMode;
  bool           wantedClose;
  QTimer*        monitorTimer;

  //FIXME: using the indices here
  // is propably very bad. We should
  // use a persistent reference instead.
  int            schema_no;
  int            font_no;
  int            silence_seconds;

  int            font_h;
  int            font_w;

  QString        title;
  QString        userTitle;
  QString        iconName;
  QString        iconText; // as set by: echo -en '\033]1;IconText\007
  bool           fullScripting;

  QString	 stateIconName;

  QString        device;
  ulong          winId;
  QString        sessionId;

  // ZModem
  bool           zmodemBusy;
  KProcIO*       zmodemProc;
  ZModemDialog*  zmodemProgress;

  // Color/Font Changes by ESC Sequences

  QColor         modifiedBackground; // as set by: echo -en '\033]11;Color\007
  int            encoding_no;
};

#endif
