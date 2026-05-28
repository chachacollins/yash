#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <print>
#include <ranges>
#include <vector>

std::vector<const char*> split_args(const std::string& s1, const std::string& del = " ")
{
    return std::views::split(s1, del)
           | std::views::transform([](const auto& x) { return std::string{x.begin(), x.end()};})
           | std::ranges::to<std::vector<std::string>>()
           | std::views::transform([](const auto& x) { return x.c_str(); })
           | std::ranges::to<std::vector<const char*>>();
}

int exec_command(const std::string& command)
{
    int status = 0;
    auto args = split_args(command);
    args.push_back(nullptr);
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("ERROR: ");
        exit(1);
    }
    if (pid == 0)
    {
        if(execvp(args[0], const_cast<char**>(&args[0])) < 0)
        {
            perror("ERROR: ");
            _exit(127);
        }
    }
    else
    {
        int wpid_result = waitpid(pid, &status, 0);
        if (wpid_result < 0)
        {
            perror("ERROR: ");
            exit(1);
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            return status;
        }
    }
    return status;
}

int main(void)
{
    while (!feof(stdin))
    {
        std::string command = readline("yash> ");
        if (command == "exit") break;
        exec_command(command);
    }
    return 0;
}
