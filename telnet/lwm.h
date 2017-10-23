#include <sys/types.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#define _XOPEN_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <pcre.h>
#include <sys/time.h>
#include <rsyslog.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iconv.h>

#include "/opt/freesvr/sql/include/mysql/mysql.h"
extern char bs_1000[];
extern struct timeval tv;
extern struct timezone tz;
extern int fd1,fd2,freesvr_autologin,sid,g_bytes,invim,wincmd_count,justoutvim,pass_prompt_count,net,get_first_prompt,encode,monitor_fd_tm,monitor_fd_fm;
extern char * host;
extern char * cmd;
extern char * commandline;
extern char * inputcommandline;
extern char * cache1;
extern char * cache2;
extern char * linebuffer;
extern char * sql_query;
extern char * alarm_content;
extern char winopenfile[256];
extern char     mysql_host[],
				mysql_user[],
				mysql_passwd[],
				mysql_db[],
				mysql_serv_port[];
extern char wincmd[16][16];
extern char wincmd_ant[16][16];
extern char pass_prompt[16][16];

extern char myprompt[50][128];
extern char admin_password[256];
extern struct auto_login_config login_config;


extern char * device_id;
extern char * member_user;
extern char * source_ip;


extern char syslogserver[128];
extern char syslogfacility[128];
extern char mailserver[128];
extern char mailaccount[128];
extern char mailpassword[128];
extern char adminmailaccount[10][128];
extern int syslogalarm;
extern int mailalarm;
extern int adminmailaccount_num;

static int waitforline=0;
MYSQL my_connection;
MYSQL_RES *res_ptr;
MYSQL_ROW sqlrow;

int in_login=1;
extern char * monitor_file_tm;
static int string_length=100000;
struct auto_login_config
{
    char * remoteuser;
    char * password;
};

struct black_cmd
{
	int level;
	char cmd[50];
};

struct black_cmd black_cmd_list[50];
int black_cmd_num=0;
int black_or_white=0;

char * autosu_pass_check(char * username)
{
    char autosu_password[1024];
    bzero(sql_query,string_length);
    sprintf(sql_query,
            "select udf_decrypt(cur_password) from devices where username=(select username from \n\r"
            "(select distinct member.uid,member.username webuser,member.realname webrealname,\n\r"
            "member.groupid,member.lastdate,luser.devicesid,devices.device_ip,devices.username,\n\r"
            "devices.login_method,devices.device_type,luser.forbidden_commands_groups,\n\r"
            "luser.weektime,luser.sourceip,luser.autosu,luser.syslogalert,luser.mailalert,\n\r"
            "luser.loginlock from luser left join member on luser.memberid=member.uid left \n\r"
            "join devices on luser.devicesid=devices.id where member.uid and luser.devicesid \n\r"
            "AND 1 AND device_ip = '%s' AND devices.username = '%s' \n\r"
            "AND member.username = '%s' AND devices.login_method=3 union select \n\r"
            "distinct member.uid,member.username webuser,member.realname webrealname,\n\r"
            "member.groupid,member.lastdate,t.devicesid,devices.device_ip,devices.username,\n\r"
            "devices.login_method,devices.device_type,a.forbidden_commands_groups,a.weektime,\n\r"
            "a.sourceip,a.autosu,a.syslogalert,a.mailalert,a.loginlock from luser_resourcegrp \n\r"
            "a left join (select a.id,b.devicesid from resourcegroup a left join resourcegroup \n\r"
            "b on a.groupname=b.groupname where a.devicesid=0 ) t on a.resourceid=t.id left \n\r"
            "join member on a.memberid=member.uid left join devices on t.devicesid=devices.id\n\r"
            " where t.id and member.uid and t.devicesid AND 1 AND device_ip = '%s'\n\r"
            " AND devices.username = '%s' AND member.username = '%s' AND\n\r"
            " devices.login_method=3 union select distinct member.uid,member.username webuser,\n\r"
            "member.realname webrealname,member.groupid,member.lastdate,lgroup.devicesid,\n\r"
            "devices.device_ip,devices.username,devices.login_method,devices.device_type,\n\r"
            "lgroup.forbidden_commands_groups,lgroup.weektime,lgroup.sourceip,lgroup.autosu,\n\r"
            "lgroup.syslogalert,lgroup.mailalert,lgroup.loginlock from lgroup left join member\n\r"
            " on lgroup.groupid=member.groupid left join devices on lgroup.devicesid=devices.id\n\r"
            " where member.uid and lgroup.devicesid AND 1 AND device_ip = '%s'\n\r"
            " AND devices.username = '%s' AND member.username = '%s' AND\n\r"
            " devices.login_method=3 union select distinct member.uid,member.username webuser,\n\r"
            "member.realname webrealname,member.groupid,member.lastdate,t.devicesid,devices.device_ip,\n\r"
            "devices.username,devices.login_method,devices.device_type,a.forbidden_commands_groups,\n\r"
            "a.weektime,a.sourceip,a.autosu,a.syslogalert,a.mailalert,a.loginlock from\n\r"
            " lgroup_resourcegrp a left join (select a.id,b.devicesid from resourcegroup\n\r"
            " a left join resourcegroup b on a.groupname=b.groupname where a.devicesid=0 )\n\r"
            " t on a.resourceid=t.id left join member on a.groupid=member.groupid left join\n\r"
            " devices on t.devicesid=devices.id where t.id and member.uid and t.devicesid\n\r"
            " AND 1 AND device_ip = '%s' AND devices.username = '%s'\n\r"
            " AND member.username = '%s' ORDER BY device_ip asc, device_ip ASC,\n\r"
            " username ASC, webuser ASC) as mt) and device_ip='%s'",
             host,username,member_user,host,username,member_user,host,username,member_user,host,username,member_user,host);

    int res = mysql_query(&my_connection,sql_query);
    if(res)
    {
    }
    else
    {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
                printf("NO pass found\n");
                mysql_free_result(res_ptr);
                return 0;
            }
            while ((sqlrow = mysql_fetch_row(res_ptr)))
            {
                bzero(autosu_password,1024);
                strcpy(autosu_password,sqlrow[0]);
                break;
            }
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }
        }
        mysql_free_result(res_ptr);
    }
    return autosu_password;
}


void * auto_su_thread_callback(char * password)
{
//    printf("\n\rpassword=%s\n\r",password);
    extern int freesvr_autologin_thread;
    freesvr_autologin_thread=1;
    sleep(3);
    if(pcre_match(commandline,"assword:")==0)
    {
//        printf("HREE!!!!\n\r");
        write(net,password,strlen(password));
        write(net,"\n",1);
    }
    freesvr_autologin_thread=0;
}

void autosu_any(char * cmd)
{
    char * p = cmd;
    int i = 0;
    int do_autosu = 0;
    int get_username =0;
    char * username;
    for(i=0;i<strlen(cmd)-1;i++)
    {
        if(p[i]=='s' && p[i+1]=='u' && (p[i+2]==0 || p[i+2]==' '))
        {
            if(p[i+2]==' ')
            {
                get_username=1;
            }
            do_autosu=1;
            break;
        }
    }

    if(get_username==1)
    {
        for(i=strlen(cmd);i>0;i--)
        {
            if(p[i]==' ')
            {
                break;
            }
        }
        username=p+i+1;
//        printf("\n\rusername=%s\n\r",username);
//        printf("\n\rpassword=%s\n\r",autosu_pass_check(username));
           pthread_t thread;
           if(autosu_pass_check(username)!=0)
           {
        int ret = pthread_create(&thread,NULL,auto_su_thread_callback,autosu_pass_check(username));
        if(ret<0)
        {
            printf("auto su error,will exit...\n");
            exit(0);
        } 
           }
    }
    else if(do_autosu==1)
    {
//        printf("\n\rpassword=%s\n\r",autosu_pass_check("root"));
        pthread_t thread;
        if(autosu_pass_check("root"))
        {
        int ret = pthread_create(&thread,NULL,auto_su_thread_callback,autosu_pass_check("root"));
        if(ret<0)
        {
            printf("auto su error,will exit...\n");
            exit(0);
        }
        }
    }
}

void to_get_a_prompt(char prompts[50][128],char * aprompt,int n)
{
	for (int i=49;i>0;i--)
	{
		if(strcmp(prompts[i],aprompt)==0)
		{
			return;
		}
	}

	if(n<128)
	{
		for(int i=49;i>0;i--)
		{
			bzero(prompts[i],128);
			strncpy(prompts[i],prompts[i-1],128);
		}

		bzero(prompts[0],128);
		strncpy(prompts[0],aprompt,n);

		bzero(sql_query,string_length);
		sprintf(sql_query,"insert into device_prompts values (%s,'%s',now())",device_id,prompts[0]);

		int res = mysql_query(&my_connection,sql_query);
	    if (res)
	    {
//	        printf("insert error: %s\n", mysql_error(&my_connection));
	    }
	}
}


void freesvr_alarm(char * my_alarm_content,int level)
{
	pid_t alarm_pid;

	if(syslogalarm>0)
	{
		alarm_pid=fork();
		if(alarm_pid==0)
		{
			if(level==0)
			{
				rsyslog(syslogserver,514,syslogfacility,"info",my_alarm_content);
			}
			else if(level==1)
			{
				rsyslog(syslogserver,514,syslogfacility,"emerg",my_alarm_content);
			}
			else if(level==2)
			{
				rsyslog(syslogserver,514,syslogfacility,"alert",my_alarm_content);
			}
			exit(0);
		}
	}
	if(level>0 && mailalarm>0)
	{
        alarm_pid=fork();
        if(alarm_pid==0)
        {
			
			for(int i=0;i<adminmailaccount_num;i++)
			{
				int ret=lib_send_mail(adminmailaccount[i],mailserver,mailaccount,mailpassword,"freesvr dangerous command alarm mail",my_alarm_content,0);
	//			printf("\nret=%d\n",ret);
			}
			
			exit(0);
        }
	}
}


void termfunc(char * string,char * ret1,char * ret2,int chomp)
{
    char *p=string;
	if(chomp==1 && (*(p+strlen(p)-1)=='\r' || *(p+strlen(p)-1)=='\n'))
	{
		*(p+strlen(p)-1)=0;
	}
    int i=0;


    while(i<strlen(p))
    {
        if((unsigned char)p[i]==(unsigned char)0x8a)
        {
            i+=1;
            bzero(ret2,string_length);
            continue;
        }
		if(p[i]==0x0f)
		{
			i++;
			continue;
		}

		if((i+2)<strlen(p) && p[i+2]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]=='m')
		{
            i+=3;
            continue;
		}
		if((i+3)<strlen(p) && p[i+3]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]<('9'+1) && p[i+2]>('0'-1) && p[i+3]=='m')
		{
            i+=4;
            continue;
		}
		if((i+4)<strlen(p) && p[i+4]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]<('9'+1) && p[i+2]>('0'-1) && p[i+3]<('9'+1) && p[i+3]>('0'-1) && p[i+4]=='m')
		{
            i+=5;
            continue;
		}
		if((i+5)<strlen(p) && p[i+5]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]<('9'+1) && p[i+2]>('0'-1) && p[i+3]==';' && p[i+4]<('9'+1) && p[i+4]>('0'-1) && (p[i+5]=='H' || p[i+5]=='m'))
		{
            i+=6;
            continue;
		}
		if((i+6)<strlen(p) && p[i+6]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]<('9'+1) && p[i+2]>('0'-1) && p[i+3]==';' && p[i+4]<('9'+1) && p[i+4]>('0'-1) && p[i+5]<('9'+1) && p[i+5]>('0'-1) && (p[i+6]=='H' || p[i+6]=='m'))
		{
            i+=7;
            continue;
		}
		if((i+6)<strlen(p) && p[i+6]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]<('9'+1) && p[i+2]>('0'-1) && p[i+3]<('9'+1) && p[i+3]>('0'-1) && p[i+4]==';' && p[i+5]<('9'+1) && p[i+5]>('0'-1) && (p[i+6]=='H' || p[i+6]=='m'))
		{
            i+=7;
            continue;
		}
		if((i+7)<strlen(p) && p[i+7]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]<('9'+1) && p[i+2]>('0'-1) && p[i+3]<('9'+1) && p[i+3]>('0'-1) && p[i+4]==';' && p[i+5]<('9'+1) && p[i+5]>('0'-1) && p[i+6]<('9'+1) && p[i+6]>('0'-1) && (p[i+7]=='H' || p[i+7]=='m'))
		{
            i+=8;
            continue;
		}
		if((i+6)<strlen(p) && p[i+6]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]=='0' && p[i+3]=='0' && p[i+4]==0x1b && p[i+5]=='[' && p[i+6]=='m')
		{
            i+=7;
            continue;
		}
		if((i+4)<strlen(p) && p[i+4]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]=='4' && p[i+3]<('9'+1) && p[i+3]>('0'-1) && p[i+4]=='m')
		{
            i+=5;
            continue;
		}
		if((i+10)<strlen(p) && p[i+10]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]<('9'+1) && p[i+2]>('0'-1) && p[i+3]<('9'+1) && p[i+3]>('0'-1) && p[i+4]==';' && p[i+5]<('9'+1) && p[i+5]>('0'-1) && p[i+6]=='H' && p[i+7]==0x1b && p[i+8]=='[' && p[i+9]<('9'+1) && p[i+9]>('0'-1) && p[i+10]=='K')
		{
            i+=11;
            continue;
		}
		if((i+21)<strlen(p) && p[i+21]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]=='7' && p[i+3]=='m' && p[i+4]=='S' && p[i+5]=='t' 
				&& p[i+6]=='a' && p[i+7]=='n' && p[i+8]=='d' && p[i+9]=='a' && p[i+10]=='r' && p[i+11]=='d' 
				&& p[i+12]==' ' && p[i+13]=='i' && p[i+14]=='n' && p[i+15]=='p' && p[i+16]=='u' && p[i+17]=='t' 
				&& p[i+18]==0x1b && p[i+19]=='[' && p[i+20]=='0' && p[i+21]=='m')
		{
            i+=22;
            continue;
		}


		if((i+7)<strlen(p) && p[i+7]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]=='0' && p[i+3]=='1' && p[i+4]==';' && p[i+7]=='m')
		{
			i+=8;
			continue;
		}
		if((i+4)<strlen(p) && p[i+4]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]=='0' && p[i+3]=='0' && p[i+4]=='m')
		{
			i+=5;
			continue;
		}
		if((i+2)<strlen(p) && p[i+2]!=0 && p[i]==0x1b && p[i+1]=='[' && p[i+2]=='m')
		{
			i+=3;
			continue;
		}
		if(i<strlen(p))
		{
		//	printf("\rret2=%p,p=%p,strlen(ret2)=%d,i=%d\r\n",ret2,p,strlen(ret2),i);
			strncat(ret2+strlen(ret2),p+i,1);
		}
		
        i++;
    }

	
    bzero(ret1,string_length);
    p=ret2;i=0;
    while(i<strlen(ret2))
    {
        if(p[i+2]!=0 && p[i]==0x1b && p[i+1]==0x5d && p[i+2]==0x30)
        {
            int j=1;
            while(i+j<strlen(ret2))
            {
                if(p[i+j]==0x07)
                {
                    i+=j;
                    break;
                }
                j++;
            }
        }
        strncat(ret1+strlen(ret1),ret2+i,1);
        i++;
    }

    bzero(ret2,string_length);
    if(ret1[0]==0x0d)
    {
        strcpy(ret2,ret1+1);
    }
    else
    {
        strcpy(ret2,ret1);
    }

    if(*(ret2+strlen(ret2)-1)==0x0d)
    {
        *(ret2+strlen(ret2)-1)=0;
    }

	i=strlen(ret2);
    bzero(ret1,string_length);
    p=ret2;

	
    while(i>0)
    {
        if(p[i+5]!=0 && p[i]==0x1b && p[i+1]==0x5b && p[i+2]==0x48 && p[i+3]==0x1b && p[i+4]==0x5b && p[i+5]==0x4a)
        {
            i+=6;
            break;
        }
        i--;
    }

    strcpy(ret1,ret2+i);

    bzero(ret2,string_length);
    p=ret1;
    i=0;

    while(i<strlen(ret1))
    {
        if(p[i]==0x07)
        {
            i++;
            continue;
        }
        strncpy(ret2+strlen(ret2),ret1+i,1);
        i++;
    }
    

	p=ret2;

	/*
    i=0;
    bzero(ret1,string_length);
    int H_count=0;
    int H_from=0;
    int H_to=0;

    while(i<strlen(ret2))
    {
        if(p[i]==8)
        {
            H_count++;
            if(H_count==0)
            {
                H_from=i;
            }
        }
        else
        {
            H_count=0;
            H_to=0;
            H_from=0;
        }
        if(H_count==67 && p[i+1]=='$')
        {
            H_to=i+2;
            break;
        }
        i++;
    }

	if(H_to!=0)
    {
        strncpy(ret1,ret2,H_from);
        char H_arr[37];
        for(int i=0;i<37;i++)
        {
            H_arr[i]=8;
        }
        strncpy(ret1+strlen(ret1),H_arr,37);
        strncpy(ret1+strlen(ret1),ret2+H_to,strlen(ret2)-H_to);
    }
    else
    {
        strcpy(ret1,ret2);
    }
	*/
	bzero(ret1,string_length);
	strcpy(ret1,ret2);
    p=ret1;
    i=0;
    bzero(ret2,string_length);
//	int column=132;
    int ant=0;


    for(i=0;i<strlen(ret1);i++)
    {
//        if(ant==column)
        {
//            ant=0;
        }
        if(ant<0)
        {
            ant=0;
        }
		
        if(ret1[i]==0x0d)
        {
            if(ret1[i+1]==0x0d)
            {
                bzero(ret2,string_length);
                ant=0;
                i++;
                continue;
            }
			else if(ret1[i+1]==0x1b && ret1[i+2]=='[' &&  ret1[i+3]>('0'-1) && ret1[i+3]<('9'+1) && ret1[i+4]>('0'-1) && ret1[i+4]<('9'+1) && ret1[i+5]=='G')
			{
				bzero(ret2,string_length);
				ant=0;
				i+=5;
				continue;
			}
            else
            {
                ant=0;
                continue;
            }
        }
        if(ret1[i]==0x08)
        {
            ant--;
            continue;
        }
		
        if(ret1[i]==0x1b)
        {
            if(ret1[i+1]=='[' && ret1[i+2]>('0'-1) && ret1[i+2]<('9'+1) && ret1[i+3]=='D')
            {
				int times = ret1[i+2]-'0';
//              printf("times=%d\n\r",times);
				ant=ant-times;
//              ret2[ant]=0;
				i+=3;
                continue;
            }
			else if(ret1[i+1]=='[' &&  ret1[i+2]>('0'-1) && ret1[i+2]<('9'+1) && ret1[i+3]>('0'-1) && ret1[i+3]<('9'+1) && ret1[i+4]=='D')
			{
				int times=(ret1[i+2]-'0') * 10 + (ret1[i+3]-'0');
				ant = ant-times;
				i+=4;
				continue;
			}
            else if(ret1[i+1]=='[' && ret1[i+2]>('0'-1) && ret1[i+2]<('9'+1) && ret1[i+3]=='C')
            {
                int times = ret1[i+2]-'0';
                ant=ant+times;
//              ret2[ant]=0;
                i+=3;
                continue;
            }
            else if(ret1[i+1]=='[' &&  ret1[i+2]>('0'-1) && ret1[i+2]<('9'+1) && ret1[i+3]>('0'-1) && ret1[i+3]<('9'+1) && ret1[i+4]=='C')
            {
                int times=(ret1[i+2]-'0') * 10 + (ret1[i+3]-'0');
                ant = ant+times;
                i+=4;
                continue;
            }
            else if(ret1[i+1]=='[' && ret1[i+2]=='C')
            {
                ant++;
                i+=2;
                continue;
            }
            else if(ret1[i+1]=='[' && ret1[i+2]=='K')
            {
                i+=2;
                int j=0;
                bzero(ret2+ant,string_length-ant);
                continue;
            }
            else if(ret1[i+1]=='[' && ret1[i+2]=='A')
            {
                i+=2;
                continue;
            }
            else if(ret1[i+1]=='[' &&  ret1[i+2]>('0'-1) && ret1[i+2]<('9'+1) && ret1[i+3]>('0'-1) && ret1[i+3]<('9'+1) && ret1[i+4]=='P')
            {
                int times=(ret1[i+2]-'0') * 10 + (ret1[i+3]-'0');
                char * tmp_p = ret2+strlen(ret2)-times;
                int len=strlen(ret2);
                memmove(ret2+ant,ret2+ant+times,len-ant-times);
                bzero(tmp_p,times);
                i+=4;
                continue;
            }
            else if(ret1[i+1]=='[' &&  ret1[i+2]>('0'-1) && ret1[i+2]<('9'+1) && ret1[i+3]=='P')
            {
                int times=ret1[i+2]-'0';
                char * tmp_p = ret2+strlen(ret2)-times;
                int len=strlen(ret2);
                memmove(ret2+ant,ret2+ant+times,len-ant-times);
                bzero(tmp_p,times);
                i+=3;
                continue;
            }
            else if(ret1[i+1]=='[' &&  ret1[i+2]>('0'-1) && ret1[i+2]<('9'+1) && ret1[i+3]=='@')
            {
                int len=strlen(ret2);
                memmove(ret2+ant,ret2+ant-1,len-ant+1);
                ret2[ant]=ret1[i+4];
                i+=4;
                ant++;
                //printf("ret2=%s\n",ret2);
                continue;
            }
            else if(ret1[i+1]=='[')
            {
//                ant++;
                i+=2;
                continue;
            }
        }
		
        ret2[ant]=ret1[i];
        ant++;
    }
}

void check_invim(char * p)
{
	for(int i=0;i<wincmd_count;i++)
	{
		if(pcre_match(p,wincmd[i])==0)
		{
//			printf("invim,wincmd+ant=%s\n",wincmd_ant[i]);
			char * t = strstr(p,wincmd_ant[i]);
//			printf("t=%p,p=%p\n",t,p);
			int j = t-p;
//			printf("j=%d\n",j);
			int get_a_prompt=0;
			while(j>0)
			{
				if(p+j!=' ')
				{
					get_a_prompt=1;
					break;
				}
			}
			if(get_a_prompt==1)
			{
				if(t-p<128)
				{
					//strncat(myprompt,p,t-p);
					to_get_a_prompt(myprompt,p,t-p);
				}
//				printf("myprompt=%s\n",myprompt);
			}

/*			for(int i=0;i<5;i++)
			{
				printf("myprompt[%d]=%s\n",i,myprompt[i]);
			}
			*/
			invim=2;
			return;
		}
	}
}

void check_outvim(char * p,char prompts[50][128])
{
    for(int i=0;i<50;i++)
	{
		if(strlen(prompts[i])>0 && strstr(p,prompts[i])!=0)
		{
			invim=0;
			justoutvim=1;
			return;
		}
	}
}


int pcre_match (char *src, char *pattern)
{
    pcre *re;

    const char *error;

    int erroffset;

    int rc;

    re = pcre_compile (pattern,       /* the pattern */
               0,       /* default options */
               &error,       /* for error message */
               &erroffset, /* for error offset */
               NULL);       /* use default character tables */

/* Compilation failed: print the error message and exit */
    if (re == NULL)
    {
    printf ("PCRE compilation failed at offset %d: %s\n", erroffset,
        error);
    return -1;
    }

    rc = pcre_exec (re,        /* the compiled pattern */
            NULL, /* no extra data - we didn't study the pattern */
            src, /* the src string */
            strlen (src), /* the length of the src */
            0,        /* start at offset 0 in the src */
            0,        /* default options */
            NULL, 0);

/* If Matching failed: */
    if (rc < 0)
    {
    free (re);
    return -1;
    }

    free (re);
    return rc;
}

int get_pcre(char * name)
{
    int res;
	bzero(sql_query,string_length);
    sprintf(sql_query,"select cmd,level from forbidden_commands_groups where gid = '%s'",name);
//	printf("sql=%s\n",sql_query);

//    mysql_init(&my_connection);
//   if (mysql_real_connect(&my_connection,mysql_host,mysql_user,mysql_passwd,mysql_db,mysql_serv_port,NULL,0))
    {
//    printf("Connection DB success\n");
        res = mysql_query(&my_connection,sql_query);
    if (res)
    {
        //printf("SELECT error: %s\n", mysql_error(&my_connection));
    }
    else
    {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
                //printf("USER not in config error\n");
                mysql_free_result(res_ptr);
                return 1;
            }
            while ((sqlrow = mysql_fetch_row(res_ptr)))
            {
				black_cmd_list[black_cmd_num];
				strcpy(black_cmd_list[black_cmd_num].cmd,sqlrow[0]);
				black_cmd_list[black_cmd_num].level=atoi(sqlrow[1]);
				black_cmd_num++;
            }
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }
        }
        mysql_free_result(res_ptr);
    }

	bzero(sql_query,string_length);
	sprintf(sql_query,"select black_or_white from forbidden_groups where gname='%s'",name);
//	printf("sql=%s\n",sql_query);

	res = mysql_query(&my_connection,sql_query);
	if(res)
	{
	}
	else
	{
		res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
                //printf("USER not in config error\n");
                mysql_free_result(res_ptr);
                return 1;
            }   
            while ((sqlrow = mysql_fetch_row(res_ptr)))
            {
                black_or_white=atoi(sqlrow[0]);
			//	printf("black_or_white=%d\n",black_or_white);
            }   
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }   
        }   
        mysql_free_result(res_ptr);
	}
    }
}

void encryp(char * src,char * dest)
{
    char * s=src;
    char * d=dest;
    int i=10;
    while((*s)!='\0')
    {
        *d=*s^i;
        s++;
        d++;
        i++;
    }
    *d='\0';
}


void octtodec(char * num,char *ret) //将十进制num 转为二进制 写入ret
{
    char buf[8]={0};
    strcpy(buf,num);
    int a=atoi(buf);
    int i=7;
    char b[8]={0};
    for(i=7;i>=0;i--)
    {
        if(a>(int)pow(2,i) || a==(int)pow(2,i))
        {
            a%=(int)pow(2,i);
            sprintf(b,"%s%s",b,"1");
        }
        else
        {
            sprintf(b,"%s%s",b,"0");
        }
    }
    strcpy(ret,b);
}

int match(char * buf1,int mask,char * buf2) //进行掩码匹配 buf1 buf2 为十进制 mask 为掩码长度
{
    char * string1=malloc(sizeof(char)*8);
    char * string2=malloc(sizeof(char)*8);
    int i;
    octtodec(buf1,string1);
    octtodec(buf2,string2);
//    printf("%s\n",string1);
//    printf("%s\n",string2);

    for(i=0;i<mask;i++)
    {
//        printf("第%d位  ",i);

        if(string1[i]!=string2[i])
        {
//            printf("NO!\n");
            return -1;
        }
        else
        {
//            printf("OK!\n");
        }
    }
    return 1;
}

int netmask(char * sourceip,int netmask,char * destip)
{
    char tmp1[20]={0};
    char tmp2[20]={0};
    strcpy(tmp1,sourceip);
    char *s_a, *s_b, *s_c, *s_d,*d_a,*d_b,*d_c,*d_d;
    char *token1 = strtok(tmp1, ".");
    s_a = token1;
    token1 = strtok(NULL, ".");
    s_b = token1;
    token1 = strtok(NULL, ".");
    s_c = token1;
    token1 = strtok(NULL, ".");
    s_d = token1;

    strcpy(tmp2,destip);
    char *token2 = strtok(tmp2, ".");
    d_a = token2;
    token2 = strtok(NULL, ".");
    d_b = token2;
    token2 = strtok(NULL, ".");
    d_c = token2;
    token2 = strtok(NULL, ".");
    d_d = token2;
//    printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",s_a,s_b,s_c,s_d,d_a,d_b,d_c,d_d);

    if(netmask>24)
    {
        if(match(s_a,8,d_a)<0)
        {
            return -1;
        }
        if(match(s_b,8,d_b)<0)
        {
            return -1;
        }
        if(match(s_c,8,d_c)<0)
        {
            return -1;
        }
        if(match(s_d,(netmask-24),d_d)<0)
        {
            return -1;
        }
    }
    else if(netmask>16)
    {
        if(match(s_a,8,d_a)<0)
        {
            return -1;
        }
        if(match(s_b,8,d_b)<0)
        {
            return -1;
        }
        if(match(s_c,(netmask-16),d_c)<0)
        {
            return -1;
        }
    }
    else if(netmask>8)
    {
        if(match(s_a,8,d_a)<0)
        {
            return -1;
        }
        if(match(s_a,(netmask-16),d_a)<0)
        {
            return -1;
        }
        if(match(s_a,(netmask-16),d_a)<0)
        {
            return -1;
        }
    }
    else
    {
        if(match(s_a,netmask,d_a)<0)
        {
            return -1;
        }
    }
//    printf("netmask match\n");
    return 1;
}


int access_check(char * ip,char * local_user)
{
    int res;
	bzero(sql_query,string_length);
    sprintf(sql_query,"select ip,netmask from ac_network where groupname in (select Value from ac_group where UserName='%s');",local_user);
//	printf("%s\n",sql_query);
//  mysql_init(&my_connection);
//  if (mysql_real_connect(&my_connection,mysql_host,mysql_user,mysql_passwd,mysql_db,mysql_serv_port,NULL,0))
    {
        //printf("Connection DB success\n");
        res = mysql_query(&my_connection,sql_query);
    if (res)
    {
        //printf("SELECT error: %s\n", mysql_error(&my_connection));
    }       
    else
    {   
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
                //printf("USER not in config error\n");
                mysql_free_result(res_ptr);
                return 1;
            }
            while ((sqlrow = mysql_fetch_row(res_ptr)))
            {
                if(sqlrow[0]==NULL || strcmp(sqlrow[0],ip)==0 || (strcmp(sqlrow[1],"32")!=0 && (netmask(sqlrow[0],atoi(sqlrow[1]),ip))>0))
                {
                    //printf("%s\n",sqlrow[0]);
                    mysql_free_result(res_ptr);
                    return 1;
                }
            }
            //printf("ip not in config error\n");
            return -1;
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }
        }
        mysql_free_result(res_ptr);
    }
	}
}

int get_default_login_config(struct auto_login_config * login_config,char * localuser)
{
    int res;

    char local_user[50];
	bzero(sql_query,string_length);
	encryp(localuser,local_user);
	sprintf(sql_query,"select remoteuser,password from autologin_users where uid=(select uid from member where username='%s')",local_user);
//	printf("%s\n",sql_query);
//  mysql_init(&my_connection);
//  if (mysql_real_connect(&my_connection,mysql_host,mysql_user,mysql_passwd,mysql_db,mysql_serv_port,NULL,0))
    {
#ifdef DEBUG
        //printf("Connection DB success\n");
#endif
        res = mysql_query(&my_connection,sql_query);
    if (res)
    {
        //printf("SELECT error: %s\n", mysql_error(&my_connection));
    }
    else
    {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
                //printf("IP not permited in ACL\n");
                mysql_free_result(res_ptr);
                return -1;
            }
            if(sqlrow = mysql_fetch_row(res_ptr))
            {
                if(sqlrow[0]==NULL)
                {
                    freesvr_autologin=0;
                }
                else
                {
					memset(login_config->remoteuser,'\0',255);
					memset(login_config->password,'\0',255);
                    encryp(sqlrow[0],login_config->remoteuser);
                    encryp(sqlrow[1],login_config->password);
                }
                return 1;
            }
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }
        }
        mysql_free_result(res_ptr);
    }
    }
    return -1;
}

int get_login_config(struct auto_login_config * login_config,char * ip,char * local_user)
{
    int res;
	bzero(sql_query,string_length);
    char localuser[50];
    encryp(local_user,localuser);
	sprintf(sql_query,"select ip,netmask,remoteuser,password from autologin_network where uid=(select uid from member where username='%s') order by netmask desc",localuser);
//	printf("%s\n",sql_query);
//	mysql_init(&my_connection);
//  if (mysql_real_connect(&my_connection,mysql_host,mysql_user,mysql_passwd,mysql_db,mysql_serv_port,NULL,0))
	{
        //printf("Connection DB success\n");
        res = mysql_query(&my_connection,sql_query);
    if (res)
    {
        //printf("SELECT error: %s\n", mysql_error(&my_connection));
    }
    else
    {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
                //printf("USER not in config error\n");
                mysql_free_result(res_ptr);
                return -1;
            }
            while ((sqlrow = mysql_fetch_row(res_ptr)))
            {
                if(strcmp(sqlrow[0],ip)==0 || (strcmp(sqlrow[1],"32")!=0 && netmask(sqlrow[0],atoi(sqlrow[1]),ip)>0))
                {
					memset(login_config->remoteuser,'\0',255);
					memset(login_config->password,'\0',255);
					sprintf(login_config->remoteuser,"%s",sqlrow[2]);
			//		encryp(sqlrow[2],login_config->remoteuser);
					encryp(sqlrow[3],login_config->password);
                    mysql_free_result(res_ptr);
                    return 1;
                }
            }
            //printf("ip not in config error\n");
            return -1;
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }
        }
        mysql_free_result(res_ptr);
    }
    }
    return -1;
}


int get_login_config2(struct auto_login_config * login_config,char * ip,char * local_user, char * ruser) /* with password monitor*/
{
    int res;
    bzero(sql_query,string_length);

    char uid[5];
    bzero(uid,5);
	
//	mysql_init(&my_connection);
//  if (mysql_real_connect(&my_connection,mysql_host,mysql_user,mysql_passwd,mysql_db,mysql_serv_port,NULL,0))
	{
		bzero(sql_query,string_length);
		sprintf(sql_query,"select username,cur_password,autosu from devices where device_ip=\"%s\" and username = '%s';",ip,ruser);
	//	printf("%s\n",sql_query);
		res = mysql_query(&my_connection,sql_query);
    if (res)
    {
      //  printf("SELECT error: %s\n", mysql_error(&my_connection));
    }
    else
    {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
            //    printf("USER not in config error\n");
                mysql_free_result(res_ptr);
                return -1;
            }
            while ((sqlrow = mysql_fetch_row(res_ptr)))
            {
					memset(login_config->remoteuser,'\0',255);
					memset(login_config->password,'\0',255);
					sprintf(login_config->remoteuser,"%s",sqlrow[0]);
					sprintf(login_config->password,"%s",sqlrow[1]);
                    mysql_free_result(res_ptr);
                    return 1;
            }
            return -1;
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }
        }
        mysql_free_result(res_ptr);
    }
    }
    return -1;
}


int get_login_config3(struct auto_login_config * login_config,char * device_id,char * host,char * port,int * device_type,  int * freesvr_autosu, char * enable_password) /* with password monitor*/
{
    int res;
    
    char uid[5];
    bzero(uid,5);
	
//	mysql_init(&my_connection);
//  if (mysql_real_connect(&my_connection,mysql_host,mysql_user,mysql_passwd,mysql_db,mysql_serv_port,NULL,0))
	{
		bzero(sql_query,string_length);
		sprintf(sql_query,"select devices.username,devices.cur_password,devices.device_ip,devices.port,devices.device_type,devices.autosu, servers.superpassword from devices,servers where devices.id = %s and servers.device_ip = devices.device_ip",device_id);
	//	printf("%s\n",sql_query);
		res = mysql_query(&my_connection,sql_query);
    if (res)
    {
        printf("SELECT error: %s\n", mysql_error(&my_connection));
    }
    else
    {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr)
        {
            if((unsigned long)mysql_num_rows(res_ptr)==0)
            {
            //    printf("USER not in config error\n");
                mysql_free_result(res_ptr);
                return -1;
            }
            while ((sqlrow = mysql_fetch_row(res_ptr)))
            {
					memset(login_config->remoteuser,'\0',255);
					memset(login_config->password,'\0',255);
					memset(enable_password,0,50);
					sprintf(login_config->remoteuser,"%s",sqlrow[0]);
					sprintf(login_config->password,"%s",sqlrow[1]);
					sprintf(host,"%s",sqlrow[2]);
					sprintf(port,"%s",sqlrow[3]);
					*device_type = atoi(sqlrow[4]);
					*freesvr_autosu = atoi(sqlrow[5]);
					if(sqlrow[6]!=NULL)
					{
						sprintf(enable_password,"%s",sqlrow[6]);
					}
                    mysql_free_result(res_ptr);
					return 1;
            }
            if (mysql_errno(&my_connection))
            {
                fprintf(stderr, "Retrive error: s\n",mysql_error(&my_connection));
            }
        }
        mysql_free_result(res_ptr);
    }
    }
    return -1;
}

void writelogfile(char * buff,int n)
{
	extern struct auto_login_config login_config;
	int alarm_length=74;


	//printf("\n\r\n\r\n\rn=%d\n\r\n\r\n\r",n);
	g_bytes+=n;
	if(monitor_fd_tm>0)
	{
		write(monitor_fd_tm,buff,n);
	}
	

//	printf("\n\rcommandline11=%p\n\r",commandline);

	gettimeofday (&tv , &tz);
	write(fd2,&tv,sizeof(tv));
    write(fd2,"1",1);   //1:content 2:command
	write(fd2,&n,sizeof(n));
    write(fd2,buff,n);
	int i=0;
	char * p=buff;

	if(invim==1)
	{
		bzero(commandline,string_length);
		bzero(inputcommandline,string_length);
		bzero(linebuffer,string_length);
		check_outvim(buff,myprompt);
		return;
	}
	else if(invim==2)
	{
		bzero(cache1,string_length);
		bzero(cache2,string_length);
		termfunc(linebuffer,cache1,cache2,1);

		write(fd1,cache2,strlen(cache2));
		write(fd1,"\n",1);
		char * t = strlen(cmd)>0 ? strstr(cache2,cmd) : 0;

		if(justoutvim==0 && t!=0 && t!=cache2 && (t-cache2)<128)
		{
            to_get_a_prompt(myprompt,cache2,t-cache2);
			if(get_first_prompt>0)
			{
				bzero(sql_query,string_length);
				sprintf(sql_query,"update devices set first_prompt='%s' where id=%s",myprompt[0],device_id);
//				printf("sql_qeury1=%s\n",sql_query);
				mysql_query(&my_connection,sql_query);
				get_first_prompt=0;
			}
//	        printf("cmd=%s\n,myprompt=%s\n",cmd,myprompt);
		}
		invim--;
		return;
	}

//	printf("\n\rcommandline12=%p\n\r",commandline);
	
	while(i<n)
	{
//		printf("\n\rstrlen(linebuffer)=%d\n\r",strlen(linebuffer));
		if(p[i]==0x0a)
		{
		//	printf("myprompt=%s\n",myprompt);
			bzero(cache1,string_length);
			bzero(cache2,string_length);
			termfunc(linebuffer,cache1,cache2,1);

			write(fd1,cache2,strlen(cache2));
			write(fd1,"\n",1);
			char * t = strlen(cmd)>0 ? strstr(cache2,cmd) : 0;

			if(justoutvim==0 && t!=0 && t!=cache2 && t-cache2<128)
			{
				to_get_a_prompt(myprompt,cache2,t-cache2);
				if(get_first_prompt>0)
				{
					bzero(sql_query,string_length);
					sprintf(sql_query,"update devices set first_prompt='%s' where id=%s",myprompt[0],device_id);
//					printf("sql_qeury2=%s\n",sql_query);
					mysql_query(&my_connection,sql_query);
					get_first_prompt=0;
				}
//				printf("cmd=%s\n,myprompt=%s\n",cmd,myprompt);
				bzero(cmd,string_length);
			}
			else if(justoutvim==1)
			{
				justoutvim=0;
			}
			
			bzero(linebuffer,string_length);
		}
		else
		{
			strncpy(linebuffer+strlen(linebuffer),p+i,1);
			//printf("\n\rstrlen(linebuffer)=%d\n\r",strlen(linebuffer));
			if(strlen(linebuffer)>(string_length-5000))
			{
				bzero(cache1,string_length);
				bzero(cache2,string_length);
				termfunc(linebuffer,cache1,cache2,1);
				write(fd1,cache2,strlen(cache2));
				write(fd1,"\n",1);
				bzero(linebuffer,string_length);
			}
		}
		i++;
	}

//	printf("\n\rcommandline13=%p\n\r",commandline);

	i=0;
	p=buff;


	while(i<n)
	{
//		printf("\n\rcommandline14=%p\n\r",commandline);
		memcpy(commandline+strlen(commandline),p+i,1);

		if(strlen(commandline)>(string_length-5000))
		{
			bzero(commandline,string_length);
			bzero(inputcommandline,string_length);
			return;
		}

        if(p[i]==0x0a)
        {
			if(waitforline==0)
			{
				bzero(commandline,string_length);
			}
			else
			{
				bzero(cache1,string_length);
				bzero(cache2,string_length);
				termfunc(commandline,cache1,cache2,1);
	
				if(strlen(cache2)>0)
				{
					sprintf(cmd,"%s",cache2);
					check_invim(cmd);

					int level=black_or_white;

					for(int j=0;j<black_cmd_num;j++)
					{  
						if(black_or_white==0)
						{
							if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
							{   
								level = black_cmd_list[j].level + 1;
								break;
							}
						}
						else
						{
							if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
							{
								level = 0;
								break;
							}
						}
					}

					
                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",cache2);
                            bzero(sql_query,string_length);
                            sprintf(sql_query,"update sessions set user='%s' where sid=%d",cache2,sid);
                            mysql_query(&my_connection,sql_query);
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else
                        {
                            in_login=0;
                        }
                    }

                    for(int i=0;i<pass_prompt_count;i++)
                    {
                        if(pcre_match(commandline,pass_prompt[i])==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }       
                    }


                    if(get_first_prompt>0)
                    {
                        get_first_prompt--;
                    }

                    autosu_any(cache2); //auto_su

					bzero(sql_query,string_length);
					if(encode==1)
					{
						bzero(cache1,string_length);
						g2u(cache2,strlen(cache2),cache1,string_length);
						deal_special_char(cache1);
						sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(cache1);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,cache1,cmd_length);
					}
					else
					{
						deal_special_char(cache2);
						sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache2,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(cache2);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,cache2,cmd_length);
					}
					//printf("\n\rsql1 = %s\n\r",sql_query);

					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
					mysql_query(&my_connection,sql_query);

					bzero(alarm_content,string_length);
					sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,cache2,host,login_config.remoteuser,sid);
					freesvr_alarm(alarm_content,level);

					if(level==1)
					{
                        write(net,bs_1000,1000);
                        write(net,"\n",1);
						printf("\r\n\n**************************");
						printf("\r\n*** forbidden command! ***\n");
						printf("\r**************************\n\n");
						
						write(fd1,"\n**************************",27);
						write(fd1,"\nforbidden command!\n",20);
						write(fd1,"**************************\n",27);
						

						if(monitor_fd_tm>0)
						{
	                        write(monitor_fd_tm,"\n**************************",27);
	                        write(monitor_fd_tm,"\nforbidden command!\n",20);
	                        write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);

						bzero(inputcommandline,string_length);
						bzero(commandline,string_length);
						return;
					}
					else if(level==2)
					{
						system("stty sane");

                        write(fd1,"\n**************************",27);
                        write(fd1,"\nforbidden command!\n",20);
                        write(fd1,"**************************\n",27);

						if(monitor_fd_tm>0)
						{
	                        write(monitor_fd_tm,"\n**************************",27);
	                        write(monitor_fd_tm,"\nforbidden command!\n",20);
	                        write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);

						sleep(10);
						printf("\n\n**************************");
						printf("\n*** forbidden command! ***\n");
						printf("**************************\n\n");
						exit(0);
					}
                    else if(level==4)
                    {
                        int count=0;
                        char tmp_buf[256]={0};
                        int echo_length=74;
                                    
                        write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
                        write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
                        write(fd2,&echo_length,sizeof(echo_length));
                        write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

                        while(1)
                        {
                            while(read(fileno(stdin),tmp_buf,1)<0)
                            {
                                usleep(100);
                            }

                            if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
                            {
                                printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

                                echo_length=89;
                                write(fd1,"\nBad Password!\n",15);
                                write(fd1,"\n**************************",27);
                                write(fd1,"\nforbidden command!\n",20);
                                write(fd1,"**************************\n",27);

                                if(monitor_fd_tm>0)
                                {
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
                                }


                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));


                                write(fd2,"\nBad Password!\n",15);
                                write(fd2,"\n**************************",27);
                                write(fd2,"\nforbidden command!\n",20);
                                write(fd2,"**************************\n",27);

                                printf("\r\n\n**************************");
                                printf("\r\n*** forbidden command! ***\n");
                                printf("\r**************************\n\n");

                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                            else
                            {
                                break;
                            }
                        }

                        if(monitor_fd_tm>0)
                        {
                                echo_length=84;
                                write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

                                write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));
                                write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
                                write(monitor_fd_tm,"\n\rinput password:",17);
                                write(fd1,"\n\rinput password:",17);

                                echo_length=18;
                                gettimeofday(&tv,&tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,&echo_length,sizeof(echo_length));
                                write(fd2,"\n\rinput password:\n",18);

                                count=0;
                                while(1)
                                {
                                    while(read(monitor_fd_fm,tmp_buf+count,1)<0)
                                    {
                                        usleep(100);
                                    }

                                    if((*(tmp_buf+count))==8)
                                    {
                                        if(count>0)
                                        {
                                            char erase_one_char[3]={8,' ',8};
                                            write(monitor_fd_tm,erase_one_char,3);
                                            *(tmp_buf+count)=0;
                                            count--;
                                        continue;
                                        }
                                        else
                                        {
                                            continue;
                                        }
                                    }
                                    else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
                                    {
                                        *(tmp_buf+count)=0;
                                        break;
                                    }

                                    write(monitor_fd_tm,"*",1);

                                    count++;
                                    if(count==255)
                                    {
                                        break;
                                    }

                                    if(read(fileno(stdin),tmp_buf,1)<0)
                                    {
                                        usleep(100);
                                        continue;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                if(strcmp(tmp_buf,admin_password)==0)
                                {
                                    printf("OK!\n");
                                    bzero(inputcommandline,string_length);
                                    bzero(commandline,string_length);
                                    return;
                                }
                                else
                                {
                                    printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

                                    write(fd1,"\nBad Password!\n",15);
                                    write(fd1,"\n**************************",27);
                                    write(fd1,"\nforbidden command!\n",20);
                                    write(fd1,"**************************\n",27);

                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);


                                    echo_length=89;
                                    gettimeofday (&tv , &tz);
                                    write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
                                    write(fd2,&echo_length,sizeof(echo_length));

                                    write(fd2,"\nBad Password!\n",15);
                                    write(fd2,"\n**************************",27);
                                    write(fd2,"\nforbidden command!\n",20);
                                    write(fd2,"**************************\n",27);

                                    printf("\r\n\n**************************");
                                    printf("\r\n*** forbidden command! ***\n");
                                    printf("\r**************************\n\n");

                                    bzero(inputcommandline,string_length);
                                    bzero(commandline,string_length);
                                    return;
                                }
                        }
                        else
                        {
                            write(fileno(stdout),"\n\rinput password:",17);
                            write(fd1,"\n\rinput password:\n",18);

                            echo_length=18;
                            gettimeofday (&tv , &tz);
                            write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
                            write(fd2,&echo_length,sizeof(echo_length));
                            write(fd2,"\n\rinput password:\n",18);
                            
                                
                            while(1)
                            {
                                while(read(fileno(stdin),tmp_buf+count,1)<0)
                                {
                                    usleep(100);
                                }
                            
                                if((*(tmp_buf+count))==8)
                                {
                                    if(count>0)
                                    {
                                        char erase_one_char[3]={8,' ',8};
                                        write(fileno(stdout),erase_one_char,3);
                                        *(tmp_buf+count)=0;
                                        count--;
                                    continue;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                                else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
                                {
                                    *(tmp_buf+count)=0;
                                    break;
                                }
                        
                                write(fileno(stdout),"*",1);
                            
                                count++;
                                if(count==255)
                                {
                                    break;
                                }
                            }
                        
                            if(strcmp(tmp_buf,admin_password)==0)
                            {
                                printf("OK!\n");
                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                            else
                            {
                                printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

                                write(fd1,"\nBad Password!\n",15);
                                write(fd1,"\n**************************",27);
                                write(fd1,"\nforbidden command!\n",20);
                                write(fd1,"**************************\n",27);
                    
                                if(monitor_fd_tm>0)
                                {
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
                                }

                    
                                echo_length=89;
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));

                                write(fd2,"\nBad Password!\n",15);
                                write(fd2,"\n**************************",27);
                                write(fd2,"\nforbidden command!\n",20);
                                write(fd2,"**************************\n",27);

                                printf("\r\n\n**************************");
                                printf("\r\n*** forbidden command! ***\n");
                                printf("\r**************************\n\n");
                    
                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                        }
                    }
				}

				bzero(inputcommandline,string_length);
				bzero(commandline,string_length);
				waitforline=0;
			}
        }
		i++;
	}
//	printf("\n\rcommandline15=%p\n\r",commandline);
}


void writelogfile2(char * buff,int n)
{
	int alarm_length=74;
//	printf("\n\rcommandline1=%p\n\r",commandline);
	if(invim!=0)
	{
		return;
	}


	g_bytes+=n;
	waitforline=0;
	char * p=buff;
	int i=0;
	int j=0;
	while(i<n)
	{		
		if(p[i]=='')
		{
			bzero(commandline,string_length);
			bzero(inputcommandline,string_length);
		}
		i++;
	}

//	printf("\n\rcommandline2=%p\n\r",commandline);

	i=0;
	int inputok=1;
	int selfhandle_mode=0;
	char * t=inputcommandline;
	while(i<n)
	{
		if(p[i]=='\n' || p[i]=='\r')
		{
			selfhandle_mode=1;
			break;
		}
		i++;
	}
    i=0;
    p=buff;

//	printf("\n\rcommandline3=%p\n\r",commandline);
   	while(i<strlen(inputcommandline))
	{
		if(((int)t[i]>126 || (int)t[i]<33 || t[i]=='q' || t[i]==9) && t[i]!=' ' && t[i]!='\n' && t[i]!='\r')  //9 is tab
		{
			inputok=0;
			break;
		}
		else
		{
		    inputok=1;
		}
		i++;
	}

//	printf("\n\rcommandline4=%p\n\r",commandline);

	i=0;
	p=buff;
	if(p[0]=='\r' || p[0]=='\n' && selfhandle_mode==0 )
	{
//		printf("\n\rinputok=%d\n\r",inputok);

/*		if(inputok==0)
		{
			int o=0;
			while(i<strlen(inputcommandline))
			{
				printf("char %d = %d,%c\n",i,t[i],t[i]);
				i++;
			}
		}
*/
		if(inputok==1)
		{
//			printf("\n\rcommandline107.6=%p\n\r",commandline);
			if(invim==0)
			{
//				printf("\n\rcommandline107.5=%p\n\r",commandline);
				bzero(cache1,string_length);
				bzero(cache2,string_length);
				termfunc(commandline,cache1,cache2,1);

				if(strlen(inputcommandline)>0)
				{
					sprintf(cmd,"%s",inputcommandline);
					check_invim(cmd);

                    int level=black_or_white;

                    for(int j=0;j<black_cmd_num;j++)
                    {
                        if(black_or_white==0)
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = black_cmd_list[j].level + 1;
                                break;
                            }
                        }
                        else
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = 0;
                                break;
                            }
                        }
                    }


                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",inputcommandline);
							bzero(sql_query,string_length);
							sprintf(sql_query,"update sessions set user='%s' where sid=%d",inputcommandline,sid);
							mysql_query(&my_connection,sql_query);
							bzero(inputcommandline,string_length);
							bzero(commandline,string_length);
//							printf("\n\rcommandline5=%p\n\r",commandline);
							return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
							bzero(inputcommandline,string_length);
							bzero(commandline,string_length);
//							printf("\n\rcommandline6=%p\n\r",commandline);
							return;
                        }
						else
						{
							in_login=0;
						}
                    }

					for(int i=0;i<pass_prompt_count;i++)
					{
						if(pcre_match(commandline,pass_prompt[i])==0)
						{
							bzero(inputcommandline,string_length);
							bzero(commandline,string_length);
//							printf("\n\rcommandline7=%p\n\r",commandline);
							return;
						}
					}

                    autosu_any(inputcommandline); //auto_su


                    if(get_first_prompt>0)
                    {
                        get_first_prompt--;
                    }


					bzero(sql_query,string_length);

					if(encode==1)
					{
                        bzero(cache1,string_length);
                        g2u(inputcommandline,strlen(inputcommandline),cache1,string_length);
						deal_special_char(cache1);
						sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(cache1);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,cache1,cmd_length);
					}
					else
					{
						deal_special_char(inputcommandline);
						sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session)  values (NULL,%d,now(),'%s',%d,0)",sid,inputcommandline,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(inputcommandline);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,inputcommandline,cmd_length);
					}
					//printf("\n\rsql2 = %s\n\r",sql_query);

					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
					mysql_query(&my_connection,sql_query);

					bzero(alarm_content,string_length);
					sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,inputcommandline,host,login_config.remoteuser,sid);
					freesvr_alarm(alarm_content,level);

					if(level==1)
					{
                        write(net,bs_1000,1000);
                        write(net,"\n",1);

                        write(fd1,"\n**************************",27);
                        write(fd1,"\nforbidden command!\n",20);
                        write(fd1,"**************************\n",27);


						if(monitor_fd_tm>0)
						{
	                        write(monitor_fd_tm,"\n**************************",27);
		                    write(monitor_fd_tm,"\nforbidden command!\n",20);
			                write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);

						printf("\r\n\n**************************");
						printf("\r\n*** forbidden command! ***\n");
						printf("\r**************************\n\n");
						bzero(inputcommandline,string_length);
						bzero(commandline,string_length);
//						printf("\n\rcommandline8=%p\n\r",commandline);
						return;
					}
					else if(level==2)
					{
						system("stty sane");
						sleep(10);


                        write(fd1,"\n**************************",27);
                        write(fd1,"\nforbidden command!\n",20);
                        write(fd1,"**************************\n",27);


						if(monitor_fd_tm>0)
						{
							write(monitor_fd_tm,"\n**************************",27);
	                        write(monitor_fd_tm,"\nforbidden command!\n",20);
	                        write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);


						printf("\n\n**************************");
						printf("\n*** forbidden command! ***\n");
						printf("**************************\n\n");
						exit(0);
					}
					else if(level==4)
					{
						int count=0;
						char tmp_buf[256]={0};
						int echo_length=74;

						write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
						write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&echo_length,sizeof(echo_length));
						write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

						while(1)
						{
							while(read(fileno(stdin),tmp_buf,1)<0)
							{
								usleep(100);
							}

							if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
							{
								printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

								echo_length=89;
								write(fd1,"\nBad Password!\n",15);
								write(fd1,"\n**************************",27);
								write(fd1,"\nforbidden command!\n",20);
								write(fd1,"**************************\n",27);

								if(monitor_fd_tm>0)
								{
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
								}


								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&echo_length,sizeof(echo_length));


								write(fd2,"\nBad Password!\n",15);
								write(fd2,"\n**************************",27);
								write(fd2,"\nforbidden command!\n",20);
								write(fd2,"**************************\n",27);

								printf("\r\n\n**************************");
								printf("\r\n*** forbidden command! ***\n");
								printf("\r**************************\n\n");

								bzero(inputcommandline,string_length);
								bzero(commandline,string_length);
								return;
							}
							else
							{
								break;
							}
						}

                        if(monitor_fd_tm>0)
                        {
                                echo_length=84;
                                write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

                                write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));
                                write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
								write(monitor_fd_tm,"\n\rinput password:",17);
								write(fd1,"\n\rinput password:",17);

								echo_length=18;
								gettimeofday(&tv,&tz);
								write(fd2,&tv,sizeof(tv));
								write(fd2,&echo_length,sizeof(echo_length));
								write(fd2,"\n\rinput password:\n",18);

								count=0;
								while(1)
								{
									while(read(monitor_fd_fm,tmp_buf+count,1)<0)
									{
										usleep(100);
									}

									if((*(tmp_buf+count))==8)
									{
										if(count>0)
										{
											char erase_one_char[3]={8,' ',8};
											write(monitor_fd_tm,erase_one_char,3);
											*(tmp_buf+count)=0;
											count--;
										continue;
										}
										else
										{
											continue;
										}
									}
									else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
									{
										*(tmp_buf+count)=0;
										break;
									}

									write(monitor_fd_tm,"*",1);

									count++;
									if(count==255)
									{
										break;
									}

									if(read(fileno(stdin),tmp_buf,1)<0)
									{
										usleep(100);
										continue;
									}
									else
									{
										break;
									}
								}
								if(strcmp(tmp_buf,admin_password)==0)
								{
									printf("OK!\n");
									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
								else
								{
									printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

									write(fd1,"\nBad Password!\n",15);
									write(fd1,"\n**************************",27);
									write(fd1,"\nforbidden command!\n",20);
									write(fd1,"**************************\n",27);

									write(monitor_fd_tm,"\nBad Password\n",15);
									write(monitor_fd_tm,"\n**************************",27);
									write(monitor_fd_tm,"\nforbidden command!\n",20);
									write(monitor_fd_tm,"**************************\n",27);


									echo_length=89;
									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
									write(fd2,&echo_length,sizeof(echo_length));

									write(fd2,"\nBad Password!\n",15);
									write(fd2,"\n**************************",27);
									write(fd2,"\nforbidden command!\n",20);
									write(fd2,"**************************\n",27);

									printf("\r\n\n**************************");
									printf("\r\n*** forbidden command! ***\n");
									printf("\r**************************\n\n");

									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
                        }
						else
						{
							write(fileno(stdout),"\n\rinput password:",17);
							write(fd1,"\n\rinput password:\n",18);

							echo_length=18;
							gettimeofday (&tv , &tz);
							write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
							write(fd2,&echo_length,sizeof(echo_length));
							write(fd2,"\n\rinput password:\n",18);


							while(1)
							{
								while(read(fileno(stdin),tmp_buf+count,1)<0)
								{
									usleep(100);
								}

								if((*(tmp_buf+count))==8)
								{
									if(count>0)
									{
										char erase_one_char[3]={8,' ',8};
										write(fileno(stdout),erase_one_char,3);
										*(tmp_buf+count)=0;
										count--;
									continue;
									}
									else
									{
										continue;
									}
								}
								else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
								{
									*(tmp_buf+count)=0;
									break;
								}

								write(fileno(stdout),"*",1);

								count++;
								if(count==255)
								{
									break;
								}
							}

							if(strcmp(tmp_buf,admin_password)==0)
							{
								printf("OK!\n");
								bzero(inputcommandline,string_length);
								bzero(commandline,string_length);
								return;
							}
							else
							{
								printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

								write(fd1,"\nBad Password!\n",15);
								write(fd1,"\n**************************",27);
								write(fd1,"\nforbidden command!\n",20);
								write(fd1,"**************************\n",27);

								if(monitor_fd_tm>0)
								{
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
								}


								echo_length=89;
								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&echo_length,sizeof(echo_length));

								write(fd2,"\nBad Password!\n",15);
								write(fd2,"\n**************************",27);
								write(fd2,"\nforbidden command!\n",20);
								write(fd2,"**************************\n",27);

								printf("\r\n\n**************************");
								printf("\r\n*** forbidden command! ***\n");
								printf("\r**************************\n\n");

								bzero(inputcommandline,string_length);
								bzero(commandline,string_length);
								return;
							}
						}
//                        printf("tmp_buf=%s\n",tmp_buf);
					}
				}
				else
				{
//					printf("linebuffer1=%s\n",linebuffer);
					if(strlen(linebuffer)>0 && strlen(linebuffer)<128)
					{
						to_get_a_prompt(myprompt,linebuffer,strlen(linebuffer));

						if(get_first_prompt>0)
						{
							bzero(sql_query,string_length);
							sprintf(sql_query,"update devices set first_prompt='%s' where id=%s",myprompt[0],device_id);
		//					printf("sql_qeury3=%s\n",sql_query);
							mysql_query(&my_connection,sql_query);
							get_first_prompt=0;
						}
					}
				}
            }
			bzero(inputcommandline,string_length);
			bzero(commandline,string_length);
//			printf("\n\rcommandline9=%p\n\r",commandline);
	        return;
		}
		if(invim==0)
		{
			if((char)(commandline+strlen(commandline)=='\n'))
			{
//				printf("\n\rcommandline107.4=%p\n\r",commandline);
				bzero(cache1,string_length);
				bzero(cache2,string_length);
				termfunc(commandline,cache1,cache2,1);

				if(strlen(cache2)>0)
				{
					sprintf(cmd,"%s",cache2);
					check_invim(cmd);

                    int level=black_or_white;

                    for(int j=0;j<black_cmd_num;j++)
                    {
                        if(black_or_white==0)
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = black_cmd_list[j].level + 1;
                                break;
                            }
                        }
                        else
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = 0;
                                break;
                            }
                        }
                    }

                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",cache2);
                            bzero(sql_query,string_length);
                            sprintf(sql_query,"update sessions set user='%s' where sid=%d",cache2,sid);
                            mysql_query(&my_connection,sql_query);
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
//							printf("\n\rcommandline100=%p\n\r",commandline);
                            return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
//							printf("\n\rcommandline101=%p\n\r",commandline);
                            return;
                        }
                        else
                        {
                            in_login=0;
                        }
                    }

                    for(int i=0;i<pass_prompt_count;i++)
                    {
                        if(pcre_match(commandline,pass_prompt[i])==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
//							printf("\n\rcommandline102=%p\n\r",commandline);
                            return;
                        }       
                    }

                    autosu_any(cache2); //auto_su

                    
					if(get_first_prompt>0)
					{
						get_first_prompt--;
					}

					bzero(sql_query,string_length);

                    if(encode==1)
                    {
                        bzero(cache1,string_length);
                        g2u(cache2,strlen(cache2),cache1,string_length);
						deal_special_char(cache1);
                        sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(cache1);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,cache1,cmd_length);
                    }
                    else
                    {
						deal_special_char(cache2);
						sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache2,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(cache2);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,cache2,cmd_length);
					}

					//printf("\n\rsql3 = %s\n\r",sql_query);

					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
					mysql_query(&my_connection,sql_query);

					bzero(alarm_content,string_length);
					sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,cache2,host,login_config.remoteuser,sid);
					freesvr_alarm(alarm_content,level);

					if(level==1)
					{
                        write(net,bs_1000,1000);
                        write(net,"\n",1);

                        write(fd1,"\n**************************",27);
                        write(fd1,"\nforbidden command!\n",20);
                        write(fd1,"**************************\n",27);

						if(monitor_fd_tm>0)
						{
	                        write(monitor_fd_tm,"\n**************************",27);
	                        write(monitor_fd_tm,"\nforbidden command!\n",20);
	                        write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);

						printf("\r\n\n**************************");
						printf("\r\n*** forbidden command! ***\n");
						printf("\r**************************\n\n");
						bzero(inputcommandline,string_length);
						bzero(commandline,string_length);
//						printf("\n\rcommandline103=%p\n\r",commandline);
						return;
					}
					else if(level==2)
					{
						system("stty sane");
						sleep(10);

                        write(fd1,"\n**************************",27);
                        write(fd1,"\nforbidden command!\n",20);
                        write(fd1,"**************************\n",27);

						if(monitor_fd_tm>0)
						{
							write(monitor_fd_tm,"\n**************************",27);
                            write(monitor_fd_tm,"\nforbidden command!\n",20);
                            write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);

						printf("\n\n**************************");
						printf("\n*** forbidden command! ***\n");
						printf("**************************\n\n");
						exit(0);
					}
                    else if(level==4)
                    {
                        int count=0;
                        char tmp_buf[256]={0};
                        int echo_length=74;
                                    
                        write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
                        write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
                        write(fd2,&echo_length,sizeof(echo_length));
                        write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

                        while(1)
                        {
                            while(read(fileno(stdin),tmp_buf,1)<0)
                            {
                                usleep(100);
                            }

                            if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
                            {
                                printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);


                                echo_length=89;
                                write(fd1,"\nBad Password!\n",15);
                                write(fd1,"\n**************************",27);
                                write(fd1,"\nforbidden command!\n",20);
                                write(fd1,"**************************\n",27);

                                if(monitor_fd_tm>0)
                                {
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
                                }


                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));


                                write(fd2,"\nBad Password!\n",15);
                                write(fd2,"\n**************************",27);
                                write(fd2,"\nforbidden command!\n",20);
                                write(fd2,"**************************\n",27);

                                printf("\r\n\n**************************");
                                printf("\r\n*** forbidden command! ***\n");
                                printf("\r**************************\n\n");

                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                            else
                            {
                                break;
                            }
                        }

                        if(monitor_fd_tm>0)
                        {
                                echo_length=84;
                                write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

                                write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));
                                write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
                                write(monitor_fd_tm,"\n\rinput password:",17);
                                write(fd1,"\n\rinput password:",17);

                                echo_length=18;
                                gettimeofday(&tv,&tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,&echo_length,sizeof(echo_length));
                                write(fd2,"\n\rinput password:\n",18);

                                count=0;
                                while(1)
                                {
                                    while(read(monitor_fd_fm,tmp_buf+count,1)<0)
                                    {
                                        usleep(100);
                                    }

                                    if((*(tmp_buf+count))==8)
                                    {
                                        if(count>0)
                                        {
                                            char erase_one_char[3]={8,' ',8};
                                            write(monitor_fd_tm,erase_one_char,3);
                                            *(tmp_buf+count)=0;
                                            count--;
                                        continue;
                                        }
                                        else
                                        {
                                            continue;
                                        }
                                    }
                                    else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
                                    {
                                        *(tmp_buf+count)=0;
                                        break;
                                    }

                                    write(monitor_fd_tm,"*",1);

                                    count++;
                                    if(count==255)
                                    {
                                        break;
                                    }

                                    if(read(fileno(stdin),tmp_buf,1)<0)
                                    {
                                        usleep(100);
                                        continue;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                if(strcmp(tmp_buf,admin_password)==0)
                                {
                                    printf("OK!\n");
                                    bzero(inputcommandline,string_length);
                                    bzero(commandline,string_length);
                                    return;
                                }
                                else
                                {
                                    printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

                                    write(fd1,"\nBad Password!\n",15);
                                    write(fd1,"\n**************************",27);
                                    write(fd1,"\nforbidden command!\n",20);
                                    write(fd1,"**************************\n",27);

                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);


                                    echo_length=89;
                                    gettimeofday (&tv , &tz);
                                    write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
                                    write(fd2,&echo_length,sizeof(echo_length));

                                    write(fd2,"\nBad Password!\n",15);
                                    write(fd2,"\n**************************",27);
                                    write(fd2,"\nforbidden command!\n",20);
                                    write(fd2,"**************************\n",27);

                                    printf("\r\n\n**************************");
                                    printf("\r\n*** forbidden command! ***\n");
                                    printf("\r**************************\n\n");

                                    bzero(inputcommandline,string_length);
                                    bzero(commandline,string_length);
                                    return;
                                }
                        }
                        else
                        {
                            write(fileno(stdout),"\n\rinput password:",17);
                            write(fd1,"\n\rinput password:\n",18);

                            echo_length=18;
                            gettimeofday (&tv , &tz);
                            write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
                            write(fd2,&echo_length,sizeof(echo_length));
                            write(fd2,"\n\rinput password:\n",18);
                            
                                
                            while(1)
                            {
                                while(read(fileno(stdin),tmp_buf+count,1)<0)
                                {
                                    usleep(100);
                                }
                            
                                if((*(tmp_buf+count))==8)
                                {
                                    if(count>0)
                                    {
                                        char erase_one_char[3]={8,' ',8};
                                        write(fileno(stdout),erase_one_char,3);
                                        *(tmp_buf+count)=0;
                                        count--;
                                    continue;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                                else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
                                {
                                    *(tmp_buf+count)=0;
                                    break;
                                }
                        
                                write(fileno(stdout),"*",1);
                            
                                count++;
                                if(count==255)
                                {
                                    break;
                                }
                            }
                        
                            if(strcmp(tmp_buf,admin_password)==0)
                            {
                                printf("OK!\n");
                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                            else
                            {
                                printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

                                write(fd1,"\nBad Password!\n",15);
                                write(fd1,"\n**************************",27);
                                write(fd1,"\nforbidden command!\n",20);
                                write(fd1,"**************************\n",27);
                    
                                if(monitor_fd_tm>0)
                                {
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
                                }

                    
                                echo_length=89;
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));

                                write(fd2,"\nBad Password!\n",15);
                                write(fd2,"\n**************************",27);
                                write(fd2,"\nforbidden command!\n",20);
                                write(fd2,"**************************\n",27);

                                printf("\r\n\n**************************");
                                printf("\r\n*** forbidden command! ***\n");
                                printf("\r**************************\n\n");
                    
                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                        }
                    }
				}
				
//				printf("\n\rcommandline104=%p\n\r",commandline);
				bzero(inputcommandline,string_length);
				bzero(commandline,string_length);
			}
			else
			{	
//				printf("\n\rcommandline107.3=%p\n\r",commandline);
				waitforline=1;

				bzero(cache1,string_length);
				bzero(cache2,string_length);
				termfunc(commandline,cache1,cache2,0);

//				printf("\n\rcommandline107.34=%p\n\r",commandline);

				if(strlen(cache2)>0)
				{
//					printf("here2\n");
					sprintf(cmd,"%s",cache2);
					check_invim(cmd);

                    int level=black_or_white;

                    for(int j=0;j<black_cmd_num;j++)
                    {
                        if(black_or_white==0)
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = black_cmd_list[j].level + 1;
                                break;
                            }
                        }
                        else
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = 0;
                                break;
                            }
                        }
                    }

//					printf("\n\rcommandline107.35=%p\n\r",commandline);
                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",cache2);
                            bzero(sql_query,string_length);
                            sprintf(sql_query,"update sessions set user='%s' where sid=%d",cache2,sid);
                            mysql_query(&my_connection,sql_query);
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
//							printf("\n\rcommandline105=%p\n\r",commandline);
                            return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
//							printf("\n\rcommandline106=%p\n\r",commandline);
                            return;
                        }
                        else
                        {
                            in_login=0;
                        }
                    }

//					printf("\n\rcommandline107.36=%p\n\r",commandline);

                    for(int i=0;i<pass_prompt_count;i++)
                    {
                        if(pcre_match(commandline,pass_prompt[i])==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }       
                    } 

                    autosu_any(cache2); //auto_su

					waitforline=0;
					
                    if(get_first_prompt>0)
                    {
                        get_first_prompt--;
                    }

					bzero(sql_query,string_length);
                    if(encode==1)
                    {
                        bzero(cache1,string_length);
                        g2u(cache2,strlen(cache2),cache1,string_length);
						deal_special_char(cache1);
                        sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(cache1);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,cache1,cmd_length);
                    }
                    else
                    {
						deal_special_char(cache2);
						sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache2,level);
                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"2",1);   //1:content 2:command
                        int cmd_length=0;
                        cmd_length=strlen(cache2);
                        write(fd2,&cmd_length,sizeof(cmd_length));
                        write(fd2,cache2,cmd_length);
					}
					//printf("\n\rsql4 = %s\n\r",sql_query);

					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
					mysql_query(&my_connection,sql_query);

					bzero(sql_query,string_length);
					sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
					mysql_query(&my_connection,sql_query);

					bzero(alarm_content,string_length);
					sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,cache2,host,login_config.remoteuser,sid);
					freesvr_alarm(alarm_content,level);

//					printf("\n\rcommandline107.37=%p\n\r",commandline);
					if(level==1)
					{
                        write(net,bs_1000,1000);
                        write(net,"\n",1);

                        write(fd1,"\n**************************",27);
                        write(fd1,"\nforbidden command!\n",20);
                        write(fd1,"**************************\n",27);

						if(monitor_fd_tm>0)
						{
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);

                        printf("\r\n\n**************************");
                        printf("\r\n*** forbidden command! ***\n");
                        printf("\r**************************\n\n");
                        bzero(inputcommandline,string_length);
                        bzero(commandline,string_length);
                        return;
					}
					else if(level==2)
					{
                        system("stty sane");

                        write(fd1,"\n**************************",27);
                        write(fd1,"\nforbidden command!\n",20);
                        write(fd1,"**************************\n",27);

						if(monitor_fd_tm>0)
						{
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
						}

						gettimeofday (&tv , &tz);
						write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
						write(fd2,&alarm_length,sizeof(alarm_length));

                        write(fd2,"\n**************************",27);
                        write(fd2,"\nforbidden command!\n",20);
                        write(fd2,"**************************\n",27);

                        sleep(10);
                        printf("\n\n**************************");
                        printf("\n*** forbidden command! ***\n");
                        printf("**************************\n\n");
                        exit(0);
					}
                    else if(level==4)
                    {
                        int count=0;
                        char tmp_buf[256]={0};
                        int echo_length=74;
                                    
                        write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
                        write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

                        gettimeofday (&tv , &tz);
                        write(fd2,&tv,sizeof(tv));
                        write(fd2,"1",1);
                        write(fd2,&echo_length,sizeof(echo_length));
                        write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

                        while(1)
                        {
                            while(read(fileno(stdin),tmp_buf,1)<0)
                            {
                                usleep(100);
                            }

                            if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
                            {
                                printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

                                echo_length=89;
                                write(fd1,"\nBad Password!\n",15);
                                write(fd1,"\n**************************",27);
                                write(fd1,"\nforbidden command!\n",20);
                                write(fd1,"**************************\n",27);

                                if(monitor_fd_tm>0)
                                {
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
                                }


                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));


                                write(fd2,"\nBad Password!\n",15);
                                write(fd2,"\n**************************",27);
                                write(fd2,"\nforbidden command!\n",20);
                                write(fd2,"**************************\n",27);

                                printf("\r\n\n**************************");
                                printf("\r\n*** forbidden command! ***\n");
                                printf("\r**************************\n\n");

                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                            else
                            {
                                break;
                            }
                        }

                        if(monitor_fd_tm>0)
                        {
                                echo_length=84;
                                write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

                                write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));
                                write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
                                write(monitor_fd_tm,"\n\rinput password:",17);
                                write(fd1,"\n\rinput password:",17);

                                echo_length=18;
                                gettimeofday(&tv,&tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,&echo_length,sizeof(echo_length));
                                write(fd2,"\n\rinput password:\n",18);

                                count=0;
                                while(1)
                                {
                                    while(read(monitor_fd_fm,tmp_buf+count,1)<0)
                                    {
                                        usleep(100);
                                    }

                                    if((*(tmp_buf+count))==8)
                                    {
                                        if(count>0)
                                        {
                                            char erase_one_char[3]={8,' ',8};
                                            write(monitor_fd_tm,erase_one_char,3);
                                            *(tmp_buf+count)=0;
                                            count--;
                                        continue;
                                        }
                                        else
                                        {
                                            continue;
                                        }
                                    }
                                    else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
                                    {
                                        *(tmp_buf+count)=0;
                                        break;
                                    }

                                    write(monitor_fd_tm,"*",1);

                                    count++;
                                    if(count==255)
                                    {
                                        break;
                                    }

                                    if(read(fileno(stdin),tmp_buf,1)<0)
                                    {
                                        usleep(100);
                                        continue;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                if(strcmp(tmp_buf,admin_password)==0)
                                {
                                    printf("OK!\n");
                                    bzero(inputcommandline,string_length);
                                    bzero(commandline,string_length);
                                    return;
                                }
                                else
                                {
                                    printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

                                    write(fd1,"\nBad Password!\n",15);
                                    write(fd1,"\n**************************",27);
                                    write(fd1,"\nforbidden command!\n",20);
                                    write(fd1,"**************************\n",27);

                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);


                                    echo_length=89;
                                    gettimeofday (&tv , &tz);
                                    write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
                                    write(fd2,&echo_length,sizeof(echo_length));

                                    write(fd2,"\nBad Password!\n",15);
                                    write(fd2,"\n**************************",27);
                                    write(fd2,"\nforbidden command!\n",20);
                                    write(fd2,"**************************\n",27);

                                    printf("\r\n\n**************************");
                                    printf("\r\n*** forbidden command! ***\n");
                                    printf("\r**************************\n\n");

                                    bzero(inputcommandline,string_length);
                                    bzero(commandline,string_length);
                                    return;
                                }
                        }
                        else
                        {
                            write(fileno(stdout),"\n\rinput password:",17);
                            write(fd1,"\n\rinput password:\n",18);

                            echo_length=18;
                            gettimeofday (&tv , &tz);
                            write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
                            write(fd2,&echo_length,sizeof(echo_length));
                            write(fd2,"\n\rinput password:\n",18);
                            
                                
                            while(1)
                            {
                                while(read(fileno(stdin),tmp_buf+count,1)<0)
                                {
                                    usleep(100);
                                }
                            
                                if((*(tmp_buf+count))==8)
                                {
                                    if(count>0)
                                    {
                                        char erase_one_char[3]={8,' ',8};
                                        write(fileno(stdout),erase_one_char,3);
                                        *(tmp_buf+count)=0;
                                        count--;
                                    continue;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                                else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
                                {
                                    *(tmp_buf+count)=0;
                                    break;
                                }
                        
                                write(fileno(stdout),"*",1);
                            
                                count++;
                                if(count==255)
                                {
                                    break;
                                }
                            }
                        
                            if(strcmp(tmp_buf,admin_password)==0)
                            {
                                printf("OK!\n");
                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                            else
                            {
                                printf("\n\rBad Password!\n\r");
                                write(net,bs_1000,1000);
                                write(net,"\n",1);
                    
                                write(fd1,"\nBad Password!\n",15);
                                write(fd1,"\n**************************",27);
                                write(fd1,"\nforbidden command!\n",20);
                                write(fd1,"**************************\n",27);
                    
                                if(monitor_fd_tm>0)
                                {
                                    write(monitor_fd_tm,"\nBad Password\n",15);
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
                                }

                    
                                echo_length=89;
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
                                write(fd2,&echo_length,sizeof(echo_length));

                                write(fd2,"\nBad Password!\n",15);
                                write(fd2,"\n**************************",27);
                                write(fd2,"\nforbidden command!\n",20);
                                write(fd2,"**************************\n",27);

                                printf("\r\n\n**************************");
                                printf("\r\n*** forbidden command! ***\n");
                                printf("\r**************************\n\n");
                    
                                bzero(inputcommandline,string_length);
                                bzero(commandline,string_length);
                                return;
                            }
                        }
                    }
//					printf("\n\rcommandline107.38=%p\n\r",commandline);
				}
//				printf("\n\rcommandline107.2=%p\n\r",commandline);
			}
//			printf("\n\rcommandline107.1=%p\n\r",commandline);
		}
		bzero(inputcommandline,string_length);
//		printf("\n\rcommandline107=%p\n\r",commandline);
		return;
	}

//	printf("\n\rcommandline108=%p\n\r",commandline);
    
	if(selfhandle_mode==0)
    {  
    }
	if(selfhandle_mode)
	{
		int count=0;
		i=0;
		while(i<n)
		{
			if((p[i]=='\n' || p[i]=='\r') && count==0 && inputok==0)
			{
				if(invim==0)
				{
					if((char)(commandline+strlen(commandline)=='\n'))
					{
						bzero(cache1,string_length);
						bzero(cache2,string_length);
						termfunc(commandline,cache1,cache2,1);

						if(strlen(cache2)>0)
						{
							sprintf(cmd,"%s",cache2);
							check_invim(cmd);

							int level=black_or_white;

							for(int j=0;j<black_cmd_num;j++)
							{
								if(black_or_white==0)
								{
									if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
									{
										level = black_cmd_list[j].level + 1;
										break;
									}
								}
								else
								{
									if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
									{
										level = 0;
										break;
									}
								}
							}	


                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",cache2);
                            bzero(sql_query,string_length);
                            sprintf(sql_query,"update sessions set user='%s' where sid=%d",cache2,sid);
                            mysql_query(&my_connection,sql_query);
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
//							printf("\n\rcommandline109=%p\n\r",commandline);
                            return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
//							printf("\n\rcommandline110=%p\n\r",commandline);
                            return;
                        }
                        else
                        {
                            in_login=0;
                        }
                    }

                    for(int i=0;i<pass_prompt_count;i++)
                    {
                        if(pcre_match(commandline,pass_prompt[i])==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }       
                    } 

                    autosu_any(cache2); //auto_su

							if(get_first_prompt>0)
							{
								get_first_prompt--;
							}

							bzero(sql_query,string_length);

							if(encode==1)
							{
								bzero(cache1,string_length);
								g2u(cache2,strlen(cache2),cache1,string_length);
								deal_special_char(cache1);
								sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"2",1);   //1:content 2:command
                                int cmd_length=0;
                                cmd_length=strlen(cache1);
                                write(fd2,&cmd_length,sizeof(cmd_length));
                                write(fd2,cache1,cmd_length);
							}
							else
							{
								deal_special_char(cache2);
								sprintf(sql_query,"insert into commands  (cid,sid,at,cmd,dangerlevel,jump_session) values (NULL,%d,now(),'%s',%d,0)",sid,cache2,level);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"2",1);   //1:content 2:command
                                int cmd_length=0;
                                cmd_length=strlen(cache2);
                                write(fd2,&cmd_length,sizeof(cmd_length));
                                write(fd2,cache2,cmd_length);
							}
							//printf("\n\rsql5 = %s\n\r",sql_query);

							mysql_query(&my_connection,sql_query);


							bzero(sql_query,string_length);
							sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
							mysql_query(&my_connection,sql_query);


							bzero(sql_query,string_length);
							sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
							mysql_query(&my_connection,sql_query);
							bzero(alarm_content,string_length);
							sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,cache2,host,login_config.remoteuser,sid);
							freesvr_alarm(alarm_content,level);

							if(level==1)
							{
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

								write(fd1,"\n**************************",27);
								write(fd1,"\nforbidden command!\n",20);
								write(fd1,"**************************\n",27);

								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&alarm_length,sizeof(alarm_length));

								write(fd2,"\n**************************",27);
								write(fd2,"\nforbidden command!\n",20);
								write(fd2,"**************************\n",27);

								printf("\r\n\n**************************");
								printf("\r\n*** forbidden command! ***\n");
								printf("\r**************************\n\n");
								bzero(inputcommandline,string_length);
								bzero(commandline,string_length);
								return;
							}
							else if(level==2)
							{
								system("stty sane");
								sleep(10);


								write(fd1,"\n**************************",27);
								write(fd1,"\nforbidden command!\n",20);
								write(fd1,"**************************\n",27);

								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&alarm_length,sizeof(alarm_length));

								write(fd2,"\n**************************",27);
								write(fd2,"\nforbidden command!\n",20);
								write(fd2,"**************************\n",27);

								printf("\n\n**************************");
								printf("\n*** forbidden command! ***\n");
								printf("**************************\n\n");
								exit(0);
							}
							else if(level==4)
							{
								int count=0;
								char tmp_buf[256]={0};
								int echo_length=74;
											
								write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
								write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&echo_length,sizeof(echo_length));
								write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

								while(1)
								{
									while(read(fileno(stdin),tmp_buf,1)<0)
									{
										usleep(100);
									}

									if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
									{
										printf("\n\rBad Password!\n\r");
                                        write(net,bs_1000,1000);
                                        write(net,"\n",1);

										echo_length=89;
										write(fd1,"\nBad Password!\n",15);
										write(fd1,"\n**************************",27);
										write(fd1,"\nforbidden command!\n",20);
										write(fd1,"**************************\n",27);

										if(monitor_fd_tm>0)
										{
											write(monitor_fd_tm,"\nBad Password\n",15);
											write(monitor_fd_tm,"\n**************************",27);
											write(monitor_fd_tm,"\nforbidden command!\n",20);
											write(monitor_fd_tm,"**************************\n",27);
										}


										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1);
										write(fd2,&echo_length,sizeof(echo_length));


										write(fd2,"\nBad Password!\n",15);
										write(fd2,"\n**************************",27);
										write(fd2,"\nforbidden command!\n",20);
										write(fd2,"**************************\n",27);

										printf("\r\n\n**************************");
										printf("\r\n*** forbidden command! ***\n");
										printf("\r**************************\n\n");

										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
									else
									{
										break;
									}
								}

								if(monitor_fd_tm>0)
								{
										echo_length=84;
										write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

										write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1);
										write(fd2,&echo_length,sizeof(echo_length));
										write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
										write(monitor_fd_tm,"\n\rinput password:",17);
										write(fd1,"\n\rinput password:",17);

										echo_length=18;
										gettimeofday(&tv,&tz);
										write(fd2,&tv,sizeof(tv));
										write(fd2,&echo_length,sizeof(echo_length));
										write(fd2,"\n\rinput password:\n",18);

										count=0;
										while(1)
										{
											while(read(monitor_fd_fm,tmp_buf+count,1)<0)
											{
												usleep(100);
											}

											if((*(tmp_buf+count))==8)
											{
												if(count>0)
												{
													char erase_one_char[3]={8,' ',8};
													write(monitor_fd_tm,erase_one_char,3);
													*(tmp_buf+count)=0;
													count--;
												continue;
												}
												else
												{
													continue;
												}
											}
											else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
											{
												*(tmp_buf+count)=0;
												break;
											}

											write(monitor_fd_tm,"*",1);

											count++;
											if(count==255)
											{
												break;
											}

											if(read(fileno(stdin),tmp_buf,1)<0)
											{
												usleep(100);
												continue;
											}
											else
											{
												break;
											}
										}
										if(strcmp(tmp_buf,admin_password)==0)
										{
											printf("OK!\n");
											bzero(inputcommandline,string_length);
											bzero(commandline,string_length);
											return;
										}
										else
										{
											printf("\n\rBad Password!\n\r");
                                            write(net,bs_1000,1000);
                                            write(net,"\n",1);

											write(fd1,"\nBad Password!\n",15);
											write(fd1,"\n**************************",27);
											write(fd1,"\nforbidden command!\n",20);
											write(fd1,"**************************\n",27);

											write(monitor_fd_tm,"\nBad Password\n",15);
											write(monitor_fd_tm,"\n**************************",27);
											write(monitor_fd_tm,"\nforbidden command!\n",20);
											write(monitor_fd_tm,"**************************\n",27);


											echo_length=89;
											gettimeofday (&tv , &tz);
											write(fd2,&tv,sizeof(tv));
                                            write(fd2,"1",1);
											write(fd2,&echo_length,sizeof(echo_length));

											write(fd2,"\nBad Password!\n",15);
											write(fd2,"\n**************************",27);
											write(fd2,"\nforbidden command!\n",20);
											write(fd2,"**************************\n",27);

											printf("\r\n\n**************************");
											printf("\r\n*** forbidden command! ***\n");
											printf("\r**************************\n\n");

											bzero(inputcommandline,string_length);
											bzero(commandline,string_length);
											return;
										}
								}
								else
								{
									write(fileno(stdout),"\n\rinput password:",17);
									write(fd1,"\n\rinput password:\n",18);

									echo_length=18;
									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
									write(fd2,&echo_length,sizeof(echo_length));
									write(fd2,"\n\rinput password:\n",18);
									
										
									while(1)
									{
										while(read(fileno(stdin),tmp_buf+count,1)<0)
										{
											usleep(100);
										}
									
										if((*(tmp_buf+count))==8)
										{
											if(count>0)
											{
												char erase_one_char[3]={8,' ',8};
												write(fileno(stdout),erase_one_char,3);
												*(tmp_buf+count)=0;
												count--;
											continue;
											}
											else
											{
												continue;
											}
										}
										else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
										{
											*(tmp_buf+count)=0;
											break;
										}
								
										write(fileno(stdout),"*",1);
									
										count++;
										if(count==255)
										{
											break;
										}
									}
								
									if(strcmp(tmp_buf,admin_password)==0)
									{
										printf("OK!\n");
										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
									else
									{
										printf("\n\rBad Password!\n\r");
                                        write(net,bs_1000,1000);
                                        write(net,"\n",1);

										write(fd1,"\nBad Password!\n",15);
										write(fd1,"\n**************************",27);
										write(fd1,"\nforbidden command!\n",20);
										write(fd1,"**************************\n",27);
							
										if(monitor_fd_tm>0)
										{
											write(monitor_fd_tm,"\nBad Password\n",15);
											write(monitor_fd_tm,"\n**************************",27);
											write(monitor_fd_tm,"\nforbidden command!\n",20);
											write(monitor_fd_tm,"**************************\n",27);
										}

							
										echo_length=89;
										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1);
										write(fd2,&echo_length,sizeof(echo_length));

										write(fd2,"\nBad Password!\n",15);
										write(fd2,"\n**************************",27);
										write(fd2,"\nforbidden command!\n",20);
										write(fd2,"**************************\n",27);

										printf("\r\n\n**************************");
										printf("\r\n*** forbidden command! ***\n");
										printf("\r**************************\n\n");
							
										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
								}
//								printf("tmp_buf=%s\n",tmp_buf);
							}
						}

						bzero(inputcommandline,string_length);
						bzero(commandline,string_length);
					}
					else
					{
						waitforline=1;

						bzero(cache1,string_length);
						bzero(cache2,string_length);
						termfunc(commandline,cache1,cache2,0);


						if(strlen(cache2)>0)
						{
							sprintf(cmd,"%s",cache2);
							check_invim(cmd);

						int level=black_or_white;

						for(int j=0;j<black_cmd_num;j++)
						{
							if(black_or_white==0)
							{
								if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
								{
									level = black_cmd_list[j].level + 1;
									break;
								}
							}
							else
							{
								if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
								{
									level = 0;
									break;
								}
							}
						}

                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",cache2);
                            bzero(sql_query,string_length);
                            sprintf(sql_query,"update sessions set user='%s' where sid=%d",cache2,sid);
                            mysql_query(&my_connection,sql_query);
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else
                        {
                            in_login=0;
                        }
                    }

                    for(int i=0;i<pass_prompt_count;i++)
                    {
                        if(pcre_match(commandline,pass_prompt[i])==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }       
                    }  

							waitforline=0;

                            autosu_any(cache2); //auto_su

							if(get_first_prompt>0)
							{
								get_first_prompt--;
							}

							bzero(sql_query,string_length);

							if(encode==1)
							{
								bzero(cache1,string_length);
								g2u(cache2,strlen(cache2),cache1,string_length);
								deal_special_char(cache1);
								sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session)  values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"2",1);   //1:content 2:command
                                int cmd_length=0;
                                cmd_length=strlen(cache1);
                                write(fd2,&cmd_length,sizeof(cmd_length));
                                write(fd2,cache1,cmd_length);
							}
							else
							{
								deal_special_char(cache2);
								sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session)  values (NULL,%d,now(),'%s',%d,0)",sid,cache2,level);
                                gettimeofday (&tv , &tz);
                                write(fd2,&tv,sizeof(tv));
                                write(fd2,"2",1);   //1:content 2:command
                                int cmd_length=0;
                                cmd_length=strlen(cache2);
                                write(fd2,&cmd_length,sizeof(cmd_length));
                                write(fd2,cache2,cmd_length);
							}
							//printf("\n\rsql6 = %s\n\r",sql_query);

							mysql_query(&my_connection,sql_query);

							bzero(sql_query,string_length);
							sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
							mysql_query(&my_connection,sql_query);

							bzero(sql_query,string_length);
							sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
							mysql_query(&my_connection,sql_query);

							bzero(alarm_content,string_length);
							sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,cache2,host,login_config.remoteuser,sid);
							freesvr_alarm(alarm_content,level);

							if(level==1)
							{
                                write(net,bs_1000,1000);
                                write(net,"\n",1);

								write(fd1,"\n**************************",27);
								write(fd1,"\nforbidden command!\n",20);
								write(fd1,"**************************\n",27);


								if(monitor_fd_tm>0)
								{
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
								}

								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&alarm_length,sizeof(alarm_length));

								write(fd2,"\n**************************",27);
								write(fd2,"\nforbidden command!\n",20);
								write(fd2,"**************************\n",27);

								printf("\r\n\n**************************");
								printf("\r\n*** forbidden command! ***\n");
								printf("\r**************************\n\n");
								bzero(inputcommandline,string_length);
								bzero(commandline,string_length);
								return;
							}
							else if(level==2)
							{
								system("stty sane");


								write(fd1,"\n**************************",27);
								write(fd1,"\nforbidden command!\n",20);
								write(fd1,"**************************\n",27);

								if(monitor_fd_tm>0)
								{
                                    write(monitor_fd_tm,"\n**************************",27);
                                    write(monitor_fd_tm,"\nforbidden command!\n",20);
                                    write(monitor_fd_tm,"**************************\n",27);
								}

								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&alarm_length,sizeof(alarm_length));

								write(fd2,"\n**************************",27);
								write(fd2,"\nforbidden command!\n",20);
								write(fd2,"**************************\n",27);

								sleep(10);
								printf("\n\n**************************");
								printf("\n*** forbidden command! ***\n");
								printf("**************************\n\n");
								exit(0);
							}
							else if(level==4)
							{
								int count=0;
								char tmp_buf[256]={0};
								int echo_length=74;
											
								write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
								write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&echo_length,sizeof(echo_length));
								write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

								while(1)
								{
									while(read(fileno(stdin),tmp_buf,1)<0)
									{
										usleep(100);
									}

									if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
									{
										printf("\n\rBad Password!\n\r");
                                        write(net,bs_1000,1000);
                                        write(net,"\n",1);

										echo_length=89;
										write(fd1,"\nBad Password!\n",15);
										write(fd1,"\n**************************",27);
										write(fd1,"\nforbidden command!\n",20);
										write(fd1,"**************************\n",27);

										if(monitor_fd_tm>0)
										{
											write(monitor_fd_tm,"\nBad Password\n",15);
											write(monitor_fd_tm,"\n**************************",27);
											write(monitor_fd_tm,"\nforbidden command!\n",20);
											write(monitor_fd_tm,"**************************\n",27);
										}


										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1);
										write(fd2,&echo_length,sizeof(echo_length));


										write(fd2,"\nBad Password!\n",15);
										write(fd2,"\n**************************",27);
										write(fd2,"\nforbidden command!\n",20);
										write(fd2,"**************************\n",27);

										printf("\r\n\n**************************");
										printf("\r\n*** forbidden command! ***\n");
										printf("\r**************************\n\n");

										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
									else
									{
										break;
									}
								}

								if(monitor_fd_tm>0)
								{
										echo_length=84;
										write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

										write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1);
										write(fd2,&echo_length,sizeof(echo_length));
										write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
										write(monitor_fd_tm,"\n\rinput password:",17);
										write(fd1,"\n\rinput password:",17);

										echo_length=18;
										gettimeofday(&tv,&tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1); 
										write(fd2,&echo_length,sizeof(echo_length));
										write(fd2,"\n\rinput password:\n",18);

										count=0;
										while(1)
										{
											while(read(monitor_fd_fm,tmp_buf+count,1)<0)
											{
												usleep(100);
											}

											if((*(tmp_buf+count))==8)
											{
												if(count>0)
												{
													char erase_one_char[3]={8,' ',8};
													write(monitor_fd_tm,erase_one_char,3);
													*(tmp_buf+count)=0;
													count--;
												continue;
												}
												else
												{
													continue;
												}
											}
											else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
											{
												*(tmp_buf+count)=0;
												break;
											}

											write(monitor_fd_tm,"*",1);

											count++;
											if(count==255)
											{
												break;
											}

											if(read(fileno(stdin),tmp_buf,1)<0)
											{
												usleep(100);
												continue;
											}
											else
											{
												break;
											}
										}
										if(strcmp(tmp_buf,admin_password)==0)
										{
											printf("OK!\n");
											bzero(inputcommandline,string_length);
											bzero(commandline,string_length);
											return;
										}
										else
										{
											printf("\n\rBad Password!\n\r");
                                            write(net,bs_1000,1000);
                                            write(net,"\n",1);

											write(fd1,"\nBad Password!\n",15);
											write(fd1,"\n**************************",27);
											write(fd1,"\nforbidden command!\n",20);
											write(fd1,"**************************\n",27);

											write(monitor_fd_tm,"\nBad Password\n",15);
											write(monitor_fd_tm,"\n**************************",27);
											write(monitor_fd_tm,"\nforbidden command!\n",20);
											write(monitor_fd_tm,"**************************\n",27);


											echo_length=89;
											gettimeofday (&tv , &tz);
											write(fd2,&tv,sizeof(tv));
                                            write(fd2,"1",1);
											write(fd2,&echo_length,sizeof(echo_length));

											write(fd2,"\nBad Password!\n",15);
											write(fd2,"\n**************************",27);
											write(fd2,"\nforbidden command!\n",20);
											write(fd2,"**************************\n",27);

											printf("\r\n\n**************************");
											printf("\r\n*** forbidden command! ***\n");
											printf("\r**************************\n\n");

											bzero(inputcommandline,string_length);
											bzero(commandline,string_length);
											return;
										}
								}
								else
								{
									write(fileno(stdout),"\n\rinput password:",17);
									write(fd1,"\n\rinput password:\n",18);

									echo_length=18;
									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
									write(fd2,&echo_length,sizeof(echo_length));
									write(fd2,"\n\rinput password:\n",18);
									
										
									while(1)
									{
										while(read(fileno(stdin),tmp_buf+count,1)<0)
										{
											usleep(100);
										}
									
										if((*(tmp_buf+count))==8)
										{
											if(count>0)
											{
												char erase_one_char[3]={8,' ',8};
												write(fileno(stdout),erase_one_char,3);
												*(tmp_buf+count)=0;
												count--;
											continue;
											}
											else
											{
												continue;
											}
										}
										else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
										{
											*(tmp_buf+count)=0;
											break;
										}
								
										write(fileno(stdout),"*",1);
									
										count++;
										if(count==255)
										{
											break;
										}
									}
								
									if(strcmp(tmp_buf,admin_password)==0)
									{
										printf("OK!\n");
										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
									else
									{
										printf("\n\rBad Password!\n\r");
                                        write(net,bs_1000,1000);
                                        write(net,"\n",1);

										write(fd1,"\nBad Password!\n",15);
										write(fd1,"\n**************************",27);
										write(fd1,"\nforbidden command!\n",20);
										write(fd1,"**************************\n",27);
							
										if(monitor_fd_tm>0)
										{
											write(monitor_fd_tm,"\nBad Password\n",15);
											write(monitor_fd_tm,"\n**************************",27);
											write(monitor_fd_tm,"\nforbidden command!\n",20);
											write(monitor_fd_tm,"**************************\n",27);
										}

							
										echo_length=89;
										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1);
										write(fd2,&echo_length,sizeof(echo_length));

										write(fd2,"\nBad Password!\n",15);
										write(fd2,"\n**************************",27);
										write(fd2,"\nforbidden command!\n",20);
										write(fd2,"**************************\n",27);

										printf("\r\n\n**************************");
										printf("\r\n*** forbidden command! ***\n");
										printf("\r**************************\n\n");
							
										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
								}
							}
						}
					}
				}
				count=1;
				i++;
//				printf("\n\rcommandline111=%p\n\r",commandline);
				continue;
			}
			else if((p[i]=='\n' || p[i]=='\r') && count==0 && inputok==1)
			{
				if(invim==0)
				{
					if(strlen(inputcommandline)>0)
					{
						sprintf(cmd,"%s",inputcommandline);
						check_invim(cmd);

						int level=black_or_white;

						for(int j=0;j<black_cmd_num;j++)
						{
							if(black_or_white==0)
							{
								if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
								{
									level = black_cmd_list[j].level + 1;
									break;
								}
							}
							else
							{
								if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
								{
									level = 0;
									break;
								}
							}
						}

                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",inputcommandline);
                            bzero(sql_query,string_length);
                            sprintf(sql_query,"update sessions set user='%s' where sid=%d",inputcommandline,sid);
                            mysql_query(&my_connection,sql_query);
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else
                        {
                            in_login=0;
                        }
                    }

                    for(int i=0;i<pass_prompt_count;i++)
                    {
                        if(pcre_match(commandline,pass_prompt[i])==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }       
                    } 

                    autosu_any(inputcommandline); //auto_su
						if(get_first_prompt>0)
						{
							get_first_prompt--;
						}

						bzero(sql_query,string_length);

						if(encode==1)
						{
							bzero(cache1,string_length);
							g2u(inputcommandline,strlen(inputcommandline),cache1,string_length);
							deal_special_char(cache1);
							sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session)  values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                            gettimeofday (&tv , &tz);
                            write(fd2,&tv,sizeof(tv));
                            write(fd2,"2",1);   //1:content 2:command
                            int cmd_length=0;
                            cmd_length=strlen(cache1);
                            write(fd2,&cmd_length,sizeof(cmd_length));
                            write(fd2,cache1,cmd_length);
						}
						else
						{
							deal_special_char(inputcommandline);
							sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session)  values (NULL,%d,now(),'%s',%d,0)",sid,inputcommandline,level);
                            gettimeofday (&tv , &tz);
                            write(fd2,&tv,sizeof(tv));
                            write(fd2,"2",1);   //1:content 2:command
                            int cmd_length=0;
                            cmd_length=strlen(inputcommandline);
                            write(fd2,&cmd_length,sizeof(cmd_length));
                            write(fd2,inputcommandline,cmd_length);
						}
						//printf("\n\rsql7 = %s\n\r",sql_query);

						mysql_query(&my_connection,sql_query);

						bzero(sql_query,string_length);
						sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
						mysql_query(&my_connection,sql_query);

						bzero(sql_query,string_length);
						sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
						mysql_query(&my_connection,sql_query);

						bzero(alarm_content,string_length);
						sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,inputcommandline,host,login_config.remoteuser,sid);
						freesvr_alarm(alarm_content,level);

						if(level==1)
						{
                            write(net,bs_1000,1000);
                            write(net,"\n",1);

							write(fd1,"\n**************************",27);
							write(fd1,"\nforbidden command!\n",20);
							write(fd1,"**************************\n",27);


							if(monitor_fd_tm>0)
							{
								write(monitor_fd_tm,"\n**************************",27);
								write(monitor_fd_tm,"\nforbidden command!\n",20);
								write(monitor_fd_tm,"**************************\n",27);
							}

							gettimeofday (&tv , &tz);
							write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
							write(fd2,&alarm_length,sizeof(alarm_length));

							write(fd2,"\n**************************",27);
							write(fd2,"\nforbidden command!\n",20);
							write(fd2,"**************************\n",27);

							printf("\r\n\n**************************");
							printf("\r\n*** forbidden command! ***\n");
							printf("\r**************************\n\n");
							bzero(inputcommandline,string_length);
							bzero(commandline,string_length);
							return;
						}
						else if(level==2)
						{   
							system("stty sane");

							write(fd1,"\n**************************",27);
							write(fd1,"\nforbidden command!\n",20);
							write(fd1,"**************************\n",27);


							if(monitor_fd_tm>0)
							{
								write(monitor_fd_tm,"\n**************************",27);
								write(monitor_fd_tm,"\nforbidden command!\n",20);
								write(monitor_fd_tm,"**************************\n",27);
							}

							gettimeofday (&tv , &tz);
							write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
							write(fd2,&alarm_length,sizeof(alarm_length));

							write(fd2,"\n**************************",27);
							write(fd2,"\nforbidden command!\n",20);
							write(fd2,"**************************\n",27);

							sleep(10);
							printf("\n\n**************************");
							printf("\n*** forbidden command! ***\n");
							printf("**************************\n\n");
							exit(0);
						}
						else if(level==4)
						{
							int count=0;
							char tmp_buf[256]={0};
							int echo_length=74;
										
							write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
							write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

							gettimeofday (&tv , &tz);
							write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
							write(fd2,&echo_length,sizeof(echo_length));
							write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

							while(1)
							{
								while(read(fileno(stdin),tmp_buf,1)<0)
								{
									usleep(100);
								}

								if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
								{
									printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

									echo_length=89;
									write(fd1,"\nBad Password!\n",15);
									write(fd1,"\n**************************",27);
									write(fd1,"\nforbidden command!\n",20);
									write(fd1,"**************************\n",27);

									if(monitor_fd_tm>0)
									{
										write(monitor_fd_tm,"\nBad Password\n",15);
										write(monitor_fd_tm,"\n**************************",27);
										write(monitor_fd_tm,"\nforbidden command!\n",20);
										write(monitor_fd_tm,"**************************\n",27);
									}


									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
									write(fd2,&echo_length,sizeof(echo_length));


									write(fd2,"\nBad Password!\n",15);
									write(fd2,"\n**************************",27);
									write(fd2,"\nforbidden command!\n",20);
									write(fd2,"**************************\n",27);

									printf("\r\n\n**************************");
									printf("\r\n*** forbidden command! ***\n");
									printf("\r**************************\n\n");

									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
								else
								{
									break;
								}
							}

							if(monitor_fd_tm>0)
							{
									echo_length=84;
									write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

									write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
									write(fd2,&echo_length,sizeof(echo_length));
									write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
									write(monitor_fd_tm,"\n\rinput password:",17);
									write(fd1,"\n\rinput password:",17);

									echo_length=18;
									gettimeofday(&tv,&tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1); 
									write(fd2,&echo_length,sizeof(echo_length));
									write(fd2,"\n\rinput password:\n",18);

									count=0;
									while(1)
									{
										while(read(monitor_fd_fm,tmp_buf+count,1)<0)
										{
											usleep(100);
										}

										if((*(tmp_buf+count))==8)
										{
											if(count>0)
											{
												char erase_one_char[3]={8,' ',8};
												write(monitor_fd_tm,erase_one_char,3);
												*(tmp_buf+count)=0;
												count--;
											continue;
											}
											else
											{
												continue;
											}
										}
										else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
										{
											*(tmp_buf+count)=0;
											break;
										}

										write(monitor_fd_tm,"*",1);

										count++;
										if(count==255)
										{
											break;
										}

										if(read(fileno(stdin),tmp_buf,1)<0)
										{
											usleep(100);
											continue;
										}
										else
										{
											break;
										}
									}
									if(strcmp(tmp_buf,admin_password)==0)
									{
										printf("OK!\n");
										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
									else
									{
										printf("\n\rBad Password!\n\r");
                                        write(net,bs_1000,1000);
                                        write(net,"\n",1);

										write(fd1,"\nBad Password!\n",15);
										write(fd1,"\n**************************",27);
										write(fd1,"\nforbidden command!\n",20);
										write(fd1,"**************************\n",27);

										write(monitor_fd_tm,"\nBad Password\n",15);
										write(monitor_fd_tm,"\n**************************",27);
										write(monitor_fd_tm,"\nforbidden command!\n",20);
										write(monitor_fd_tm,"**************************\n",27);


										echo_length=89;
										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1);
										write(fd2,&echo_length,sizeof(echo_length));

										write(fd2,"\nBad Password!\n",15);
										write(fd2,"\n**************************",27);
										write(fd2,"\nforbidden command!\n",20);
										write(fd2,"**************************\n",27);

										printf("\r\n\n**************************");
										printf("\r\n*** forbidden command! ***\n");
										printf("\r**************************\n\n");

										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
							}
							else
							{
								write(fileno(stdout),"\n\rinput password:",17);
								write(fd1,"\n\rinput password:\n",18);

								echo_length=18;
								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1);
								write(fd2,&echo_length,sizeof(echo_length));
								write(fd2,"\n\rinput password:\n",18);
								
									
								while(1)
								{
									while(read(fileno(stdin),tmp_buf+count,1)<0)
									{
										usleep(100);
									}
								
									if((*(tmp_buf+count))==8)
									{
										if(count>0)
										{
											char erase_one_char[3]={8,' ',8};
											write(fileno(stdout),erase_one_char,3);
											*(tmp_buf+count)=0;
											count--;
										continue;
										}
										else
										{
											continue;
										}
									}
									else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
									{
										*(tmp_buf+count)=0;
										break;
									}
							
									write(fileno(stdout),"*",1);
								
									count++;
									if(count==255)
									{
										break;
									}
								}
							
								if(strcmp(tmp_buf,admin_password)==0)
								{
									printf("OK!\n");
									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
								else
								{
									printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

									write(fd1,"\nBad Password!\n",15);
									write(fd1,"\n**************************",27);
									write(fd1,"\nforbidden command!\n",20);
									write(fd1,"**************************\n",27);
						
									if(monitor_fd_tm>0)
									{
										write(monitor_fd_tm,"\nBad Password\n",15);
										write(monitor_fd_tm,"\n**************************",27);
										write(monitor_fd_tm,"\nforbidden command!\n",20);
										write(monitor_fd_tm,"**************************\n",27);
									}

						
									echo_length=89;
									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1);
									write(fd2,&echo_length,sizeof(echo_length));

									write(fd2,"\nBad Password!\n",15);
									write(fd2,"\n**************************",27);
									write(fd2,"\nforbidden command!\n",20);
									write(fd2,"**************************\n",27);

									printf("\r\n\n**************************");
									printf("\r\n*** forbidden command! ***\n");
									printf("\r**************************\n\n");
						
									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
							}
						}
					}
					else
					{
//	                    printf("linebuffer2=%s\n",linebuffer);
						if(strlen(linebuffer)>0 && strlen(linebuffer)<128)
						{
							to_get_a_prompt(myprompt,linebuffer,strlen(linebuffer));

							if(get_first_prompt>0)
							{
								bzero(sql_query,string_length);
								sprintf(sql_query,"update devices set first_prompt='%s' where id=%s",myprompt[0],device_id);
			//                  printf("sql_qeury3=%s\n",sql_query);
								mysql_query(&my_connection,sql_query);
								get_first_prompt=0;
							}
						}
					}
                }
				bzero(inputcommandline,string_length);
				bzero(commandline,string_length);
				i++;
//				printf("\n\rcommandline112=%p\n\r",commandline);
				continue;
			}
			else if((p[i]=='\n' || p[i]=='\r') && count==1)
			{
				if(invim==0)
				{
					if(strlen(inputcommandline)>0)
					{
						sprintf(cmd,"%s",inputcommandline);
						check_invim(cmd);

                    int level=black_or_white;

                    for(int j=0;j<black_cmd_num;j++)
                    {
                        if(black_or_white==0)
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = black_cmd_list[j].level + 1;
                                break;
                            }
                        }
                        else
                        {
                            if(pcre_match(cmd,black_cmd_list[j].cmd)==0)
                            {
                                level = 0;
                                break;
                            }
                        }
                    }

                    if(strlen(login_config.remoteuser)==0 && in_login==1)
                    {
                        if(pcre_match(commandline,"ogin:")==0 || pcre_match(commandline,"ername:")==0)
                        {
							sprintf(login_config.remoteuser,"%s",inputcommandline);
                            bzero(sql_query,string_length);
                            sprintf(sql_query,"update sessions set user='%s' where sid=%d",inputcommandline,sid);
                            mysql_query(&my_connection,sql_query);
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else if(pcre_match(commandline,"assword:")==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }
                        else
                        {
                            in_login=0;
                        }
                    }

                    for(int i=0;i<pass_prompt_count;i++)
                    {
                        if(pcre_match(commandline,pass_prompt[i])==0)
                        {
                            bzero(inputcommandline,string_length);
                            bzero(commandline,string_length);
                            return;
                        }       
                    } 

                    autosu_any(inputcommandline); //auto_su

						if(get_first_prompt>0)
						{
							get_first_prompt--;
						}

						bzero(sql_query,string_length);
                        if(encode==1)
                        {
                            bzero(cache1,string_length);
                            g2u(inputcommandline,strlen(inputcommandline),cache1,string_length);
							deal_special_char(cache1);
                            sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session)  values (NULL,%d,now(),'%s',%d,0)",sid,cache1,level);
                            gettimeofday (&tv , &tz);
                            write(fd2,&tv,sizeof(tv));
                            write(fd2,"2",1);   //1:content 2:command
                            int cmd_length=0;
                            cmd_length=strlen(cache1);
                            write(fd2,&cmd_length,sizeof(cmd_length));
                            write(fd2,cache1,cmd_length);
                        }
                        else
                        {
							deal_special_char(inputcommandline);
							sprintf(sql_query,"insert into commands (cid,sid,at,cmd,dangerlevel,jump_session)  values (NULL,%d,now(),'%s',%d,0)",sid,inputcommandline,level);
                            gettimeofday (&tv , &tz);
                            write(fd2,&tv,sizeof(tv));
                            write(fd2,"2",1);   //1:content 2:command
                            int cmd_length=0;
                            cmd_length=strlen(inputcommandline);
                            write(fd2,&cmd_length,sizeof(cmd_length));
                            write(fd2,inputcommandline,cmd_length);
						}
						//printf("\n\rsql8 = %s\n\r",sql_query);
						mysql_query(&my_connection,sql_query);

						bzero(sql_query,string_length);
						sprintf(sql_query,"update sessions set total_cmd=total_cmd+1,end=now(),s_bytes=%lf where sid=%d",(float)g_bytes/1000,sid);
						mysql_query(&my_connection,sql_query);

						bzero(sql_query,string_length);
						sprintf(sql_query,"update sessions set dangerous=%d where sid=%d and dangerous<%d",level,level);
						mysql_query(&my_connection,sql_query);

						bzero(alarm_content,string_length);
						sprintf(alarm_content,"%s run command '%s' on device '%s' as the account '%s' in session %d",member_user,inputcommandline,host,login_config.remoteuser,sid);
						freesvr_alarm(alarm_content,level);

						if(level==1)
						{
                            write(net,bs_1000,1000);
                            write(net,"\n",1);

							write(fd1,"\n**************************",27);
							write(fd1,"\nforbidden command!\n",20);
							write(fd1,"**************************\n",27);


							if(monitor_fd_tm>0)
							{
								write(monitor_fd_tm,"\n**************************",27);
								write(monitor_fd_tm,"\nforbidden command!\n",20);
								write(monitor_fd_tm,"**************************\n",27);
							}

							gettimeofday (&tv , &tz);
							write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
							write(fd2,&alarm_length,sizeof(alarm_length));

							write(fd2,"\n**************************",27);
							write(fd2,"\nforbidden command!\n",20);
							write(fd2,"**************************\n",27);

							printf("\r\n\n**************************");
							printf("\r\n*** forbidden command! ***\n");
							printf("\r**************************\n\n");
							bzero(inputcommandline,string_length);
							bzero(commandline,string_length);
							return;
						}
						else if(level==2)
						{
							system("stty sane");

							write(fd1,"\n**************************",27);
							write(fd1,"\nforbidden command!\n",20);
							write(fd1,"**************************\n",27);


							if(monitor_fd_tm>0)
							{
								write(monitor_fd_tm,"\n**************************",27);
								write(monitor_fd_tm,"\nforbidden command!\n",20);
								write(monitor_fd_tm,"**************************\n",27);
							}

							gettimeofday (&tv , &tz);
							write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
							write(fd2,&alarm_length,sizeof(alarm_length));

							write(fd2,"\n**************************",27);
							write(fd2,"\nforbidden command!\n",20);
							write(fd2,"**************************\n",27);

							sleep(10);
							printf("\n\n**************************");
							printf("\n*** forbidden command! ***\n");
							printf("**************************\n\n");
							exit(0);
						}
						else if(level==4)
						{
							int count=0;
							char tmp_buf[256]={0};
							int echo_length=74;
										
							write(fileno(stdout),"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);
							write(fd1,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

							gettimeofday (&tv , &tz);
							write(fd2,&tv,sizeof(tv));
                            write(fd2,"1",1);
							write(fd2,&echo_length,sizeof(echo_length));
							write(fd2,"\n\r AUTH_TERM: this command needs authority from manager,are you sure?[Y/n]",74);

							while(1)
							{
								while(read(fileno(stdin),tmp_buf,1)<0)
								{
									usleep(100);
								}

								if((*tmp_buf)!='Y' && (*tmp_buf)!='y')
								{
									printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

									echo_length=89;
									write(fd1,"\nBad Password!\n",15);
									write(fd1,"\n**************************",27);
									write(fd1,"\nforbidden command!\n",20);
									write(fd1,"**************************\n",27);

									if(monitor_fd_tm>0)
									{
										write(monitor_fd_tm,"\nBad Password\n",15);
										write(monitor_fd_tm,"\n**************************",27);
										write(monitor_fd_tm,"\nforbidden command!\n",20);
										write(monitor_fd_tm,"**************************\n",27);
									}


									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1); 
									write(fd2,&echo_length,sizeof(echo_length));


									write(fd2,"\nBad Password!\n",15);
									write(fd2,"\n**************************",27);
									write(fd2,"\nforbidden command!\n",20);
									write(fd2,"**************************\n",27);

									printf("\r\n\n**************************");
									printf("\r\n*** forbidden command! ***\n");
									printf("\r**************************\n\n");

									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
								else
								{
									break;
								}
							}

							if(monitor_fd_tm>0)
							{
									echo_length=84;
									write(fileno(stdout),"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);

									write(fd1,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1); 
									write(fd2,&echo_length,sizeof(echo_length));
									write(fd2,"\n\rwaiting confirm until timeout, you can cancel the request by pressing any key...\n\r",84);
									write(monitor_fd_tm,"\n\rinput password:",17);
									write(fd1,"\n\rinput password:",17);

									echo_length=18;
									gettimeofday(&tv,&tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1); 
									write(fd2,&echo_length,sizeof(echo_length));
									write(fd2,"\n\rinput password:\n",18);

									count=0;
									while(1)
									{
										while(read(monitor_fd_fm,tmp_buf+count,1)<0)
										{
											usleep(100);
										}

										if((*(tmp_buf+count))==8)
										{
											if(count>0)
											{
												char erase_one_char[3]={8,' ',8};
												write(monitor_fd_tm,erase_one_char,3);
												*(tmp_buf+count)=0;
												count--;
											continue;
											}
											else
											{
												continue;
											}
										}
										else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
										{
											*(tmp_buf+count)=0;
											break;
										}

										write(monitor_fd_tm,"*",1);

										count++;
										if(count==255)
										{
											break;
										}

										if(read(fileno(stdin),tmp_buf,1)<0)
										{
											usleep(100);
											continue;
										}
										else
										{
											break;
										}
									}
									if(strcmp(tmp_buf,admin_password)==0)
									{
										printf("OK!\n");
										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
									else
									{
										printf("\n\rBad Password!\n\r");
                                        write(net,bs_1000,1000);
                                        write(net,"\n",1);

										write(fd1,"\nBad Password!\n",15);
										write(fd1,"\n**************************",27);
										write(fd1,"\nforbidden command!\n",20);
										write(fd1,"**************************\n",27);

										write(monitor_fd_tm,"\nBad Password\n",15);
										write(monitor_fd_tm,"\n**************************",27);
										write(monitor_fd_tm,"\nforbidden command!\n",20);
										write(monitor_fd_tm,"**************************\n",27);


										echo_length=89;
										gettimeofday (&tv , &tz);
										write(fd2,&tv,sizeof(tv));
                                        write(fd2,"1",1); 
										write(fd2,&echo_length,sizeof(echo_length));

										write(fd2,"\nBad Password!\n",15);
										write(fd2,"\n**************************",27);
										write(fd2,"\nforbidden command!\n",20);
										write(fd2,"**************************\n",27);

										printf("\r\n\n**************************");
										printf("\r\n*** forbidden command! ***\n");
										printf("\r**************************\n\n");

										bzero(inputcommandline,string_length);
										bzero(commandline,string_length);
										return;
									}
							}
							else
							{
								write(fileno(stdout),"\n\rinput password:",17);
								write(fd1,"\n\rinput password:\n",18);

								echo_length=18;
								gettimeofday (&tv , &tz);
								write(fd2,&tv,sizeof(tv));
                                write(fd2,"1",1); 
								write(fd2,&echo_length,sizeof(echo_length));
								write(fd2,"\n\rinput password:\n",18);
								
									
								while(1)
								{
									while(read(fileno(stdin),tmp_buf+count,1)<0)
									{
										usleep(100);
									}
								
									if((*(tmp_buf+count))==8)
									{
										if(count>0)
										{
											char erase_one_char[3]={8,' ',8};
											write(fileno(stdout),erase_one_char,3);
											*(tmp_buf+count)=0;
											count--;
										continue;
										}
										else
										{
											continue;
										}
									}
									else if(*(tmp_buf+count)=='\r' || *(tmp_buf+count)=='\n')
									{
										*(tmp_buf+count)=0;
										break;
									}
							
									write(fileno(stdout),"*",1);
								
									count++;
									if(count==255)
									{
										break;
									}
								}
							
								if(strcmp(tmp_buf,admin_password)==0)
								{
									printf("OK!\n");
									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
								else
								{
									printf("\n\rBad Password!\n\r");
                                    write(net,bs_1000,1000);
                                    write(net,"\n",1);

									write(fd1,"\nBad Password!\n",15);
									write(fd1,"\n**************************",27);
									write(fd1,"\nforbidden command!\n",20);
									write(fd1,"**************************\n",27);
						
									if(monitor_fd_tm>0)
									{
										write(monitor_fd_tm,"\nBad Password\n",15);
										write(monitor_fd_tm,"\n**************************",27);
										write(monitor_fd_tm,"\nforbidden command!\n",20);
										write(monitor_fd_tm,"**************************\n",27);
									}

						
									echo_length=89;
									gettimeofday (&tv , &tz);
									write(fd2,&tv,sizeof(tv));
                                    write(fd2,"1",1); 
									write(fd2,&echo_length,sizeof(echo_length));

									write(fd2,"\nBad Password!\n",15);
									write(fd2,"\n**************************",27);
									write(fd2,"\nforbidden command!\n",20);
									write(fd2,"**************************\n",27);

									printf("\r\n\n**************************");
									printf("\r\n*** forbidden command! ***\n");
									printf("\r**************************\n\n");
						
									bzero(inputcommandline,string_length);
									bzero(commandline,string_length);
									return;
								}
							}
						}
					}
					else
					{
//	                    printf("linebuffer3=%s\n",linebuffer);
						if(strlen(linebuffer)>0 && strlen(linebuffer)<128)
						{
							to_get_a_prompt(myprompt,linebuffer,strlen(linebuffer));

							if(get_first_prompt>0)
							{
								bzero(sql_query,string_length);
								sprintf(sql_query,"update devices set first_prompt='%s' where id=%s",myprompt[0],device_id);
			//                  printf("sql_qeury3=%s\n",sql_query);
								mysql_query(&my_connection,sql_query);
								get_first_prompt=0;
							}
						}
					}
                }
				bzero(inputcommandline,string_length);
				bzero(commandline,string_length);
				i++;
//				printf("\n\rcommandline120=%p\n\r",commandline);
				continue;
			}
			memcpy(inputcommandline+strlen(inputcommandline),p+i,1);

			if(strlen(inputcommandline)>(string_length-5000))
			{
				bzero(inputcommandline,string_length);
			}
			i++;
		}
//		printf("\n\rcommandline121=%p\n\r",commandline);
		return;
	}
//	printf("\n\rcommandline6=%p\n\r",commandline);
	memcpy(inputcommandline+strlen(inputcommandline),buff,n);

	if(strlen(inputcommandline)>(string_length-5000))
	{
		bzero(inputcommandline,string_length);
	}
}


void RakSleep(unsigned int ms)
{
	pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;
	struct timespec timeToWait;
	struct timeval now;
	int rt;


	gettimeofday(&now,NULL);


	long seconds = ms/1000;
	long nanoseconds = (ms - seconds * 1000) * 1000000;
	timeToWait.tv_sec = now.tv_sec + seconds;
	timeToWait.tv_nsec = now.tv_usec*1000 + nanoseconds;
	
	if (timeToWait.tv_nsec >= 1000000000)
	{
	       timeToWait.tv_nsec -= 1000000000;
	       timeToWait.tv_sec++;
	}


	pthread_mutex_lock(&fakeMutex);
	rt = pthread_cond_timedwait(&fakeCond, &fakeMutex, &timeToWait);
	pthread_mutex_unlock(&fakeMutex);
}

void * autologin_thread_callback(void * arg)
{
	extern int telnet_wait_time;
	extern int freesvr_autologin_thread;
	if(freesvr_autologin==1)
	{
//		sleep(5);
		RakSleep(telnet_wait_time);
		int re;
		
		extern int freesvr_autologin; 
		extern char * su_command;
		extern struct auto_login_config login_config;
		extern int freesvr_autosu;
		extern char * enable_password;
	

		re=strlen(login_config.remoteuser);
		if(re>0)
		{
			in_login=0;
			write(net,login_config.remoteuser,re);
			write(net,"\r",1);
			fsync(net);
			RakSleep(telnet_wait_time);
		}
		
			re=strlen(login_config.password);
		if(re>0)
		{
			write(net,login_config.password,re);
			write(net,"\r",1);
			fsync(net);
			RakSleep(telnet_wait_time);
		}

		
		if(freesvr_autosu>0)
		{
            re=strlen(enable_password);
			if(re>0)
			{   
				write(net,su_command,strlen(su_command));
				write(net,"\r",1);
				fsync(net);
			}
			RakSleep(telnet_wait_time);

			write(net,enable_password,re);
			write(net,"\r",1);
			fsync(net);

			RakSleep(telnet_wait_time);
		}
	}
	freesvr_autologin_thread=0;
	pthread_exit(0);
}


void * jumplogin_thread_callback(void * arg)
{
	extern int freesvr_autologin_thread;
	extern char jump_command[100];
	extern char jump_username[100];
	extern char jump_password[100];
	freesvr_autologin_thread=1;
	RakSleep(2000);
	int re = strlen(jump_command);
	if(strstr(jump_command,"telnet"))
	{
        write(net,jump_command,re);
        write(net,"\n",1);
        fsync(net);

        //writelogfile2(jump_command,re);
        //writelogfile2("\n",1);

		RakSleep(2000);
		re =strlen(jump_username);
		if(re>0)
		{
			write(net,jump_username,re);
			write(net,"\n",1);
			fsync(net);

		//	writelogfile2(jump_username,re);
		//	writelogfile2("\n",1);
		}

		re = strlen(jump_password);
        write(net,jump_password,re);
        write(net,"\n",1);
        fsync(net);
	}
	else if(strstr(jump_command,"ssh"))
	{
        write(net,jump_command,re);
        write(net,"\n",1);
        fsync(net);

      //  writelogfile2(jump_command,re);
      //  writelogfile2("\n",1);

        RakSleep(2000);
        
		re = strlen(jump_password);
        write(net,jump_password,re);
        write(net,"\n",1);
        fsync(net);
	}
	freesvr_autologin_thread=0;
	pthread_exit(0);
}


int create_autologin_thread()
{
    pthread_t thread;
    int ret = pthread_create(&thread,NULL,autologin_thread_callback,NULL);
	if(ret<0)
	{
		printf("auto error,will exit...\n");
		exit(0);
	}
}

int create_jumplogin_thread()
{
	pthread_t thread;
	extern char jump_command[];
	if(strstr(jump_command,"Non"))
	{
		return;
	}
	int ret = pthread_create(&thread,NULL,jumplogin_thread_callback,NULL);
	if(ret<0)
	{
		printf("jump error,will exit...\n");
		exit(0);
	}
}

int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
{
	iconv_t cd;
	int rc;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset,from_charset);
	if (cd==0) return -1;
	memset(outbuf,0,outlen);
	if (iconv(cd,pin,&inlen,pout,&outlen)==-1) return -1;
	iconv_close(cd);
	return 0;
}

int u2g(char *inbuf,int inlen,char *outbuf,int outlen)
{
	return code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
}

int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	return code_convert("gb2312","utf-8",inbuf,inlen,outbuf,outlen);
}

void deal_special_char(char * sql)
{
    int length=strlen(sql);
    int i = 0;
    while(i<length+1)
    {
        if(sql[i]=='\\' || sql[i]=='\'' || sql[i]=='(' || sql[i]==')')
        {
            for(int j=length;j>i-1;j--)
            {
                sql[j+1]=sql[j];
            }
            length++;
            sql[i]='\\';
            i++;
        }
        i++;
    }
}
