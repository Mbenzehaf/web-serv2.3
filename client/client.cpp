#include "client.hpp"

char *ft_strjoin(char *s1, char *s2) {
    char *newStr = (char *)malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(newStr, s1);
    strcat(newStr, s2);
    return newStr;
}

Client::Client(int& clientSocket, sockaddr_in& clientAddress, socklen_t& clientAddressLength, int ServerSocket, GlobalConfig *config) {
    this->clientSocket = clientSocket;
    this->clientAddress = clientAddress;
    this->clientAddressLength = clientAddressLength;
    this->ServerSocket = ServerSocket;
    this->config = config;
    this->request2 = new Request2(clientSocket, config);
    this->response = new Response(request2);
    this->request2->buffer = buffer;
    this->addrLen = sizeof(clientAddr);
    index = 0;
    makeRequest = true;
    this->responceReady = false;
    this->requestFinished = false;
    if (getsockname(clientSocket, (struct sockaddr*)&clientAddr, &addrLen) == -1) {
        perror("Error getting client address");
        return;
    }
    port = ntohs(clientAddr.sin_port);
    counter = 0;
    std::cout << "Client connected from port: " << port << std::endl;
}

void Client::setIp() {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ip, INET_ADDRSTRLEN);
    this->ip = ip;
}

const string Client::getIp() const {
    return ip;
}

void Client::setRequestFinished(int& index) {
    // this->requestFinished = this->request2->requestFinished;
    for (int i = 0; i < requests.size(); i++) {
        if (!requests[i].first->requestFinished) {
            requestFinished = false;
            return ;
        }
    }
    requestFinished = true;
}

int& Client::getSocketDescriptor() const {
    return (const_cast<int&>(clientSocket));
}

Request2* Client::setRequest2() {
    return request2;
}

Request2* Client::getRequest2(int& index) const {
    return (const_cast<Request2*> (requests[index].first));
}

Response* Client::getResponse() const {
    return response;
}

void Client::setResponse(Response *response) {
    this->response = response;
}

const sockaddr_in& Client::getClientAddress() const {
    return clientAddress;
}

char* Client::getBuffer() {
    return buffer;
}

ssize_t Client::getBytesRead() const {
    return bytesRead;
}

void Client::setBytesRead(ssize_t bytesRead) {
    this->bytesRead = bytesRead;
}

Client* Client::getNext() const {
    return next;
}

void Client::setNext(Client* next) {
    this->next = next;
}

void Client::setHost(string host) {
    this->host = host;
}

void Client::reset () {
    request2->reset();
    response->reset();
    requestFinished = false;
}

Client::~Client() {
    delete request2;
    delete response;
}