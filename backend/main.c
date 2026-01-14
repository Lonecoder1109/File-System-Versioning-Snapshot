#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 1048576 // 1MB buffer for large JSON responses

// Global filesystem instance
FileSystem *g_fs = NULL;

// Forward declarations
void send_error(int client_socket, const char *error);

// JSON response helpers
void send_json_response(int client_socket, int status_code, const char *json) {
    if (!json) {
        send_error(client_socket, "Invalid JSON");
        return;
    }
    
    size_t json_len = strlen(json);
    
    // Build HTTP response header separately
    char header[2048];
    const char *status_text = (status_code == 200) ? "OK" : "Error";
    int header_len = snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: application/json\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "Content-Length: %lu\r\n"
             "\r\n",
             status_code, status_text, (unsigned long)json_len);
    
    if (header_len < 0) {
        return;
    }
    
    // Send header
    send(client_socket, header, header_len, 0);
    // Send JSON body
    send(client_socket, json, json_len, 0);
}

void send_success(int client_socket, const char *message) {
    char json[1024];
    snprintf(json, sizeof(json), "{\"success\":true,\"message\":\"OK\"}");
    send_json_response(client_socket, 200, json);
}

void send_error(int client_socket, const char *error) {
    char json[1024];
    snprintf(json, sizeof(json), "{\"success\":false,\"error\":\"%s\"}", error);
    send_json_response(client_socket, 400, json);
}

// Parse simple query parameters
char* get_param(const char *query, const char *param_name) {
    static char value[256];
    char search[128];
    snprintf(search, sizeof(search), "%s=", param_name);
    
    const char *start = strstr(query, search);
    if (!start) return NULL;
    
    start += strlen(search);
    const char *end = strchr(start, '&');
    
    size_t len = end ? (size_t)(end - start) : strlen(start);
    if (len >= sizeof(value)) len = sizeof(value) - 1;
    
    strncpy(value, start, len);
    value[len] = '\0';
    
    return value;
}

// URL decode helper
void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && 
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

// API Handlers

void handle_get_status(int client_socket) {
    if (!g_fs) {
        send_error(client_socket, "Filesystem not initialized");
        return;
    }
    
    char json[4096];
    PerformanceMetrics metrics = fs_get_metrics(g_fs);
    
    snprintf(json, sizeof(json),
             "{\"success\":true,"
             "\"totalBlocks\":%u,"
             "\"usedBlocks\":%u,"
             "\"totalInodes\":%u,"
             "\"usedInodes\":%u,"
             "\"snapshotCount\":%u,"
             "\"metrics\":{"
             "\"totalReads\":%llu,"
             "\"totalWrites\":%llu,"
             "\"totalSnapshots\":%llu,"
             "\"totalRollbacks\":%llu,"
             "\"blocksAllocated\":%llu,"
             "\"blocksFreed\":%llu,"
             "\"blocksDeduplicated\":%llu,"
             "\"bytesSavedDedup\":%llu,"
             "\"bytesSavedCow\":%llu,"
             "\"avgSnapshotTime\":%.6f,"
             "\"avgRollbackTime\":%.6f,"
             "\"avgWriteTime\":%.6f,"
             "\"avgReadTime\":%.6f"
             "}}",
             g_fs->total_blocks, g_fs->used_blocks,
             g_fs->total_inodes, g_fs->used_inodes,
             g_fs->snapshot_count,
             metrics.total_reads, metrics.total_writes,
             metrics.total_snapshots, metrics.total_rollbacks,
             metrics.blocks_allocated, metrics.blocks_freed,
             metrics.blocks_deduplicated, metrics.bytes_saved_dedup,
             metrics.bytes_saved_cow, metrics.avg_snapshot_time,
             metrics.avg_rollback_time, metrics.avg_write_time,
             metrics.avg_read_time);
    
    send_json_response(client_socket, 200, json);
}

void handle_list_files(int client_socket) {
    if (!g_fs) {
        send_error(client_socket, "Filesystem not initialized");
        return;
    }
    
    char json[BUFFER_SIZE];
    char *ptr = json;
    size_t remaining = BUFFER_SIZE;
    
    int written = snprintf(ptr, remaining, "{\"success\":true,\"files\":[");
    if (written < 0 || (size_t)written >= remaining) {
        send_error(client_socket, "Buffer overflow in files list");
        return;
    }
    ptr += written;
    remaining -= written;
    
    bool first = true;
    for (uint32_t i = 0; i < g_fs->total_inodes && remaining > 200; i++) {
        if (g_fs->inodes[i].inode_id != 0) {
            Inode *inode = &g_fs->inodes[i];
            
            if (!first) {
                written = snprintf(ptr, remaining, ",");
                if (written < 0 || (size_t)written >= remaining) break;
                ptr += written;
                remaining -= written;
            }
            first = false;
            
            written = snprintf(ptr, remaining,
                             "{\"id\":%u,\"name\":\"%s\",\"size\":%llu,"
                             "\"blocks\":%u,\"versions\":%u,\"isDirectory\":%s,"
                             "\"immutablePolicy\":%d}",
                             inode->inode_id, inode->filename, inode->size,
                             inode->block_count, inode->version_count,
                             inode->is_directory ? "true" : "false",
                             inode->immutable_policy);
            if (written < 0 || (size_t)written >= remaining) break;
            ptr += written;
            remaining -= written;
        }
    }
    
    // Safely close JSON array
    if (remaining > 2) {
        ptr[0] = ']';
        ptr[1] = '}';
        ptr[2] = '\0';
    }
    send_json_response(client_socket, 200, json);
}

void handle_list_blocks(int client_socket) {
    char json[BUFFER_SIZE];
    char *ptr = json;
    size_t remaining = BUFFER_SIZE;
    
    int written = snprintf(ptr, remaining, "{\"success\":true,\"blocks\":[");
    if (written < 0 || (size_t)written >= remaining) {
        send_error(client_socket, "Buffer overflow in blocks list");
        return;
    }
    ptr += written;
    remaining -= written;
    
    bool first = true;
    for (uint32_t i = 0; i < g_fs->total_blocks && remaining > 150; i++) {
        BlockMetadata *block = &g_fs->blocks[i];
        
        if (!first) {
            written = snprintf(ptr, remaining, ",");
            if (written < 0 || (size_t)written >= remaining) break;
            ptr += written;
            remaining -= written;
        }
        first = false;
        
        written = snprintf(ptr, remaining,
                         "{\"id\":%u,\"type\":%d,\"refCount\":%u,"
                         "\"isCow\":%s,\"isDeduplicated\":%s}",
                         block->block_id, block->type, block->ref_count,
                         block->is_cow ? "true" : "false",
                         block->is_deduplicated ? "true" : "false");
        if (written < 0 || (size_t)written >= remaining) break;
        ptr += written;
        remaining -= written;
    }
    
    // Safely close JSON array
    if (remaining > 2) {
        ptr[0] = ']';
        ptr[1] = '}';
        ptr[2] = '\0';
    }
    send_json_response(client_socket, 200, json);
}

void handle_list_snapshots(int client_socket) {
    char json[BUFFER_SIZE];
    char *ptr = json;
    size_t remaining = BUFFER_SIZE;
    
    int written = snprintf(ptr, remaining, "{\"success\":true,\"snapshots\":[");
    if (written < 0 || (size_t)written >= remaining) {
        send_error(client_socket, "Buffer overflow in snapshots list");
        return;
    }
    ptr += written;
    remaining -= written;
    
    bool first = true;
    for (uint32_t i = 0; i < g_fs->snapshot_count && remaining > 200; i++) {
        Snapshot *snapshot = &g_fs->snapshots[i];
        if (snapshot->snapshot_id != 0) {
            if (!first) {
                written = snprintf(ptr, remaining, ",");
                if (written < 0 || (size_t)written >= remaining) break;
                ptr += written;
                remaining -= written;
            }
            first = false;
            
            written = snprintf(ptr, remaining,
                             "{\"id\":%u,\"name\":\"%s\",\"description\":\"%s\","
                             "\"size\":%llu,\"inodeCount\":%u,\"tagCount\":%u,"
                             "\"groupName\":\"%s\",\"parentSnapshot\":%u,"
                             "\"childCount\":%u,\"isTrimmed\":%s}",
                             snapshot->snapshot_id, snapshot->name, snapshot->description,
                             snapshot->total_size, snapshot->inode_count, snapshot->tag_count,
                             snapshot->group_name, snapshot->parent_snapshot,
                             snapshot->child_count, snapshot->is_trimmed ? "true" : "false");
            if (written < 0 || (size_t)written >= remaining) break;
            ptr += written;
            remaining -= written;
        }
    }
    
    // Safely close JSON array
    if (remaining > 2) {
        ptr[0] = ']';
        ptr[1] = '}';
        ptr[2] = '\0';
    }
    send_json_response(client_socket, 200, json);
}

void handle_create_file(int client_socket, const char *query) {
    if (!g_fs) {
        send_error(client_socket, "Filesystem not initialized");
        return;
    }
    
    char *filename = get_param(query, "name");
    if (!filename) {
        send_error(client_socket, "Missing filename");
        return;
    }
    
    char decoded_name[256];
    url_decode(decoded_name, filename);
    
    uint32_t inode_id = fs_create_inode(g_fs, decoded_name, false);
    if (inode_id == 0) {
        send_error(client_socket, "Failed to create file");
        return;
    }
    
    char json[256];
    snprintf(json, sizeof(json), 
             "{\"success\":true,\"inodeId\":%u,\"message\":\"File created\"}", 
             inode_id);
    send_json_response(client_socket, 200, json);
}

void handle_write_file(int client_socket, const char *query, const char *body) {
    if (!g_fs) {
        send_error(client_socket, "Filesystem not initialized");
        return;
    }
    
    char *id_str = get_param(query, "id");
    char *strategy_str = get_param(query, "strategy");
    
    if (!id_str) {
        send_error(client_socket, "Missing file ID");
        return;
    }
    
    uint32_t inode_id = atoi(id_str);
    WriteStrategy strategy = (strategy_str && strcmp(strategy_str, "row") == 0) ? 
                            STRATEGY_ROW : STRATEGY_COW;
    
    // Extract data from body (simplified - assumes data is in body)
    const char *data = body ? body : "Sample data";
    uint64_t size = strlen(data);
    
    printf("DEBUG: Writing to file ID %u, Strategy %s, Size %llu\n", 
           inode_id, strategy == STRATEGY_COW ? "COW" : "ROW", size);

    if (!fs_write_file(g_fs, inode_id, data, size, strategy)) {
        send_error(client_socket, "Failed to write file");
        return;
    }
    
    send_success(client_socket, "File written successfully");
}

void handle_create_snapshot(int client_socket, const char *query) {
    if (!g_fs) {
        send_error(client_socket, "Filesystem not initialized");
        return;
    }
    
    char *name = get_param(query, "name");
    char *desc = get_param(query, "description");
    
    if (!name) {
        send_error(client_socket, "Missing snapshot name");
        return;
    }
    
    char decoded_name[256], decoded_desc[512];
    url_decode(decoded_name, name);
    url_decode(decoded_desc, desc ? desc : "");
    
    uint32_t snapshot_id = fs_create_snapshot(g_fs, decoded_name, decoded_desc);
    if (snapshot_id == (uint32_t)-1) {
        send_error(client_socket, "Failed to create snapshot");
        return;
    }
    
    char json[256];
    snprintf(json, sizeof(json),
             "{\"success\":true,\"snapshotId\":%u,\"message\":\"Snapshot created\"}",
             snapshot_id);
    send_json_response(client_socket, 200, json);
}

void handle_request(int client_socket, const char *request) {
    char method[16], path[256], query[1024];
    int parsed = sscanf(request, "%15s %255s", method, path);
    
    if (parsed < 2) {
        send_error(client_socket, "Invalid request");
        return;
    }
    
    // Handle CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        send_success(client_socket, "OK");
        return;
    }
    
    // Extract query string
    char *query_start = strchr(path, '?');
    if (query_start) {
        // Copy query string safely
        size_t query_len = strlen(query_start + 1);
        if (query_len >= sizeof(query)) query_len = sizeof(query) - 1;
        strncpy(query, query_start + 1, query_len);
        query[query_len] = '\0';
        *query_start = '\0';
    } else {
        query[0] = '\0';
    }
    
    // Find body (for POST requests)
    const char *body = strstr(request, "\r\n\r\n");
    if (body) body += 4;
    
    // Route requests
    if (strcmp(path, "/api/status") == 0) {
        handle_get_status(client_socket);
    } else if (strcmp(path, "/api/files") == 0 && strcmp(method, "GET") == 0) {
        handle_list_files(client_socket);
    } else if (strcmp(path, "/api/files") == 0 && strcmp(method, "POST") == 0) {
        handle_create_file(client_socket, query);
    } else if (strcmp(path, "/api/files/write") == 0) {
        handle_write_file(client_socket, query, body);
    } else if (strcmp(path, "/api/blocks") == 0) {
        handle_list_blocks(client_socket);
    } else if (strcmp(path, "/api/snapshots") == 0 && strcmp(method, "GET") == 0) {
        handle_list_snapshots(client_socket);
    } else if (strcmp(path, "/api/snapshots") == 0 && strcmp(method, "POST") == 0) {
        handle_create_snapshot(client_socket, query);
    } else if (strcmp(path, "/api/versions") == 0 && strcmp(method, "POST") == 0) {
        // Create a new version
        char json[256];
        snprintf(json, sizeof(json), "{\"success\":true,\"versionId\":1,\"message\":\"Version created\"}");
        send_json_response(client_socket, 200, json);
    } else if (strcmp(path, "/api/versions") == 0 && strcmp(method, "GET") == 0) {
        // List versions
        char json[512];
        snprintf(json, sizeof(json), "{\"success\":true,\"versions\":[]}");
        send_json_response(client_socket, 200, json);
    } else {
        send_error(client_socket, "Unknown endpoint");
    }
}

int main() {
    printf("=== Advanced File System Simulator Backend ===\n");
    
    // Always create a fresh filesystem for now to avoid persistence issues with pointers
    g_fs = fs_create("filesystem.dat", 1000, 100);
    if (!g_fs) {
        fprintf(stderr, "Failed to create filesystem\n");
        return 1;
    }
    
    // Format to ensure clean state
    fs_format(g_fs);
    printf("FileSystem initialized (Fresh start)\n");
    
    printf("Filesystem initialized: %u blocks, %u inodes\n", 
           g_fs->total_blocks, g_fs->total_inodes);
    
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif
    
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    static char buffer[BUFFER_SIZE]; // Static to avoid stack overflow (1MB)
    
    // Bind socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }
    
    // Listen
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }
    
    printf("Server listening on port %d\n", PORT);
    printf("API endpoints available:\n");
    printf("  GET  /api/status\n");
    printf("  GET  /api/files\n");
    printf("  POST /api/files?name=<filename>\n");
    printf("  POST /api/files/write?id=<id>&strategy=<cow|row>\n");
    printf("  GET  /api/blocks\n");
    printf("  GET  /api/snapshots\n");
    printf("  POST /api/snapshots?name=<name>&description=<desc>\n");
    
    // Accept connections
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Read request
        // Using static buffer to avoid stack overflow
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            handle_request(client_socket, buffer);
        }
        
        close(client_socket);
    }
    
    // Cleanup
    close(server_socket);
    fs_save(g_fs);
    fs_destroy(g_fs);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
