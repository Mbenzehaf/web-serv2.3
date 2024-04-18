#ifndef CGI_HPP
#define CGI_HPP

#include "../webServ.hpp"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <map>
#include <string>
#include <vector>


class CGI {

    private:
        char** argv;
        char** envp;
        std::string runner;
        std::string ex;
        std::string path;
        std::vector<std::string> params;
        size_t bodylen;
        const std::string& request;
        const std::string& upload;
        const std::string& file;
        const std::string& method;
        CGI(void);

    public:
        bool cgiSuccess;
        CGI(const std::string& method, const std::vector<std::string> &env, const std::string &ex, size_t bodylen, const std::string &upload, const std::string &request, const std::string& file);
        ~CGI(void);

        std::string response;

        class CGIEX : public std::exception {
        public:
        std::string message;
        CGIEX(std::string message) : message("Error: " + message){};
        ~CGIEX() throw(){};
        virtual const char* what() const throw() {
            return message.c_str();
        };
    };

    void writeOnPipe(int fd, const std::string& file);
    std::string getOutputPath () const;
    void execute_script(void);
    void preparenv_args(const std::vector<std::string>& envVars);
   
};

#endif