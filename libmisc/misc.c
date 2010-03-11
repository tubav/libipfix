/*

 */
/**************************************************************************
 *
 * misc.c - miscellaneous io and help functions etc.
 *
 * Copyright Fraunhoer FOKUS
 *
 * $Date: 2010-02-24 15:32:09 +0100 (Wed, 24 Feb 2010) $
 *
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/param.h>

#include "misc.h"

/*------ revision id -----------------------------------------------------*/

static char const cvsid[]="$Id: misc.c 1122 2010-02-24 14:32:09Z csc $";

/*------ export funcs ----------------------------------------------------*/

#if !defined(HAVE_BASENAME)

char *basename(const char *path)
{
        static char bname[MAXPATHLEN];
        char        *endp, *startp;

        /* Empty or NULL string gets treated as "." */
        if (path == NULL || *path == '\0') {
                (void)strcpy(bname, ".");
                return(bname);
        }

        /* Strip trailing slashes */
        endp = (char*)path + strlen(path) - 1;
        while (endp > path && *endp == '/')
                endp--;

        /* All slashes becomes "/" */
        if (endp == path && *endp == '/') {
                (void)strcpy(bname, "/");
                return(bname);
        }

        /* Find the start of the base */
        startp = endp;
        while (startp > path && *(startp - 1) != '/')
                startp--;

        if ((size_t)(endp - startp + 2) > sizeof(bname)) {
                errno = ENAMETOOLONG;
                return(NULL);
        }
        (void)strncpy(bname, startp, endp - startp + 1);
        bname[endp - startp + 1] = '\0';
        return(bname);
}

char *dirname(const char *path)
{
        static char bname[MAXPATHLEN];
        char        *endp;

        /* Empty or NULL string gets treated as "." */
        if (path == NULL || *path == '\0') {
                (void)strcpy(bname, ".");
                return(bname);
        }

        /* Strip trailing slashes */
        endp = (char*)path + strlen(path) - 1;
        while (endp > path && *endp == '/')
                endp--;

        /* Find the start of the dir */
        while (endp > path && *endp != '/')
                endp--;

        /* Either the dir is "/" or there are no slashes */
        if (endp == path) {
                (void)strcpy(bname, *endp == '/' ? "/" : ".");
                return(bname);
        } else {
                do {
                        endp--;
                } while (endp > path && *endp == '/');
        }

        if ((size_t)(endp - path + 2) > sizeof(bname)) {
                errno = ENAMETOOLONG;
                return(NULL);
        }
        (void)strncpy(bname, path, endp - path + 1);
        bname[endp - path + 1] = '\0';
        return(bname);
}

#endif

#ifndef HAVE_DAEMON

int daemon( int nochdir, int noclose )
{
    int fd;

    switch (fork()) {
      case -1:
          return (-1);
      case 0:
          break;
      default:
          _exit(0);
    }

    if (setsid() == -1)
        return (-1);

    if (!nochdir)
        (void)chdir("/");

    if (!noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        (void)dup2(fd, STDIN_FILENO);
        (void)dup2(fd, STDOUT_FILENO);
        (void)dup2(fd, STDERR_FILENO);
        if (fd > 2)
            (void)close(fd);
    }
    return (0);
}

#endif

/*--------------------------------------------------
 * Name      : writen
 * Parameter : > int   fd      file 
 *             > char  *ptr    buffer
 *             > int   nbytes  number of bytes to write
 * Purpose   : write 'n' bytes to file
 * Remarks   : replacement of write if fd is a
 *             stream socket.
 * Returns number of written bytes.
 *-------------------------------------------------*/

int writen (int  fd,
	    char  *ptr,
	    int   nbytes)
{
    int     nleft, nwritten;

    nleft = nbytes;
    while (nleft > 0)
    {
	nwritten = write(fd, ptr, nleft);
	if (nwritten <= 0)
	    return(nwritten);               /* error */

	nleft -= nwritten;
	ptr   += nwritten;
    }
    return(nbytes - nleft);
}

/*--------------------------------------------------
 * Name      : read_line
 * Parameter : >  int   fd      file 
 *             <> char  *ptr    buffer
 *             >  int   maxlen  max. line length
 * Purpose   : Read a line from a descriptor
 * Remarks   :
 *  Read the line one byte at a time, looking for
 *  the newline.  We store the newline in the buffer,
 *  then follow it with a null (the same as fgets(3)).
 *  We return the number of characters up to, but not
 *  including, the null (the same as strlen(3)).
 --------------------------------------------------*/

#ifdef __STDC__
int read_line ( int fd, char *ptr, int maxlen)
#else
int read_line(fd, ptr, maxlen)
    int    fd;
    char   *ptr;
    int    maxlen;
#endif
{
    int     n, rc;
    char    c;

    for (n = 1; n < maxlen; n++)
    {
	if ( (rc = read(fd, &c, 1)) == 1)
	{
	    *ptr++ = c;
	    if (c == '\n')
		break;

	} else if (rc == 0)
	{
	    if (n == 1)
		return(0); /* EOF, no data read */
	    else
		break; /* EOF, some data was read */
	}
	else
	    return(-1);	/* error */
    }
    *ptr = 0;

    return(n);

}/*read_line*/

/*--------------------------------------------------
 * Name      : readselect
 * Parameter : >  int   fd      file descr 
 *             >  int   sec     time
 * Returns   : 1  <- input pending
 *             0  <- otherwise
 * Purpose   : check input fd
 * Remarks   :
 --------------------------------------------------*/

int readselect ( int fd, int sec)
{
    fd_set         perm;
    int            fds, ret;
    struct timeval timeout;

    FD_ZERO (&perm);
    FD_SET  (fd, &perm);
    fds = fd +1;

    timeout.tv_sec  = sec;
    timeout.tv_usec = 1;

    ret = select (fds, &perm, NULL, NULL, &timeout );

    return (FD_ISSET(fd, &perm));

}/*readselect*/

/*--------------------------------------------------
 * Name         : mgettimestr
 * Parameter    : > time_t t
 * Purpose      : print timestring
 * Return values: timestring
 *-------------------------------------------------*/

char *mgettimestr ( time_t t )
{
    static char timestring[31];

    strftime( timestring, 30, "%Y-%m-%d %T", localtime( &t ));
    return timestring;
}

/*--------------------------------------------------
 * Name         : timegm
 * Parameter    : > filled struct tm
 * Purpose      : implement the GNU timegm func
 * Return values: unix time
 *-------------------------------------------------*/

#ifndef HAVE_TIMEGM

static int is_leap(unsigned y)
{
    y += 1900;
    return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

time_t timegm (struct tm *tm)
{
    static const unsigned ndays[2][12] ={
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
    time_t   res = 0;
    int      i;

    for (i = 70; i < tm->tm_year; ++i)
        res += is_leap(i) ? 366 : 365;

    for (i = 0; i < tm->tm_mon; ++i)
        res += ndays[is_leap(tm->tm_year)][i];
    res += tm->tm_mday - 1;
    res *= 24;
    res += tm->tm_hour;
    res *= 60;
    res += tm->tm_min;
    res *= 60;
    res += tm->tm_sec;
    return res;
}

#endif /* HAVE_TIMEGM */

