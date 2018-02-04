#include <string>

#include "utils.h"

/*
    If not directory, return 0
*/
int isDirectory(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

/*
    not exist,   return -1
    directory,   return 0
    normal file, return 1
*/
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

/*
    Parse Configuration File
*/
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

    return EXIT_SUCCESS;
}

/*
    A wrapper of fprintf for writing line to LOG File
*/
 
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
