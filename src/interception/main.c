/*
 * tcpcopy - an online replication replication tool
 *
 *  Copyright 2011 Netease, Inc.  All rights reserved.
 *  Use and distribution licensed under the BSD license.  See
 *  the LICENSE file for full text.
 *
 *  Authors:
 *      bin wang <wangbin579@gmail.com>
 *      bo  wang <wangbo@corp.netease.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "../core/xcopy.h"

xcopy_srv_settings srv_settings;

static void release_resources()
{
	log_info(LOG_NOTICE, "release_resources begin");
	interception_over();
	log_info(LOG_NOTICE, "release_resources end except log file");
	log_end();
}

static void signal_handler(int sig)
{
	log_info(LOG_ERR,"set signal handler:%d", sig);
	printf("set signal handler:%d\n", sig);
	if(SIGSEGV == sig){    
		log_info(LOG_ERR, "SIGSEGV error");
		release_resources();
		/* Avoid dead loop*/
		signal(SIGSEGV, SIG_DFL);
		kill(getpid(), sig);
	}else{    
		exit(EXIT_SUCCESS);
	} 
}

static void set_signal_handler(){
	int i=1;
	atexit(release_resources);
	for(; i<SIGTTOU; i++)	
	{
		signal(i, signal_handler);
	}
}

static int retrieve_ip_addr()
{
	size_t      len;
	int         count=0;
	const char  *split, *p = srv_settings.raw_ip_list;
	char        tmp[32];
	uint32_t    address;

	memset(tmp, 0, 32);

	while(1){
		split = strchr(p, ',');
		if(split != NULL){   
			len = (size_t)(split-p);
		}else{   
			len = strlen(p);
		}   
		strncpy(tmp, p, len);
		address = inet_addr(tmp);    
		srv_settings.passed_ips.ips[count++] = address;

		if(count == MAX_ALLOWED_IP_NUM){
			log_info(LOG_WARN,"reach the limit for passing firewall");
			break;
		}

		if(NULL == split){
			break;
		}else{
			p = split + 1;
		}

		memset(tmp, 0, 32);
	}

	srv_settings.passed_ips.num = count;

	return 1;
}

static void usage(void) {  
	printf("intercept " VERSION "\n");
	printf("-x <passlist,> passed ip list through firewall\n"
		   "               format:\n"
		   "               ip1,ip2,...\n"
		   "-l <file>      log file path\n"
		   "-P <file>      save PID in <file>, only used with -d option\n"
		   "-v             intercept version\n"
		   "-h             help\n"
		   "-d             run as a daemon\n");
	return;
}

static int read_args(int argc, char **argv){
	int  c;
	while (-1 != (c = getopt(argc, argv,
		 "x:" /* ip list passed through ip firewall */
		 "h"  /* print this help and exit */   
		 "l:" /* error log file path */
		 "P:" /* save PID in file */
		 "v"  /* print version and exit*/
		 "d"  /* daemon mode */
	    ))) {
		switch (c) {
			case 'x':
				srv_settings.raw_ip_list = strdup(optarg);
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case 'l':
				srv_settings.log_path = strdup(optarg);
				break;
			case 'P':
				srv_settings.pid_file = optarg;
				break;
			case 'v':
				printf ("intercept version:%s\n", VERSION);
				exit(EXIT_SUCCESS);
			case 'd':
				srv_settings.do_daemonize = 1;
				break;
			default:
				fprintf(stderr, "Illegal argument \"%c\"\n", c);
				exit(EXIT_FAILURE);
		}

	}
	return 0;
}

static int set_details()
{
	/* Set signal handler */	
	set_signal_handler();
	/* Retrieve ip address */
	retrieve_ip_addr();
}

int main(int argc ,char **argv){
	/* Read args */
	read_args(argc, argv);
	/* Init log */
	log_init();
	/* Set details */
	set_details(); 
	/* Init interception */
	interception_init();
	/* Run now */
	interception_run();

	return 0;
}

