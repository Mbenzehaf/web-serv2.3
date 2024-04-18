#include "response.hpp"

Response::Response(Request2 *request) : cgi(request->cgi) {
    this->request = request;
    client = request->client;
    this->config = request->config;
    this->responseSent = true;
    this->responseReady = true;
}

std::string ToString(int number) {
    std::stringstream ss;
    ss << number;
    return ss.str();
}

void Response::setRequest(Request2 *request) {
    this->request = request;
}

bool Response::isDirectory(const std::string& path) {
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) != 0) {
        return false;
    }
    return S_ISDIR(fileInfo.st_mode);
}

// Function to check if a file exists at the given path
bool Response::fileExists(const std::string& filePath) {
    // if (isDirectory(filePath)) {
    //     return false;
    // }
    //cout << "Checking if file exists: " << filePath << endl;
    std::ifstream file(filePath.c_str());
    return file.good();
}

void Response::sendErrorResponse(int errorCode) {
    if (errno == EBADF) {
        //cout << "Invalid file descriptor" << endl;
        return ;
    }
    cout << "Sending error response: " << errorCode <<  endl;
    //return ;
    string response;
    string errorBody;
    char *body = new char[2000 + 1];
    string errorPath = "error_pages/" + ToString(errorCode) + ".html";
    if (fileExists(errorPath)) {
        //cout << "Error file exists: " << errorPath << endl;
        // getchar();
        //cout << "1" << endl;
        bodyFile.open(errorPath.c_str());
        //cout << "2" << endl;
        if (bodyFile.is_open()) {
            response_line = "HTTP/1.1 " + ToString(errorCode) + " Not Found\r\n";
            response_headers = "Content-Type: text/html\r\n";
            bodyFile.seekg(0, ios::end); // Move to the end of the file
            contentLength = bodyFile.tellg(); // Get the current position (which is the file size)
            bodyFile.seekg(0, ios::beg); // Move back to the beginning of the file
            response_headers += "Content-Length: " + ToString(contentLength) + "\r\n";
            response_headers += "Connection: keep-alive\r\n";
            response_headers += "\r\n";
            response = response_line + response_headers;
            ////cout << "3" << endl;

            int check = send(request->client, response.c_str(), response.size(), 0);
            ////cout << "4" << endl;
            if (check == -1) {
                perror("Error sending response");
                responseReady = true;
                responseSent = true;
                bodyFile.close();
                delete[] body;
                return;
            }
            if (contentLength > 0) {
                bodyFile.read(body, 2000);
                //bodyFile.flush();
                // if (bodyFile.fail()) {
                //     perror("2-Error reading file");
                //     responseReady = true;
                //     responseSent = true;
                //     bodyFile.close();
                //     delete[] body;
                //     return;
                // }
                ////cout << "5" << endl;
                int readBytes = bodyFile.gcount();
                check = send(request->client, body, readBytes, 0);
                if (check == -1) {
                    perror("Error sending response");
                    responseReady = true;
                    responseSent = true;
                    bodyFile.close();
                    delete[] body;
                    return;
                }
                contentLength -= readBytes;
            }
            // this is for acceding the first time to header creation
            responseReady = true;
            // this is for acceding the responce, if it s true than the responce is finished and it will wait for a new request
            responseSent = true;
            bodyFile.close();
        } else {
            sendErrorResponse(505);
        }
    } else {
        sendErrorResponse(505);
    }
    delete[] body;
}


// Handle directory listing
std::string Response::generateDirectoryListing(const std::string& directoryPath) {
    std::stringstream listing;
    ////cout << "directoryPath: " << directoryPath << endl;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir) {
        listing << "<html><head><title>Directory Listing</title></head><body><h1>Directory Listing</h1><ul>";
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string filename = entry->d_name;
            std::string fullPath = directoryPath + "/" + filename;

            struct stat fileStat;
            if (stat(fullPath.c_str(), &fileStat) == 0 && S_ISDIR(fileStat.st_mode)) {
                filename += "/";
            }

            std::string url;
            if (!directoryPath.empty() && directoryPath[directoryPath.length() - 1] == '/') {
                url = request->originalPath + filename;
            } else {
                url = request->originalPath + "/" + filename;
            }
            // //cout << "request->path: " << request->originalPath << endl;
            ////cout << "filename: " << filename << endl;   
            // //cout << "url: " << url << endl;
            //getchar();

            listing << "<li><a href=\"" << url << "\">" << filename << "</a></li>";
        }
        listing << "</ul></body></html>";
        closedir(dir);
    } else {
        listing << "<html><head><title>Error</title></head><body><h1>Error</h1><p>Failed to open directory.</p></body></html>";
    }
    return listing.str();
}


// Split a string into multiple based on whitespace
std::vector<std::string> Response::splitString(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool Response::handleIndexFile(const std::string& path) {
    string indexs;
    string host = request->getHost();
    if (config->servers.find(host) != config->servers.end()) {
        if (config->servers[host].locations.find(request->location) != config->servers[host].locations.end()) {
            if (config->servers[host].locations[request->location].config.find("default_page") != config->servers[host].locations[request->location].config.end()) {
                indexs = config->servers[host].locations[request->location].config["default_page"].at(0);
            }
            else if (config->servers[host].config.find("default_page") != config->servers[host].config.end()) {
                                    indexs = config->servers[host].config["default_page"].at(0);
            }
            else if (config->config.find("default_page") != config->config.end()) {
                indexs = config->config["default_page"].at(0);
            }
        }
        else if (config->servers[host].config.find("default_page") != config->servers[host].config.end()) {
            indexs = config->servers[host].config["default_page"].at(0);
        }
        else if (config->config.find("default_page") != config->config.end()) {
            indexs = config->config["default_page"].at(0);
        }
    }
    if (indexs.empty()) {
        return false;
    }
    std::vector<std::string> indexFiles = splitString(indexs);
    for (size_t i = 0; i < indexFiles.size(); i++) {
        if (fileExists(path + "/" + indexFiles[i])) {
            this->path = path + "/" + indexFiles[i];
            // //cout << "this->path: " << this->path << endl;
            return true;
        }
    }
    return false;
}

void Response::handleDirectoryRequest(const std::string& directoryPath, int clientSocket) {
    // check auto_index
    string host = request->getHost();
    if (config->servers.find(host) != config->servers.end()) {
        if (config->servers[host].locations.find(request->location) != config->servers[host].locations.end()) {
            if (config->servers[host].locations[request->location].config.find("autoindex") != config->servers[host].locations[request->location].config.end()) {
                if (config->servers[host].locations[request->location].config["autoindex"].at(0) == "off") {
                    sendErrorResponse(404);
                    return;
                }
            }
            else if (config->servers[host].config.find("autoindex") != config->servers[host].config.end()) {
                if (config->servers[host].config["autoindex"].at(0) == "off") {
                    sendErrorResponse(404);
                    return;
                }
            }
            else if (config->config.find("autoindex") != config->config.end()) {
                if (config->config["autoindex"].at(0) == "off") {
                    sendErrorResponse(404);
                    return;
                }
            }
        }
        else if (config->servers[host].config.find("autoindex") != config->servers[host].config.end()) {
            if (config->servers[host].config["autoindex"].at(0) == "off") {
                sendErrorResponse(404);
                return;
            }
        }
        else if (config->config.find("autoindex") != config->config.end()) {
            if (config->config["autoindex"].at(0) == "off") {
                sendErrorResponse(404);
                return;
            }
        }
    }
    // end check auto_index
    std::string directoryListing = generateDirectoryListing(directoryPath);
    std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + ToString(directoryListing.size()) + "\r\n\r\n" + directoryListing;
    send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);
    responseSent = true;
    responseReady = true;
}

void Response::sendGetResponse() {
    string response;
    // path = request->requestPath + request->path;
    path = request->path;
    static int counter;
    long readBytes;
    char* body = new char[BUFFER_SIZE + 1];
    // Check if the file exists
    if (fileExists(path)) {
        if (isDirectory(path)) {
            // Check if the index file exists first
            if (!handleIndexFile(path)) {
                handleDirectoryRequest(path, request->client);
                delete[] body;
                return;
            }
        }
        bodyFile.open(path.c_str());
        if (bodyFile.is_open() && responseReady) {
            counter = 0;
            response_line = "HTTP/1.1 200 OK\r\n";
            response_headers = "Content-Type: " + getMimeType(path) + "\r\n";
            usleep (1000);
            ////cout << "content type: " << getMimeType(path) << endl;
            bodyFile.seekg(0, ios::end); // Move to the end of the file
            contentLength = bodyFile.tellg(); // Get the current position (which is the file size)
            bodyFile.seekg(0, ios::beg); // Move back to the beginning of the file
            // ofstream file("outfile", ios::app | ios::binary);
            // while (bodyFile.read (body, 1)) {
            //     file << body;
            // }
            // file.close();
            //getchar();

            response_headers += "Content-Length: " + ToString(contentLength) + "\r\n";
            cout << "contentLength: " << contentLength << endl;
            // response_headers += "Connection: keep-alive\r\n";
            response_headers += "\r\n";
            response = response_line + response_headers;
            int check = send(request->client, response.c_str(), response.size(), 0);
            if (check == -1) {
                perror("Error sending response");
                bodyFile.close();
                sendErrorResponse(505);
                delete[] body;
                return ;
            }

            if (contentLength > 0) {

                // this is for acceding the first time to header creation
                bodyFile.seekg(0);
                long reading = std::min((long)BUFFER_SIZE, contentLength);
                bodyFile.read(body, reading);
                // if (bodyFile.fail()) {
                //     bodyFile.close();
                //     sendErrorResponse(505);
                //     delete[] body;
                //     return ;
                // }
                //read (bodyFile, body, BUFFER_SIZE);
                readBytes = bodyFile.gcount();
                counter += readBytes;
                body[readBytes] = '\0'; // Null-terminate the buffer
                readBytes = std::min(readBytes, contentLength);
                ////cout << "readBytes: " << readBytes << endl;
                int bytes = send(request->client, body, readBytes, 0);
                if (bytes == -1) {
                    perror("Error sending response");
                    bodyFile.close();
                    sendErrorResponse(505);
                    delete[] body;
                    return ;
                }
                contentLength -= readBytes;
                responseReady = false;
                
            }
            if (contentLength <= 0) {
                // this is for acceding the first time to header creation
                responseReady = true;
                // this is for acceding the responce, if it s true than the responce is finished and it will wait for a new request
                responseSent = true;
                counter = 0;
            }
            bodyFile.close();

        } else if (!responseReady && bodyFile.is_open()) {
            //cout << "im actually here " << endl;
            bodyFile.seekg(counter);
            long reading = std::min((long)BUFFER_SIZE, contentLength);
            bodyFile.read(body, reading);
            if (bodyFile.fail()) {
                bodyFile.close();
                perror("Error reading file 2");
                sendErrorResponse(505);
                delete[] body;
                return ;
            }
            //cout << "1" << endl;
            readBytes = bodyFile.gcount();
            counter += readBytes;
            body[readBytes] = '\0'; // Null-terminate the buffer
            readBytes = std::min(readBytes, contentLength);
            //cout << "2" << endl;
            ofstream file("outfile", ios::app | ios::binary);
            file << body;
            file.close();
            //cout << "readBytes: " << readBytes << endl;
            int bytes = send(request->client, body, readBytes, 0);
            if (bytes == -1) {
                bodyFile.close();
                perror("Error sending response 2");
                sendErrorResponse(505);
                delete[] body;
                return ;
            
            }
            usleep (100);
            contentLength -= readBytes;
            ////cout << "content_l: " << contentLength << endl;
            if (contentLength <= 0) {
                // this is for acceding the first time to header creation
                responseReady = true;
                // this is for acceding the responce, if it s true than the responce is finished and it will wait for a new request
                responseSent = true;
                counter = 0;
            }
            bodyFile.close();
        } else {
            //delete[] body;
            cerr << "Error opening file: " << path << endl;
            sendErrorResponse(505);
        }
    } else {
        //delete[] body;
        // here where we need to check permissions too
        cerr << "File not found: " << path << endl;
        sendErrorResponse(404);
    }
    delete[] body;
}

void Response::sendPostResponse() {
    // Check if the file exists
    string response;
    if (request->status == 201) {
        ifstream bodyFile;
        bodyFile.open(request->file.c_str());
        if (bodyFile.is_open() || request->file.empty()) {
            if (bodyFile.is_open()) {
                bodyFile.seekg(0, ios::end); // Move to the end of the file
                contentLength = bodyFile.tellg(); // Get the current position (which is the file size)
                bodyFile.seekg(0, ios::beg); // Move back to the beginning of the file
                if (contentLength != request->fileSize) {
                    ////cout << "contentLength: " << contentLength << " request->fileSize: " << request->fileSize << endl;
                    sendErrorResponse(505);
                    bodyFile.close();
                    return;
                }
            }
            response_line = "HTTP/1.1 201 Created\r\n";
            response_headers = "Content-Type: text/plain\r\n";
            response_headers += "Location: " + request->file + "\r\n";
            response_headers += "Content-Length: 18\r\n\r\n";
            response_headers += "Everything is good";
            response = response_line + response_headers;
            if (send(request->client, response.c_str(), response.size(), 0) == -1) {
                perror("Error sending response");
                sendErrorResponse(505);
                return ;
            }
            //cout << "Response sent" << endl;
            responseSent = true;
            bodyFile.close();
        } else {
            sendErrorResponse(404);
        }
    }
    else if (request->status == 500) {
        sendErrorResponse(505);
    }
    else {
        cout << request->status << endl;
        sendErrorResponse(404);
    }
}

std::string Response::normalizePath(const std::string& path) {
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string component;

    // Split the path into components
    while (std::getline(ss, component, '/')) {
        if (component.empty() || component == ".") {
            // Skip empty components and "."
            continue;
        }
        if (component == "..") {
            // If encountering "..", remove the last component
            if (!components.empty()) {
                components.pop_back();
            }
        }
        else
            components.push_back(component);
    }

    // Reconstruct the normalized path
    std::stringstream normalizedPath;
    for (size_t i = 0; i < components.size(); i++) {
        normalizedPath << '/' << components[i];
    }
    return normalizedPath.str();
}

bool Response::isPathWithinDirectory(const std::string& path, const std::string& directory) {
    std::string normalizedPath = normalizePath(path);
    return normalizedPath.find(directory) == 0;
}

bool Response::deleteFile(const std::string& filePath) {
    // Attempt to delete the file using the remove function
    if (!isPathWithinDirectory(filePath, request->getPath())) {
        std::cerr << "Error deleting file: " << filePath << " (not within directory)" << std::endl;
        return false;
    }
    if (remove(filePath.c_str()) != 0) {
        std::cerr << "Error deleting file: " << filePath << std::endl;
        //Error 403 forbidden
        return false;
    } 
    //cout << "File deleted successfully: " << filePath << std::endl;
    return true;
}

void Response::sendDeleteResponse() {
    // Check if the file exists
    string response;
    if (request->status == 200) {
        // if (fileExists((request->getPath() + "/" + request->path))) {
        if (fileExists((request->path))) {
            // if (deleteFile((request->getPath() + "/" + request->path).c_str())) {
            if (deleteFile((request->path).c_str())) {
                response_line = "HTTP/1.1 204 No Content\r\n";
                response_headers = "Content-Type: text/plain\r\n";
                response_headers += "Content-Length: 18\r\n\r\n";
                response_headers += "Everything is good";
                response = response_line + response_headers;
                if (send(request->client, response.c_str(), response.size(), 0) == -1) {
                    perror("Error sending response");
                    sendErrorResponse(505);
                    return ;
                }
                //cout << "Response sent" << endl;
                responseSent = true;
            } else {
                sendErrorResponse(505);
            }
        }
        else {
            sendErrorResponse(404);
        }
    }
    else if (request->status == 500) {
        sendErrorResponse(505);
    }
    else {
        sendErrorResponse(404);
    }
}

void Response::sendCGIResponse() {
    cout << "why am i here?" << endl;
    if (request->method == "POST") {
        std::vector<std::string> envVars;
        envVars.push_back("SCRIPT_FILENAME=" + request->path);
        envVars.push_back("REQUEST_METHOD=POST");
        envVars.push_back("QUERY_STRING=" + request->query);
        envVars.push_back("CONTENT_LENGTH=" + request->fileSize);
        std::string requestContent = request->file;
        int position = request->path.find_last_of(".");
        std::string ex;
        if (position != std::string::npos)
            ex = request->path.substr(request->path.find_last_of("."));
        else
            ex = "";
        std::string uploadTo = request->getPath();
        try {
            CGI cgiHandler(request->method, envVars, ex, requestContent.size(), uploadTo, requestContent, request->file);
            cgiHandler.execute_script();
            request->path = cgiHandler.getOutputPath();
            if (responseReady) {
                bodyFile.open(request->path.c_str());
                bodyFile.seekg(0, ios::end); // Move to the end of the file
                contentLength = bodyFile.tellg(); // Get the current position (which is the file size)
                bodyFile.seekg(0, ios::beg); // Move back to the beginning of the file
                bodyFile.close();
            }
            responseReady = false;
            sendGetResponse();
        } catch (const CGI::CGIEX& e) {
            std::cerr << "CGI exception occurred: " << e.what() << std::endl;
            sendErrorResponse(505);
        }
    }
    else if (request->method == "GET") {
        std::vector<std::string> envVars = {
            "SCRIPT_FILENAME=" + request->path,
            "REQUEST_METHOD=GET",
            "QUERY_STRING=" + request->query
        };
        std::string requestContent = request->query;
        int position = request->path.find_last_of(".");
        std::string ex;
        if (position != std::string::npos)
            ex = request->path.substr(request->path.find_last_of("."));
        else
            ex = "";
        std::string uploadTo = request->getPath();
        try {
            CGI cgiHandler(request->method, envVars, ex, requestContent.size(), uploadTo, requestContent, request->file);
            //cgiHandler.execute_script();
            request->path = cgiHandler.getOutputPath();
            if (responseReady) {
                bodyFile.open(request->path.c_str());
                bodyFile.seekg(0, ios::end); // Move to the end of the file
                contentLength = bodyFile.tellg(); // Get the current position (which is the file size)
                bodyFile.seekg(0, ios::beg); // Move back to the beginning of the file
                bodyFile.close();
            }
            responseReady = false;
            sendGetResponse();
        } catch (const CGI::CGIEX& e) {
            std::cerr << "CGI exception occurred: " << e.what() << std::endl;
            sendErrorResponse(505);
        }
    }
    else {
        sendErrorResponse(403);
    
    }
}

void Response::sendResponse() {
    //getchar();
    if (responseSent) {
        return;
    }
    if (cgi) {
        cout << "from here :" << endl;
        sendCGIResponse();
        return;
    }
    if (request->method == "GET") {
        sendGetResponse();
    } else if (request->method == "POST") {
        sendPostResponse();
    } else if (request->method == "DELETE") {
            sendDeleteResponse();
    } else {
        sendErrorResponse(404);
    }
}


std::string Response::getMimeType(const std::string& filePath) {
    // Map file extensions to MIME types
    // std::map<std::string, std::string> mimeTypes {
    //     {".html", "text/html"},
    //     {".htm", "text/html"},
    //     {".txt", "text/plain"},
    //     {".jpg", "image/jpeg"},
    //     {".jpeg", "image/jpeg"},
    //     {".png", "image/png"},
    //     {".gif", "image/gif"},
    //     {".pdf", "application/pdf"},
    //     {".css", "text/css"},
    //     {".js", "application/javascript"},
    //     {".mp3", "audio/mpeg"},
    //     {".mp4", "video/mp4"},
    //     {".ico", "image/x-icon"},
    //     // Add more mappings as needed
    // };
    std::map<std::string, std::string> mimeTypes = request->getMimeTypes();
    string extension;

    // Find the file extension
    size_t pos = filePath.find_last_of('.');
    if (pos != std::string::npos) {
        std::string ext = filePath.substr(pos);
        // Check if the extension exists in the map
        // std::map<string, string>::iterator it = mimeTypes.find(ext);
        // if (it != mimeTypes.end()) {
        //     return it->second; // Return the corresponding MIME type
        // }
        for ( std::map<string, string>::iterator it = mimeTypes.begin(); it != mimeTypes.end(); ++it ) {
            if (it->first == ext) {
                return it->second;
            }
        }
    }

    // Default MIME type if extension is not found
    return "application/octet-stream";
}

bool Response::isValidFileDescriptor() {
    if (errno == PIPE_BUF) {
        //cout << "valid file descriptor" << endl;
        return true;
    } else {
        // Check errno to determine the type of error
        if (errno == EBADF) {
            // EBADF indicates that fd is not a valid file descriptor
            return false;
        } else {
            // Handle other errors if needed
            return false;
        }
    }
}

void Response::reset() {
    if (bodyFile.is_open()) {
        bodyFile.close();
    }
    responseReady = true;
    responseSent = true;
    response_line = "";
    response_headers = "";
    contentLength = 0;
    path = "";
}

Response::~Response() {
    if (bodyFile.is_open()) {
        bodyFile.close();
    }
}