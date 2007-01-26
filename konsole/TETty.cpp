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

/* If you're compiling konsole on non-Linux platforms and find
   problems that you can track down to this file, please have
   a look into ../README.ports, too.
*/

/*! \file
*/


/*! \class TETty

*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
                     
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <kpty.h>

#include "TETty.h"

#ifdef HAVE_TERMIOS_H
/* for HP-UX (some versions) the extern C is needed, and for other
   platforms it doesn't hurt */
extern "C" {
# include <termios.h>
}
#endif

#if !defined(__osf__)
# ifdef HAVE_TERMIO_H
/* needed at least on AIX */
#  include <termio.h>
# endif
#endif

#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || defined (__bsdi__) || defined(__APPLE__) || defined (__DragonFly__)
# define _tcgetattr(fd, ttmode) ioctl(fd, TIOCGETA, (char *)ttmode)
#else
# if defined(_HPUX_SOURCE) || defined(__Lynx__) || defined (__CYGWIN__)
#  define _tcgetattr(fd, ttmode) tcgetattr(fd, ttmode)
# else
#  define _tcgetattr(fd, ttmode) ioctl(fd, TCGETS, (char *)ttmode)
# endif
#endif

#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || defined (__bsdi__) || defined(__APPLE__) || defined (__DragonFly__)
# define _tcsetattr(fd, ttmode) ioctl(fd, TIOCSETA, (char *)ttmode)
#else
# if defined(_HPUX_SOURCE) || defined(__CYGWIN__)
#  define _tcsetattr(fd, ttmode) tcsetattr(fd, TCSANOW, ttmode) 
# else
#  define _tcsetattr(fd, ttmode) ioctl(fd, TCSETS, (char *)ttmode)
# endif
#endif

#if defined (_HPUX_SOURCE)
# define _TERMIOS_INCLUDED
# include <bsdtty.h>
#endif

// not defined on HP-UX for example
#ifndef CTRL
# define CTRL(x) ((x) & 037)
#endif

void TETty::setSize(int lines, int cols)
{
  winSize.ws_row = (unsigned short)lines;
  winSize.ws_col = (unsigned short)cols;
  if (ttyfd >= 0)
    ioctl( ttyfd, TIOCSWINSZ, (char *)&winSize );
}

void TETty::setXonXoff(bool on)
{
  if (ttyfd < 0) return;

  // without the '::' some version of HP-UX thinks, this declares
  // the struct in this class, in this method, and fails to find
  // the correct tc[gs]etattr
  struct ::termios ttmode;

  _tcgetattr(ttyfd, &ttmode);

  if (!on)
    ttmode.c_iflag &= ~(IXOFF | IXON);
  else
    ttmode.c_iflag |= (IXOFF | IXON);

  _tcsetattr(ttyfd, &ttmode);
}

void TETty::useUtf8(bool on)
{
  if ( ttyfd < 0 ) return;

#ifdef IUTF8
  // without the '::' some version of HP-UX thinks, this declares
  // the struct in this class, in this method, and fails to find
  // the correct tc[gs]etattr
  struct ::termios ttmode;

  _tcgetattr(ttyfd, &ttmode);

  if (!on)
    ttmode.c_iflag &= ~IUTF8;
  else
    ttmode.c_iflag |= IUTF8;

  _tcsetattr(ttyfd, &ttmode);
#endif
}

void TETty::setErase(char erase)
{
  struct ::termios tios;
  
  _tcgetattr(ttyfd, &tios);
  tios.c_cc[VERASE] = erase;

  _tcsetattr(ttyfd, &tios);
}

/*!
    Create an instance.
*/
TETty::TETty(const QString &_tty)
{
  m_bufferFull = false;
  ttyName = _tty;

  ttyfd = open(ttyName.latin1(), O_RDWR|O_NOCTTY|O_NONBLOCK);

  struct ::termios options;
  _tcgetattr(ttyfd, &options);
  cfsetispeed(&options, B115200);
  cfsetospeed(&options, B115200);
  options.c_cflag |= (CLOCAL | CREAD | CS8 | CRTSCTS);
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | PARENB | CSTOPB | CSIZE);
  _tcsetattr(ttyfd, &options);

  m_notifier = new QSocketNotifier( ttyfd, QSocketNotifier::Read, this );
  connect( m_notifier, SIGNAL(activated(int)), this, SLOT(dataReceived()) );
}

/*!
    Destructor.
*/
TETty::~TETty()
{
  delete m_notifier;
  close(ttyfd);
}

void TETty::dataReceived()
{
  char buffer[4096] = { 0, };
  int r = read( ttyfd, buffer, 4095 );

  emit block_in(buffer, r);
}

/*! sends a character through the line */
void TETty::send_byte(char c)
{
  send_bytes(&c,1);
}

/*! sends a 0 terminated string through the line */
void TETty::send_string(const char* s)
{
  send_bytes(s,strlen(s));
}

void TETty::writeReady()
{
  pendingSendJobs.remove(pendingSendJobs.begin());
  m_bufferFull = false;
  doSendJobs();
}

void TETty::doSendJobs() {
  if(pendingSendJobs.isEmpty())
  {
     emit buffer_empty();
     return;
  }
  
  SendJob& job = pendingSendJobs.first();
  int result = ::write(ttyfd, job.buffer.data(), job.length);
  if ( result < 0 )
  {
    qWarning("Uh oh.. can't write data..");
    return;
  }
  m_bufferFull = true;
}

void TETty::appendSendJob(const char* s, int len)
{
  pendingSendJobs.append(SendJob(s,len));
}

/*! sends len bytes through the line */
void TETty::send_bytes(const char* s, int len)
{
  appendSendJob(s,len);
  if (!m_bufferFull)
     doSendJobs();
}

bool TETty::sendBreak()
{
  if ( ttyfd < 0 ) return false;

  tcsendbreak(ttyfd, 0);
  return true;
}

#include "TETty.moc"
