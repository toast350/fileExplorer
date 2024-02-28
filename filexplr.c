#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/uio.h>

#define CTRL_KEY(a) ((a) & 0x1f)

struct fileInfo
{
    unsigned char d_type;
    char d_name[256];
};

void die(const char *);
void myWrite(int fd, char *s, size_t len);

void bufferWrite(char *);
void bufferResize(int);
void bufferShow(void);

struct fileInfo *getCurDirArr(int *);
void printFileInfoArr(struct fileInfo *, int);
char *toLowerString(char *);
int fileInfoCmp(const void *, const void *);

struct termios orig_termios;

void enableRawMode(void);
void disableRawMode(void);
void clearScreen(void);
void mvCursorDown(void);
void mvCursorUp(void);

struct iovec *bufferIOVecPtr;
int bufferIOVecCount;
int bufferIOVecCurrent;

int cursorY = 0;

int main(void)
{
    int dirArrLen;
    struct fileInfo *dirArr;
    int c;

    enableRawMode();

    while (1)
    {
        dirArr = getCurDirArr(&dirArrLen);
        printFileInfoArr(dirArr, dirArrLen);

        c = 0;
        read(STDIN_FILENO, &c, 1);
        switch (c)
        {
            case CTRL_KEY('q'):
                clearScreen();
                exit(1);
                break;
            case 'j':
                if (cursorY != dirArrLen-1) ++cursorY;
                break;
            case 'k':
                if (cursorY != 0) --cursorY;
                break;
            case '\r':
                if (dirArr[cursorY].d_type == DT_DIR) chdir(dirArr[cursorY].d_name);
                break;
            /*case 'r':
                rename(dirArr[cursorY].d_name, 
                */
        }

        free(dirArr);
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

    for (numEntries = 0; (dirEnt = readdir(dir)) != NULL; numEntries++);
    rewinddir(dir);

    fileInfoArr = malloc(numEntries * sizeof(struct fileInfo));
    while (i < numEntries)
    {
        dirEnt = readdir(dir);
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

    bufferResize(arrLen+2);

    char *curDirName = getcwd(NULL, 0);    // glibc specific, should fix later
    char *curDirNameBanner = malloc((strlen(curDirName) + 19) * sizeof(char));
    sprintf(curDirNameBanner, "\033[H\033[4;1;35m%s\033[0m\r\n", curDirName);
    bufferWrite(curDirNameBanner);
    free(curDirName);

    if (arr == NULL) return;
    for (i = 0; i < arrLen; i++)
    {
        if (arr[i].d_type == DT_DIR)
        {
            char *string = malloc((strlen(arr[i].d_name) + 13) * sizeof(char));
            sprintf(string, "\033[36m%s/\033[0m\r\n", arr[i].d_name);
            bufferWrite(string);
        } else
        {
            char *string = malloc((strlen(arr[i].d_name) + 3) * sizeof(char));
            sprintf(string, "%s\r\n", arr[i].d_name);
            bufferWrite(string);
        }
    }

    char *cursorString = malloc(17 * sizeof(char));
    if (cursorY >= arrLen) cursorY = arrLen-1;
    sprintf(cursorString, "\033[%d;0H", cursorY+2);
    bufferWrite(cursorString);

    clearScreen();
    bufferShow();
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

    if (strncmp(ptr1->d_name, ".", 2) == 0)
    {
        return -1;
    }
    if (strncmp(ptr1->d_name, "..", 3) == 0)
    {
        if (strncmp(ptr2->d_name, ".", 2) == 0)
        {
            return 1;
        }
        return -1;
    }
    if ((strncmp(ptr2->d_name, ".", 2) == 0) || (strncmp(ptr2->d_name, "..", 3) == 0))
    {
        return 1;
    }

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

void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    {
        die("tcsetattr");
    }
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON | ICRNL);     // allows Ctrl-S and Ctrl-Q
    raw.c_oflag &= ~(OPOST);            // doesn't translate carriage returns and newlines
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;                // allows read to return in 1/10 of a second

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
        die("tcsetattr");
    }
}

void disableRawMode(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    {
        die("tcsetattr");
    }
}

void die(const char *s)
{
    perror(s);
    exit(1);
}

void clearScreen(void)
{
    myWrite(STDOUT_FILENO, "\033[2J\033[H", 7);
}

void bufferWrite(char *s)
{
    if (bufferIOVecCurrent >= bufferIOVecCount) return;
    bufferIOVecPtr[bufferIOVecCurrent].iov_base = s;
    bufferIOVecPtr[bufferIOVecCurrent].iov_len = strlen(s);
    bufferIOVecCurrent++;
}

void bufferResize(int size)
{
    bufferIOVecCount = size;
    free(bufferIOVecPtr);
    bufferIOVecPtr = calloc(size, sizeof(struct iovec));
    bufferIOVecCurrent = 0;
}

void bufferShow(void)
{
    writev(STDOUT_FILENO, bufferIOVecPtr, bufferIOVecCurrent);
    // TODO: make this less likely to fail

    for (int i = 0; i < bufferIOVecCurrent; i++)
    {
        free(bufferIOVecPtr[i].iov_base);
    }
}

void myWrite(int fd, char *s, size_t len)
{
    ssize_t ret;

    while (len != 0 && (ret = write(fd, s, len)) != 0)
    {
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            die("write");
        }

        len -= ret;
        s += ret;
    }
}
