#include <iostream>
#include <string>
#include <thread>
#include <atomic> // for std::atomic - to safely print from multiple threads
#include <mutex> // for std::mutex - to synchronize access to std::cout
#include <sstream> // for std::stringstream
#include <netinet/in.h> // for sockaddr_in - to create server socket
#include <arpa/inet.h> // for inet_pton - to convert IP address
#include <unistd.h> // for close - to close sockets
#include <cstring> // for memset - to clear buffers
#include <netdb.h> // for gethostbyname - to resolve server hostname

#define BUFFER_SIZE 8192

std::string server_host = "server"; // hostname of server (from docker-compose)
int server_port = 8080;
std::mutex print_mutex;

std::string send_request(const std::string& request) { // send http request to server and get response
    int sock = socket(AF_INET, SOCK_STREAM, 0); // create socket AF_INET - ipv4, SOCK_STREAM - tcp
    if (sock < 0) return "";

    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // ipv4
    addr.sin_port = htons(server_port); // port 8080
    struct hostent* he = gethostbyname(server_host.c_str()); // resolve server hostname to IP address
    if (!he) { close(sock); return ""; }
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length); // copy resolved IP address to sockaddr_in

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { // connect to server
        close(sock);
        return "";
    }

    send(sock, request.c_str(), request.size(), 0); // send http request to server

    char buffer[BUFFER_SIZE]; // buffer to receive response
    memset(buffer, 0, BUFFER_SIZE); // clear buffer before receiving response
    read(sock, buffer, BUFFER_SIZE); // read response from server (blocking call, will wait until response is received)
    close(sock);

    std::string response(buffer);
    size_t body_pos = response.find("\r\n\r\n"); // find position of body | format: headers\r\n\r\nbody
    if (body_pos == std::string::npos) return "";
    return response.substr(body_pos + 4); // return body of response (after \r\n\r\n)
}

// POST /message
void post_message(const std::string& user, const std::string& text) { // send new message to server
    std::string body = "{\"user\":\"" + user + "\",\"text\":\"" + text + "\"}";

    std::string request = // format of http request:
        "POST /message HTTP/1.1\r\n"
        "Host: server\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n\r\n" + body;

    send_request(request);
}

// GET /messages
void print_messages(const std::string& json) { // parse json array of messages and print them to console
    size_t pos = 0;
    while ((pos = json.find("{", pos)) != std::string::npos) {
        size_t end = json.find("}", pos);
        if (end == std::string::npos) break;

        std::string obj = json.substr(pos, end - pos + 1);

        auto extract = [&](const std::string& key) -> std::string {
            size_t k = obj.find("\"" + key + "\"");
            if (k == std::string::npos) return "";
            size_t vs = obj.find("\"", k + key.size() + 2) + 1;
            size_t ve = obj.find("\"", vs);
            return obj.substr(vs, ve - vs);
        };

        std::string user = extract("user");
        std::string text = extract("text");
        std::string time = extract("createdAt");

        if (!user.empty() && !text.empty())
            std::cout << "[" << time << "] " << user << ": " << text << "\n";

        pos = end + 1;
    }
}

void polling_thread() { // thread to poll messages from server every 2 seconds and print them to console
    std::string request =
        "GET /messages HTTP/1.1\r\n"
        "Host: server\r\n"
        "Connection: close\r\n\r\n";

    while (true) {
        std::string body = send_request(request);
        if (!body.empty() && body != "{\"error\":\"db error\"}") {
            std::lock_guard<std::mutex> lock(print_mutex); // lock mutex to safely print from multiple threads
            std::cout << "\033[2J\033[H"; // clear screen
            std::cout << "=== chat ===\n";
            print_messages(body); // print messages to console
            std::cout << "> " << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2)); // wait for 2 seconds before polling again
    }
}

int main() {
    const char* env_host = std::getenv("SERVER_HOST"); // get server hostname from docker compose
    if (env_host) server_host = env_host;

    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);

    if (username.empty()) {
        std::cerr << "Username cannot be empty\n";
        return 1;
    }

    std::thread(polling_thread).detach(); // detach thread (will keep running until program exits)

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input.empty()) continue;
        if (input == "/exit") break;
        post_message(username, input); // send new message to server (will be saved to db and then received by polling thread and printed to console)
    }

    return 0;
}