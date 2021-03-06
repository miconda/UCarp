#include <config.h>
#include "ucarp.h"
#include "spawn.h"
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#include "log.h"

#ifdef WITH_DMALLOC
# include <dmalloc.h>
#endif

static int state_write(const char * const script)
{
	FILE *sf;
	const char *nstate;

	if(script==NULL || state_file==NULL)
		return 0;
	if(script==upscript) {
		nstate = "MASTER";
	} else if(script==downscript) {
		nstate = "BACKUP";
	} else {
		logfile(LOG_ERR, _("Cannot get new state from script: %s"), script);
		return -1;
	}

	if ((sf = fopen(state_file, "w"))==NULL) {
		logfile(LOG_ERR, _("Unable to open state file: %s"), state_file);
		return -1;
	}
	fprintf(sf, "%s iface=%s;vid=%u;lip=%s;vip=%s\n", nstate, interface,
			vhid, srcip_arg, vaddr_arg);
	fclose(sf);
	return 0;
}

int spawn_handler(const int dev_desc_fd, const char * const script)
{
    pid_t pid;
    
    if (script == NULL || *script == 0) {
        return 0;
    }
    pid = fork();
    if (pid == (pid_t) 0) {
        (void) close(dev_desc_fd);
		(void) state_write(script);
	execl(script, script, interface, vaddr_arg, xparam, (char *) NULL);
	logfile(LOG_ERR, _("Unable to exec %s %s %s%s%s: %s"), 
		script, interface, vaddr_arg, 
                (xparam ? " " : ""), (xparam ? xparam : ""), 
                strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid != (pid_t) -1) {
        logfile(LOG_WARNING, _("Spawning [%s %s %s%s%s]"), 
                script, interface, vaddr_arg, 
                (xparam ? " " : ""), (xparam ? xparam : ""));
#ifdef HAVE_WAITPID
	{
	    while (waitpid(pid, NULL, 0) == (pid_t) -1 && errno == EINTR);
	}
#else
        {
            pid_t foundpid;
            
	    do {
		foundpid = wait3(NULL, 0, NULL);
		if (foundpid == (pid_t) -1 && errno == EINTR) {
		    continue;
		}
	    } while (foundpid != (pid_t) -1 && foundpid != pid);
        }
#endif
    } else {
        logfile(LOG_ERR, _("Unable to spawn the script: %s"),
                strerror(errno));
        return -1;
    }
    return 0;
}
