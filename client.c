#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
# include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#define EVENT_BASE_SOCKET 0

static const int PORT = 8888;
const char* server_ip = "0.0.0.0";
char user[100]={0};
int status = 0;//1:o 2:x
int player2 = 0;
int board[3][3]={{0,0,0},{0,0,0},{0,0,0}};
int chi_cnt = 0;
int turn = 0; //1:ok 2:not
/*事件处理回调函数*/
void print_board()
{
    for(int i = 0 ; i <= 3;i++)
    {   
        for(int j = 0; j <= 3; j++)
        {
            if(board[i][j] == 1)
                printf("o");
            if(board[i][j] == 2)
                printf("x");
            if(board[i][j] == 0)
                printf(" ");
            if(j!= 2) printf("|");
            else printf("\n");
        }
        if(i != 2) printf("------\n");
    }
}
void read_cb_sorry()
{
    printf("piease wait for a minute\n");
    exit(1);
}
void read_cb_hi(char *buf)
{
    char *ptr = buf+3;
	strcpy(user,ptr);
	printf("server:\n%s\r\n",buf);
}
void read_cb_logout()
{
    memset(user,'\0',sizeof(char));
    printf("Bye Bye\n");
}
void read_cb_conn_from(char *buf,struct bufferevent* bev,size_t len)
{
    printf("server: %s\r\n",buf);
    char ans, *ptr, str[100] = "ans:\0";
    if(buf[len-1]=='\n')
        buf[len-1] = '\0';
    ptr = buf + 22;
    scanf("%c",&ans);
    str[4] = ans;
    str[5] = '\0';
    strcat(str,ptr);
    bufferevent_write(bev, str,strlen(str));
}
void read_cb_add(char *buf)
{
    int x = atoi(buf + 4), y = atoi(buf + 6);
    turn = 1;
    board[x-1][y-1] = player2;
    printf("%s\n",buf);
    if(strstr(buf,"lose")!= NULL)
    {
        print_board();
        printf("\nYou lose\nleave game\n");
        player2 = 0;
        status = 0;
        turn = 0;
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                board[i][j] = 0;
        chi_cnt = 0;
        return;
    }
    else if(strstr(buf,"No one win")!= NULL)
    {
        print_board();
        player2 = 0;
        status = 0;
        turn = 0;
        for(int i = 0 ; i < 3; i++)
            for(int j = 0 ; j < 3; j++)
                board[i][j] = 0;
        chi_cnt = 0;
        return;
    }
    chi_cnt++;
    printf("Your turn\n");
    print_board();
}
void read_cb_else(char *buf)
{
    if(!strncmp(buf,"start",5))
    {
        if(strlen(buf) < 8)
        {
            status = 2;
            player2 = 1;
            turn = 2;
        }
        else
        {
            status = 1;
            player2 = 2;
            turn = 1;
        }
    }
    printf("server:\n%s\r\n",buf);
}
int check_win()
{
    int my_pos[10]={0};
    int check_table[8][3] ={{1,2,3},{1,4,7},{1,5,9},{2,5,8},{3,5,7},{3,6,9},{4,5,6},{7,8,9}};
    int id = 0;
    for(int i = 0 ; i < 3; i++)
        for(int j= 0; j < 3; j++)
        {
            id++;
            if(board[i][j] == status) 
                my_pos[id] = 1;
        }
    for(int i = 0; i < 8; i++)
    {
        int fst = check_table[i][0], sec = check_table[i][1], thr = check_table[i][2];
        if(my_pos[fst] == 1&&my_pos[sec] == 1&&my_pos[thr] == 1)
            return 1;
    }
    return 0;
}
void event_cb(struct bufferevent* bev,short events,void* ptr)
{
    if(events & BEV_EVENT_CONNECTED)//连接建立成功
    {   
        printf("connected to server successed!\n\n");
        printf("insert format: acc:[xxx],pwd:[xxx]\n\n");
        printf("------- you have to login first ------\n\n");
        printf("show ---> check player online\n");
        printf("logout ----> logout\n");
        printf("conn:xxx --- conntect with palyer\n");
        printf("add:x-x ----> insert postition\n");
        printf("--------------------------------------\n\n");
    }    
    else if(events & BEV_EVENT_ERROR)
        printf("connect error happened!\n");
}
void read_cb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer* input = bufferevent_get_input(bev);
	size_t len = 0;
	len = evbuffer_get_length(input);
    char *buf;
    buf = (char *)malloc(sizeof(char)*len);
    if(buf == NULL) return;
    evbuffer_remove(input,buf,len);
    if(!strcmp(buf,"sorry\0")) read_cb_sorry();

    else if(!strncmp(buf,"hi ",3)) read_cb_hi(buf);

    else if(!strncmp(buf,"logout",6)) read_cb_logout();
    else if(!strncmp(buf,"Connect message from :",22)) read_cb_conn_from(buf,bev,len);
    else if(!strncmp(buf,"add:",4))  read_cb_add(buf);
    else read_cb_else(buf);
}
int tcp_connect_server(const char* server_ip,int port)
{
    struct sockaddr_in server_addr;
    int status = -1;
    int sockfd;

    server_addr.sin_family = AF_INET;  
    server_addr.sin_port = htons(port);  
    status = inet_aton(server_ip, &server_addr.sin_addr);
    if(0 == status)
    {
        errno = EINVAL;
        return -1;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    if( sockfd == -1 )  
        return sockfd;  


    status = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr) );  
    if( status == -1 )  
    {
        close(sockfd);
        return -1;  
    }  
    evutil_make_socket_nonblocking(sockfd);
    return sockfd;
}
void cmd_msg_cb(int fd, short events, void *arg)
{
    char msg[1024];
    int ret = read(fd,msg,sizeof(msg));
    if(ret < 0 )
    {
        perror("read failed");
        exit(1);
    }
    struct bufferevent* bev = (struct bufferevent*)arg;
    msg[ret] = '\0';
    if(strncmp(msg,"add:",4) && status != 0)
    {
        printf("u are in the game\n");
        return;
    }
    if(!strncmp(msg,"acc:",4))
    {
        if(strlen(user))
        {
            printf("piease logout first");
            return;
        }
    }
    if(!strncmp(msg,"logout",6))
    {
        if(!strlen(user))
        {
            printf("u r not login\n");
            return;
        }    
    }
    if(!strncmp(msg,"add:",4))
    {
        int flag = 0;
        int x = atoi(msg + 4),y = atoi(msg + 6);
        if(turn != 1)
        {
                printf("wrong format\n");
                return;
        }
        if(x < 1 || x > 3 || y < 1 || y > 3)
        {
                printf("Not a valid position\n");
                return;
        }
        else if(board[x-1][y-1] != 0)
        {
                printf("The position has been occupied\n");
                return;
        }
        else
        {
            board[x-1][y-1] = status;
            turn = 2;
            chi_cnt++;
            if( check_win() )
            {
                if(msg[strlen(msg)- 1] == '\n')
                    msg[strlen(msg) - 1] = '\0';
                flag = 1;
                print_board();
                printf("You win\n");
                player2 = 0;
                status = 0;
                turn = 0;
                for(int i = 0 ; i <3; i++)
                    for(int j = 0 ; j <3 ; j++)
                        board[i][j] = 0;
                chi_cnt = 0;
			    strcat(msg,"You lose ! over\n");
            }
            else if(chi_cnt == 9)
            {
                if(msg[strlen(msg)-1] == '\n')
                    msg[strlen(msg)-1] = '\0';
                flag=1;
			    print_board();
			    printf("No one win the game!\n");
			    strcat(msg,"No one win the game!over\n");
			    player2 = 0;
			    status = 0;
			    turn = 0;
			    for(int i=0;i<3;i++)
				    for(int j=0;j<3;j++)
					    board[i][j] = 0;
			    chi_cnt = 0;
            }
        }
        if(!flag) print_board();
    }
    bufferevent_write(bev,msg,strlen(msg));
}
int main()
{
   struct event_base* base = NULL;
    struct sockaddr_in server_addr;
    struct bufferevent *bev = NULL;
    int status;
    int sockfd;
    //申请event_base对象
    base = event_base_new();
    if(!base)
        printf("event_base_new() function called error!");

    //初始化server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port= htons(PORT);
    status = inet_aton(server_ip, &server_addr.sin_addr);
#if EVENT_BASE_SOCKET

    sockfd = tcp_connect_server(server_ip,PORT);
    bev = bufferevent_socket_new(base, sockfd,BEV_OPT_CLOSE_ON_FREE);
#else
    bev = bufferevent_socket_new(base, -1,BEV_OPT_CLOSE_ON_FREE);//第二个参数传-1,表示以后设置文件描述符

    //调用bufferevent_socket_connect函数
    bufferevent_socket_connect(bev,(struct sockaddr*)&server_addr,sizeof(server_addr));
#endif
    //监听终端的输入事件
    struct event* ev_cmd = event_new(base, STDIN_FILENO,EV_READ | EV_PERSIST, cmd_msg_cb, (void*)bev);  

    //添加终端输入事件
    event_add(ev_cmd, NULL);

    //设置bufferevent各回调函数
    bufferevent_setcb(bev,read_cb, NULL, event_cb, (void*)NULL);  

    //启用读取或者写入事件
    bufferevent_enable(bev, EV_READ | EV_PERSIST);

    //开始事件管理器循环
    event_base_dispatch(base);

    event_base_free(base);
    return 0;
}