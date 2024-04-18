#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../webServ.hpp"
class Request2;
class Response;

char *ft_strjoin(char *s1, char *s2);

class Client {
    int clientSocket;
    sockaddr_in clientAddress;
    socklen_t clientAddressLength;
    char buffer[BUFFER_SIZE + 1];
    ssize_t bytesRead;
    char body[BUFFER_SIZE ];
    char *FullRequest;
    Client* next;
    Request2 *request2;
    Response *response;
    int ServerSocket;
    struct sockaddr_in clientAddr;
    socklen_t addrLen;
    int port;
    std::string ip;
    string host;
    bool responceReady;
    GlobalConfig *config;
    std::vector<int> requestNumber;
    std::vector <Response> responses;

    public:
        int counter;   
        std::vector <std::pair <Request2*, Response*> > requests;
        bool makeRequest;
        int index;
        bool requestFinished;

        Client () {
            request2 = NULL;
            response = NULL;
            next = NULL;
            makeRequest = true;
            responceReady = false;
            requestFinished = false;
        }

        Client(int& clientSocket, sockaddr_in& clientAddress, socklen_t& clientAddressLength, int ServerSocket, GlobalConfig *config);

        void setIp();

        const string getIp() const;

        void setRequestFinished(int& index);
        
        int& getSocketDescriptor() const;

        Request2* setRequest2();

        Request2* getRequest2(int& index) const;

        Response* getResponse() const;

        void setResponse(Response *response);

        const sockaddr_in& getClientAddress() const;

        char* getBuffer();

        ssize_t getBytesRead() const;

        void setBytesRead(ssize_t bytesRead);

        Client* getNext() const;

        void setNext(Client* next);

        void setHost(string host);

        void reset ();

        ~Client();
};

#endif