// Upgraded by github copilot

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600
#define BUFFER_SIZE 1024*1024*4  // 4MB buffer for image data
#define HTTP_RESPONSE_BUFFER 1024*1024*8  // 8MB for HTTP response

typedef struct {
    char* data;
    size_t size;
} ResponseData;

typedef struct {
    char* ascii_code;
    char* button_code;
    char* point_y;
    char* point_x;
    int window_height;
    int window_width;
} InputData;

// Socket helper functions
typedef struct {
    char* host;
    char* port;
    char* path;
} URLParts;

URLParts parse_url(const char* url) {
    URLParts parts = {0};
    char* temp = strdup(url);
    char* cursor = temp;

    // Skip http:// if present
    if (strncmp(cursor, "http://", 7) == 0) {
        cursor += 7;
    }

    // Find host
    char* slash = strchr(cursor, '/');
    if (slash) {
        *slash = '\0';
        parts.host = strdup(cursor);
        parts.path = strdup(slash + 1);
        *slash = '/';
    } else {
        parts.host = strdup(cursor);
        parts.path = strdup("");
    }

    // Extract port if present
    char* colon = strchr(parts.host, ':');
    if (colon) {
        *colon = '\0';
        parts.port = strdup(colon + 1);
    } else {
        parts.port = strdup("80");
    }

    free(temp);
    return parts;
}

void free_url_parts(URLParts* parts) {
    if (parts) {
        free(parts->host);
        free(parts->port);
        free(parts->path);
    }
}

int create_socket_connection(const char* host, const char* port) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

// Global variables
Display* main_display = NULL;
Window main_window;
int window_width = DEFAULT_WIDTH;
int window_height = DEFAULT_HEIGHT;
volatile int running = 1;
char* target_url = NULL;
pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
InputData current_input = {0};
unsigned char* global_image_data = NULL;
XImage* current_image = NULL;
GC gc = None;
Visual* visual = NULL;
int depth = 0;
int screen = 0;

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    ResponseData* resp = (ResponseData*)userp;
    
    char* ptr = realloc(resp->data, resp->size + realsize + 1);
    if(!ptr) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return 0;
    }
    
    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;
    
    return realsize;
}

void safe_free(void** ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

void cleanup_input_data() {
    safe_free((void**)&current_input.ascii_code);
    safe_free((void**)&current_input.button_code);
    safe_free((void**)&current_input.point_y);
    safe_free((void**)&current_input.point_x);
}

void cleanup_x11() {
    if (main_display) {
        pthread_mutex_lock(&display_mutex);
        if (current_image) {
            current_image->data = NULL;  // Prevent double free
            XDestroyImage(current_image);
            current_image = NULL;
        }
        if (gc != None) {
            XFreeGC(main_display, gc);
            gc = None;
        }
        if (main_window) {
            XDestroyWindow(main_display, main_window);
            main_window = 0;
        }
        XCloseDisplay(main_display);
        main_display = NULL;
        pthread_mutex_unlock(&display_mutex);
    }
}

void show_image_data(const unsigned char* data, size_t data_len) {
    if (!data || !main_display || data_len == 0) return;

    pthread_mutex_lock(&display_mutex);
    
    // Match Python's XCreateImage parameters exactly
    XImage* new_image = XCreateImage(main_display, 
                                   visual,
                                   24,    // depth
                                   2,     // format (ZPixmap in Python)
                                   0,     // offset
                                   (char*)data,
                                   window_width,
                                   window_height,
                                   32,    // bitmap_pad
                                   0);    // bytes_per_line
    
    if (new_image) {
        if (current_image) {
            current_image->data = NULL;
            XDestroyImage(current_image);
        }
        
        XPutImage(main_display, main_window, gc,
                  new_image, 0, 0, 0, 0,
                  window_width, window_height);
        XFlush(main_display);
        
        current_image = new_image;
    }
    
    pthread_mutex_unlock(&display_mutex);
}

// Similarly fix the send_input_data function:
void send_input_data() {
    URLParts parts = parse_url(target_url);
    char* endpoint = "handle_input";
    
    int sockfd = create_socket_connection(parts.host, parts.port);
    if (sockfd < 0) {
        free_url_parts(&parts);
        return;
    }

    // Set send/receive timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    char post_data[1024];
    pthread_mutex_lock(&input_mutex);
    snprintf(post_data, sizeof(post_data),
            "ascii_code=%s\nbutton_code=%s\npoint_y=%s\npoint_x=%s\nheight=%d\nwidth=%d",
            current_input.ascii_code ? current_input.ascii_code : "None",
            current_input.button_code ? current_input.button_code : "None",
            current_input.point_y ? current_input.point_y : "None",
            current_input.point_x ? current_input.point_x : "None",
            current_input.window_height,
            current_input.window_width);
    pthread_mutex_unlock(&input_mutex);

    char request[2048];
    snprintf(request, sizeof(request),
             "POST /%s%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             parts.path, endpoint,
             parts.host,
             strlen(post_data),
             post_data);

    if (send(sockfd, request, strlen(request), MSG_NOSIGNAL) < 0) {
        close(sockfd);
        free_url_parts(&parts);
        return;
    }

    // Read complete response
    char response[1024];
    ssize_t n;
    size_t total = 0;
    while ((n = recv(sockfd, response + total, sizeof(response) - total - 1, 0)) > 0) {
        total += n;
    }

    // Proper socket shutdown
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    free_url_parts(&parts);
}

void* input_sender_thread(void* arg) {
    while(running) {
        send_input_data();
        usleep(100000); // Sleep for 100ms
    }
    return NULL;
}

unsigned char hex_to_byte(const char* hex) {
    unsigned char byte = 0;
    for(int i = 0; i < 2; i++) {
        char c = hex[i];
        byte <<= 4;
        if(c >= '0' && c <= '9') byte |= c - '0';
        else if(c >= 'a' && c <= 'f') byte |= c - 'a' + 10;
        else if(c >= 'A' && c <= 'F') byte |= c - 'A' + 10;
    }
    return byte;
}

// In image_receiver_thread, let's properly handle HTTP response and improve error handling:
void* image_receiver_thread(void* arg) {
    unsigned char* image_buffer = malloc(BUFFER_SIZE);
    char* response_buffer = malloc(HTTP_RESPONSE_BUFFER);
    
    if (!image_buffer || !response_buffer) {
        free(image_buffer);
        free(response_buffer);
        return NULL;
    }

    URLParts parts = parse_url(target_url);
    char* endpoint = "download_picture";

    while(running) {
        int sockfd = create_socket_connection(parts.host, parts.port);
        if (sockfd < 0) {
            usleep(1000000); // Wait 1s before retry
            continue;
        }

        // Set receive timeout to prevent hanging
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        char request[1024];
        snprintf(request, sizeof(request),
                 "POST /%s%s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Content-Length: 0\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 parts.path, endpoint,
                 parts.host);

        if (send(sockfd, request, strlen(request), MSG_NOSIGNAL) < 0) {
            close(sockfd);
            usleep(100000);  // Wait 100ms before retry
            continue;
        }

        // Read response headers first
        size_t total = 0;
        ssize_t n;
        char* header_end = NULL;
        
        while ((n = recv(sockfd, response_buffer + total, 
               HTTP_RESPONSE_BUFFER - total - 1, 0)) > 0) {
            total += n;
            response_buffer[total] = '\0';
            
            header_end = strstr(response_buffer, "\r\n\r\n");
            if (header_end) {
                header_end += 4;  // Skip \r\n\r\n
                break;
            }
        }

        if (!header_end) {
            close(sockfd);
            usleep(100000);
            continue;
        }

        // Read the rest of the body
        size_t body_offset = header_end - response_buffer;
        while ((n = recv(sockfd, response_buffer + total, 
               HTTP_RESPONSE_BUFFER - total - 1, 0)) > 0) {
            total += n;
            response_buffer[total] = '\0';
        }

        // Proper socket shutdown
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);

        // Process the response body
        char* data_start = response_buffer + body_offset;
        int comma_count = 0;  // Changed from char* to int
        char* p = data_start;
        
        // Find start of hex data (after two commas)
        while (*p && comma_count < 2) {
            if (*p == ',') comma_count++;
            p++;
        }

        if (comma_count == 2) {
            // Skip whitespace
            while (isspace(*p)) p++;
            
            // Convert hex string to binary
            size_t remaining = strlen(p);
            size_t data_len = remaining / 2;
            
            if (data_len > 0 && data_len <= BUFFER_SIZE) {
                for(size_t i = 0; i < data_len; i++) {
                    image_buffer[i] = hex_to_byte(p + i*2);
                }
                show_image_data(image_buffer, data_len);
            }
        }

        // Maintain ~25 FPS
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        long ms = (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
        static long last_ms = 0;
        if (last_ms > 0) {
            long diff = ms - last_ms;
            if (diff < 40) { // Target 40ms per frame (25 FPS)
                usleep((40 - diff) * 1000);
            }
        }
        last_ms = ms;
    }

    free(image_buffer);
    free(response_buffer);
    free_url_parts(&parts);
    return NULL;
}

int main(int argc, char** argv) {
    // Initialize the target URL with default or provided value
    if (argc < 2) {
        printf("Usage: %s <target_url> [width]x[height]\n", argv[0]);
        printf("Example: %s http://localhost:7777 800x600\n", argv[0]);
        target_url = strdup("http://localhost:7777");  // Use strdup to allocate new memory
    } else {
        target_url = strdup(argv[1]);  // Use strdup to allocate new memory
    }

    // Add trailing slash if needed
    if (target_url[strlen(target_url) - 1] != '/') {
        char* new_url = malloc(strlen(target_url) + 2);  // +2 for '/' and null terminator
        if (!new_url) {
            fprintf(stderr, "Failed to allocate memory for URL\n");
            free(target_url);
            return 1;
        }
        sprintf(new_url, "%s/", target_url);
        free(target_url);  // Free old URL before replacing
        target_url = new_url;
    }

    // Parse window size if provided
    if (argc >= 3) {
        char* size_string = argv[2];
        if (strchr(size_string, 'x')) {
            sscanf(size_string, "%dx%d", &window_width, &window_height);
        }
    }

    // Initialize X11
    if (!XInitThreads()) {
        fprintf(stderr, "Failed to initialize X11 threads\n");
        free(target_url);
        return 1;
    }

    main_display = XOpenDisplay(NULL);
    if (!main_display) {
        fprintf(stderr, "Cannot open display\n");
        free(target_url);
        return 1;
    }

    screen = DefaultScreen(main_display);
    visual = DefaultVisual(main_display, screen);
    depth = DefaultDepth(main_display, screen);
    Window root = DefaultRootWindow(main_display);

    XSetWindowAttributes attrs = {0};
    attrs.event_mask = KeyPressMask | KeyReleaseMask | PointerMotionMask | 
                      ButtonPressMask | ButtonReleaseMask | ExposureMask;
    attrs.background_pixel = BlackPixel(main_display, screen);

    main_window = XCreateWindow(main_display, root,
                              0, 0, window_width, window_height, 0,
                              depth, InputOutput, visual,
                              CWEventMask | CWBackPixel, &attrs);

    if (!main_window) {
        fprintf(stderr, "Failed to create window\n");
        cleanup_x11();
        return 1;
    }

    XMapWindow(main_display, main_window);
    gc = XCreateGC(main_display, main_window, 0, NULL);
    XFlush(main_display);

    // Create threads
    pthread_t input_thread, image_thread;
    pthread_create(&input_thread, NULL, input_sender_thread, NULL);
    pthread_create(&image_thread, NULL, image_receiver_thread, NULL);

    // Initialize input data
    current_input.window_height = window_height;
    current_input.window_width = window_width;

    XEvent event;
    char key_buffer[32];
    KeySym key;
    XComposeStatus compose;
    running = 1;

    while (running) {
        XNextEvent(main_display, &event);
        pthread_mutex_lock(&input_mutex);

        switch(event.type) {
            case KeyPress:
                break;

            case KeyRelease: {
                int len = XLookupString(&event.xkey, key_buffer, sizeof(key_buffer), &key, &compose);
                if (len > 0) {
                    key_buffer[len] = '\0';
                    safe_free((void**)&current_input.ascii_code);
                    current_input.ascii_code = strdup(key_buffer);
                }
                if (XkbKeycodeToKeysym(main_display, event.xkey.keycode, 0, 0) == XK_Escape) {
                    running = 0;
                }
            } break;

            case MotionNotify: {
                char str[32];
                safe_free((void**)&current_input.point_y);
                safe_free((void**)&current_input.point_x);
                snprintf(str, sizeof(str), "%d", event.xmotion.y);
                current_input.point_y = strdup(str);
                snprintf(str, sizeof(str), "%d", event.xmotion.x);
                current_input.point_x = strdup(str);
            } break;

            case ButtonPress: {
                char str[32];
                safe_free((void**)&current_input.button_code);
                safe_free((void**)&current_input.point_y);
                safe_free((void**)&current_input.point_x);
                snprintf(str, sizeof(str), "%d", event.xbutton.button);
                current_input.button_code = strdup(str);
                snprintf(str, sizeof(str), "%d", event.xbutton.y);
                current_input.point_y = strdup(str);
                snprintf(str, sizeof(str), "%d", event.xbutton.x);
                current_input.point_x = strdup(str);
            } break;
        }

        pthread_mutex_unlock(&input_mutex);
    }

    // Cleanup
    running = 0;
    pthread_join(input_thread, NULL);
    pthread_join(image_thread, NULL);
    
    cleanup_input_data();
    cleanup_x11();
    free(target_url);  // Free the URL at the very end
    
    return 0;
}
