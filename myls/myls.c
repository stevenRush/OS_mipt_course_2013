#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define FLAG_SHOWALMOSTALL      0x1     // -A
#define FLAG_SHOWALL            0x2     // -a
#define FLAG_SHOWHIDDEN         (FLAG_SHOWALMOSTALL | FLAG_SHOWALL)  
#define FLAG_RECURSIVE          0x4     // -R
#define FLAG_LONGLISTING        0x8     // -l
#define TEXTCOLOR_EXEC          "\033[1;32m%s\033[0m"
#define TEXTCOLOR_DIRECTORY     "\033[1;34m%s\033[0m"
#define TEXTCOLOR_DEFAULT       "%s"

enum FileType
{
    ftExecutive = 0,
    ftDirectory,
    ftRegular,
    ftSymbLink
};

void PrintPermissionFlags(mode_t mode)
{
    printf("%s", 	S_ISDIR(mode) ? "d" : "-");
    printf("%s%s%s",    mode & S_IRUSR ? "r" : "-",
                        mode & S_IWUSR ? "w" : "-",
                        mode & S_IXUSR ? "x" : "-");
    printf("%s%s%s",    mode & S_IRGRP ? "r" : "-",
                        mode & S_IWGRP ? "w" : "-",
                        mode & S_IXGRP ? "x" : "-");
    printf("%s%s%s ",   mode & S_IROTH ? "r" : "-",
                        mode & S_IWOTH ? "w" : "-",
                        mode & S_IXOTH ? "x" : "-");
}

void PrintFile(const char *, const char *, int);

enum FileType GetFileType(mode_t mode)
{
    if (S_ISREG(mode) && (mode & 0111)) // EXECUTABLE
        return ftExecutive;
    if (S_ISDIR(mode))
        return ftDirectory;
    if (S_ISLNK(mode))
        return ftSymbLink;
    return ftRegular;
}

int GetFileCount(const char* path)
{
    int count;
    count = 0;
    DIR *hDir = opendir(path);
	if (hDir == 0)
	{
		printf("Cannot open directory %s\n", path);
		return 0;
	}

    struct dirent *entry;
    while(entry = readdir(hDir))
    {
        if (strcmp(entry->d_name, ".") * strcmp(entry->d_name, "..") == 0)
            continue;
        count++;
    }
    closedir(hDir);
    return count;
}

void PrintDir(const char* path, int flag)
{
    DIR *hDir = opendir(path);
	if (hDir == 0)
	{
		printf("Cannot open directory %s\n", path);
		return;
	}
    struct dirent *entry;
    char filePath[250];
    int isDirEmpty = 1;
    if (flag & FLAG_RECURSIVE)
        printf("%s:\n", path);
    if (flag & FLAG_LONGLISTING)
        printf("total %d\n", GetFileCount(path));
    while(entry = readdir(hDir))
    {
        if (entry->d_name[0] == '.')
        {
            if ((flag & FLAG_SHOWHIDDEN) == 0)
                continue;
            if (strcmp(entry->d_name, ".") * strcmp(entry->d_name, "..") == 0 && (flag & FLAG_SHOWALMOSTALL) == 0)
                continue;
        }
        isDirEmpty = 0;
        memset(filePath, 0, 250);
        strcpy(filePath, path);
        strcat(filePath, "/");
        strcat(filePath, entry->d_name);
        PrintFile(filePath, entry->d_name, flag);
    }
    if (!isDirEmpty && ((flag & FLAG_LONGLISTING) == 0))
        printf("\n");
    closedir(hDir);
    if ((flag & FLAG_RECURSIVE) == 0)
        return;
    printf("\n");
    hDir = opendir(path);
	if (hDir == 0)
	{
		printf("Cannot open directory %s", path);
		return;
	}

    while(entry = readdir(hDir))
    {
        if (strcmp(entry->d_name, ".") * strcmp(entry->d_name, "..") == 0)
            continue;
        if (entry->d_name[0] == '.' && (flag & FLAG_SHOWHIDDEN) == 0)
            continue;
        memset(filePath, 0, 250);
        strcpy(filePath, path);
        strcat(filePath, "/");
        strcat(filePath, entry->d_name);  
        struct stat buf; 
        lstat(filePath, &buf);
        if (S_ISLNK(buf.st_mode))
        {
        	continue;
        }
        if (S_ISDIR(buf.st_mode))
            PrintDir(filePath, flag); 
    }
    closedir(hDir);
}

int IsFile(const char * filePath)
{
    struct stat buf;
	stat(filePath, &buf);
	return S_ISREG(buf.st_mode);
}

void PrintFile(const char* filePath, const char * fileName, int flag)
{
    struct stat buf;
	stat(filePath, &buf);
    if (flag & FLAG_LONGLISTING)
    {
        char timeStr[30];
        struct group *grpInfo;
        struct passwd *pwd;
        grpInfo = getgrgid(buf.st_gid);
        pwd = getpwuid(buf.st_uid);   
        if (grpInfo == NULL || pwd == NULL)
			return;        
        struct tm *time;
        time = localtime(&buf.st_mtime); 
        strftime(timeStr, 30, "%b %e %R", time);  
        PrintPermissionFlags(buf.st_mode);
        printf("%2d %s %s %6u %s ", buf.st_nlink, grpInfo->gr_name, pwd->pw_name,(size_t)buf.st_size, timeStr);        
    }
    switch(GetFileType(buf.st_mode))
    {
        case ftExecutive:
            printf(TEXTCOLOR_EXEC, fileName);
            break;
        case ftDirectory:
            printf(TEXTCOLOR_DIRECTORY, fileName);
            break;
        default:
            printf(TEXTCOLOR_DEFAULT, fileName);
    }
    if (flag & FLAG_LONGLISTING)
        printf("\n");
    else 
        printf("  ");
}

void ParseFlags(const char * string, int *flag)
{
	if (string[0] != '-')
	{
		return;
	}
	
	int i;
	for(i = 1; i < strlen(string); ++i)
	{
		switch(string[i])
		{
			case 'a':
				*flag |= FLAG_SHOWALMOSTALL;
				break;
			case 'A':
				*flag |= FLAG_SHOWALL;
				break;
			case 'R':
				*flag |= FLAG_RECURSIVE;
				break;
			case 'l':
				*flag |= FLAG_LONGLISTING;
				break;
		}
	}
}

int main(int argc, char** argv)
{
	int flagsCount = 0;
    int flag = 0;
	size_t i;
    for(i = 0; i < argc; ++i)
    {
    	if (argv[i][0] == '-')
    		flagsCount += 1;
        ParseFlags(argv[i], &flag);
    }
    if (argc - flagsCount == 1)
	    PrintDir(".", flag);
	else
	{
		for(i=1; i < argc; ++i)
			if (argv[i][0] != '-')
			{
				if (IsFile(argv[i]))
				{
					char buf[300];
					strcpy(buf, "./");
					strcat(buf, argv[i]);
					PrintFile(buf, argv[i], flag);
				}
				else
				{
					printf("%s:\n", argv[i]);
					PrintDir(argv[i], flag);
				}
				printf("\n");
			}
	}
	return 0;
}
