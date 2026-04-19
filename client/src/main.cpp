#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <netdb.h>

#define BUFFER_SIZE 8192

std::string server_host = "server";
int server_port = 8080;
std::mutex print_mutex;

// send and get http
std::string send_request(const std::string& request) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    struct hostent* he = gethostbyname(server_host.c_str());
    if (!he) { close(sock); return ""; }
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return "";
    }

    send(sock, request.c_str(), request.size(), 0);

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    close(sock);

    std::string response(buffer);
    size_t body_pos = response.find("\r\n\r\n");
    if (body_pos == std::string::npos) return "";
    return response.substr(body_pos + 4);
}

// POST /message
void post_message(const std::string& user, const std::string& text) {
    std::string body = "{\"user\":\"" + user + "\",\"text\":\"" + text + "\"}";

    std::string request =
        "POST /message HTTP/1.1\r\n"
        "Host: server\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n\r\n" + body;

    send_request(request);
}

// GET /messages
void print_messages(const std::string& json) {
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

// поток polling — каждые 2 секунды получает сообщения
void polling_thread() {
    std::string request =
        "GET /messages HTTP/1.1\r\n"
        "Host: server\r\n"
        "Connection: close\r\n\r\n";

    while (true) {
        std::string body = send_request(request);
        if (!body.empty() && body != "{\"error\":\"db error\"}") {
            // очистить экран и перепечатать
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "\033[2J\033[H"; // clear screen
            std::cout << "=== chat ===\n";
            print_messages(body);
            std::cout << "> " << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    // get from dockerfile
    const char* env_host = std::getenv("SERVER_HOST");
    if (env_host) server_host = env_host;

    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);

    if (username.empty()) {
        std::cerr << "Username cannot be empty\n";
        return 1;
    }

    // start polling in thread
    std::thread(polling_thread).detach();

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input.empty()) continue;
        if (input == "/exit") break;
        post_message(username, input);
    }

    return 0;
}