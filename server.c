/*
    GEN-Scroll DSA Project
    ---------------------------------------
    A single C HTTP server that:
    - Handles login/signup (persistent using users.txt)
    - Simulates a DSA stack feed (push/pop/traverse)
    - Serves frontend HTML/CSS/JS files to the browser

    Built using Winsock2 for Windows
    Author: Kundan
    Year: 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

// -------------- CONSTANTS -----------------
#define MAX_POSTS 50
#define MAX_TEXT_LENGTH 256
#define MAX_TYPE_LENGTH 10

// -------------- STRUCTURES -----------------
typedef struct {
    char type[MAX_TYPE_LENGTH];    // "text" or "video"
    char content[MAX_TEXT_LENGTH]; // post content or video URL
} Post;

typedef struct {
    Post posts[MAX_POSTS];
    int top;               // index of top element
    int push_count;        // number of pushes
    int pop_count;         // number of pops
    int traverse_count;    // number of traverses
} Stack;

// Global stack instance
Stack feed = {.top = -1, .push_count=0, .pop_count=0, .traverse_count=0};

// -------------- STACK OPERATIONS -----------------
int isFull(Stack* stack) { return stack->top == MAX_POSTS - 1; }
int isEmpty(Stack* stack) { return stack->top == -1; }

// Push a new post
void push(Stack* stack, const char* type, const char* content) {
    if (isFull(stack)) return;
    stack->top++;
    strncpy(stack->posts[stack->top].type, type, MAX_TYPE_LENGTH-1);
    strncpy(stack->posts[stack->top].content, content, MAX_TEXT_LENGTH-1);
    stack->push_count++;
    printf("Pushed: %s (%s)\n", content, type);
}

// Pop top post
void pop(Stack* stack) {
    if (isEmpty(stack)) return;
    printf("Popped: %s (%s)\n", stack->posts[stack->top].content, stack->posts[stack->top].type);
    stack->top--;
    stack->pop_count++;
}

// Increment traverse count (used for GET /api/posts)
void traverse(Stack* stack) { stack->traverse_count++; }

// -------------- USER AUTH FUNCTIONS -----------------

// Check if a username already exists
int userExists(const char* username) {
    FILE* file = fopen("users.txt", "r");
    if (!file) return 0;
    char u[50], p[50];
    while (fscanf(file, "%49s %49s", u, p) == 2) {
        if (strcmp(u, username) == 0) {
            fclose(file);
            return 1; // username found
        }
    }
    fclose(file);
    return 0;
}

// Validate username/password pair
int validateUser(const char* username, const char* password) {
    FILE* file = fopen("users.txt", "r");
    if (!file) return 0;
    char u[50], p[50];
    while (fscanf(file, "%49s %49s", u, p) == 2) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            fclose(file);
            return 1; // valid credentials
        }
    }
    fclose(file);
    return 0;
}

// -------------- HTTP HELPER FUNCTION -----------------

// Send response to browser
void send_response(FILE* client, const char* status, const char* content_type, const char* body) {
    fprintf(client, "HTTP/1.1 %s\r\n", status);
    fprintf(client, "Content-Type: %s\r\n", content_type);
    fprintf(client, "Access-Control-Allow-Origin: *\r\n");
    fprintf(client, "Connection: close\r\n\r\n");
    fprintf(client, "%s", body);
}

// -------------- REQUEST HANDLER -----------------
void handle_request(FILE* client, const char* request) {

    // ---------- SIGNUP ----------
    if (strncmp(request, "GET /signup?", 12) == 0) {
        char* user = strstr(request, "user=");
        char* pass = strstr(request, "pass=");
        if (user && pass) {
            user += 5; pass += 5;
            char username[50], password[50];
            sscanf(user, "%[^&]", username);
            sscanf(pass, "%s", password);

            if (userExists(username)) {
                send_response(client, "409 Conflict", "application/json", "{\"error\":\"User already exists\"}");
            } else {
                FILE* file = fopen("users.txt", "a");
                fprintf(file, "%s %s\n", username, password);
                fclose(file);
                send_response(client, "200 OK", "application/json", "{\"status\":\"Signup successful\"}");
                printf("✅ User signed up: %s\n", username);
            }
        } else {
            send_response(client, "400 Bad Request", "application/json", "{\"error\":\"Missing username or password\"}");
        }
    }

    // ---------- LOGIN ----------
    else if (strncmp(request, "GET /login?", 11) == 0) {
        char* user = strstr(request, "user=");
        char* pass = strstr(request, "pass=");
        if (user && pass) {
            user += 5; pass += 5;
            char username[50], password[50];
            sscanf(user, "%[^&]", username);
            sscanf(pass, "%s", password);
            if (validateUser(username, password)) {
                send_response(client, "200 OK", "application/json", "{\"status\":\"Login successful\"}");
                printf("👤 Login: %s\n", username);
            } else {
                send_response(client, "401 Unauthorized", "application/json", "{\"error\":\"Invalid credentials\"}");
            }
        } else {
            send_response(client, "400 Bad Request", "application/json", "{\"error\":\"Missing username or password\"}");
        }
    }

    // ---------- STACK API ----------
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
        char* text = strstr(request, "text=");
        char* type = strstr(request, "type=");
        if (text && type) {
            text += 5; type += 5;
            char decoded_text[MAX_TEXT_LENGTH];
            char decoded_type[MAX_TYPE_LENGTH];
            sscanf(text, "%[^&]", decoded_text);
            sscanf(type, "%s", decoded_type);
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

    // ---------- DEFAULT PAGES ----------
    else if (strncmp(request, "GET /auth.html", 14) == 0) {
        // Serve login/signup page
        FILE* f = fopen("auth.html", "r");
        if (f) {
            char html[8192] = "";
            fread(html, 1, sizeof(html)-1, f);
            fclose(f);
            send_response(client, "200 OK", "text/html", html);
        } else {
            send_response(client, "404 Not Found", "text/plain", "auth.html not found");
        }
    }

    else if (strncmp(request, "GET /dsa.html", 13) == 0) {
        // Serve feed page
        FILE* f = fopen("dsa.html", "r");
        if (f) {
            char html[8192] = "";
            fread(html, 1, sizeof(html)-1, f);
            fclose(f);
            send_response(client, "200 OK", "text/html", html);
        } else {
            send_response(client, "404 Not Found", "text/plain", "dsa.html not found");
        }
    }

    else {
        send_response(client, "404 Not Found", "text/plain", "Not Found");
    }
}

// -------------- PRELOAD SOME STACK POSTS --------------
void preloadPosts() {
    push(&feed, "text", "Welcome to GEN-Scroll DSA Project!");
    push(&feed, "video", "https://youtu.be/xuP4g7IDgDM");
    push(&feed, "video", "https://youtu.be/-oOoTIuoL8M");
    push(&feed, "video", "https://youtu.be/278IRQ6HSi4");
}

// -------------- MAIN FUNCTION -----------------
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("❌ Socket creation failed.\n");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5500); // Run on port 5500
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("❌ Bind failed. Port might be in use.\n");
        return 1;
    }

    listen(server_fd, 5);
    printf("✅ GEN-Scroll Server running at http://127.0.0.1:5500/auth.html\n");

    preloadPosts(); // Load initial posts

    while (1) {
        SOCKET client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == INVALID_SOCKET) continue;
        FILE* client = _fdopen(client_fd, "r+");
        char buffer[4096];
        fgets(buffer, sizeof(buffer), client);
        handle_request(client, buffer);
        fclose(client);
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}