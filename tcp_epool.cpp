#include "webServ.hpp"
/*
int main(int argc, char **argv) {
    // Parsing config file
    if (argc != 2) {
        std::cout << "Check your input again..." << std::endl;
        return 1;
    }

    GlobalConfig config(argv[1]);
    vector < int > Ports = config.getPorts();
    vector <int> serverSockets;

    // Set up the signal handler for SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    // Listening on all ports
    // create a socket for each port and add it to the epool or a vector and then listen on all of them
    for (size_t i = 0; i < Ports.size(); i++) {
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            perror("Error creating socket");
            return EXIT_FAILURE;
        }
        serverSockets.push_back(serverSocket);
        struct sockaddr_in serverAddress;
        memset( & serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(Ports[i]);
        cout << "1- " << Ports[i] << endl;
        int on = 1;
        int rc = setsockopt(serverSocket, SOL_SOCKET,  SO_REUSEADDR,
                   (char *)&on, sizeof(on));
        if (rc == -1) {
            std::cerr << "Error: setsockopt failed" << std::endl;
            continue;
        }
        if (bind(serverSocket, (struct sockaddr * ) & serverAddress, sizeof(serverAddress)) == -1) {
            perror("Error binding socket");
            close(serverSocket);
            return EXIT_FAILURE;
        }

        if (listen(serverSocket, MAX_CLIENTS) == -1) {
            perror("Error listening on socket");
            close(serverSocket);
            return EXIT_FAILURE;
        }

        std::cout << "Server is listening on port " << Ports[i] << "...\n";
    }

    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("Error creating epoll instance");
        for (size_t i = 0; i < serverSockets.size(); i++) {
            close(serverSockets[i]);
        }
        //close(serverSocket);
        return EXIT_FAILURE;
    }

    // Create a vector to store epoll events for each server socket
    std::vector<struct epoll_event> serverEvents(serverSockets.size());

    // Add each server socket to the epoll instance
    for (size_t i = 0; i < serverSockets.size(); i++) {
        serverEvents[i].events = EPOLLIN;
        serverEvents[i].data.fd = serverSockets[i];
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSockets[i], &serverEvents[i]) == -1) {
            perror("Error adding server socket to epoll instance");
            close(serverSockets[i]);
            close(epollFd);
            return EXIT_FAILURE;
        }
    }

    struct epoll_event events[((MAX_CLIENTS + 1) * serverSockets.size())]; // Events array for epoll_wait

    Client* head = NULL; // Head of the linked list

    while (true) {
        int numEvents = epoll_wait(epollFd, events, ((MAX_CLIENTS + 1) * serverSockets.size()), -1);
        if (numEvents == -1) {
            perror("Error in epoll_wait");
            break;
        }
        //cout << "2- " << numEvents << endl;

        for (int i = 0; i < numEvents; ++i) {
            int fd = events[i].data.fd;
            //usleep (10000);

            vector<int>::iterator it = find(serverSockets.begin(), serverSockets.end(), fd);

            if (it != serverSockets.end()) {
                // New connection
                sockaddr_in clientAddress;
                socklen_t clientAddressLen = sizeof(clientAddress);
                int clientSocket = accept(*it, (struct sockaddr*)&clientAddress, &clientAddressLen);
                if (clientSocket == -1) {
                    perror("Error accepting connection");
                } else {
                    std::cout << "New connection accepted. Client socket: " << clientSocket << std::endl;

                    // Create a new client object
                    //getpeername(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
                    Client* newClient = new Client(clientSocket, clientAddress, clientAddressLen, *it, &config);

                    // Add the new client to the linked list
                    newClient->setNext(head);
                    head = newClient;

                    // Add the new client socket to the epoll instance
                    struct epoll_event event;
                    event.events = EPOLLIN | EPOLLOUT;
                    event.data.fd = clientSocket;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
                        perror("Error adding client socket to epoll instance");
                        close(clientSocket);
                    }
                }
            } else {
                // Data received on a client socket
                Client* current = head;
                while (current != NULL) {
                    if (current->getSocketDescriptor() == fd && (events[i].events & EPOLLIN )){
                       // need to handle pos in chunked too, fuck it....
                        ssize_t bytesRead = recv(fd, current->getBuffer(), BUFFER_SIZE, 0);
                        if (bytesRead <= 0) {
                            cerr << "Client didnt send a thing" << endl;
                            current->counter++;
                            // current->counter++;
                            if (current->counter >= 1000) {
                            // Connection closed
                            std::cout << "Client " << fd << " disconnected.\n";
                            // make the client disappear
                            struct epoll_event event;
                            event.events = 0; // No events to monitor
                            event.data.fd = fd;
                            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                                perror("Error removing client socket from epoll instance");
                            }
                            close(fd);
                            // Remove the client from the linked list
                            if (current == head) {
                                head = current->getNext();
                            } else {
                                Client* prev = head;
                                while (prev->getNext() != current) {
                                    prev = prev->getNext();
                                }
                                prev->setNext(current->getNext());
                            }
                            delete current;
                            i--;
                            break;
                        }
                            // Connection closed or error
                            // if (bytesRead == 0) {
                            //     std::cout << "Client " << fd << " disconnected.\n";
                            // } else {
                            //     perror("Error receiving data");
                            // }
                            // // make the client disappear
                            // struct epoll_event event;
                            // event.events = 0; // No events to monitor
                            // event.data.fd = fd;
                            // if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                            //     perror("Error removing client socket from epoll instance");
                            // }
                            // close(fd);
                            // // Remove the client from the linked list
                            // if (current == head) {
                            //     head = current->getNext();
                            // } else {
                            //     Client* prev = head;
                            //     while (prev->getNext() != current) {
                            //         prev = prev->getNext();
                            //     }
                            //     prev->setNext(current->getNext());
                            // }
                            // delete current;
                            break;
                        } else {
                       current->counter = 0;
                            // Process received data
                            if (current->makeRequest) {
                                // Request2 request2(current->getSocketDescriptor(), &config);
                                Request2 *request2 = new Request2(current->getSocketDescriptor(), &config);
                                Response *response = new Response(request2);
                                request2->buffer = current->getBuffer();
                                current->requests.push_back(make_pair(request2, response));
                               // current->requests.emplace_back(Request2(current->getSocketDescriptor(), &config), Response(&current->requests[current->index].first));

                                current->makeRequest = false;
                            }
                            current->requestFinished = false;
                            current->getRequest2(current->index)->requestFinished = false;
                            current->getBuffer()[bytesRead] = '\0';
                            current->setBytesRead(bytesRead);
                            /////
                            current->getRequest2(current->index)->parse(bytesRead);
                            current->setRequestFinished(current->index);
                            current->getRequest2(current->index)->printHeaders();
                            // getchar();
                            if (current->getRequest2(current->index)->requestFinished) {
                                cout << "request finished" << endl;
                                current->requests[current->index].second->responseSent = false;
                                current->index++;
                                current->makeRequest = true;
                                cout << "***********************" << endl;
                                //sleep (1);
                                // current->getResponse()->responseSent = false;
                            }
                           // current->setHost(current->getRequest2(current->index)->getHost());

                        }
                        //if (!current->requestFinished)
                         //   break;
                    }
                    if (current->getSocketDescriptor() == fd && (events[i].events & EPOLLOUT)) {
                       current->counter = 0;
                       // getchar();
                       int counter = 0;
                       int i = 0;
                       for ( i = 0; i < current->requests.size(); i++) {
                            if (current->requests[i].first->requestFinished) {
                                current->requests[i].second->responseSent = false;
                                current->requests[i].second->sendResponse();
        
                            }
                            if (current->requests[i].second->responseSent) {
                                counter++;
                            }
                       }
                        // if (i == counter && counter != 0) {
                        //     cout << "responses sent" << endl;
                        //     usleep(1000);
                        //     // if (current->getRequest2()->keepAlive) {
                        //     //     //current->getRequest2()->reset();
                        //     //     current->reset();
                        //     // } else {
                        //         // Connection closed
                        //         std::cout << "Client " << fd << " disconnected.\n";
                        //         // make the client disappear
                        //         struct epoll_event event;
                        //         event.events = 0; // No events to monitor
                        //         event.data.fd = fd;
                        //         if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                        //             perror("Error removing client socket from epoll instance");
                        //         }
                        //         close(fd);
                        //         // Remove the client from the linked list
                        //         if (current == head) {
                        //             head = current->getNext();
                        //         } else {
                        //             Client* prev = head;
                        //             while (prev->getNext() != current) {
                        //                 prev = prev->getNext();
                        //             }
                        //             prev->setNext(current->getNext());
                        //         }
                        //         delete current;
                        //     //}
                        // }
                        break;
                    }
                    else {
                        current->counter++;
                        if (current->counter == 1000) {
                            // Connection closed
                            std::cout << "Client " << fd << " disconnected.\n";
                            // make the client disappear
                            struct epoll_event event;
                            event.events = 0; // No events to monitor
                            event.data.fd = fd;
                            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                                perror("Error removing client socket from epoll instance");
                            }
                            close(fd);
                            // Remove the client from the linked list
                            if (current == head) {
                                head = current->getNext();
                            } else {
                                Client* prev = head;
                                while (prev->getNext() != current) {
                                    prev = prev->getNext();
                                }
                                prev->setNext(current->getNext());
                            }
                            delete current;
                            i--;
                            break;
                        }
                    }
                    current = current->getNext();
                }
            }
        }
    }

    // Close all client sockets and delete client objects
    Client* current = head;
    while (current != NULL) {
        close(current->getSocketDescriptor());
        Client* next = current->getNext();
        delete current;
        current = next;
    }

    // Close the server socket
    // close(serverSocket);
    for (size_t i = 0; i < serverSockets.size(); i++) {
        close(serverSockets[i]);
    }
    close(epollFd);

    return 0;
}*/


void close(const std::map<int,sockaddr_in> & map)
{
    std::map<int,sockaddr_in>::const_iterator it_serverSockets;
    for(it_serverSockets = map.begin(); it_serverSockets != map.end(); it_serverSockets++)
    {
        close(it_serverSockets->first);
    }
}

void close(const std::map<int,epoll_event> & map)
{
    std::map<int,epoll_event>::const_iterator it;
    for(it = map.begin(); it != map.end(); it++)
    {
        close(it->first);
    }
}

void close(const std::vector<int> & arr)
{
    std::vector<int>::const_iterator it;
    for(it= arr.begin(); it != arr.end(); it++)
    {
        close(*it);
    }
}
int main(int ac,char **av)
{
    //int epollFd;
    //sockaddr_in serverAdress;
    std::map < std::string, serverConfig >::iterator it_servers;
    std::vector<int> serverSockets;
    std::vector<int>::iterator it;
    std::vector<Client> Clients;
    int serverSocket,epollFd;
    //char buff[BUFFER_SIZE];
    //epoll_event events[MAX_CLIENTS];
    signal(SIGPIPE, SIG_IGN);
    //epollFd = -1;
    //(void)av;
    try
    {
        if(ac != 2)
           throw std::runtime_error("./webserv [configuration file]");
        GlobalConfig config(av[1]);
        for(it_servers = config.servers.begin();it_servers !=config.servers.end() ; it_servers++ )
        {
            for(size_t i = 0 ;i < it_servers->second.config["listen"].size() ; i++)
            {
                serverSocket = socket(AF_INET,SOCK_STREAM,0);
                if(serverSocket == -1)
                    throw std::runtime_error("Failed to create socket.");
                serverSockets.push_back(serverSocket);
                sockaddr_in serverAddress;
                memset(&serverAddress,0,sizeof(serverAddress));
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(atoi(it_servers->second.config["listen"].at(i).c_str()));
                serverAddress.sin_addr.s_addr = inet_addr(it_servers->first.c_str());
                int on = 1;
                if (setsockopt(serverSocket, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) == -1) {
                std::cerr << "Error: setsockopt failed" << std::endl;
                continue;
                }
                // Bind socket to address and port
                if(bind(serverSocket,(sockaddr*)&serverAddress,sizeof(serverAddress)) == -1)
                    throw std::runtime_error("Failed to bind socket.");
                // Listen for incoming connections
                if(listen(serverSocket,5) == -1)
                    throw std::runtime_error("Failed to listen.");
                std::cout << "Server started. Listening on port :" << it_servers->second.config["listen"].at(i) << std::endl;
             }
        }

    // Add server socket to epoll
       epollFd = epoll_create(1);
       if(epollFd == -1)
            throw std::runtime_error("Failed to create epoll instance.");
        std::vector<epoll_event> serverEvents(serverSockets.size());
        for(size_t i = 0 ; i < serverSockets.size() ; i++)
        {
            serverEvents[i].data.fd = serverSockets.at(i);
            serverEvents[i].events = EPOLLIN;
            if(epoll_ctl(epollFd,EPOLL_CTL_ADD,serverSockets[i],&serverEvents[i]) == -1)
                throw std::runtime_error("Failed to add server socket to epoll instance.");
        }


    struct epoll_event events[((MAX_CLIENTS + 1) * serverSockets.size())]; // Events array for epoll_wait

    Client* head = NULL; // Head of the linked list

    while (true) {
        int numEvents = epoll_wait(epollFd, events, ((MAX_CLIENTS + 1) * serverSockets.size()), -1);
        if (numEvents == -1) {
            perror("Error in epoll_wait");
            break;
        }
        //cout << "2- " << numEvents << endl;

        for (int i = 0; i < numEvents; ++i) {
            int fd = events[i].data.fd;
            //usleep (10000);

            vector<int>::iterator it = find(serverSockets.begin(), serverSockets.end(), fd);

            if (it != serverSockets.end()) {
                // New connection
                sockaddr_in clientAddress;
                socklen_t clientAddressLen = sizeof(clientAddress);
                int clientSocket = accept(*it, (struct sockaddr*)&clientAddress, &clientAddressLen);
                if (clientSocket == -1) {
                    perror("Error accepting connection");
                } else {
                    std::cout << "New connection accepted. Client socket: " << clientSocket << std::endl;

                    // Create a new client object
                    //getpeername(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
                    Client* newClient = new Client(clientSocket, clientAddress, clientAddressLen, *it, &config);

                    // Add the new client to the linked list
                    newClient->setNext(head);
                    head = newClient;

                    // Add the new client socket to the epoll instance
                    struct epoll_event event;
                    event.events = EPOLLIN | EPOLLOUT;
                    event.data.fd = clientSocket;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
                        perror("Error adding client socket to epoll instance");
                        close(clientSocket);
                    }
                }
            } else {
                // Data received on a client socket
                Client* current = head;
                while (current != NULL) {
                    if (current->getSocketDescriptor() == fd && (events[i].events & EPOLLIN )){
                       // need to handle pos in chunked too, fuck it....
                        ssize_t bytesRead = recv(fd, current->getBuffer(), BUFFER_SIZE, 0);
                        if (bytesRead <= 0) {
                            cerr << "Client didnt send a thing" << endl;
                            current->counter++;
                            // current->counter++;
                            if (current->counter >= 1000) {
                            // Connection closed
                            std::cout << "Client " << fd << " disconnected.\n";
                            // make the client disappear
                            struct epoll_event event;
                            event.events = 0; // No events to monitor
                            event.data.fd = fd;
                            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                                perror("Error removing client socket from epoll instance");
                            }
                            close(fd);
                            // Remove the client from the linked list
                            if (current == head) {
                                head = current->getNext();
                            } else {
                                Client* prev = head;
                                while (prev->getNext() != current) {
                                    prev = prev->getNext();
                                }
                                prev->setNext(current->getNext());
                            }
                            delete current;
                            i--;
                            break;
                        }
                            // Connection closed or error
                            // if (bytesRead == 0) {
                            //     std::cout << "Client " << fd << " disconnected.\n";
                            // } else {
                            //     perror("Error receiving data");
                            // }
                            // // make the client disappear
                            // struct epoll_event event;
                            // event.events = 0; // No events to monitor
                            // event.data.fd = fd;
                            // if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                            //     perror("Error removing client socket from epoll instance");
                            // }
                            // close(fd);
                            // // Remove the client from the linked list
                            // if (current == head) {
                            //     head = current->getNext();
                            // } else {
                            //     Client* prev = head;
                            //     while (prev->getNext() != current) {
                            //         prev = prev->getNext();
                            //     }
                            //     prev->setNext(current->getNext());
                            // }
                            // delete current;
                            break;
                        } else {
                       current->counter = 0;
                            // Process received data
                            if (current->makeRequest) {
                                // Request2 request2(current->getSocketDescriptor(), &config);
                                Request2 *request2 = new Request2(current->getSocketDescriptor(), &config);
                                Response *response = new Response(request2);
                                request2->buffer = current->getBuffer();
                                current->requests.push_back(make_pair(request2, response));
                               // current->requests.emplace_back(Request2(current->getSocketDescriptor(), &config), Response(&current->requests[current->index].first));

                                current->makeRequest = false;
                            }
                            current->requestFinished = false;
                            current->getRequest2(current->index)->requestFinished = false;
                            current->getBuffer()[bytesRead] = '\0';
                            current->setBytesRead(bytesRead);
                            /////
                            current->getRequest2(current->index)->parse(bytesRead);
                            current->setRequestFinished(current->index);
                            current->getRequest2(current->index)->printHeaders();
                            // getchar();
                            if (current->getRequest2(current->index)->requestFinished) {
                                cout << "request finished" << endl;
                                current->requests[current->index].second->responseSent = false;
                                current->index++;
                                current->makeRequest = true;
                                cout << "***********************" << endl;
                                //sleep (1);
                                // current->getResponse()->responseSent = false;
                            }
                           // current->setHost(current->getRequest2(current->index)->getHost());

                        }
                        //if (!current->requestFinished)
                         //   break;
                    }
                    if (current->getSocketDescriptor() == fd && (events[i].events & EPOLLOUT)) {
                       current->counter = 0;
                       // getchar();
                       int counter = 0;
                       size_t i = 0;
                       for ( i = 0; i < current->requests.size(); i++) {
                            if (current->requests[i].first->requestFinished) {
                                current->requests[i].second->responseSent = false;
                                current->requests[i].second->sendResponse();
        
                            }
                            if (current->requests[i].second->responseSent) {
                                counter++;
                            }
                       }
                        // if (i == counter && counter != 0) {
                        //     cout << "responses sent" << endl;
                        //     usleep(1000);
                        //     // if (current->getRequest2()->keepAlive) {
                        //     //     //current->getRequest2()->reset();
                        //     //     current->reset();
                        //     // } else {
                        //         // Connection closed
                        //         std::cout << "Client " << fd << " disconnected.\n";
                        //         // make the client disappear
                        //         struct epoll_event event;
                        //         event.events = 0; // No events to monitor
                        //         event.data.fd = fd;
                        //         if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                        //             perror("Error removing client socket from epoll instance");
                        //         }
                        //         close(fd);
                        //         // Remove the client from the linked list
                        //         if (current == head) {
                        //             head = current->getNext();
                        //         } else {
                        //             Client* prev = head;
                        //             while (prev->getNext() != current) {
                        //                 prev = prev->getNext();
                        //             }
                        //             prev->setNext(current->getNext());
                        //         }
                        //         delete current;
                        //     //}
                        // }
                        break;
                    }
                    else {
                        current->counter++;
                        if (current->counter == 1000) {
                            // Connection closed
                            std::cout << "Client " << fd << " disconnected.\n";
                            // make the client disappear
                            struct epoll_event event;
                            event.events = 0; // No events to monitor
                            event.data.fd = fd;
                            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
                                perror("Error removing client socket from epoll instance");
                            }
                            close(fd);
                            // Remove the client from the linked list
                            if (current == head) {
                                head = current->getNext();
                            } else {
                                Client* prev = head;
                                while (prev->getNext() != current) {
                                    prev = prev->getNext();
                                }
                                prev->setNext(current->getNext());
                            }
                            delete current;
                            i--;
                            break;
                        }
                    }
                    current = current->getNext();
                }
            }
        }
    }
    }
    catch(const std::exception& e)
    {
        close(serverSockets);
        close(epollFd);
        std::cerr  << BOLDRED << e.what() << '\n';
        return(1);
    }
    return (0);
}