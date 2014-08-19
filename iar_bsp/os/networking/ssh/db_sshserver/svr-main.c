/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002-2006 Matt Johnston
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#include "nussh_includes.h"
#include "dbutil.h"
#include "session.h"
#include "buffer.h"
#include "signkey.h"
#include "runopts.h"
#include "random.h"

/* MGC GLOBALS */
STATUS  sshsrv_running;

static size_t listensockets(int *sock, size_t sockcount, int *maxfd);
#ifndef SSH_SYS_OS_NUCLEUS /* Commented to remove warnings . */
static void sigchld_handler(int dummy);
static void sigsegv_handler(int);
static void sigintterm_handler(int fish);
#endif
#ifdef INETD_MODE
static void main_inetd();
#endif

/* MGC Check for pre-requisutes */
#if (INCLUDE_IPV6 == NU_FALSE) && (INCLUDE_IPV4 == NU_FALSE)
#error "SSH Server: SSH Requires at least one of IPv4 or IPv6 to compile."
#endif

#if (INCLUDE_IPV4 == NU_FALSE) && (SSH_ENABLE_IPV6 == NU_FALSE)
#error "SSH Server: Enable IP6 option from configuration."
#endif

#if (SSH_ENABLE_IPV6 == NU_TRUE) && (INCLUDE_IPV6 == NU_FALSE)
#error "SSH Server: IP6 in SSH requires IPv6 product to be enabled."
#endif

#if (INCLUDE_LOOPBACK_DEVICE == NU_FALSE)
#error "SSH Server requires loopback device to be functional."
#endif

#if (!defined(CFG_NU_OS_NET_SSH_LIBTOMMATH_ENABLE) && !defined(CFG_NU_OS_NET_SSL_LITE_ENABLE))
#error "SSH Server: Needs Libtommath library to compile."
#endif

/* MGC Check for conflicts */
#if (defined(CFG_NU_OS_NET_SSH_LIBTOMMATH_ENABLE) && defined(CFG_NU_OS_NET_SSL_LITE_ENABLE))
#error "SSH Server: CyaSSL has conflicts with Libtommath. Disable Libtommath to compile successfully."
#endif

#ifdef NON_INETD_MODE
static void main_noinetd();
#endif
static void commonsetup();
#ifdef USE_NU_PTY
INT NUPTY_Parent_Socket = -1;
#endif /*USE_NU_PTY*/
#if defined(DBMULTI_dropbear) || !defined(DROPBEAR_MULTI)
#if defined(DBMULTI_dropbear) && defined(DROPBEAR_MULTI)
int dropbear_main(int argc, char ** argv)
#else
int sshs_main(int argc, char ** argv)
#endif
{
	_dropbear_exit = svr_dropbear_exit;
	_dropbear_log = svr_dropbear_log;

	/* MGC Setup the globals for SSH Server */
	exitflag = 0;

	disallow_core();

	/* Wait for network to be available. */
	NETBOOT_Wait_For_Network_Up(NU_SUSPEND);

#if SSH_INCLUDE_INITIAL_FILES
	/* Load all file dumps to memory. */
	SSHF_Write_File_System();
#endif

	/* get commandline options */
	svr_getopts(argc, argv);

#ifdef INETD_MODE
	/* service program mode */
	if (svr_opts.inetdmode) {
		main_inetd();
		/* notreached */
	}
#endif

#ifdef NON_INETD_MODE
	main_noinetd();
	/* notreached */
#endif

	/* Shutdown SSH server task. */
	dropbear_close("SSH server shutdown.\n");
	return -1;
}
#endif

#ifdef INETD_MODE
static void main_inetd() {
	char *host, *port = NULL;

	/* Set up handlers, syslog, seed random */
	commonsetup();

	/* In case our inetd was lax in logging source addresses */
	get_socket_address(0, NULL, NULL, &host, &port, 0);
	dropbear_log(LOG_INFO, "Child connection from %s:%s", host, port);
	m_free(host);
	m_free(port);

	/* Don't check the return value - it may just fail since inetd has
	 * already done setsid() after forking (xinetd on Darwin appears to do
	 * this */
	setsid();

	/* Start service program 
	 * -1 is a dummy childpipe, just something we can close() without 
	 * mattering. */
	svr_session(0, -1);

	/* notreached */
}
#endif /* INETD_MODE */
#ifdef NON_INETD_MODE
void main_noinetd() {
	fd_set fds;
	unsigned int i, j;
	int val;
	int maxsock = -1;
	int listensocks[MAX_LISTEN_ADDR];
	size_t listensockcount = 0;
	FILE *pidfile = NULL;
	struct timeval timeout;
	NU_TASK *svrsession_taks;
	int svrsess_argv[2];

	int childpipes[MAX_UNAUTH_CLIENTS];
	char * preauth_addrs[MAX_UNAUTH_CLIENTS];

	int childsock;
	int childpipe[2];

	sshsrv_running = NU_TRUE;

	/* Note: commonsetup() must happen before we daemon()ise. Otherwise
	   daemon() will chdir("/"), and we won't be able to find local-dir
	   hostkeys. */
	commonsetup();

	/* sockets to identify pre-authenticated clients */
	for (i = 0; i < MAX_UNAUTH_CLIENTS; i++) {
		childpipes[i] = -1;
	}
	memset(preauth_addrs, 0x0, sizeof(preauth_addrs));

	/* Set up the listening sockets */
	listensockcount = listensockets(listensocks, MAX_LISTEN_ADDR, &maxsock);
	if (listensockcount == 0)
	{
		dropbear_exit("No listening ports available.");
	}

	/* fork */
#if 0 /* MGC No forking alternative. */
	if (svr_opts.forkbg) {
		int closefds = 0;
#ifndef DEBUG_TRACE
		if (!svr_opts.usingsyslog) {
			closefds = 1;
		}
#endif
		if (daemon(0, closefds) < 0) {
			dropbear_exit("Failed to daemonize: %s", strerror(errno));
		}
	}
#endif
	/* should be done after syslog is working */
	if (svr_opts.forkbg) {
		dropbear_log(LOG_INFO, "Running in background");
	} else {
		dropbear_log(LOG_INFO, "Not backgrounding");
	}

	/* create a PID file so that we can be killed easily */
	pidfile = fopen(svr_opts.pidfile, "w");
	if (pidfile) {
		fprintf(pidfile, "%d\n", getpid());
		fclose(pidfile);
	}

	/* incoming connection select loop */
	for(;;) {

		FD_ZERO(&fds);
		
		/* listening sockets */
		for (i = 0; i < listensockcount; i++) {
			FD_SET(listensocks[i], &fds);
		}

		/* pre-authentication clients */
		for (i = 0; i < MAX_UNAUTH_CLIENTS; i++) {
			if (childpipes[i] >= 0) {
				FD_SET(childpipes[i], &fds);
				maxsock = MAX(maxsock, childpipes[i]);
			}
		}

		/* MGC We are just setting this timeout out so we can resume
		 * task from select. Setting it to MAX value possible. */
		timeout.tv_sec = (UINT_MAX/2) / NU_PLUS_TICKS_PER_SEC;
		timeout.tv_usec = 0;
		val = select(maxsock+1, &fds, NULL, NULL, &timeout);

		if (val < 0) {
#ifndef SSH_SYS_OS_NUCLEUS
			if (errno == EINTR) {
#else
			if (val == NU_NO_DATA) {
#endif
				if (exitflag) {
					unlink(svr_opts.pidfile);
					TRACE(("Terminated by signal"))
					break;
				}
				else
					continue;
			}
			dropbear_exit("Listening socket error");
		}

		/* close fds which have been authed or closed - svr-auth.c handles
		 * closing the auth sockets on success */
		for (i = 0; i < MAX_UNAUTH_CLIENTS; i++) {
			if (childpipes[i] >= 0 && FD_ISSET(childpipes[i], &fds)) {
				m_close(childpipes[i]);
				childpipes[i] = -1;
				m_free(preauth_addrs[i]);
			}
		}

		/* handle each socket which has something to say */
		for (i = 0; i < listensockcount; i++) {
			size_t num_unauthed_for_addr = 0;
			size_t num_unauthed_total = 0;
			char *remote_host = NULL, *remote_port = NULL;
			pid_t fork_ret = 0;
			size_t conn_idx = 0;
			struct sockaddr_storage remoteaddr;
			socklen_t remoteaddrlen;

			if (!FD_ISSET(listensocks[i], &fds)) 
				continue;

			remoteaddrlen = sizeof(remoteaddr);
			childsock = accept(listensocks[i], 
					(struct sockaddr*)&remoteaddr, &remoteaddrlen);

			if (childsock < 0) {
				/* accept failed */
				continue;
			}

			/* Limit the number of unauthenticated connections per IP */
			getaddrstring(&remoteaddr, &remote_host, NULL, 0);

			num_unauthed_for_addr = 0;
			num_unauthed_total = 0;
			for (j = 0; j < MAX_UNAUTH_CLIENTS; j++) {
				if (childpipes[j] >= 0) {
					num_unauthed_total++;
					if (strcmp(remote_host, preauth_addrs[j]) == 0) {
						num_unauthed_for_addr++;
					}
				} else {
					/* a free slot */
					conn_idx = j;
				}
			}

			if (num_unauthed_total >= MAX_UNAUTH_CLIENTS
					|| num_unauthed_for_addr >= MAX_UNAUTH_PER_IP) {
				goto out;
			}

			if (pipe(childpipe) < 0) {
				TRACE(("error creating child pipe"))
				goto out;
			}

#ifdef DEBUG_NOFORK
			fork_ret = 0;
#else
			fork_ret = fork();
#endif
			if (fork_ret < 0) {
				dropbear_log(LOG_WARNING, "Error forking: %s", strerror(errno));
				goto out;

			} else if (fork_ret > 0) {

				/* parent */
				childpipes[conn_idx] = childpipe[0];
				m_close(childpipe[1]);
				preauth_addrs[conn_idx] = remote_host;
				remote_host = NULL;

			} else {

				/* child */
#ifdef DEBUG_FORKGPROF
				extern void _start(void), etext(void);
				monstartup((u_long)&_start, (u_long)&etext);
#endif /* DEBUG_FORKGPROF */

				getaddrstring(&remoteaddr, NULL, &remote_port, 0);
				dropbear_log(LOG_INFO, "Child connection from %s:%s", remote_host, remote_port);
				m_free(remote_host);
				m_free(remote_port);

#ifndef DEBUG_NOFORK
				if (setsid() < 0) {
					dropbear_exit("setsid: %s", strerror(errno));
				}
#endif
				svrsess_argv[0] = childsock;
				svrsess_argv[1] = childpipe[1];

				/* start the session */
				if(NU_Create_Auto_Clean_Task(&svrsession_taks, "SVRSESSION",
										(VOID (*)(UNSIGNED, VOID *))psvr_session,
										sizeof(svrsess_argv), svrsess_argv,
										SSHS_Mem_Pool, 10000,20, 0, NU_PREEMPT,
										NU_START) == NU_SUCCESS)
				{
					if( NU_Suspend_Task(NU_Current_Task_Pointer()) == NU_SUCCESS )
					{
						/* External api call to shutdown SSH. */
						if(exitflag)
						{
							/* Signal svrsession task to terminate. */
							if(NU_Resume_Task(svrsession_taks) == NU_SUCCESS)
							{
								/* Wait at most 1/2 second for a reply from svrsession task
								 * for successful termination.*/
								NU_Sleep(NU_PLUS_Ticks_Per_Second/2);
							}
							goto out;
						}
					}
					else
					{
						TRACE(("Failed to retrieve events from session task"))
						goto out;
					}

				}
			}

out:
			/* This section is important for the parent too */
			m_close(childsock);
			if (remote_host) {
				m_free(remote_host);
			}
		}
		if(exitflag) {
			unlink(svr_opts.pidfile);
			TRACE(("Terminated by signal"))
			break;
		}
	} /* for(;;) loop */
	/* don't reach here */

	/* MGC Clean up. */
	/* close listening sockets */
	for (i = 0; i < listensockcount; i++) {
		m_close(listensocks[i]);
	}
}
#endif /* NON_INETD_MODE */

#ifndef SSH_SYS_OS_NUCLEUS /* AZ: Commented to compile. */
/* catch + reap zombie children */
static void sigchld_handler(int UNUSED(unused)) {

	struct sigaction sa_chld;

	while(waitpid(-1, NULL, WNOHANG) > 0); 

	sa_chld.sa_handler = sigchld_handler;
	sa_chld.sa_flags = SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
		dropbear_exit("signal() error");
	}

}

/* catch any segvs */
static void sigsegv_handler(int UNUSED(unused)) {
	fprintf(stderr, "Aiee, segfault! You should probably report "
			"this as a bug to the developer\n");
	exit(EXIT_FAILURE);
}

/* catch ctrl-c or sigterm */
static void sigintterm_handler(int UNUSED(unused)) {

	exitflag = 1;
}
#endif /* !SSH_SYS_OS_NUCLEUS */
/* Things used by inetd and non-inetd modes */
static void commonsetup() {
	struct addr_struct  servaddr;   /* holds the server address structure */
#ifndef SSH_SYS_OS_NUCLEUS /* !!! AZ: Commented to compile. */
	struct sigaction sa_chld;
#ifndef DISABLE_SYSLOG
	if (svr_opts.usingsyslog) {
		startsyslog();
	}
#endif

	/* set up cleanup handler */
	if (signal(SIGINT, sigintterm_handler) == SIG_ERR || 
#ifndef DEBUG_VALGRIND
		signal(SIGTERM, sigintterm_handler) == SIG_ERR ||
#endif
		signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		dropbear_exit("signal() error");
	}

	/* catch and reap zombie children */
	sa_chld.sa_handler = sigchld_handler;
	sa_chld.sa_flags = SA_NOCLDSTOP;
	sigemptyset(&sa_chld.sa_mask);
	if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
		dropbear_exit("signal() error");
	}
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		dropbear_exit("signal() error");
	}
#endif /* SSH_SYS_OS_NUCLEUS */
	/* Now we can setup the hostkeys - needs to be after logging is on,
	 * otherwise we might end up blatting error messages to the socket */
	loadhostkeys();

	seedrandom();

	/* Initialize RTL commponents. */
	if (NU_Create_Semaphore(&SSH_Timer_Semaphore, "SSH_SEM,", 1, NU_FIFO) != NU_SUCCESS)
		dropbear_exit("Failed to create SSH Semaphore.");

#ifdef USE_NU_PTY
	/* Establish a server socket for nu_pty pipes to be created. */
#if SSH_ENABLE_IPV6 == NU_TRUE
	NUPTY_Parent_Socket = NU_Socket(NU_FAMILY_IP6, NU_TYPE_STREAM, 0);
#else
	NUPTY_Parent_Socket = NU_Socket(NU_FAMILY_IP, NU_TYPE_STREAM, 0);
#endif
	if (NUPTY_Parent_Socket >=0 )
	{
		/* Fill in a structure with the server address. */
#if SSH_ENABLE_IPV6 == NU_TRUE
		servaddr.family = NU_FAMILY_IP6;
		/* loop back address IPv6 */
		memcpy(servaddr.id.is_ip_addrs, IP6_Loopback_Address, IP6_ADDR_LEN);
#else
		servaddr.family = NU_FAMILY_IP;
		/* loop back address 127.0.0.1 */
		PUT32(servaddr.id.is_ip_addrs, 0, 0x7f000001UL);
#endif
		servaddr.port = NUPTY_PORT;
		servaddr.name = "NUPTY_P";

		/* Bind the server's address. */
		if (NU_Bind(NUPTY_Parent_Socket, &servaddr, 0) >= 0)
		{
			/* Prepare to accept connection requests. */
			if(NU_Listen(NUPTY_Parent_Socket, 10) == NU_SUCCESS)
			{
				TRACE(("Listening on NU_PTY server socket."))
			}
			else
				dropbear_exit("Failed to create NU_PTY server socket.");
		}
		else
			dropbear_exit("Failed to create NU_PTY server socket.");
	}
	else
		dropbear_exit("Failed to create NU_PTY server socket.");
#endif
}

/* Set up listening sockets for all the requested ports */
static size_t listensockets(int *sock, size_t sockcount, int *maxfd) {
	
	unsigned int i;
	char* errstring = NULL;
	size_t sockpos = 0;
	int nsock;

	TRACE(("listensockets: %d to try\n", svr_opts.portcount))

	for (i = 0; i < svr_opts.portcount; i++) {

		TRACE(("listening on '%s:%s'", svr_opts.addresses[i], svr_opts.ports[i]))

		nsock = dropbear_listen(svr_opts.addresses[i], svr_opts.ports[i], &sock[sockpos], 
				sockcount - sockpos,
				&errstring, maxfd);

		if (nsock < 0) {
			dropbear_log(LOG_WARNING, "Failed listening on '%s': %s", 
							svr_opts.ports[i], errstring);
			m_free(errstring);
			continue;
		}

		sockpos += nsock;

	}
	return sockpos;
}
