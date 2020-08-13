#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <glob.h>
#include <limits.h>
#include <pwd.h>
#include <assert.h>


#define OPT_STR "+ilR"

#define TR_INODE    1
#define TR_LONG     2
#define TR_RECUR    4

char* parse_file_mode(const mode_t st_mode)
{
    static char file_mode[12];
    file_mode[0] = '-';
    switch (st_mode & S_IFMT)
    {
    case S_IFSOCK:
        file_mode[0] = 's';
        break;
    
    case S_IFLNK:
        file_mode[0] = 'l';
        break;

    case S_IFDIR:
        file_mode[0] = 'd';
        break;

    default:
        break;
    }

    file_mode[1] = (st_mode & S_IRUSR) ?  'r' : '-';
    file_mode[2] = (st_mode & S_IWUSR) ?  'w' : '-';
    file_mode[3] = (st_mode & S_ISUID) ?  's' : ((st_mode & S_IXUSR) ? 'x' : '-');

    file_mode[4] = (st_mode & S_IRGRP) ?  'r' : '-';
    file_mode[5] = (st_mode & S_IWGRP) ?  'w' : '-';
    file_mode[6] = (st_mode & S_ISGID) ?  's' : ((st_mode & S_IXUSR) ? 'x' : '-');

    file_mode[7] = (st_mode & S_IROTH) ?  'r' : '-';
    file_mode[8] = (st_mode & S_IWOTH) ?  'w' : '-';
    file_mode[9] = (st_mode & S_ISVTX) ?  't' : ((st_mode & S_IXUSR) ? 'x' : '-');
    
    file_mode[10] = '.';
    file_mode[11] = '\0';

    return file_mode;
}

/**
 * Print file status accroding to the mode.
*/
void print_by_mode(const char* p_full_path, const struct stat* p_stbuf, const int mode)
{
    static char out_str_buf[PATH_MAX] = {0};
    static char time_fmt_buf[PATH_MAX] = {0};
    char* p_mode_str = NULL;
    const char* p_file_name = NULL;
    struct tm* p_st_tm = NULL;
    size_t prefix_len = 0;
    assert(p_stbuf);

    out_str_buf[0] = '\0';
    time_fmt_buf[0] = '\0';

    // get filename without prefix path
    p_file_name = strrchr(p_full_path, '/');
    p_file_name = p_file_name ? p_file_name : p_full_path;
    ++p_file_name;  // go pass over the last '/'

    if (mode & TR_INODE)
    {
        sprintf(out_str_buf, "%ld ", p_stbuf->st_ino);
    }

    prefix_len = strlen(out_str_buf);
    if (mode & TR_LONG)
    {
        p_st_tm = localtime(&p_stbuf->st_mtim.tv_sec);
        if (p_st_tm == NULL)
        {
            perror("local()");
            exit(1);
        }

        strftime(time_fmt_buf, sizeof(time_fmt_buf), "%b %e %Y %H:%M", p_st_tm);
        p_mode_str = parse_file_mode(p_stbuf->st_mode);
        assert(p_mode_str);
        sprintf(out_str_buf + prefix_len, "%s %ld %s %s %4ld %s %s\n"
            , p_mode_str
            , p_stbuf->st_nlink
            , getpwuid(p_stbuf->st_uid)->pw_name
            , getpwuid(p_stbuf->st_gid)->pw_name
            , p_stbuf->st_size
            , time_fmt_buf      //"Replace by time format"
            , p_file_name);
    }
    else
    {
        sprintf(out_str_buf + prefix_len, "%s ", p_file_name);
    }
    printf("%s", out_str_buf);
}

/**
 * If recursivly mode is set, it will traverse by depth-first.
 */
void traverse(const char* path, const int mode)
{
    static char new_path[PATH_MAX] = {0};
    struct stat stbuf = {0};
    const char* p_file_name = NULL;
    glob_t glob_res = {0};

    if (lstat(path, &stbuf) < 0)
    {
        perror("stat()");
        exit(1);
    }

    // is a normal file
    if (S_ISDIR(stbuf.st_mode) == 0)
    {
        print_by_mode(p_file_name, &stbuf, mode);
        return;
    }

    // is a directory
    strncpy(new_path, path, PATH_MAX);
    strncat(new_path, "/*", PATH_MAX - strlen(new_path));
    glob(new_path, 0, NULL, &glob_res);

    // strncpy(new_path, path, PATH_MAX);
    // strncat(new_path, "/.*", PATH_MAX-strlen(new_path));
    // glob(new_path, GLOB_APPEND, NULL, &glob_res);

    for (int i = 0; i < glob_res.gl_pathc && glob_res.gl_pathv[i]; ++i)
    {
        // puts(glob_res.gl_pathv[i]);
        p_file_name = strrchr(glob_res.gl_pathv[i], '/');
        if (NULL == p_file_name)
        {
            perror("strrchr()");
            exit(1);
        }
        if (lstat(glob_res.gl_pathv[i], &stbuf) < 0)
        {
            perror("lstat()");
            exit(1);
        }
        print_by_mode(glob_res.gl_pathv[i], &stbuf, mode);
    }

    if (mode & TR_RECUR)
    {
        for (int i = 0; i < glob_res.gl_pathc; i++)
        {
            if (lstat(glob_res.gl_pathv[i], &stbuf) < 0)
            {
                perror("lstat()");
                exit(1);
            }
            if (S_ISDIR(stbuf.st_mode))
            {
                puts("");
                print_by_mode(glob_res.gl_pathv[i], &stbuf, 0);
                puts(":");
                traverse(glob_res.gl_pathv[i], mode);
            }
        }
    }
    globfree(&glob_res);
}

int main(int argc, char** argv)
{
    int opt, tr_mode;
    
    tr_mode = 0;
    while ((opt = getopt(argc, argv, OPT_STR)) >= 0)
    {
        switch (opt)
        {
        case 'i':
            tr_mode |= TR_INODE;
            puts("i");
            break;

        case 'l':
            tr_mode |= TR_LONG;
            break;

        case 'R':
            tr_mode |= TR_RECUR;
            puts("R");
            break;

        default:
            break;
        }
    }

    // The rest parameters of argv[] are all non-option parameters.
    while (argv[optind])
    {
        traverse(argv[optind++], tr_mode);
    }
    return 0;
}