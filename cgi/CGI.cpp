#include "CGI.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>


#define WRITE_END 1
#define READ_END 0

static std::string scriptname(const std::vector<std::string> env) {
    std::vector<std::string>::const_iterator it;

    for (it = env.begin(); it != env.end(); it++) {
        size_t index = it->find("SCRIPT_FILENAME=");
        if (index != std::string::npos) {
            return it->substr(index + 16);
        }
    }
    return "";
};

void CGI::preparenv_args(const std::vector<std::string>& env) {
    envp = new char*[env.size() + 1];

    size_t i = 0;
    for (std::vector<std::string>::const_iterator it = env.begin(); it != env.end(); it++) {
        envp[i] = strdup(it->c_str());
        ++i;
    }
    envp[i] = NULL;

    std::map<std::string, std::string> interpreterPaths = {
            {".py", "/usr/bin/python3"},
            {".php", "/usr/bin/php"},
            {".pl", "/usr/bin/perl"},
            {".rb", "/usr/bin/ruby"},
            {".sh", "/bin/sh"},
            {".js", "/usr/bin/node"}
            
        };
    std::map<std::string, std::string>::iterator iter = interpreterPaths.find(ex);
        if (iter != interpreterPaths.end()) {
            runner = iter->second;
            argv = new char*[4];
            argv[0] = strdup(runner.c_str());
            argv[1] = strdup(this->path.c_str());
            argv[2] = strdup(this->upload.c_str());
            argv[3] = NULL;
        } else {
            throw CGIEX("Unsupported script extension");
        }
}

void CGI::writeOnPipe(int fd, const std::string& file) {
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs.is_open())
        throw CGI::CGIEX("can't open the file");

    char *buff = new char[2];
    //std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    std::string content;
    while (ifs.read(buff, 1))
    {
        content.append(buff, 1);
    }

    this->bodylen = content.size();
    if (this->bodylen > 0)
        fcntl(fd, F_SETPIPE_SZ, this->bodylen);
    write(fd, content.c_str(), content.size());
    delete[] buff;
    ifs.close();
}

void CGI::execute_script(void) {
    pid_t pid;

    int ofs = open(".CGIoutFile", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (ofs == -1)
        throw CGIEX("open filat :/");

    int fdsotre[2];
    if (pipe(fdsotre) == -1)
        throw CGIEX("pipe filat :/");

    if (this->method == "POST")
        writeOnPipe(fdsotre[1], file);
    else {
        if (this->bodylen > 0)
            fcntl(fdsotre[1], F_SETPIPE_SZ, this->bodylen);
        write(fdsotre[1], this->request.data(), this->bodylen);
    }
    close(fdsotre[1]);

    pid = fork();
    if (pid == -1)
        throw CGIEX("fork filat :/");

    if (pid == 0) {
        dup2(fdsotre[0], STDIN_FILENO);
        close(fdsotre[0]);

        dup2(ofs, STDOUT_FILENO);
        close(ofs);
        if (execve(runner.c_str(), this->argv, this->envp) == -1)
            throw CGIEX("execv filat :/");
    } else {
        int state;
        close(fdsotre[0]);
        close(ofs);
        waitpid(pid, &state, 0);
        if (state == 0)
            cgiSuccess = true;
    }
}

std::string CGI::getOutputPath() const{
    return ".CGIoutFile";
}

CGI::CGI (const std::string& method, const std::vector<std::string> &env, const std::string& ex,size_t bodylen, const std::string& upload, const std::string& request, const std::string& file ): method(method), request(request), bodylen(bodylen), upload(upload), file (file) {
    cgiSuccess = false;
    path = scriptname(env);
    if (path.empty())
        throw CGIEX("can't find the script in the path you provided");
    this->ex = ex;

    size_t exPos = path.find_last_of('.');
    if (exPos == std::string::npos)
        throw CGIEX("wrong extension");

    std::string ext = path.substr(exPos);
    if (ext != ex)
        throw CGIEX("the extension file you provided is not supported here");
    
    //std::cout << "path ==>" << path <<std::endl;
    if (access(this->path.c_str(), F_OK) != 0)
        throw CGIEX("your CGI file is not provided or invalide");
    this->preparenv_args(env);
    this->execute_script();
}

CGI::~CGI(void) {
    //remove(".CGIoutFile");
    for (size_t i = 0; argv[i]; i++)
        free(argv[i]);
    delete[] argv;

    for (size_t i = 0; envp[i]; i++)
        free(envp[i]);
    delete[] this->envp;
}

// int main()
// {
//     std::vector<std::string> envVars = {
//         "SCRIPT_FILENAME=/home/kali/Desktop/webserv/cgiphp.php",
//         "REQUEST_METHOD=POST",
//         "QUERY_STRING=query=example",
//         "CONTENT_LENGTH=11"
//     };

//     std::string requestContent = "Hey, this is a test with php script :)";

//     std::string ex = ".php";

//     std::string uploadTo = "/home/kali/Desktop/webserv";

//     //
//     std::string file = "/home/kali/Desktop/webserv/cgiphp.php";
//     std::string method = "POST";
//     //

//     try {
//         CGI cgiHandler(method, envVars, ex, requestContent.size(), uploadTo, requestContent, file);

//         cgiHandler.execute_script();

//     } catch (const CGI::CGIEX& e) {
//         std::cerr << "CGI exception occurred: " << e.what() << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "General exception occurred: " << e.what() << std::endl;
//     }

//     return 0;
// }