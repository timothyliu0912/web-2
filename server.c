#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int client_cnt = 0;
int client_status[4] = {0,0,0,0}; // 0 = free 1 = handle 2 = busy
int client_id[4] = {-1,-1,-1,-1};
char client_account[4][4] = {"aaa\0","bbb\0","ccc\0","ddd\0"};
char client_password[4][4] = {"aaa\0","bbb\0","ccc\0","ddd\0"};
int b_info[2][4] = {{0,0,0,0},{0,0,0,0}};
void show(evutil_socket_t fd, struct bufferevent* bev)
{
    int login_flag = 0;
    for(int i = 0; i < 4; i++)
        if( client_id[i] == (int)fd )
            login_flag = 1;
    if(login_flag)
    {
        char list[1024];
        memset(list,'\0',sizeof(char));
        for(int i = 0 ; i < 4; i++)
        {
            if(client_id[i] != -1)
            {
                char stat[2];
                memset(stat,0,sizeof(char)); 
                stat[0] = client_status[i] + '0';
                stat[1] = '\0';
                strcat(list,stat);
                strcat(list," ");
                strcat(list,client_account[i]);
                strcat(list,"\n");
            }
        }
        bufferevent_write(bev,list,strlen(list));
    }
    else bufferevent_write(bev,"login first",12);   
}
void acc(char *buf,evutil_socket_t fd, struct bufferevent* bev)
{
    if(client_cnt < 4)
    {
        char *ptr = buf + 4;
        int flag = 0, len = 0;
        char user_account[100],user_password[100];
        while(*ptr != '\n')
        {
            if(!strncmp(ptr,",pwd:",5))
            {
                user_account[len] = '\0';
                ptr = ptr + 5;
                len = 0;
                flag = 1;
            }
            if(flag)
                user_password[len] = *ptr;
            else
                user_account[len] = *ptr;
            len++;
            ptr++;
        }
        user_password[len] = '\0';
        int flag2 = -1;
        for(int i = 0 ; i < 4 ;i++)
            if(client_id[i] == -1)
                if(!strcmp(client_account[i], user_account) && !strcmp(client_password[i], user_password))
                {
                    client_id[i] = (int) fd;
                    flag2 = i;
                    client_cnt++;
                    break;
                }
        if(flag2 != -1)
        {
            char str[10] = "hi ";
            bufferevent_write(bev,strcat(str,client_account[flag2]), 7);
        }
        else  bufferevent_write(bev,"failed",7);
    }        
    else   bufferevent_write(bev,"sorry\0",5);
}  
void conn(char *buf,evutil_socket_t fd, struct bufferevent* bev)
{
    if(buf[strlen(buf)-1]=='\n')
        buf[strlen(buf)-1] ='\0';
    char *ptr;
    ptr = buf+5;
    int from  = -1, tar = -1;
    for(int i = 0; i < 4; i++)
        if(fd == client_id[i])
            from = i;
    if(from == -1) 
        bufferevent_write(bev,"Please login first\0",20);
    else
    {
        int flag = -1;
        for(int i= 0 ; i < 4; i++)
            if( client_id[i] != -1 && !strcmp(ptr,client_account[i]) )
            {
                tar = client_id[i];
                if(client_status[i] != 0)
                    flag = i;
            }
        if(flag != -1) bufferevent_write(bev, "Player is busy",15);
        else if (tar != -1)
        {
            char msg[100] = "Connect message from :";
            int msglen = strlen(client_account[from]) + strlen(msg);
            client_status[flag] = 1;
            client_status[from] = 1;
            write(tar,strcat(msg,client_account[from]),msglen);
        }
        else bufferevent_write(bev,"Player is not exist\0",21);   
    }  
}
void ans(char *buf,evutil_socket_t fd, struct bufferevent* bev)
{
    if(buf[strlen(buf)-1]=='\n')
			buf[strlen(buf)-1] = '\0';
    char *ptr = buf+5;
	char ans = buf[4];
	int b_id = -1;
	int target,idx1,idx2;
	for(int i=0;i<4;i++)
    {
		if(strcmp(ptr,client_account[i])==0)
        {
			target = client_id[i];
			idx1 = i;
		}
		if(fd == client_id[i])
			idx2 = i;
	} 	
    if(ans == 'y')
    {
        if(b_info[0][0] == 0)  b_id = 0;
        else b_id = 1;
        client_status[idx1] = 2;
        client_status[idx2] = 2;
        b_info[b_id][0] = 1;
        b_info[b_id][1] = target;
        b_info[b_id][2] = target;
        b_info[b_id][3] = fd;
        write(target,"start\nit's your turn\n",22);
        bufferevent_write(bev,"start\n\0",7);
    }
    else if(ans == 'n')
    {
        client_status[idx1] = 0;
        client_status[idx2] = 0;
        write(target,"invite reject",13);
    }
}
void add(char *buf, evutil_socket_t fd, struct bufferevent* bev)
{
    int b_id;
    if(fd == b_info[0][2] || fd == b_info[0][3])
        b_id = 0;
    else b_id = 1;
    if(b_info[b_id][1] != fd)
        bufferevent_write(bev,"not ypur turn",13);
    else
    {
        int b_me = b_info[b_id][2];
        b_info[b_id][1] = b_info[b_id][3];
        if(b_me != fd)
        {
            b_me = b_info[b_id][3];
            b_info[b_id][1] = b_info[b_id][2];
        }
        write(b_info[b_id][1],buf,strlen(buf));
    }
    if(strstr(buf,"over")!= NULL)
    {
        int p1 = b_info[b_id][2], p2 = b_info[b_id][3];
        for(int i = 0; i < 4; i++)
            b_info[b_id][i] = 0;
        for(int i = 0 ; i < 4; i++)
            if(client_id[i] == p1 || client_id[i] == p2)
                client_status[i] = 0;
    }
}
void logout(evutil_socket_t fd, struct bufferevent* bev)
{
    for(int i = 0 ; i < 4; i++)
    {
        if(client_id[i] ==fd)
        {
            client_id[i] = -1;
            client_cnt --;
        }
    }
    bufferevent_write(bev,"logout",7);
}
static void server_on_read(struct bufferevent* bev,void* arg)
{
    struct evbuffer* input = bufferevent_get_input(bev);
    size_t len = 0;
    len = evbuffer_get_length(input);
    char* buf;
    buf = (char*)malloc(sizeof(char)*len);
    if(NULL==buf) return;
    evbuffer_remove(input,buf,len);
    evutil_socket_t fd = bufferevent_getfd(bev);
    printf("%d %s",fd,buf);
    if(!strncmp(buf,"show",4)) show(fd,bev);
    if(!strncmp("acc:",buf,4)) acc(buf,fd,bev);
    if(!strncmp("conn:",buf,5)) conn(buf,fd,bev);
    if(!strncmp(buf,"ans:",4))  ans(buf,fd,bev);
    if(!strncmp(buf,"add:",4)) add(buf,fd,bev);
    if(!strncmp(buf,"logout",6)) logout(fd,bev);
    buf = NULL;
    free(buf);
    return;
}
void server_on_accept(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr *server, int socklen, void *arg)
{
    printf("accept a client : %d\n", fd);
    if(client_cnt < 4)
    {
        int enable  = 1;
        if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, ( void *)&enable, sizeof(enable)) < 0 )
            printf("Consensus-side: TCP_NODELAY SETTING ERROR!\n");
        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *new_buff_event = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(new_buff_event, server_on_read, NULL, NULL, NULL);
        bufferevent_enable(new_buff_event, EV_READ|EV_WRITE);
    }
    else
    {
        char sorr[100] = "sorry fail\0";
        write((int)fd,sorr,strlen(sorr));
    }
    return;
}
int main()
{
    int port = 8888;

    struct sockaddr_in server_in;

    memset(&server_in, 0,sizeof(server_in));
    server_in.sin_family = AF_INET;
    server_in.sin_addr.s_addr = htonl(INADDR_ANY);
    server_in.sin_port = htons(port);

    struct event_base *base = event_base_new();
    struct evconnlistener *listener = evconnlistener_new_bind(base, server_on_accept, NULL, LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (struct sockaddr*)&server_in, sizeof(server_in));
    printf("Server online\n");
    if (!listener) 
    {
        printf("Could not create a listener!\n");
        return 1;
    }
    event_base_dispatch(base);
    evconnlistener_free(listener);
    event_base_free(base);

    return 0;
}