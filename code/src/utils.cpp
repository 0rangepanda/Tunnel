#include <string>

#include "utils.h"

/**************************************************************************
    Check a filepath, if not directory, return 0
**************************************************************************/
int isDirectory(const char *path)
{
        struct stat statbuf;
        if (stat(path, &statbuf) != 0)
                return 0;
        return S_ISDIR(statbuf.st_mode);
}

/**************************************************************************
   Check a filepath
    not exist,   return -1
    directory,   return 0
    normal file, return 1
**************************************************************************/
int checkFile(const char *path)
{
        //verify if FILEPATH exist
        if(access(path, F_OK ) != 0 ) {
                fprintf(stderr, "File doesn't exist!\n");
                return -1;
        }
        //check if FILEPATH is a directory
        if(isDirectory(path)==1) {
                fprintf(stderr, "File is a directory!\n");
                return 0;
        }
        return 1;
}

/**************************************************************************
    Parse Configuration File
**************************************************************************/
int parseConf(const char *path)
{
        ifstream fin(path);
        string line;

        while(getline(fin,line))
        {
                if (line[0]!='#')
                {
                        istringstream iss(line);
                        vector<string> tokens;
                        vector<string>::iterator i;
                        copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

                        i = tokens.begin();
                        if (*i == "stage")
                                stage = std::stoi(*++i);
                        else if (*i == "num_routers")
                                num_routers = std::stoi(*++i);
                }
        }

        cout << "stage: " << stage << '\t' << "num_routers: " << num_routers << endl;
        return EXIT_SUCCESS;
}

/**************************************************************************
    A wrapper of fprintf for writing line to LOG File
**************************************************************************/
void LOG(FILE *fp, const char *format,...)
{
        va_list args;
        char msg[MAXSIZE];
        int msgsize;
        va_start(args,format);
        msgsize = vsnprintf(msg,sizeof(msg),format,args);
        va_end(args);
        if(msgsize < 0)
                return;
        fprintf(fp,"%s",msg);
}

/**************************************************************************
* tun_alloc: allocates or reconnects to a tun/tap device.
* copy from from simpletun.c
* refer to http://backreference.org/2010/03/26/tuntap-interface-tutorial/ for more info
**************************************************************************/

int tun_alloc(char *dev, int flags)
{
        struct ifreq ifr;
        int fd, err;
        char *clonedev = (char*)"/dev/net/tun";

        if( (fd = open(clonedev, O_RDWR)) < 0 )
        {
                perror("Opening /dev/net/tun");
                return fd;
        }

        memset(&ifr, 0, sizeof(ifr));

        ifr.ifr_flags = flags;

        if (*dev)
        {
                strncpy(ifr.ifr_name, dev, IFNAMSIZ);
        }

        if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 )
        {
                perror("ioctl(TUNSETIFF)");
                close(fd);
                return err;
        }

        strcpy(dev, ifr.ifr_name);
        return fd;
}

/**************************************************************************
* Bind a socket and get a dynamic UDP port (choose by OS)
**************************************************************************/
int UDP_alloc(struct sockaddr_in *addr)
{
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        socklen_t addrlen = sizeof(struct sockaddr_in);

        memset(addr, 0, sizeof(struct sockaddr_in));
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = inet_addr("127.0.0.1");
        addr->sin_port = htons(0);
        bind(sock, (struct sockaddr*) addr, addrlen);
        getsockname(sock, (struct sockaddr*) addr, &addrlen);
        return sock;
}
