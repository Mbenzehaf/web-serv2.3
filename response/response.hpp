#ifndef RESPONSE_HPP
#define RESPONSE_HPP

class Request2;
class GlobalConfig;
#include "../webServ.hpp"
#include "../request/request.hpp"
#include "../cgi/CGI.hpp"

using namespace std;

class Response {
    private:
    public:
        Request2 *request;
        int client;
        string response_line;
        string response_headers;
        ifstream bodyFile;
        char *body;
        bool responseReady;
        bool responseSent;
        long contentLength;
        GlobalConfig *config;
        string path;
        bool& cgi;


        Response(Request2 *request);


        // Function to check if a file exists at the given path
        bool fileExists(const std::string& filePath);

        void setRequest(Request2 *request);

        std::vector<std::string> splitString(const std::string& input);

        void sendErrorResponse(int errorCode);

        bool isDirectory(const std::string& path) ;

        // Handle directory listing
        std::string generateDirectoryListing(const std::string& directoryPath);

        bool handleIndexFile(const std::string& path);

        void handleDirectoryRequest(const std::string& directoryPath, int clientSocket);

        void sendGetResponse();

        void sendPostResponse();

        std::string normalizePath(const std::string& path);

        bool isPathWithinDirectory(const std::string& path, const std::string& directory);

        bool deleteFile(const std::string& filePath);

        void sendDeleteResponse();

        void sendCGIResponse();

        void sendResponse();

        std::string getMimeType(const std::string& filePath);

        bool isValidFileDescriptor();

        void reset();

        ~Response();
};

#endif