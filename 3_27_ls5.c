#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<sys/stat.h>
#include<unistd.h>
#include<sys/types.h>
#include<limits.h>
#include<dirent.h>
#include<grp.h>
#include<pwd.h>
#include<errno.h>
#include<stdlib.h>
#include<stding.h>

#define PARAM_NONE 0        //由于后面要用到位运算，所以介绍一下，当没有参数的时候flag=0；然后依次是-a,-l,-R,-r，定义为1,2,4,8
#define PARAM_A    1        //刚好就是二进制中的1，10，100，1000。这样方便为运算中的|和&，比如同时有a和r参数，那么flag就为1001.
#define PARAM_L    2        //当要判断是否含有这两个参数中的一个时(比如r参数)就可以用flag&PARAM_r,如果为0的话就没有r这个参数。
#define PARAM_R    4        //其他的也类似
#define PARAM_r    8    
#define PARAM_I    16
#define PARAM_T    32
#define PARAM_S    64

#define MAXROWLEN  80    //我定义的一行的最大长度

char PATH[PATH_MAX+1];      //用于存储路径
int flag;

int g_leave_len = MAXROWLEN;  //初始时一行剩余的长度就是一行的最大长度
int g_maxlen;   //最大文件名的长度

void my_err(const char* err_string,int line);
void display_dir(char* path);

void my_err(const char* err_string,int line)
{
    fprintf(stderr,"line:%d",__LINE__);
    perror(err_string);
    exit(1);
}

//这个颜色打印就自行百度吧
void cprint(char* name,mode_t st_mode)
{

    if(S_ISLNK(st_mode))   //链接文件
        printf("\033[1;36m%-*s\033[0m",g_maxlen,name);
    else if(S_ISDIR(st_mode)&&(st_mode&000777)==0777)   //满权限的目录
        printf("\033[1;34;42m%-*s  \033[0m",g_maxlen,name);
    else if(S_ISDIR(st_mode))  //目录
                printf("\033[1;34m%-*s  \033[0m",g_maxlen,name);
    else if(st_mode&S_IXUSR||st_mode&S_IXGRP||st_mode&S_IXOTH) //可执行文件
        printf("\033[1;32m%-*s  \033[0m",g_maxlen,name);
    else   //其他文件
        printf("%*s  ",g_maxlen,name);
}

//这个函数建议去看一本书:linux编程实践,里面的实现ls的章节中有详解讲解这块内容
void  display_attribute(char* name)  //-l参数时按照相应格式打印
{
    struct stat buf;
    char buff_time[32];
    struct passwd* psd;  //从该结构体接收文件所有者用户名
    struct group* grp;   //获取组名
    if(lstat(name,&buf)==-1)
    {
        my_err("stat",__LINE__);
    }
    if(S_ISLNK(buf.st_mode))
        printf("l");
    else if(S_ISREG(buf.st_mode))
        printf("-");
    else if(S_ISDIR(buf.st_mode))
        printf("d");
    else if(S_ISCHR(buf.st_mode))
        printf("c");
    else if(S_ISBLK(buf.st_mode))
        printf("b");
    else if(S_ISFIFO(buf.st_mode))
        printf("f");
    else if(S_ISSOCK(buf.st_mode))
        printf("s");
    //获取打印文件所有者权限
    if(buf.st_mode&S_IRUSR)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWUSR)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXUSR)
        printf("x");
    else
        printf("-");

    //所有组权限
    if(buf.st_mode&S_IRGRP)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWGRP)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXGRP)
        printf("x");
    else
        printf("-");

    //其他人权限
    if(buf.st_mode&S_IROTH)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWOTH)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXOTH)
        printf("x");
    else
        printf("-");

//  printf(" ");
    //根据uid和gid获取文件所有者的用户名和组名
    psd=getpwuid(buf.st_uid);
    grp=getgrgid(buf.st_gid);
    printf("%4d ",buf.st_nlink);  //链接数
    printf("%-8s ",psd->pw_name);
    printf("%-8s ",grp->gr_name);

    printf("%6d",buf.st_size);
    strcpy(buff_time,ctime(&buf.st_mtime));
    buff_time[strlen(buff_time)-1]='\0'; //buff_time自带换行，因此要去掉后面的换行符
    printf("  %s  ",buff_time);
    cprint(name,buf.st_mode);
    printf("\n");
}
void  displayR_attribute(char* name)  //当l和R都有时，先调用display_attribute打印，然后该函数负责递归
{
    struct stat buf;

    if(lstat(name,&buf)==-1)
    {
        my_err("stat",__LINE__);
    }
    if(S_ISDIR(buf.st_mode))
    {
            display_dir(name);
            free(name);
            char* p=PATH;
            while(*++p);
            while(*--p!='/');
            *p='\0';             //每次递归完成之后将原来的路径回退至递归前
            chdir("..");         //跳转到当前目录的上一层目录
            return;
    }
}
void display_single(char* name)   //打印文件
{
    int len ;
    struct stat buf;
    if(lstat(name,&buf)==-1)
    {
        return;
    }

    if(g_leave_len<g_maxlen)   //最大文件名长度大于当前行剩余长度
    {                           //另起一行,剩余长度变回一行最大长度
        printf("\n");
        g_leave_len=MAXROWLEN;
    }

    cprint(name,buf.st_mode);  //根据文件的不同类型显示不同颜色
    g_leave_len=g_leave_len-(g_maxlen+2);

}

void displayR_single(char* name)  //当有-R参数时打印文件名并调用display_dir
{
    int len ;
    struct stat buf;
    if(lstat(name,&buf)==-1)
    {
        return;
    }
    if(S_ISDIR(buf.st_mode))    
    {
            printf("\n");

            g_leave_len=MAXROWLEN;
    //注意!这里是全代码最精华的地方,-R递归就在次,多看一遍吧
    //调用顺序:display_dir  ->  display  ->displayR_single ->  display_dir ...
            display_dir(name);
            free(name);   //将之前filenames[i]中的空间释放
            char*p=PATH;

            //先把指针p移到PATH最后,知道/0停止
            //接着指针p往回移动,遇到/,返回上级目录
            while(*++p);   
            while(*--p!='/');
            *p='\0';
            chdir("..");  //返回上层目录
    }
}

void display(char **name ,int count)  //根据flag去调用不同的函数
{
    switch(flag)
    {
        int i;
        case PARAM_r:  //注意,这里的-r不用管,因为如果参数有r,在display_dir函数中,就已经把文件名逆序了
        case PARAM_NONE:
            for(i=0;i<count;i++)
            {
                if(name[i][0]!='.') //排除.., .,以及隐藏文件 , 和带-a参数区分开
                    display_single(name[i]);
            }
            break;
        case PARAM_r+PARAM_A:  //不用管,道理跟前面PARAM_r一样
        case PARAM_A:  //直接打,包括文件名.和..
            for(i=0;i<count;i++)
            {
                display_single(name[i]);  //单打文件名
            }
            break;
        case PARAM_r+PARAM_L:
        case PARAM_L:
        for(i=0;i<count;i++)
        {
            if(name[i][0]!='.')
            {
                display_attribute(name[i]);//打印详细信息
            }
        }
            break;
        case PARAM_R+PARAM_r:
        case PARAM_R:
            for(i=0;i<count;i++)
            {
                if(name[i][0]!='.')
                {
                    display_single(name[i]);//单打文件名
                }
            }
            for(i=0;i<count;i++)
            {
                if(name[i][0]!='.')   //排除 "."和".."两个目录，防止死循环，下同
                {
                    displayR_single(name[i]);
                }
            }
            break;
    //后面,就都是一系列拍立组合啦~
        case PARAM_L+PARAM_r+PARAM_R:
        case PARAM_R+PARAM_L:
            for(i=0;i<count;i++)
            {
                if(name[i][0]!='.')
                {
                    display_attribute(name[i]);
                }
            }
            for(i=0;i<count;i++)
            {
                if(name[i][0]!='.')
                {
                    displayR_attribute(name[i]);
                }
            }
            break;
        case PARAM_A+PARAM_r+PARAM_R:
        case PARAM_R+PARAM_A:
            for(i=0;i<count;i++)
            {
                    display_single(name[i]);
            }
            for(i=0;i<count;i++)
            {
                if(name[i][0]!='.')
                {
                    displayR_single(name[i]);
                }
            }
            break;

        case PARAM_A+PARAM_L+PARAM_r:
        case PARAM_A+PARAM_L:
            for(i=0;i<count;i++)
            {
                display_attribute(name[i]);
            }
            break;
        case PARAM_A+PARAM_L+PARAM_R+PARAM_r:
        case PARAM_A+PARAM_L+PARAM_R:
            for(i=0;i<count;i++)
            {
                display_attribute(name[i]);
            }
            for(i=0;i<count;i++)
            {
                if(name[i][0]!='.')
                {
                    displayR_attribute(name[i]);
                }
            }
            break;
        default:
            break;
    }
}


void display_dir(char* path)      //该函数用以对目录进行处理
{
    DIR* dir;                     //接受opendir返回的文件描述符
    struct dirent* ptr;           //接受readdir返回的结构体
    int count=0;
    //char filenames[300][PATH_MAX+1],temp[PATH_MAX+1];  这里是被优化掉的代码，由于函数中定义的变量
    //是在栈上分配空间，因此当多次调用的时候会十分消耗栈上的空间，最终导致栈溢出，在linux上的表现就是核心已转储
    //并且有的目录中的文件数远远大于300

    if((flag&PARAM_R)!=0)               //作为一个强迫症，绝不允许格式上出问题
    {
        int len =strlen(PATH);
        if(len>0)
        {
            if(PATH[len-1]=='/')
                PATH[len-1]='\0';
        }
        if(path[0]=='.'||path[0]=='/')
        {
            strcat(PATH,path);
        }
        else
        {
            strcat(PATH,"/");
            strcat(PATH,path);
        }
        printf("%s:\n",PATH);
    }
    //获取文件数和最长文件名长度 
    dir = opendir(path);
    if(dir==NULL)
        my_err("opendir",__LINE__);
    g_maxlen=0;
    while((ptr=readdir(dir))!=NULL)
    {
        if(g_maxlen<strlen(ptr->d_name))
            g_maxlen=strlen(ptr->d_name);
        count++;
    }
    closedir(dir);

    // 提示:这块有点难,可能需要较好的c语言功底才能看的懂.
    char **filenames=(char**)malloc(sizeof(char*)*count)
      //通过目录中文件数来动态的在堆上分配存储空间，首先定义了一个指针数组
    char **temp[PATH_MAX+1];  //用于后面排序时的临时交换
    int i,j;
    for(i=0;i<count;i++)                                                //然后依次让数组中每个指针指向分配好的空间，这里是对上面的优化，有效
    {                                                                       //的防止了栈溢出，同时动态分配内存，更加节省空间
        filenames[i]=(char*)malloc(sizeof(char)*PATH_MAX+1);
    }

    //获取该目录下所有的文件名
    dir=opendir(path);
    for(i=0;i<count;i++)
    {
        ptr=readdir(dir);
        if(ptr==NULL)
        {
            my_err("readdir",__LINE__);
        }
        strcpy(filenames[i],ptr->d_name);
    }
    closedir(dir);
    //冒泡法对文件名排序
    if(flag&PARAM_r)  //-r参数反向排序
    {
        for(i=0;i<count-1;i++)
        {
            for(j=0;j<count-1-i;j++)
            {
                if(strcmp(filenames[j],filenames[j+1])<0)
                {
                    strcpy(temp,filenames[j]);
                    strcpy(filenames[j],filenames[j+1]);
                    strcpy(filenames[j+1],temp);
                }
            }
        }
    }
    else //正向排序
    {
        for(i=0;i<count-1;i++)
                {
                        for(j=0;j<count-1-i;j++)
                        {
                                if(strcmp(filenames[j],filenames[j+1])>0)
                                {       
                                        strcpy(temp,filenames[j]);
                                        strcpy(filenames[j],filenames[j+1]);
                                        strcpy(filenames[j+1],temp);
                                }
                        }
                }

    }

    if(chdir(path)<0) //chdir函数:改变当前路径成功返回0,失败返回-1
    {
        my_err("chdir",__LINE__);
    }

    display(filenames,count); 

    if((flag&PARAM_L==0&&!(flag|PARAM_R)))
        printf("\n");
}

int main(int argc,char** argv)
{
    int i,j,k=0,num=0;
    char param[32]="";     //用来保存命令行参数
    char *path[1];         //保存路径，其实我也不想定义为一个指针数组，但是为了和后面的char **类型的函数参数对应只能定义成这样
    path[0]=(char*)malloc(sizeof(char)*(PATH_MAX+1));   //由于是一个指针类型，所以我们要给他分配空间  PATH_MAX是系统定义好的宏，它的值为4096,严谨一点，加1用来存'\0'
    flag=PARAM_NONE;       //初始化flag=0（由于flag是全剧变量，所以可以不用初始化）
    struct stat buf;       //该结构体以及lstat,stat，在 sys/types.h，sys/stat.h，unistd.h 中，具体 man 2 stat

    //命令行参数解析,将-后面的参数保存至param中
    for(i=1;i<argc;i++)
    {
        if(argv[i][0]=='-')
        {
            for(j=1;j<strlen(argv[i]);j++)
            {
                param[k]=argv[i][j];
                k++;    
            }
            num++;
        }
    }
    param[k]='\0';

    /* 判断参数 */
    for(i=0;i<k;i++)
    {
        if(param[i]=='a')
            flag|=PARAM_A;
        else if(param[i]=='l')
            flag|=PARAM_L;
        else if(param[i]=='R')
            flag|=PARAM_R;
        else if(param[i]=='r')
            flag|=PARAM_r;
        else
        {
            printf("my_ls:invalid option -%c\n",param[i]);
            exit(0);
        }
    }

    //如果没有输入目标文件或目录，就显示当前目录
    if(num+1==argc)
    {
        strcpy(path[0],".");
        display_dir(path[0]);
        return 0;
    }

    i=1;
    do
    {
        if(argv[i][0]=='-')
        {
            i++;
            continue;
        }
        else
        {
            strcpy(path[0],argv[i]);
            //判断目录或文件是否存在
            if(stat(argv[i],&buf)==-1)   //buf参数是一个指向stat结构的指针
            {
                my_err("stat",__LINE__);    //编译器内置的宏，插入当前源码行号
            }
            if(S_ISDIR(buf.st_mode))   //判断是否为目录
                display_dir(path[0]);
                i++;
            }
            else
            {
                display(path,1);
                i++;
            }
        }
    }while(i<argc);
    printf("\n");

    return 0;

}








//1.首先用readdir(dir)打开文件,如果打开成功,用stat(dp->d_name,&statbuf)
//    其中定义两个结构体为,struct dirent* dp    struct stat statbuf;
//    通过stat函数,讲文件名为d_name的文件的信息,存入statbuf指向的stat结构体中,文件名由dp获取,在struct dirent* dp这个结构体中可以找到文件名
//2.    用stat将文件信息存入statbuf中后,可以通过statbuf获得文件的类型,权限,链接数
//    文件大小,
//3. struct passwd *pwd ,这个pwd可以用来存放用户名和组id,用户id可以用fetpwuid函数找
//4.getwuid 拿到用户id后,可以返回一个指针,这个指针指向一个存有用户各种信息的结构体,这个结构体就是struct passwd ,所以我们用getwuid这个函数,传入用户id,得到这个结构体的指针,然后存入struct passwd* pwd中   
//
/*The  passwd structure is defined in <pwd.h> as fol‐
       lows:

           struct passwd {
//               char   *pw_name;       /* username */
//               char   *pw_passwd;     /* user password */
//               uid_t   pw_uid;        /* user ID */
//               gid_t   pw_gid;        /* group ID */
//               char   *pw_gecos;      /* user information */
//               char   *pw_dir;        /* home directory */
//               char   *pw_shell;      /* shell program */
//           };



//      The  group  structure is defined in <grp.h> as follows:

   //        struct group {
   //            char   *gr_name;       /* group name */
   //            char   *gr_passwd;     /* group password */
   //            gid_t   gr_gid;        /* group ID */
   //            char  **gr_mem;        /* group members */
   //        };



//struct dirent {
//               ino_t          d_ino;       /* inode number */
//               off_t          d_off;       /* not an offset; see NOTES */
//               unsigned short d_reclen;    /* length of this record */
//               unsigned char  d_type;      /* type of file; not supported
//                                              by all file system types */
//               char           d_name[256]; /* filename */
//            };

// 在dirent结构体中可以获得文件的inode值和文件名filename




