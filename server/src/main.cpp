#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#include <pqxx/pqxx>

#define PORT 8080
#define BUFFER_SIZE 4096

const char* env = std::getenv("DATABASE_URL");
std::string conn_str = env ? env : "";


// db
void save_message(const std::string& user, const std::string& text) {
    try {
        pqxx::connection conn(conn_str);
        pqxx::work txn(conn);

        txn.exec_params(
            "INSERT INTO messages (\"user\", text) VALUES ($1, $2)",
            user, text
        );

        txn.commit();
    } catch (const std::exception &e) {
        std::cerr << "DB error: " << e.what() << std::endl;
    }
}

std::string get_messages() {
    try {
        pqxx::connection conn(conn_str);
        pqxx::work txn(conn);

        pqxx::result r = txn.exec("SELECT \"user\", text, createdAt FROM messages ORDER BY id DESC LIMIT 50");

        std::stringstream json;
        json << "[";

        for (size_t i = 0; i < r.size(); ++i) {
            json << "{"
                 << "\"user\":\"" << r[i]["user"].c_str() << "\","
                 << "\"text\":\"" << r[i]["text"].c_str() << "\","
                 << "\"createdAt\":\"" << r[i]["createdat"].c_str() << "\""
                 << "}";

            if (i != r.size() - 1)
                json << ",";
        }

        json << "]";
        return json.str();

    } catch (const std::exception &e) {
        return "{\"error\":\"db error\"}";
    }
}

// json
bool parse_json(const std::string& body, std::string& user, std::string& text) {
    size_t u1 = body.find("\"user\"");
    size_t t1 = body.find("\"text\"");

    if (u1 == std::string::npos || t1 == std::string::npos)
        return false;

    size_t u_start = body.find("\"", u1 + 6) + 1;
    size_t u_end = body.find("\"", u_start);

    size_t t_start = body.find("\"", t1 + 6) + 1;
    size_t t_end = body.find("\"", t_start);

    user = body.substr(u_start, u_end - u_start);
    text = body.substr(t_start, t_end - t_start);

    return true;
}

std::string http_response(const std::string& body, const std::string& status = "200 OK") {
    std::stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << body;
    return response.str();
}

// seerver
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);

    if (bytes_read < 0) {
        perror("read failed");
        close(client_socket);
        return;
    }

    if (bytes_read == 0) {
        std::cout << "Client disconnected\n";
        close(client_socket);
        return;
    }

    std::string request(buffer, bytes_read);

    if (request.find("POST /message") != std::string::npos) {

        size_t body_pos = request.find("\r\n\r\n");
        std::string body = request.substr(body_pos + 4);

        std::string user, text;

        if (parse_json(body, user, text)) {
            save_message(user, text);
            std::string res = http_response("{\"status\":\"ok\"}");
            send(client_socket, res.c_str(), res.size(), 0);
        } else {
            std::string res = http_response("{\"error\":\"invalid json\"}", "400 Bad Request");
            send(client_socket, res.c_str(), res.size(), 0);
        }

    } else if (request.find("GET /messages") != std::string::npos) {

        std::string messages = get_messages();
        std::string res = http_response(messages);
        send(client_socket, res.c_str(), res.size(), 0);

    } else {
        std::string res = http_response("{\"error\":\"not found\"}", "404 Not Found");
        send(client_socket, res.c_str(), res.size(), 0);
    }

    close(client_socket);
}


// main
int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    std::cout << "HTTP Messenger running on port " << PORT << std::endl;

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        std::thread(handle_client, client_socket).detach();
    }

    return 0;
}