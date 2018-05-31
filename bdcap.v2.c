/*
v2.0: 由于程序自动强制退出，修改计时器方式    20180515
v2.1: 修改管道为select模式
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <regex.h> 
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/wait.h>
#include "log.h"

void bdcap_main();
int wlanmonup();
void start_sniff(int filenum);
int start_tcpdump();
int restart_tcpdump(int handle);
int stop_tcpdump(FILE *handle);
void newfile(int *filenum, int *linenum, char *filename);
void strip(const char *strIn, char *strOut, int len);
int compile_pattern(regex_t *reg, const char *pattern);
bool match(char *src, regex_t *reg, char *destBuf, size_t length );
void getManu(char *mac, char *manu, int len);
bool getApName(const int fpOut, const fd_set *pset, char * apname);
static char * getnowdate(char *date);
int strip_mac(char *macname, char *mac6, int len);

const char *wlanname = "wlan1";   //wlanname is wlan1
const char *cardname = "wlanmon";

int one_min_flag = 0;

int main(int args,char** argv)
{
    const char *confpath= "/bdcapdir";
    const char *resultpath = "/bdcapdir/result";

    if (access(confpath, F_OK) || access(resultpath, F_OK) )
    {
        printf("mkdir -p %s\n", resultpath);

        if (system("mkdir -p /bdcapdir/result"))
        {
            LOG("mkdir %s fail", resultpath);
        }
    }
    chdir(confpath);     

    if (access("oui.txt", F_OK))
    {
        LOG("out.txt no found");
    }

    sleep(20);

    bdcap_main();

    return 0;
}


void bdcap_main()
{
    if (wlanmonup())
    {
        LOG("dev %s up Fail", cardname);
    }

    printf("wlanname : %s\n",wlanname);
    printf("cardname : %s\n",cardname);

    int filenum;
    FILE *pEndNum;
    
    if (access("./tmp_filenum", F_OK))   //has not tmp_filenum file,  filename is 0
    {
        filenum = 0;
    }
    else
    {
        pEndNum = fopen("./tmp_filenum", "r" );
        if (!pEndNum)
        {
            LOG("open tmp_filenum fail");
        }
        fscanf(pEndNum, "%u", &filenum);
        
        fclose(pEndNum);
    }

    /*touch the tmp_filenum file*/
    pEndNum = fopen("tmp_filenum", "w" );
    fprintf(pEndNum,"%d",filenum);
    fclose(pEndNum);

    start_sniff(filenum);

    return;
}


int wlanmonup()
{
    char cmd[1024];

    if (system("/sbin/ifconfig -a | /bin/grep -q mon"))
    {
        sprintf(cmd, "/sbin/ifconfig %s > /dev/null 2>&1", wlanname);
        if (system(cmd))
        {
            LOG("%s is not existent", wlanname);
        }

        sprintf(cmd, "iw dev %s interface add wlanmon type monitor" ,wlanname);
        system(cmd);

        if( system("/sbin/ifconfig -a | /bin/grep -q wlanmon") )
            LOG("%s is not existent", cardname);
    }

    sprintf(cmd,"/sbin/ifconfig %s up",cardname);
    return system(cmd);
}

void start_sniff(int filenum)
{

    char out[1024], filename[256];
    char macname[18], apname[1024], timebuf[20];
    char manu[1024];
    int linenum = 0;
    int err_fgets_time = 0;
    FILE *pOut, *fp;
    regex_t reg;

    char station[] = "teststation";
    const char *macNamePattern = "([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}";

    sprintf(filename,"./result/file%d.txt",filenum);


    if (compile_pattern(&reg, macNamePattern))
    {
        LOG("compile_pattern fail");
    }
    
    int fpOut = start_tcpdump();
    pOut = fdopen(fpOut, "r");

    int rv;
    fd_set init_set, set;
    struct timeval init_timeout, timeout;

    FD_ZERO(&init_set);
    FD_SET(fpOut, &init_set);
    init_timeout.tv_sec = 120;
    init_timeout.tv_usec = 0;

    time_t curtime, tmptime;
    curtime = tmptime = time(NULL);
    int perminnum = 0;

    while(1)
    {
        err_fgets_time = 0;
        memset(out,0,sizeof(out));
        set = init_set;
        timeout = init_timeout;
        rv = select(fpOut+1, &set, NULL, NULL, &timeout);

        if (rv == -1) {
            LOG("select fail");
        } else if (rv == 0) {
            // timeout, restart tcpdump
            LOGMSG("Tcpdump timeout");
            fpOut = restart_tcpdump(fpOut);
            pOut = fdopen(fpOut, "r");
            continue;
        } else {
            if (fgets(out,sizeof(out),pOut) == NULL) {
                LOGMSG("Fgets Tcpdump Fail");
                fpOut = restart_tcpdump(fpOut);
                pOut = fdopen(fpOut, "r");
                continue;
            }
        }
        
        curtime = time(NULL);

        if ((curtime - tmptime) > 60) {
            one_min_flag = 1;
            tmptime = curtime;
        }

        if ( one_min_flag ) {   //1 min will change the file num
            one_min_flag = 0;
            printf("\nperminnum: %d\n", perminnum);
            perminnum = 0;
            newfile(&filenum, &linenum, filename);
        }

        if(NULL == strstr(out,"Probe Request")) 
        {
            continue;
        }

        if(linenum >= 50)    //test
        {   
            newfile(&filenum, &linenum, filename);
        }

        match(out, &reg, macname, sizeof(macname));     //get macname
        getManu(macname, manu, sizeof(manu));           //get manu  
        getApName(fpOut, &set, apname);                         //get apname
        getnowdate(timebuf);                            //get timebuf
        linenum++;
        perminnum++;

        if((fp = fopen(filename,"a"))==NULL)
        {
            LOG("cannot open %s", filename);
        }

        fprintf(fp,"%s<-->%s<-->%s<-->%s<-->%s\n",macname,apname,manu,timebuf,station);

        fclose(fp);

        printf("Detect Phone Mac--> %s connected AP:%s time: %s\n",macname,apname,timebuf);
        printf("Manufacturer is %s\n", manu );
        printf("Recording....\n");
        printf("-----------------------------------\n"); 
    }

    regfree(&reg);  //release

    return;
}

// FILE * start_tcpdump()
// {
//     char cmd[100];
//     FILE *handle;

//     sprintf(cmd,"/usr/sbin/tcpdump -Xls 0 -i %s -e type mgt ",cardname);
//     if ((handle = popen(cmd,"r")) == NULL)
//     {
//         LOG("popen tcpdump fail");
//     }

//     return handle;
// }

int start_tcpdump()
{
    char cmd[100];
    int pipefd[2];
    pid_t pid;

    // sprintf(cmd,"/usr/sbin/tcpdump -Xls 0 -i %s -e type mgt ",cardname);
    if (pipe(pipefd) == -1) {
        LOG("pipe fail");
    }

    switch (pid=fork()) {
    case -1:
        LOG("fork fail");
        break;
    case 0:
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        execlp("/usr/sbin/tcpdump", "/usr/sbin/tcpdump", "-Xls", "0", "-i", cardname, "-e", "type", "mgt", NULL);        
    default:
        close(pipefd[1]);
        sleep(5);
        return pipefd[0];
    }
}


int restart_tcpdump(int handle)
{
    char cmd[200];

    close(handle);
    //kill child process
    system("killall tcpdump");
    wait(NULL);
    sleep(1);

    sprintf(cmd, "/sbin/ifconfig %s down" , cardname);
    system(cmd);
    sprintf(cmd, "iw dev %s interface add wlanmon type monitor; /sbin/ifconfig %s up" ,wlanname, cardname);
    system(cmd);

    int new_handle = start_tcpdump();

    return new_handle;
}

int stop_tcpdump(FILE *handle)
{
    return pclose(handle);
}

void newfile(int *filenum, int *linenum, char *filename)
{
    FILE *pEndNum;

    *linenum = 0;
    *filenum = (*filenum + 1)%1000000;    //maxnum file is 999999
    sprintf(filename,"./result/file%d.txt",*filenum);  //update filename

    pEndNum = fopen("tmp_filenum", "w" );             //update file "tmp_filename"
    fprintf(pEndNum,"%d", *filenum);
    fclose(pEndNum);

    return;
}

int compile_pattern(regex_t *reg, const char *pattern)
{
    int cflags = REG_EXTENDED ;
    return regcomp(reg, pattern, cflags);                   //compile
}

bool match(char *src, regex_t *reg, char *destBuf, size_t length )   //match regular
{
    int status;
    regmatch_t pmatch[10];
    const size_t nmatch = 10;

    memset(destBuf, 0, length);
    
    status = regexec(reg, src,nmatch, pmatch, 0);       //match
    if(status == REG_NOMATCH)
    {
        return false;
    }
    else if(status == 0)
    {
        int start = pmatch[0].rm_so;
        int end = pmatch[0].rm_eo;
        if(length > (end- start))
        {
            strncpy(destBuf, src + start , end -start );  
            destBuf[end-start] = '\0';
        }
        else
        {
            strncpy(destBuf, src + start , length-1 );
            destBuf[length] = '\0';
        }

        return true;
    }
    else
    {
        LOG("match fail");
    }

}

void getManu(char *mac, char *manu, int len)
{
    char cmd[1024], buffer[1024], mac6[7];
    FILE *pManu;

    strip_mac(mac, mac6, sizeof(mac6));
    mac6[6] = '\0';

    if (len <= 20) {
        LOG("manu is too small");
    } else {
        memset(manu,0,len);
    }

    bool ismatch = false;
    memset(buffer,0,sizeof(buffer));
    FILE *fp = fopen("oui.txt", "r");
    while(fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(mac6, buffer, 6) == 0) {
            // The longest manu name cannot exceed 20 characters, manu index is begin+22
            strncpy(manu, buffer+22, 20);
            manu[20] = '\0';
            // strip '\n'
            strip(manu, manu, len);
            ismatch = true;
            break;
        }
    }
    if (fclose(fp)) {
        LOG("fclose fail");
    }

    if ( !ismatch ) {
        strcpy(manu,"UnKnow");
    }
        
    return;
}

bool getApName(const int fpOut, const fd_set *pset, char * apname)   
{
    char buffer[1024]; 
    char out[1024];
    bool firstline = true;
    int i, rv;
    fd_set tmp_set;
    struct timeval timeout, tmp_timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    FILE *pOut = fdopen(fpOut, "r");

    memset(buffer,0,sizeof(buffer));

    for(i=0;i<5;i++)                        //read hex number to buffer
    {
        memset(out,0,sizeof(out));
        tmp_set = *pset;
        tmp_timeout = timeout;
        rv =select(fpOut+1, &tmp_set, NULL, NULL, &tmp_timeout);
        if(rv == -1)
        {
            LOG("getapname select fail");
        } else if (rv == 0) {
            LOGMSG("getapname timeout");
            *apname = '\0';
            return false;
        } else {
            if (fgets(out, sizeof(out), pOut) == NULL) {
                LOG("fgets fail");
            }
        }

        // find the first line which include '0x0000'
        if (firstline && strstr(out, "0x0000") == NULL) {
            continue;
        } else {
            firstline = false;
        }

        strncpy(buffer+strlen(buffer),out+10,39);
    }

    char wifiname[1024];
    memset(wifiname,0,sizeof(wifiname));
    char *pWifi = wifiname;
    char *pBuf = buffer;
    char wifinameBuf[3];
    while(*pBuf != 0)
    {
        if(*pBuf == ' ') 
        {
            pBuf++;
            continue;
        }
        for(i = 0; i<2; i++)
        {
            wifinameBuf[i] = *pBuf++ ;  
        }
        wifinameBuf[2] = '\0';
        *pWifi++ = strtol(wifinameBuf,NULL,16); //str to hex
        if( ( *(pWifi-1) == 8 || *(pWifi-1) == 4 ) && *(pWifi-2) == 1)      // 0104 or 0108 is end
            break;
    }
    
    // have to len > 2
    if (pWifi - wifiname <= 2) {
        *apname = '\0';
        return false;
    }

    *(pWifi-2) = '\0';
    // printf("wifiname[0] : %d  wifiname[1] : %d\n", wifiname[0],wifiname[1]);
    pWifi = pWifi - 2 - 1;
    while (wifiname < pWifi && *pWifi != '\0') {
        pWifi--;
    }

    if(*(pWifi+1) > 1 && *(pWifi+1) < 16)                                //000x is start
    {
        strcpy(apname,pWifi+2);
        return true;
    }
    else
    {
        *apname = '\0';
        return false;
    }
}

static char * getnowdate(char *date)
{  
    time_t timer=time(NULL);
    strftime(date,20,"%Y-%m-%d %H:%M:%S",localtime(&timer));  
    return date; 
}


/*will strip '' and '\n' and '\r' and '\t'*/
void strip(const char *strIn, char *strOut, int len)
{
    assert(strIn && strOut);

    int i, j ;

    i = 0;

    j = strlen(strIn) - 1;

    while(isspace(strIn[i]))
        ++i;

    while(isspace(strIn[j]) && i<j)
        --j;

    if(i > j)  
        strOut[0] = '\0';
    else
    {
        int stroutlen = j - i + 1;
        if (len < stroutlen + 1) {
            LOG("strip overflower");
        }

        // strncpy(strOut, strIn + i , stroutlen);
        int k;
        for (k = 0; k < stroutlen; k++)
           strOut[k] = strIn[i+k];

        strOut[stroutlen] = '\0';
    }
}


int strip_mac(char *macname, char *mac6, int len)
{
    int maclen;

    if (len < 6)
    {
        LOG("mac6 is too small");
    }

    for (maclen=0; macname != NULL && maclen<6; macname++)
    {
        if (isalnum(*macname))
        {
            *mac6 = toupper(*macname);
            mac6++;
            maclen++;
        }
    }
    return maclen==6 ? 0 : 1;
}
