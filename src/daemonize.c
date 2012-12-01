#include <config.h>
#include "daemonize.h"
#include "ucarp.h"
#include "log.h"

#ifdef WITH_DMALLOC
# include <dmalloc.h>
#endif

static unsigned int open_max(void)
{
    long z;
    
    if ((z = (long) sysconf(_SC_OPEN_MAX)) < 0L) {
        logfile(LOG_ERR, "_SC_OPEN_MAX");
        _exit(EXIT_FAILURE);
    }
    return (unsigned int) z;
}

static int closedesc_all(const int closestdin)
{
    int fodder;
    
    if (closestdin != 0) {
        (void) close(0);
        if ((fodder = open("/dev/null", O_RDONLY)) == -1) {
            return -1;
        }
        (void) dup2(fodder, 0);
        if (fodder > 0) {
            (void) close(fodder);
        }
    }
    if ((fodder = open("/dev/null", O_WRONLY)) == -1) {
        return -1;
    }
    (void) dup2(fodder, 1);
    (void) dup2(1, 2);
    if (fodder > 2) {
        (void) close(fodder);
    }    
    return 0;
}

static int create_pid_file(void)
{
	FILE *pid_stream;
	pid_t pid;
	int p;

	if (pid_file==0)
		return 0;

	p = -1;

	if ((pid_stream=fopen(pid_file, "r"))!=NULL){
		if (fscanf(pid_stream, "%d", &p) < 0) {
			logfile(LOG_WARNING, "could not parse pid file %s\n", pid_file);
		}
		fclose(pid_stream);
		if (p==-1){
			logfile(LOG_ERR, "pid file %s exists, but doesn't contain a valid"
					" pid number\n", pid_file);
			return -1;
		}
		if (kill((pid_t)p, 0)==0 || errno==EPERM){
			logfile(LOG_ERR, "running process found in the pid file %s\n",
					pid_file);
				return -1;
		} else {
			logfile(LOG_WARNING, "pid file contains old pid, replacing pid\n");
		}
	}

	pid=getpid();
	if ((pid_stream=fopen(pid_file, "w"))==NULL){
		logfile(LOG_ERR, "unable to create pid file %s: %s\n",
				pid_file, strerror(errno));
		return -1;
	} else {
		fprintf(pid_stream, "%i\n", (int)pid);
		fclose(pid_stream);
	}
	return 0;
}

void dodaemonize(void)
{ 
    pid_t child;
    unsigned int i;

    /* Contributed by Jason Lunz - also based on APUI code, see open_max() */
    if (daemonize != 0) {
        if ((child = fork()) == (pid_t) -1) {
            logfile(LOG_ERR, _("Unable to get in background: [fork: %s]"),
                    strerror(errno));
            return;
        } else if (child != (pid_t) 0) {
            _exit(EXIT_SUCCESS);       /* parent exits */
        }         
        if (setsid() == (pid_t) -1) {
            logfile(LOG_WARNING,
                    _("Unable to detach from the current session: %s"),
                    strerror(errno));  /* continue anyway */
        }

        /* Fork again so we're not a session leader */
        if ((child = fork()) == (pid_t) -1) {
            logfile(LOG_ERR, _("Unable to background: [fork: %s] #2"),
                    strerror(errno));
            return;
        } else if ( child != (pid_t) 0) {
            _exit(EXIT_SUCCESS);       /* parent exits */
        }

        chdir("/");
        i = open_max();        
        do {
            if (isatty((int) i)) {
                (void) close((int) i);
            }
            i--;
        } while (i > 2U);
        if (closedesc_all(1) != 0) {
            logfile(LOG_ERR,
                    _("Unable to detach: /dev/null can't be duplicated"));
            _exit(EXIT_FAILURE);
        }
    }

	if(create_pid_file()<0) {
           _exit(EXIT_FAILURE);
	}
}

