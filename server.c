#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define MAX_POSTS 50
#define MAX_TEXT_LENGTH 256
#define MAX_TYPE_LENGTH 16

typedef struct {
    char type[MAX_TYPE_LENGTH];
    char content[MAX_TEXT_LENGTH];
} Post;

typedef struct {
    Post posts[MAX_POSTS];
    int top;
    int push_count;
    int pop_count;
    int traverse_count;
} Stack;

Stack feed = {.top = -1, .push_count = 0, .pop_count = 0, .traverse_count = 0};

int isFull(Stack *stack) { return stack->top == MAX_POSTS - 1; }
int isEmpty(Stack *stack) { return stack->top == -1; }

void push(Stack *stack, const char *type, const char *content) {
    if (isFull(stack)) return;
    stack->top++;
    strncpy(stack->posts[stack->top].type, type, MAX_TYPE_LENGTH - 1);
    strncpy(stack->posts[stack->top].content, content, MAX_TEXT_LENGTH - 1);
    stack->push_count++;
    printf("📤 Pushed: %s (%s)\n", content, type);
}

void pop(Stack *stack) {
    if (isEmpty(stack)) return;
    printf("🗑️ Popped: %s (%s)\n", stack->posts[stack->top].content, stack->posts[stack->top].type);
    stack->top--;
    stack->pop_count++;
}

void traverse(Stack *stack) { stack->traverse_count++; }

int userExists(const char *username) {
    FILE *file = fopen("users.txt", "r");
    if (!file) return 0;
    char u[50], p[50];
    while (fscanf(file, "%49s %49s", u, p) == 2) {
        if (strcmp(u, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int validateUser(const char *username, const char *password) {
    FILE *file = fopen("users.txt", "r");
    if (!file) return 0;
    char u[50], p[50];
    while (fscanf(file, "%49s %49s", u, p) == 2) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

/* ---------- Response Helpers ---------- */
void send_response(FILE *client, const char *status, const char *content_type, const char *body) {
    fprintf(client, "HTTP/1.1 %s\r\n", status);
    fprintf(client, "Content-Type: %s\r\n", content_type);
    fprintf(client, "Access-Control-Allow-Origin: *\r\n");
    fprintf(client, "Connection: close\r\n\r\n");
    fprintf(client, "%s", body);
    fflush(client);
}

/* ---------- Serve Static Files (Improved Streaming) ---------- */
void serve_file(FILE *client, const char *filename, const char *content_type) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        send_response(client, "404 Not Found", "text/plain", "File Not Found");
        return;
    }

    fprintf(client, "HTTP/1.1 200 OK\r\n");
    fprintf(client, "Content-Type: %s\r\n", content_type);
    fprintf(client, "Access-Control-Allow-Origin: *\r\n");
    fprintf(client, "Connection: close\r\n\r\n");
    fflush(client);

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        fwrite(buffer, 1, n, client);
    }
    fclose(f);
    fflush(client);
}

/* ---------- Handle Requests ---------- */
void handle_request(FILE *client, const char *request) {
    printf("📥 Incoming Request: %s\n", request);

    if (strncmp(request, "GET /signup?", 12) == 0) {
        char *user = strstr(request, "user=");
        char *pass = strstr(request, "pass=");
        if (user && pass) {
            user += 5; pass += 5;
            char username[50] = {0}, password[50] = {0};
            sscanf(user, "%49[^&]", username);
            sscanf(pass, "%49s", password);
            if (userExists(username)) {
                send_response(client, "409 Conflict", "application/json", "{\"error\":\"User already exists\"}");
            } else {
                FILE *file = fopen("users.txt", "a");
                if (file) {
                    fprintf(file, "%s %s\n", username, password);
                    fclose(file);
                }
                send_response(client, "200 OK", "application/json", "{\"status\":\"Signup successful\"}");
                printf("✅ New user signed up: %s\n", username);
            }
        } else {
            send_response(client, "400 Bad Request", "application/json", "{\"error\":\"Missing username or password\"}");
        }
    }

    else if (strncmp(request, "GET /login?", 11) == 0) {
        char *user = strstr(request, "user=");
        char *pass = strstr(request, "pass=");
        if (user && pass) {
            user += 5; pass += 5;
            char username[50] = {0}, password[50] = {0};
            sscanf(user, "%49[^&]", username);
            sscanf(pass, "%49s", password);
            if (validateUser(username, password)) {
                send_response(client, "200 OK", "application/json", "{\"status\":\"Login successful\"}");
                printf("👤 Login successful: %s\n", username);
            } else {
                send_response(client, "401 Unauthorized", "application/json", "{\"error\":\"Invalid credentials\"}");
            }
        } else {
            send_response(client, "400 Bad Request", "application/json", "{\"error\":\"Missing username or password\"}");
        }
    }

    else if (strncmp(request, "GET /api/posts", 14) == 0) {
        char body[8192] = "[";
        for (int i = 0; i <= feed.top; i++) {
            strcat(body, "{");
            strcat(body, "\"type\":\"");
            strcat(body, feed.posts[i].type);
            strcat(body, "\",\"content\":\"");
            strcat(body, feed.posts[i].content);
            strcat(body, "\"}");
            if (i < feed.top) strcat(body, ",");
        }
        strcat(body, "]");
        traverse(&feed);
        send_response(client, "200 OK", "application/json", body);
    }

    else if (strncmp(request, "GET /api/add?", 13) == 0) {
        char *text = strstr(request, "text=");
        char *type = strstr(request, "type=");
        if (text && type) {
            text += 5; type += 5;
            char decoded_text[MAX_TEXT_LENGTH] = {0};
            char decoded_type[MAX_TYPE_LENGTH] = {0};
            sscanf(text, "%255[^&]", decoded_text);
            sscanf(type, "%15s", decoded_type);
            push(&feed, decoded_type, decoded_text);
            send_response(client, "200 OK", "application/json", "{\"status\":\"Post added\"}");
        } else {
            send_response(client, "400 Bad Request", "application/json", "{\"error\":\"Missing text or type\"}");
        }
    }

    else if (strncmp(request, "GET /api/pop", 12) == 0) {
        pop(&feed);
        send_response(client, "200 OK", "application/json", "{\"status\":\"Last post removed\"}");
    }

    else if (strncmp(request, "GET /api/stats", 14) == 0) {
        char body[256];
        sprintf(body, "{\"push\":%d,\"pop\":%d,\"traverse\":%d}", feed.push_count, feed.pop_count, feed.traverse_count);
        send_response(client, "200 OK", "application/json", body);
    }

    else if (strncmp(request, "GET /dsa.html", 13) == 0 || strncmp(request, "GET / ", 6) == 0) {
        serve_file(client, "dsa.html", "text/html");
    }

    else if (strncmp(request, "GET /dsa.css", 11) == 0) {
        serve_file(client, "dsa.css", "text/css");
    }

    else if (strncmp(request, "GET /dsa.js", 10) == 0) {
        serve_file(client, "dsa.js", "application/javascript");
    }

    else {
        send_response(client, "404 Not Found", "text/plain", "Not Found");
    }
}

/* ---------- Preload Demo Posts ---------- */
void preloadPosts() {
    push(&feed, "text", "Welcome to GEN-Scroll DSA Project!");
    push(&feed, "video", "https://youtu.be/xuP4g7IDgDM");
    push(&feed, "video", "https://youtu.be/-oOoTIuoL8M");
    push(&feed, "video", "https://youtu.be/278IRQ6HSi4");
}

/* ---------- Main ---------- */
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("❌ Socket creation failed.\n");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("❌ Bind failed. Port might be in use.\n");
        return 1;
    }

    listen(server_fd, 5);
    printf("✅ GEN-Scroll Server running at http://127.0.0.1:8080/dsa.html\n");
    preloadPosts();

    while (1) {
        SOCKET client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == INVALID_SOCKET) continue;

        FILE *client = _fdopen((int)client_fd, "r+b");
        if (!client) {
            closesocket(client_fd);
            continue;
        }

        char buffer[4096] = {0};
        if (fgets(buffer, sizeof(buffer), client) != NULL) {
            handle_request(client, buffer);
        }

        fclose(client);
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
