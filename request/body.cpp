#include "request.hpp"

void Request2::handleUnchunkedBody (const char* buffer, size_t bufferLength, size_t pos) {
    // if it s the first time we read the body, we create a file to hold it
    if (fileCreated == false) {
        file = generateUniqueFilename() + getMimeType();
        filePath = getPath() + "/";
        file = filePath + file;
        fileCreated = true;
        bodyFile.open(file.c_str(), ios::out | ios::trunc);
        if (!bodyFile.is_open()) {
            cerr << "Error: Unable to open file " << file << " for writing." << endl;
            if (access(filePath.c_str(), F_OK) == -1) {
                cerr << "Error: Directory " << filePath << " does not exist." << endl;
            }
            if (access (filePath.c_str(), W_OK) == -1) {
                cerr << "Error: Directory " << filePath << " is not writable." << endl;
                status = 403;
            }
            else
                status = 500;
            state = REQUEST_LINE;
            fileCreated = false;
            chunkState = CHUNK_SIZE;
            requestFinished = true;
            fileSize = 0;
            return;
        }
        bodyFile.close();
    }
    // Non-chunked transfer, content_length must be set beforehand
    if (bodyFile) {
        /* when the buffersize is smaller than the position of the /r/n/r/n
         in the request line (as you remember we read it fully before parsing it and getting in here)
         i do this calculation to determine it s position in the buffer considering
         it s overall position in the request line, if the overall position is not 0 but the % is 0, it means that the true position
         is at the end of the line so hence the return */
        int p_pos = pos;
        pos  = pos % bufferLength;
        if (pos == 0 && p_pos != 0 && bufferLength != p_pos) {
            return;
        }

        // chose how much to write from the body, weither the remaining body or the remaining buffer
        size_t writeSize = std::min(static_cast<size_t>(content_length), bufferLength - pos);

        bodyFile.open(file.c_str(), ios::out | ios::app);
        if (!bodyFile.is_open()) {
            if (access (file.c_str(), W_OK) == -1) {
                cerr << "Error: Directory " << filePath << " is not writable." << endl;
                status = 403;
            }
            else
                status = 500;
            cerr << "Error: Unable to open file " << file << " for writing." << endl;
            state = REQUEST_LINE;
            fileCreated = false;
            chunkState = CHUNK_SIZE;
            requestFinished = true;
            fileSize = 0;
            return;
        }
        bodyFile.write(buffer + pos, writeSize);
        if (bodyFile.fail()) {
            cerr << "Error: Unable to write to file " << file << endl;
            if (access (file.c_str(), W_OK) == -1) {
                cerr << "Error: Directory " << filePath << " is not writable." << endl;
                status = 403;
            }
            else
                status = 500;
            state = REQUEST_LINE;
            fileCreated = false;
            chunkState = CHUNK_SIZE;
            requestFinished = true;
            fileSize = 0;
            bodyFile.close();
            return;
        }
        
        bodyFile.flush();
        bodyFile.close();
        // decrease the content length by the amount written
        content_length -= writeSize;
    }
    if (content_length <= 0) {
        // End of message
        cout << "File created: " << file << endl;
        state = REQUEST_LINE;
        fileCreated = false;
        status = 201;
        requestFinished = true;
    }
}


long hexStringToLong(const std::string& hexStr) {
    std::istringstream iss(hexStr);
    long result;
    iss >> std::hex >> result;
    return result;
}

void Request2::handleChunkedBody(const char* buffer, size_t bufferLength, size_t pos) {
    // same as in unchunked
    if (fileCreated == false) {
        file = generateUniqueFilename() + getMimeType();
        filePath = getPath() + "/";
        file = filePath + file;
        fileCreated = true;
        bodyFile.open(file.c_str(), ios::out | ios::trunc);
        if (!bodyFile.is_open()) {
            cerr << "Error: Unable to open file " << file << " for writing." << endl;
            if (access(filePath.c_str(), F_OK) == -1) {
                cerr << "Error: Directory " << filePath << " does not exist." << endl;
            }
            if (access (file.c_str(), W_OK) == -1) {
                cerr << "Error: Directory " << file << " is not writable." << endl;
                status = 403;
            }
            else
                status = 500;
            state = REQUEST_LINE;
            fileCreated = false;
            chunkState = CHUNK_SIZE;
            requestFinished = true;
            fileSize = 0;
            return;
        }
        bodyFile.close();
        fileSize = 0;
    }
    bodyFile.open(file.c_str(), ios::out | ios::app);
    if (!bodyFile.is_open()) {
        if (access (file.c_str(), W_OK) == -1) {
            cerr << "Error: file " << file << " is not writable." << endl;
            status = 403;
        }
        else
            status = 500;
        state = REQUEST_LINE;
        fileCreated = false;
        chunkState = CHUNK_END;
        requestFinished = true;
        fileSize = 0;
        return;
    }

    // same cause as in unchunked
    if (chunkState == CHUNK_END) {
        int p_pos = pos;
        pos  = pos % bufferLength;
        if (pos == 0 && p_pos != 0 && bufferLength != p_pos) {
            return;
        }
        i = pos;
        chunkState = CHUNK_SIZE;
    }
    else i = 0;

    // here we hundle the body charachter by charachter in size and chunks in data
    while (i < bufferLength) {
        if (chunkState == CHUNK_SIZE) {
            if (buffer[i] == '\r') {
                // Skip '\r'.
                i++;
                continue;
            } else if (buffer[i] == '\n') {
                // Process chunk size after reaching '\n'.
                if (!chunkSizeStr.empty()) {
                    chunkSize = hexStringToLong(chunkSizeStr);
                    chunkSizeStr.clear();
                    if (chunkSize == 0) {
                        // Handle end of all chunks.
                        state = REQUEST_LINE;
                        status = 203;
                        fileCreated = false;
                        chunkState = CHUNK_END;
                        requestFinished = true;
                        fileSize = 0;
                        bodyFile.close();
                        return;
                    }
                    chunkState = CHUNK_DATA;
                    fileSize += chunkSize;
                }
                i++; // Move past '\n'.
                continue;
            } else {
                // Accumulate hex digits for chunk size.
                chunkSizeStr += buffer[i];
            }
        } else if (chunkState == CHUNK_DATA) {
            // determine how much is still not readed from the buffer
            size_t remainingDataInBuffer = bufferLength - i;
            // chose how much to write from the body, weither the remaining body or the remaining buffer based on the smallest of them
            size_t dataToWrite = std::min(static_cast<size_t>(chunkSize), remainingDataInBuffer);

            // Write the chunk data to the file.
            bodyFile.write(buffer + i, dataToWrite);
            if (bodyFile.fail()) {
                if (access (file.c_str(), W_OK) == -1) {
                    cerr << "Error: file " << file << " is not writable." << endl;
                    status = 403;
                }
                else
                    status = 500;
                state = REQUEST_LINE;
                fileCreated = false;
                chunkState = CHUNK_END;
                requestFinished = true;
                fileSize = 0;
                bodyFile.close();
                return;
            }
            // decrease the content length by the amount written
            chunkSize -= dataToWrite;
            // increase the position in the buffer by the amount written
            i += dataToWrite;

            fileSize += dataToWrite;
            // flush the buffer to the file to make sure it s written to the file and not just in the buffer in the memory
            bodyFile.flush();

            // Check if the end of the chunk was reached.
            if (chunkSize == 0) {
                // Expect the next chunk size after an additional CRLF.
                if (buffer[i] == '\n' && lastchar == '\r') {
                    chunkState = CHUNK_SIZE; // Prepare for the next chunk.
                    lastchar = 0;
                    i++; // Skip the '\n' following '\r'.
                    continue;
                }
                lastchar = buffer[i];
            }
            else i--; // Adjust because of i++ in the loop. (cause when i add the written size ill be at the next charachter to write, but it will be skipped by the next i++)
        }
        i++;
    }
    bodyFile.close();
}

void Request2::handleBody(const char* buffer, size_t bufferLength, size_t pos) {
    if (method == "GET" || method == "DELETE") {
        state = REQUEST_LINE;
        requestFinished = true;
        return;
    }
    if (chunked) {
        handleChunkedBody(buffer, bufferLength, pos);
    }

    else if (content_length >= 0 && !chunked) {
        handleUnchunkedBody(buffer, bufferLength, pos);
    }
}