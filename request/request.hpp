#ifndef REQUEST_HPP
#define REQUEST_HPP


class Response;
class GlobalConfig;
#include "../webServ.hpp"
using namespace std;

class Request2 {
    enum State {
        REQUEST_LINE,
        HEADERS,
        BODY
    };
    enum ChunkState {
        CHUNK_SIZE,
        CHUNK_DATA,
        CHUNK_END
    };
    public:
        int client;
        char *buffer;
        bool cgi;
        std::string method;
        std::string path;
        std::string version;
        std::map<std::string, std::string> headers;
        State state;
        bool lineComplete;
        bool headersComplete;
        std::string header;
        string file;
        bool bodyComplete;
        bool chunked;
        long content_length;
        int header_length;
        ofstream bodyFile;
        long chunkSize;
        string chunkSizeStr;
        ChunkState chunkState;
        char lastchar;
        int limit;
        size_t i;
        std::string request;
        std::string requestPath;
        GlobalConfig *config;
        bool fileCreated;
        bool requestFinished;
        string location;
        string filePath;
        int status;
        long fileSize;
        string query;
        string fragment;
        string originalPath;
        bool keepAlive;
        long bodyLength;
        std::string requestLine;
        // std::vector requestNumber;


        Request2(int& client, GlobalConfig *config);

        std::string to_lower(const std::string& str);


        void handleHeaders (std::string& request, size_t& bodyStart);
        void handleChunkedBody (const char* buffer, size_t bufferLength, size_t pos);
        void handleUnchunkedBody (const char* buffer, size_t bufferLength, size_t pos);

        string getrequest();

        void setrequest(const string req);

        bool containsCRLF(const char* str);

        std::string trim(const std::string& str);

        std::string generateUniqueFilename();
        string getMimeType() ;

        void handleBody(const char* buffer, size_t bufferLength, size_t pos);
        void parse(int bufferlength);

        void parsePath(string path);

        // Function to get the Path from the config file
        string getPath();
         // Function to print all headers
        void printHeaders() const;

        string getHost();
        // getLocationof the path in the config file
        string getLocation(string path);

        //Function to see if cgi is allowed
        bool isCGIAllowed();

        // Function to get the MIME type of a file
        std::map<std::string, std::string> getMimeTypes() ;
        void reset ();

        ~Request2();

};

#endif