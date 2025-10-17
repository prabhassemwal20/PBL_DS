#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

#define MAX_POSTS 50
#define MAX_TEXT_LENGTH 256
#define MAX_TYPE_LENGTH 10

typedef struct {
    char type[MAX_TYPE_LENGTH]; // "text" or "video"
    char content[MAX_TEXT_LENGTH];
} Post;

typedef struct {
    Post posts[MAX_POSTS];
    int top;
    int push_count;
    int pop_count;
    int traverse_count;
} Stack;

Stack feed = {.top = -1, .push_count=0, .pop_count=0, .traverse_count=0};

// ---------------- Stack Functions ----------------
int isFull(Stack* stack) { return stack->top == MAX_POSTS - 1; }
int isEmpty(Stack* stack) { return stack->top == -1; }

void push(Stack* stack, const char* type, const char* content) {
    if (isFull(stack)) return;
    stack->top++;
    strncpy(stack->posts[stack->top].type, type, MAX_TYPE_LENGTH-1);
    stack->posts[stack->top].type[MAX_TYPE_LENGTH-1] = '\0';
    strncpy(stack->posts[stack->top].content, content, MAX_TEXT_LENGTH-1);
    stack->posts[stack->top].content[MAX_TEXT_LENGTH-1] = '\0';
    stack->push_count++;
    printf("Pushed: %s (%s)\n", content, type);
}

void pop(Stack* stack) {
    if (isEmpty(stack)) return;
    printf("Popped: %s (%s)\n", stack->posts[stack->top].content, stack->posts[stack->top].type);
    stack->top--;
    stack->pop_count++;
}

void traverse(Stack* stack) { stack->traverse_count++; }

// ---------------- HTTP Server ----------------
void send_response(FILE* client, const char* status, const char* content_type, const char* body) {
    fprintf(client, "HTTP/1.1 %s\r\n", status);
    fprintf(client, "Content-Type: %s\r\n", content_type);
    fprintf(client, "Access-Control-Allow-Origin: *\r\n");
    fprintf(client, "Connection: close\r\n\r\n");
    fprintf(client, "%s", body);
}

void handle_request(FILE* client, const char* request) {
    if (strncmp(request, "GET /api/posts", 14) == 0) {
        char body[8192] = "[";
        for(int i=0;i<=feed.top;i++){
            strcat(body, "{");
            strcat(body, "\"type\":\"");
            strcat(body, feed.posts[i].type);
            strcat(body, "\",\"content\":\"");
            strcat(body, feed.posts[i].content);
            strcat(body, "\"}");
            if(i<feed.top) strcat(body, ",");
        }
        strcat(body, "]");
        traverse(&feed);
        send_response(client, "200 OK", "application/json", body);
    }
    else if (strncmp(request, "GET /api/add?", 13) == 0) {
        char* text = strstr(request, "text=");
        char* type = strstr(request, "type=");
        if(text && type){
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
        sprintf(body,"{\"push\":%d,\"pop\":%d,\"traverse\":%d}", feed.push_count, feed.pop_count, feed.traverse_count);
        send_response(client, "200 OK", "application/json", body);
    }
    else if (strncmp(request, "GET /dsa.html", 13) == 0 || strncmp(request, "GET /", 5) == 0) {
        // Serve frontend HTML
        const char* html = "<!DOCTYPE html>"
            "<html><head><title>GEN-Scroll DSA</title>"
            "<style>"
            "body{font-family:Arial,sans-serif;text-align:center;background:#f0f0f0;}"
            "#feed-container{width:400px;height:400px;margin:50px auto;display:flex;align-items:center;justify-content:center;background:#fff;border-radius:10px;overflow:hidden;}"
            ".post iframe,.post video{width:100%;height:100%;border:none;}"
            "#controls{margin-top:20px;}"
            "#stats{margin-top:10px;font-size:14px;color:#333;}"
            "button{padding:10px 20px;margin:0 5px;font-size:16px;}"
            "</style></head>"
            "<body>"
            "<h1>GEN-Scroll Feed</h1>"
            "<div id='feed-container'></div>"
            "<div id='controls'>"
            "<button id='prev'>Prev</button>"
            "<button id='next'>Next</button>"
            "</div>"
            "<div id='stats'>Push: <span id='push-count'>0</span> | Pop: <span id='pop-count'>0</span> | Traverse: <span id='traverse-count'>0</span></div>"
            "<script src='dsa.js'></script>"
            "</body></html>";
        send_response(client, "200 OK", "text/html", html);
    }
    else {
        send_response(client, "404 Not Found", "text/plain", "Not Found");
    }
}

// ---------------- Preload Posts ----------------
void preloadPosts() {
    push(&feed,"text","Welcome to GEN-Scroll DSA Project!");
    push(&feed,"video","https://youtu.be/xuP4g7IDgDM");
    push(&feed,"video","https://youtu.be/-oOoTIuoL8M");
    push(&feed,"video","https://youtu.be/278IRQ6HSi4");
    push(&feed,"video","https://youtu.be/IxF55qB4CuQ");
    push(&feed,"video","https://youtu.be/CsTQWvh-7A4?si=so4GDvA_pR5EsyzH");
}

// ---------------- Main ----------------
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5500);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("✅ GEN-Scroll Server running at http://127.0.0.1:5500/dsa.html\n");

    preloadPosts(); // <-- Preload initial posts

    while(1){
        SOCKET client_fd = accept(server_fd, NULL, NULL);
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
