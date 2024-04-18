// #include <fstream>
// #include <iostream>
// #include <unistd.h>
// #include "webServ.hpp"

// int main() {
//     // std::fstream file("example.txt", std::ios::in);
//     // if (!file.is_open()) {
//     //     std::cerr << "Failed to open file\n";
//     //     return 1;
//     // }

//     // // Get the file descriptor
//     // int fd = fileno(file.rdbuf());
//     // if (fd == -1) {
//     //     std::cerr << "Failed to get file descriptor\n";
//     //     return 1;
//     // }

//     // std::cout << "File descriptor: " << fd << std::endl;

//     // // Use the file descriptor as needed

//     // return 0;
//     int x = 2;
//     int &a = x;

//     std::vector <std::pair <Request2, Response> > req;
//     req.push_back(std::make_pair(Request2(a, NULL), Response(NULL)));
//     cout << req[0].first.chunked << endl;
// }


#include <iostream>
#include <vector>
#include "webServ.hpp"

// Define Request2 and Response types
// class Request2 {
//     public:
//     int chunked;
//     // Other members...
//     Request2(int val) : chunked(val) {}
// };

// class Response {
//     public:
//     int some_data;
//     // Other members...
//     Response(int val) : some_data(val) {}
// };

int main() {
    int x = 2;
    int &a = x;

    std::vector<std::pair<int, Client*> > req;
    Client *client = new Client();
    req.push_back(std::make_pair(x, client)); // Creating a Response with 0 as placeholder
    std::cout << x << std::endl;
    
    return 0;
}
