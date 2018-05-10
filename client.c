#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <dirent.h>
#include "log.h"


int filter(const struct dirent *pDir);
void sendalldata(struct sockaddr_in *sin);
int goconn(struct sockaddr_in *sin);
int try_conn(struct sockaddr_in *sin, int sock);
void getendnum(char *endNum, int size);
int sendfiledata(int sock, char *filename);
static char * getnowdate(char *date);

char buf[1024];

int main(int argc, char const *argv[])
{
    if (argc<3)
    {
    	printf("usage: %s %s %s\n", argv[0], "ip", "port");
    	exit(1);
    }

    const char *resultfile = "/bdcapdir/result";
    if (chdir(resultfile))
    {
    	LOG("chdir %s fail", resultfile);
    }

	struct sockaddr_in myname;
    struct hostent *h;
    char const *serverIP = argv[1];
    int serverport = atoi(argv[2]);


    if (((h=gethostbyname(serverIP)) == NULL))
    {
        LOG("gethostbyname fail");
        exit(1);
    }

    bzero(&myname, sizeof(myname));
 	myname.sin_family = AF_INET;
    myname.sin_port = htons(serverport);
    myname.sin_addr = *((struct in_addr *)h->h_addr);

    sendalldata(&myname);

    return 0;
}

int filter(const struct dirent *pDir)
{

	if( strncmp("file", pDir->d_name, 4) == 0 )
	{
		return 1;
	}
	return 0;
}

void sendalldata(struct sockaddr_in *sin)
{
	int sock = 0;
	char endNum[100], endFile[100], timebuf[20];
	struct dirent **namelist;
	int n;
	int connflag = 1;

    getendnum(endNum, sizeof(endNum));
	sprintf(endFile,"file%s.txt",endNum);

 	/*get all file*.txt under curren folder*/
	if( (n = scandir(".", &namelist, filter, alphasort)) < 0)
		LOG("path not found files");

	while(n--)
	{
		if(0 == strcmp(endFile,namelist[n]->d_name))
			continue;

		if (connflag) {
			sock = goconn(sin);
			connflag = 0;
		}

		if(sendfiledata(sock, namelist[n]->d_name) == 1)
        {
            break;  //when send fail and break, n >= 0
        }
		if (!access(namelist[n]->d_name, F_OK))
		{
       		remove(namelist[n]->d_name);
   		}

		free(namelist[n]);
	}

    /*if send all data success, n is -1*/
    if (n >= 0)
    {
        free(namelist[n]);
        while(n--)
        {
            free(namelist[n]);
        }
    }

	if (-1 == close(sock))
    {
        LOG("close fail");
    }
	free(namelist);

	getnowdate(timebuf);
	printf("[%s]send success and exit\n", timebuf);
}


int goconn(struct sockaddr_in *sin)
{
	int sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LOG("client socket failure");

    }

    int nNetTimeout = 1000; //1s

	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, ( char * )&nNetTimeout, sizeof(int) );

	if (try_conn(sin, sock))
    {
    	LOG("server cannot connect");
    }
    
    return sock;
}


int try_conn(struct sockaddr_in *sin, int sock)
{
	int conn_num, ret;
	int times = 3;

	for (conn_num=0; conn_num<times; conn_num++)   //try to connect the server max times is 3
	{
        ret = connect(sock, (struct sockaddr *)sin, sizeof(struct sockaddr));
	    if(ret<0)
	        sleep(1);
        else
        	break;
	}
	if (conn_num >= times)
	{
		return 1;
	}

	return 0;
}


void getendnum(char *endNum, int size)
{
	const char *filenum = "../tmp_filenum";
    FILE *fp;

	if( (fp = fopen(filenum,"r")) == NULL)
	{
		LOG("fopen %s fail", filenum);
	}

	memset(endNum, 0, size);
	fread(endNum, 1, size, fp);

	if( endNum[size-1] == '\n' )
	{
		endNum[size-1] = 0;
	}

	return;
}


int sendfiledata(int sock, char *filename)
{
	printf("send %s to server\n", filename);

    FILE *fp;
    int flag = 0;

	memset(buf,0,sizeof(buf));

	if( (fp = fopen(filename,"rb")) == NULL)
	{
		LOG("fopen %s fail", filename);
	}

	while( NULL != fgets(buf, sizeof(buf), fp) )
	{
		if(0 >= send(sock, buf, strlen(buf), 0))
        {
        	if (!strlen(buf)) {
        		flag = 2;   //the file.txt is messy, will remove the file
        		LOGMSG("%s file is messy", filename);
        	} else {
        		flag = 1;    //if send fail, will remain the file
        		LOGMSG("send fail");
        	}
 
            break;
        }
        //send a line
		memset(buf, 0, sizeof(buf));
	}
	if(-1 == fclose(fp))
	{
		LOG("fclose %s fail", filename);
	}

	return flag;
}


static char * getnowdate(char *date)
{  
	time_t timer=time(NULL);
	strftime(date,20,"%Y-%m-%d %H:%M:%S",localtime(&timer));  
	return date; 
}

