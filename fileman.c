#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

struct fileInfo
{
    unsigned char d_type;
    char d_name[256];
};

struct fileInfo *getCurDirArr(int *);
void printFileInfoArr(struct fileInfo *, int);
char *toLowerString(char *);
int fileInfoCmp(const void *, const void *);

int main(void)
{
    int dirArrLen;
    struct fileInfo *dirArr;
    char buf[256];

    while (1)
    {
        dirArr = getCurDirArr(&dirArrLen);
        printFileInfoArr(dirArr, dirArrLen);
        printf("\n\n");

        printf("type a directory name: ");
        fgets(buf, 256, stdin);

        int bufLen = strlen(buf);
        if (buf[bufLen-1] == '\n')
        {
            buf[bufLen-1] = '\0';
        }
        if (chdir(buf) == -1)
        {
            perror("chdir");
        }

        if (dirArr != NULL) free(dirArr);
    }

    return 0;
}

struct fileInfo *getCurDirArr(int *arrLen)
{
    DIR *dir = opendir(".");
    struct dirent *dirEnt;
    int numEntries;
    int i = 0;
    struct fileInfo *fileInfoArr;

    for (numEntries = -2; (dirEnt = readdir(dir)) != NULL; numEntries++); // loop to get how much to malloc(), -2 to ignore ./ and ../
    rewinddir(dir);

    if (numEntries == 0) return NULL;

    fileInfoArr = malloc(numEntries * sizeof(struct fileInfo));
    while (i < numEntries)
    {
        dirEnt = readdir(dir);
        if ((strncmp(".", dirEnt->d_name, 2) == 0) || (strncmp("..", dirEnt->d_name, 3) == 0))
        {
            continue;
        }
        strcpy(fileInfoArr[i].d_name, dirEnt->d_name);
        fileInfoArr[i].d_type = dirEnt->d_type;

        i++;
    }
    closedir(dir);

    qsort((void *) fileInfoArr, numEntries, sizeof(struct fileInfo), fileInfoCmp);

    *arrLen = numEntries;
    return fileInfoArr;
}

void printFileInfoArr(struct fileInfo *arr, int arrLen)
{
    int i;

    printf("\033[36m./\n../\033[0m\n");
    if (arr == NULL) return;
    for (i = 0; i < arrLen; i++)
    {
        if (arr[i].d_type == DT_DIR)
        {
            printf("\033[36m%s/\033[0m\n", arr[i].d_name);
        } else
        {
            printf("%s\n", arr[i].d_name);
        }
    }
}

char *toLowerString(char *string)
{
    char *ret = string;

    for (; *string != '\0'; string++)
    {
        *string = tolower(*string);
    }
    return ret;
}

int fileInfoCmp(const void *voidPtr1, const void *voidPtr2)
{
    struct fileInfo *ptr1 = (struct fileInfo *) voidPtr1;
    struct fileInfo *ptr2 = (struct fileInfo *) voidPtr2;

    if (ptr1->d_type == ptr2->d_type)
    {
        int ret;
        char *lowerString1 = toLowerString(strdup(ptr1->d_name));
        char *lowerString2 = toLowerString(strdup(ptr2->d_name));

        ret = strcmp(lowerString1, lowerString2);

        free(lowerString1);
        free(lowerString2);

        return ret;
    } else if (ptr1->d_type == DT_DIR)
    {
        return -1;
    } else
    {
        return 1;
    }
}
