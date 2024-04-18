#include "request.hpp"

Request2::Request2(int& client, GlobalConfig *config) {
    this->client = client;
    this->config = config;
    state = REQUEST_LINE;
    headersComplete = false;
    bodyComplete = true;
    chunked = false;
    content_length = LONG_MIN;
    header_length = 0;
    chunkSize = 0;
    chunkState = CHUNK_SIZE;
    fileCreated = false;
    fileSize = 0;
    lastchar = 0;
    limit = 0;
    i = 0;
    requestFinished = false;
    keepAlive = false;
    cgi = false;
    status = 200;
}

string Request2::getrequest() {
    return request;
}

void Request2::setrequest(const string req) {
    request = req;
}

bool Request2::containsCRLF(const char* str) {
    return strstr(str, "\r\n") != NULL;
}

std::string Request2::trim(const std::string& str) {
    // Find the first non-whitespace character
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos) {
        // If the string is all whitespaces, return an empty string
        return "";
    }

    // Find the last non-whitespace character
    size_t end = str.find_last_not_of(" \t");
    
    // Return the substring containing non-whitespace characters
    return str.substr(start, end - start + 1);
}

std::string Request2::to_lower(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        result.push_back(std::tolower(str[i]));
    }
    return result;
}

std::string Request2::generateUniqueFilename() {
    static int counter = 0; // Static variable to keep track of the counter
    std::time_t currentTime = std::time(NULL);

    std::ostringstream filenameStream;
    filenameStream << "file_" << currentTime << "_" << counter;
    counter++;

    return filenameStream.str();
}

string Request2::getMimeType() {
    std::map<std::string, std::string> mimeTypes = getMimeTypes();
    string extension;

    if (headers.find("content-type") != headers.end()) {
        extension = headers["content-type"];
    }
    for (std::map<std::string, std::string>::iterator it = mimeTypes.begin(); it != mimeTypes.end(); ++it) {
        if (it->second == extension) {
            return it->first;
        }
    }
    return "";
}



long stringToLong(const std::string& str) {
    std::istringstream iss(str);
    long result;
    if (!(iss >> result)) {
        cerr << "check your size again." << endl;
    }
    return result;
}

void Request2::handleHeaders (std::string& request, size_t& bodyStart) {
    std::istringstream requestStream(request);
    std::string line;
    header_length = 0;
    if (state == REQUEST_LINE) {
        std::getline(requestStream, line);
        std::istringstream lineStream(line);
        lineStream >> method >> path >> version;
        parsePath(path);
        // check Error
        state = HEADERS;
        header_length = line.length() + 1;
        chunkState = CHUNK_END;
        fileSize = 0;
    }
    while (state == HEADERS && std::getline(requestStream, line) ) {
        // if the readed line has only /r than it s an emty one, which indicate the end of the headers
        if (line[0] == '\r') {
            cout << "changed to BODY" << endl;
            headersComplete = true;
            requestPath = getPath();
            state = BODY;
            request.clear();
            break;
        }

        // Parse the header by triming spaces of the key and making it into lower case, and the value trimed only as it might be case sensitive
        try {
            std::string::size_type pos = line.find(':');
            header = to_lower(trim (line.substr(0, pos)));
            headers[header] = trim (line.substr(pos + 2));
            headers[header].erase(headers[header].size() - 1);
        } catch (const std::out_of_range& e) {
            cerr << "1-Error: " << e.what() << endl;
            return ;
        }
        
        if (header == "transfer-encoding" && headers[header] == "chunked") {
            chunked = true;
        }
        if ( header == "content-length" ) {
            content_length = stringToLong(headers[header]);
            fileSize = content_length;
        }
        if (header == "connection" && to_lower(headers[header]) == "keep-alive") {
            keepAlive = false;
        }
        // this value doesnt really matter now but well keep it for now
        header_length += line.length() + 1;
        usleep(100);
    }
    // this one will get me the location block th epath falls under
    location = getLocation(path);
    // this is to initialise cgi boolean
    cgi = isCGIAllowed();

    /* bodyStart holds the position of the first appearance of /r/n/r/n 
    so i skip it by 4 to get the start of the body in unchucked mode, and 2 in chunked mode */
    if (chunked) {
        bodyStart += 2;
    }
    else if (!chunked)
        bodyStart += 4;
}

void Request2::parse(int bufferlength) {
    // static std::string request;
    size_t bodyStart = 0;
    if (state != BODY ) {
        /* here i append what i recieved on the request string than see if i find the end of the headers indication string "/r/n/r/n"
        if i find it i parse the headers as a single string which is much easier than when it might not reach fully in one recieve */
        requestLine.append(buffer, bufferlength);
        bodyStart = requestLine.find("\r\n\r\n");
        cout << "bodyStart = " << requestLine << endl;
        if (bodyStart == std::string::npos) {
            bodyStart = 0;
            return ;
        }
        else {
            handleHeaders(requestLine, bodyStart);
        }
    }
    if (state == BODY) {
        handleBody(buffer, bufferlength, bodyStart);
    }
}


void Request2::parsePath(string path) {
    size_t query_pos = path.find("?");
    size_t fragment_pos = path.find("#");
    if (query_pos != string::npos) {
        if (fragment_pos != string::npos) {
            query = path.substr(query_pos + 1, fragment_pos - query_pos - 1);
            this->path = path.substr(0, query_pos);
            fragment = path.substr(fragment_pos + 1);
        }
        else {
            query = path.substr(query_pos + 1);
            this->path = path.substr(0, query_pos);
        }
    }
    else if (fragment_pos != string::npos) {
        fragment = path.substr(fragment_pos + 1);
        this->path = path.substr(0, fragment_pos);
    }
    else {
        this->path = path;
    }
    this->originalPath = this->path;
}

// Function to get the Path from the config file
string Request2::getPath() {
    string root;
    string host = getHost();
    
    if (config->servers.find(host) != config->servers.end()) {
        if (config->servers[host].locations.find(this->path) != config->servers[host].locations.end() 
            && config->servers[host].locations[this->location].config.find("root") != config->servers[host].locations[this->location].config.end()) {
            return config->servers[host].locations[this->location].config["root"].at(0);
        }
        else {
            if (config->servers[host].config.find("root") != config->servers[host].config.end())                        
                return config->servers[host].config["root"].at(0);
        }
    }
    else {
        if (config->getGlobalConfig().find("root") != config->getGlobalConfig().end()) {
            std::map<std::string,std::vector<std::string> > global = config->getGlobalConfig();
            return (global["root"].at(0));
        }
        else {
            return "";
        }
    }
    return "";
}

// Function to see if cgi allowed
bool Request2::isCGIAllowed() {
    string host = getHost();
    if (config->servers.find(host) != config->servers.end()) {
        if (config->servers[host].locations.find(this->location) != config->servers[host].locations.end() 
            && config->servers[host].locations[this->location].config.find("cgi") != config->servers[host].locations[this->location].config.end()) {
            return config->servers[host].locations[this->location].config["cgi"].at(0) == "on";
        }
        else {
            if (config->servers[host].config.find("cgi") != config->servers[host].config.end()) {
                return config->servers[host].config["cgi"].at(0) == "on";
            }
        }
    }
    else {
        if (config->getGlobalConfig().find("cgi") != config->getGlobalConfig().end()) {
            return config->config["cgi"].at(0) == "on";
        }
    }
    return false;
}

    // Function to print all headers
void Request2::printHeaders() const {
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        cout << it->first << ">> " << it->second << std::endl;
    }
}

string Request2::getHost() {
    string path;
    cout << "headers[host] = 1" << endl;
    istringstream ss (headers["host"]);
    cout << "headers[host] = 2" << endl;
    return (getline(ss, path, ':'), path);
}

// getLocationof the path in the config file
string Request2::getLocation(string path) {
    string root;
    string host = getHost();

    if (config->servers.find(host) != config->servers.end()) {
        std::map<std::string, locationConfig >::iterator it = config->servers[host].locations.end();
        it --;
        while (true) {
            size_t pos = path.find(it->first);
            if (pos != string::npos && pos == 0) {
                if (config->servers.find(host) != config->servers.end() && config->servers[host].locations.find(it->first) != config->servers[host].locations.end() 
                    && config->servers[host].locations[it->first].config.find("root") != config->servers[host].locations[it->first].config.end()) {
                    this->path = config->servers[host].locations[it->first].config["root"].at(0) + "/" + path.substr(it->first.length());
                    cout << "1-path = " << this->path << endl;
                    return it->first;
                }
                else if (config->servers.find(host) != config->servers.end() && config->servers[host].config.find("root") != config->servers[host].config.end()) {
                    this->path = config->servers[host].config["root"].at(0) + "/" + path.substr(it->first.length());
                    cout << "2-path = " << this->path << endl;
                    return it->first;
                }
                else if (config->getGlobalConfig().find("root") != config->getGlobalConfig().end()) {
                    this->path = config->config["root"].at(0) + "/" + path.substr(it->first.length());
                    cout << "3-path = " << this->path << endl;
                    return it->first;
                }
                return it->first;
            }
            if (it == config->servers[host].locations.begin())
                break;
            it--;
        } 
    }
    return "/";
}

// Function to get the MIME type of a file
std::map<std::string, std::string> Request2::getMimeTypes() {
    std::map<std::string, std::string> mimeTypes;

    mimeTypes.insert(std::make_pair(".atom", "application/atom+xml"));
    mimeTypes.insert(std::make_pair(".ecma", "application/ecmascript"));
    mimeTypes.insert(std::make_pair(".json", "application/json"));
    mimeTypes.insert(std::make_pair(".bin", "application/octet-stream"));
    mimeTypes.insert(std::make_pair(".pdf", "application/pdf"));
    mimeTypes.insert(std::make_pair(".xhtml", "application/xhtml+xml"));
    mimeTypes.insert(std::make_pair(".xml", "application/xml"));
    mimeTypes.insert(std::make_pair(".zip", "application/zip"));
    mimeTypes.insert(std::make_pair(".mp3", "audio/mpeg"));
    mimeTypes.insert(std::make_pair(".ogg", "audio/ogg"));
    mimeTypes.insert(std::make_pair(".gif", "image/gif"));
    mimeTypes.insert(std::make_pair(".jpeg", "image/jpeg"));
    mimeTypes.insert(std::make_pair(".png", "image/png"));
    mimeTypes.insert(std::make_pair(".css", "text/css"));
    mimeTypes.insert(std::make_pair(".csv", "text/csv"));
    mimeTypes.insert(std::make_pair(".html", "text/html"));
    mimeTypes.insert(std::make_pair(".txt", "text/plain"));
    mimeTypes.insert(std::make_pair(".xml", "text/xml"));
    mimeTypes.insert(std::make_pair(".mp4", "video/mp4"));
    mimeTypes.insert(std::make_pair(".mpeg", "video/mpeg"));
    mimeTypes.insert(std::make_pair(".ogv", "video/ogg"));
    mimeTypes.insert(std::make_pair(".mov", "video/quicktime"));
    mimeTypes.insert(std::make_pair(".webm", "video/webm"));
    mimeTypes.insert(std::make_pair(".avi", "video/x-msvideo"));
    mimeTypes.insert(std::make_pair(".m4v", "video/x-m4v"));
    mimeTypes.insert(std::make_pair(".mkv", "video/x-matroska"));
    mimeTypes.insert(std::make_pair(".wmv", "video/x-ms-wmv"));
    mimeTypes.insert(std::make_pair(".htm", "text/html"));
    mimeTypes.insert(std::make_pair(".asf", "video/x-ms-asf"));
    mimeTypes.insert(std::make_pair(".wmx", "video/x-ms-wmx"));
    mimeTypes.insert(std::make_pair(".wm", "video/x-ms-wm"));
    mimeTypes.insert(std::make_pair(".wmp", "video/x-ms-wmp"));
    mimeTypes.insert(std::make_pair(".wvx", "video/x-ms-wvx"));
    mimeTypes.insert(std::make_pair(".3gp", "video/3gpp"));
    mimeTypes.insert(std::make_pair(".3g2", "video/3gpp2"));
    mimeTypes.insert(std::make_pair(".flv", "video/x-flv"));
    // Add more MIME types here
    return mimeTypes;
}

void Request2::reset () {
    method.clear();
    path.clear();
    version.clear();
    headers.clear();
    state = REQUEST_LINE;
    lineComplete = true;
    headersComplete = false;
    header.clear();
    file.clear();
    bodyComplete = true;
    chunked = false;
    content_length = LONG_MIN;
    header_length = 0;
    if (bodyFile.is_open())
        bodyFile.close();
    chunkSize = 0;
    chunkState = CHUNK_SIZE;
    fileCreated = false;
    fileSize = 0;
    chunkSizeStr.clear();
    lastchar = 0;
    limit = 0;
    i = 0;
    request.clear();
    requestPath.clear();
    location.clear();
    filePath.clear();
    status = 200;
    fileSize = 0;
    query.clear();
    fragment.clear();
    originalPath.clear();
    requestFinished = false;
    keepAlive = false;

}

Request2::~Request2() {
    if (bodyFile.is_open())
        bodyFile.close();
}
