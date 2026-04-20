#define ARENA_IMPLEMENTATION
#include "arena.h"
#include <ctype.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define cstr_to_sv(s) ((SV){s, strlen(s)})

typedef struct
{
    char *data;
    int len;
} SV;

char *sv_to_cstr(Arena *arena, SV sv)
{
    char *buffer = arena_alloc(arena, sizeof(char) * sv.len + 1);
    mempcpy(buffer, sv.data, sv.len);
    buffer[sv.len] = '\0';
    return buffer;
}

SV sv_split_by_whitespace(SV *sv)
{
    if (sv->len <= 0) return *sv;
    SV new_sv = {0};
    new_sv.data = sv->data;
    while(sv->len > 0 && !isspace(*sv->data))
    {
        sv->data += 1;
        sv->len -= 1;
    }
    new_sv.len = (int)(sv->data - new_sv.data);
    while(sv->len > 0 && isspace(*sv->data))
    {
        sv->data += 1;
        sv->len -= 1;
    }
    return new_sv;
}

SV sv_trim_right(SV sv)
{
    int null_term_len = *(sv.data+sv.len) == '\0' ? 1 : 0;
    while(isspace(*(sv.data+sv.len-null_term_len)) && sv.len > 0)
        sv.len -= 1;
    return sv;
}

bool sv_cmp(SV s1, SV s2)
{
    if (s1.len != s2.len) return false;
    return strncmp(s1.data, s2.data, s1.len) == 0;
}

typedef struct
{
    SV *args;
    int len;
    int cap;
} Args;

void args_append(Arena *arena, Args *args, SV arg)
{
    if (args->len + 1 > args->cap)
    {
        int new_cap = sizeof(Args) * (args->cap ? args->cap * 2 : 10);
        args->args = arena_realloc(arena, args->args, args->cap, new_cap);
        args->cap = new_cap;
    }
    args->args[args->len++] = arg;
}


Args collect_args(Arena *arena, SV command)
{
    Args args = {0};
    while(command.len > 0)
    {
        SV sv = sv_split_by_whitespace(&command);
        args_append(arena, &args, sv);
    }
    return args;
}

//TODO use per command arena;
int exec_command(Arena *arena, SV command)
{
    int status;
    Args args = collect_args(arena, command);
    char **argv = arena_alloc(arena, sizeof(char*) * (args.len + 1));
    for (int i = 0; i < args.len; i++) {
        SV arg = args.args[i];
        argv[i] = sv_to_cstr(arena, arg);
    }
    argv[args.len] = NULL;
    pid_t pid = fork();
    if (pid < 0) return pid;
    if (pid == 0)
    {
        execvp(argv[0], argv);
        perror("ERROR: ");
        _exit(127);
    }
    else
    {
        int wpid_result = waitpid(pid, &status, 0);
        if (wpid_result < 0) return wpid_result;
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            fprintf(stderr,
                    "ERROR: `%s` returned non-zero status code: %d\n",
                    argv[0], status);
            return status;
        }
    }
    return status;
}

int main(void)
{
    Arena command_arena = {0};
    while(!feof(stdin))
    {
        char *buffer = readline("yash> ");
        SV command = cstr_to_sv(buffer);
        if (sv_cmp(command, cstr_to_sv("exit")))
        {
            break;
        }
        else
        {
            int result = exec_command(&command_arena, command);
            if (result < 0)
            {
                perror("ERROR: ");
                continue;
            }
        }
        free(buffer);
        arena_reset(&command_arena);
    }
    arena_free(&command_arena);
    return 0;
}
