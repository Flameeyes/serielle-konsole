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

bool TETty::setFlowControl(FlowControl flow)
{
  if (ttyfd < 0) return false;

  struct ::termios options;

  _tcgetattr(ttyfd, &options);

  options.c_iflag &= ~(IXOFF | IXON);
  options.c_cflag &= ~(CRTSCTS);
  switch(flow) {
  case fcNone: break;
  case fcSoftware:
    options.c_iflag |= (IXOFF | IXON);
    break;
  case fcHardware:
    options.c_cflag |= CRTSCTS;
  }

  if ( _tcsetattr(ttyfd, &options) < 0 ) {
    qWarning("FlowControl %d cannot be set.", flow);
    return false;
  }

  return true;
}

bool TETty::setSpeed(int speed) {
  if ( ttyfd < 0 ) return false;

  static const int baudrates[] = {
    300, 600, 1200, 2400, 4800, 9600,
    19200, 38400, 57600, 115200
  };
  static const int baudconsts[] = {
    B300, B600, B1200, B2400, B4800, B9600,
    B19200, B38400, B57600, B115200, -1
  };

  size_t i;
  for (i = 0; i < sizeof(baudrates)/sizeof(baudrates[0]); i++)
    if ( baudrates[i] == speed ) break;

  if ( baudconsts[i] == -1 ) {
    qWarning("Invalid or unsupported speed %d", speed);
    return false;
  }

  struct ::termios options;

  _tcgetattr(ttyfd, &options);
  cfsetispeed(&options, baudconsts[i]);
  cfsetospeed(&options, baudconsts[i]);

  if ( _tcsetattr(ttyfd, &options) < 0 ) {
    qWarning("Speed %d cannot be set.", speed);
    return false;
  }

  return true;
}

bool TETty::setParity(Parity parity)
{
  if (ttyfd < 0) return false;

  struct ::termios options;

  _tcgetattr(ttyfd, &options);

  options.c_cflag &= ~(PARENB|PARODD);
  switch(parity) {
  case parNone: break;
  case parOdd: options.c_cflag |= PARODD; /* Fall through intended */
  case parEven: options.c_cflag |= PARENB; break;
  }

  if ( _tcsetattr(ttyfd, &options) < 0 ) {
    qWarning("Parity %d cannot be set.", parity);
    return false;
  }

  return true;
}

bool TETty::setBits(uint8_t bits) {
  if ( ttyfd < 0 ) return false;

  struct ::termios options;

  _tcgetattr(ttyfd, &options);
  options.c_cflag &= ~(CS5|CS6|CS7|CS8);

  switch(bits) {
  case 5: options.c_cflag |= CS5; break;
  case 6: options.c_cflag |= CS6; break;
  case 7: options.c_cflag |= CS7; break;
  case 8: options.c_cflag |= CS8; break;
  default:
    qWarning("Invalid bits count %d.", bits);
    return false;
  }

  if ( _tcsetattr(ttyfd, &options) < 0 ) {
    qWarning("Bits %d cannot be set.", bits);
    return false;
  }

  return true;
}

bool TETty::setStopBits(uint8_t stopbits) {
  if ( ttyfd < 0 ) return false;

  struct ::termios options;

  _tcgetattr(ttyfd, &options);
  options.c_cflag &= ~(CSTOPB);

  switch(stopbits) {
  case 1: break;
  case 2: options.c_cflag |= CSTOPB; break;
  default:
    qWarning("Invalid stop bits count %d.", stopbits);
    return false;
  }

  if ( _tcsetattr(ttyfd, &options) < 0 ) {
    qWarning("Stop bits %d cannot be set.", stopbits);
    return false;
  }

  return true;
}

/*!
    Create an instance.
*/
TETty::TETty(const QString &_tty)
{
  m_bufferFull = false;
  ttyName = _tty;

  ttyfd = open(ttyName.latin1(), O_RDWR|O_NOCTTY|O_NONBLOCK);

  // without the '::' some version of HP-UX thinks, this declares
  // the struct in this class, in this method, and fails to find
  // the correct tc[gs]etattr
  struct ::termios options;
  _tcgetattr(ttyfd, &options);
  qWarning("Current c_oflag: %08x", options.c_oflag);

  options.c_cflag |= (CLOCAL | CREAD);
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  _tcsetattr(ttyfd, &options);

  m_readNotifier = new QSocketNotifier( ttyfd, QSocketNotifier::Read, this );
  connect( m_readNotifier, SIGNAL(activated(int)), this, SLOT(dataReceived()) );
  m_writeNotifier = new QSocketNotifier( ttyfd, QSocketNotifier::Write, this );
  connect( m_writeNotifier, SIGNAL(activated(int)), this, SLOT(writeReady()) );
}

/*!
    Destructor.
*/
TETty::~TETty()
{
  delete m_readNotifier;
  delete m_writeNotifier;
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
  pendingSendJobs.remove(pendingSendJobs.begin());
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
