#ifndef WEBSERV_HPP
#define WEBSERV_HPP

const int MAX_CLIENTS = 1024;
const int BUFFER_SIZE = 1000;

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h> // Include epoll header
#include <cstring>
#include <sys/stat.h>
#include <climits>
#include <csignal>
#include <fcntl.h>
#include <dirent.h>
#include <boost/asio.hpp>
#include "config/config.hpp"
#include "response/response.hpp"
#include "request/request.hpp"
#include "cgi/CGI.hpp"
#include "client/client.hpp"


using namespace std;




#endif