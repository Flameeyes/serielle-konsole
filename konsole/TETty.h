/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 2007 Diego Pettenò <flameeyes@gmail.com>

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

#ifndef TE_TTY_H
#define TE_TTY_H

#include <config.h>

#include <qsocketnotifier.h>
#include <qstrlist.h>
#include <qvaluelist.h>
#include <qmemarray.h>

#include <pty.h>

class TETty : public QObject
{
Q_OBJECT

  public:

    TETty(const QString &_tty);
    ~TETty();

  public:

    QString error() { return m_strError; }
    void setXonXoff(bool on);
    void setSize(int lines, int cols);
    void setErase(char erase);

  public slots:
    void useUtf8(bool on);
    void send_bytes(const char* s, int len);
    bool sendBreak();

  signals:

    /*!
        emitted when a new block of data comes in.
        \param s - the data
        \param len - the length of the block
    */
    void block_in(const char* s, int len);
    
    /*!
        emitted when buffer_full becomes false
    */
    void buffer_empty();

  public:
    void send_byte(char s);
    void send_string(const char* s);
    bool buffer_full() { return m_bufferFull; }

  private:
    void appendSendJob(const char* s, int len);

  private slots:
    void doSendJobs();
    void writeReady();
    void dataReceived();

  private:

    QString m_strError;

    struct SendJob {
      SendJob() {}
      SendJob(const char* b, int len) {
        buffer.duplicate(b,len);
        length = len;
      }
      QMemArray<char> buffer;
      int length;
    };
    QValueList<SendJob> pendingSendJobs;
    int ttyfd;
    QString ttyName;
    struct winsize winSize;

    QSocketNotifier *m_readNotifier;
    QSocketNotifier *m_writeNotifier;

    bool m_bufferFull:1;
};

#endif
