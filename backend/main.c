#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>

// Versioning function prototypes (from versioning.c)
uint32_t fs_create_version(FileSystem *fs, uint32_t inode_id, const char *description);
// Forward declaration from versioning.c / filesystem API
Inode* fs_get_inode_by_name(FileSystem *fs, const char *filename);
bool fs_rollback_version(FileSystem *fs, uint32_t inode_id, uint32_t version_id);
bool fs_add_version_tag(FileSystem *fs, uint32_t inode_id, uint32_t version_id,
                        const char *tag, const char *description);
void fs_list_versions(FileSystem *fs, uint32_t inode_id);


#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 1048576   // 1 MB

/* ================= GLOBALS ================= */
FileSystem *g_fs = NULL;

/* ================= HELPERS ================= */
void send_error(int client_socket, const char *error);

void send_json_response(int client_socket, int status_code, const char *json) {
    if (!json) {
        send_error(client_socket, "Invalid JSON");
        return;
    }

    size_t len = strlen(json);
    char header[2048];

    const char *status_text = (status_code == 200) ? "OK" : "Error";
    int header_len = snprintf(
        header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %llu\r\n"
        "\r\n",
        status_code, status_text, (unsigned long long)len
    );

    if (header_len < 0) return;

    send(client_socket, header, header_len, 0);
    send(client_socket, json, len, 0);
}

void send_success(int client_socket, const char *message) {
    char json[1024];
    snprintf(json, sizeof(json),
             "{\"success\":true,\"message\":\"%s\"}",
             message ? message : "OK");
    send_json_response(client_socket, 200, json);
}

void send_error(int client_socket, const char *error) {
    char json[1024];
    snprintf(json, sizeof(json),
             "{\"success\":false,\"error\":\"%s\"}",
             error ? error : "Unknown error");
    send_json_response(client_socket, 400, json);
}

/* ================= QUERY PARSING ================= */
char* get_param(const char *query, const char *param_name) {
    static char value[256];
    char key[128];

    snprintf(key, sizeof(key), "%s=", param_name);
    const char *start = strstr(query, key);
    if (!start) return NULL;

    start += strlen(key);
    const char *end = strchr(start, '&');

    size_t len = end ? (size_t)(end - start) : strlen(start);
    if (len >= sizeof(value)) len = sizeof(value) - 1;

    strncpy(value, start, len);
    value[len] = '\0';
    return value;
}

void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if (*src == '%' &&
            (a = src[1]) && (b = src[2]) &&
            isxdigit(a) && isxdigit(b)) {

            a = (a >= 'a') ? a - 'a' + 10 :
                (a >= 'A') ? a - 'A' + 10 : a - '0';
            b = (b >= 'a') ? b - 'a' + 10 :
                (b >= 'A') ? b - 'A' + 10 : b - '0';

            *dst++ = (char)(16 * a + b);
            src += 3;
        }
        else if (*src == '+') {
            *dst++ = ' ';
            src++;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Extract body from HTTP request
const char* get_body(const char *request) {
    const char *body = strstr(request, "\r\n\r\n");
    if (!body) return NULL;
    return body + 4;
}

// Extract JSON value from simple flat JSON
char* extract_json_value(const char *json, const char *key, char *out, size_t out_size) {
    if (!json || !key || !out) return NULL;
    char *pos = strstr(json, key);
    if (!pos) return NULL;

    pos = strchr(pos, ':');
    if (!pos) return NULL;
    pos++; // skip :

    while (*pos && isspace(*pos)) pos++;
    if (*pos == '"') pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && *pos != ',' && *pos != '}' && i < out_size - 1) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return out;
}

/* ================= API HANDLERS ================= */

void handle_get_status(int client_socket) {
    if (!g_fs) { send_error(client_socket, "Filesystem not initialized"); return; }
    PerformanceMetrics m = fs_get_metrics(g_fs);

    uint32_t dedup_count = fs_count_dedup_blocks(g_fs);  // <-- new line

    char json[4096];
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
        "\"blocksDeduplicated\":%u,"       // <-- changed to %u
        "\"bytesSavedDedup\":%llu,"
        "\"bytesSavedCow\":%llu,"
        "\"avgSnapshotTime\":%.6f,"
        "\"avgRollbackTime\":%.6f,"
        "\"avgWriteTime\":%.6f,"
        "\"avgReadTime\":%.6f"
        "}}",
        g_fs->total_blocks,
        g_fs->used_blocks,
        g_fs->total_inodes,
        g_fs->used_inodes,
        g_fs->snapshot_count,
        (unsigned long long)m.total_reads,
        (unsigned long long)m.total_writes,
        (unsigned long long)m.total_snapshots,
        (unsigned long long)m.total_rollbacks,
        (unsigned long long)m.blocks_allocated,
        (unsigned long long)m.blocks_freed,
        dedup_count,                        // <-- use helper here
        (unsigned long long)m.bytes_saved_dedup,
        (unsigned long long)m.bytes_saved_cow,
        m.avg_snapshot_time,
        m.avg_rollback_time,
        m.avg_write_time,
        m.avg_read_time
    );
    send_json_response(client_socket, 200, json);
}

void handle_list_files(int client_socket) {
    if (!g_fs) { send_error(client_socket, "Filesystem not initialized"); return; }

    char json[BUFFER_SIZE]; char *ptr = json;
    ptr += snprintf(ptr, BUFFER_SIZE, "{\"success\":true,\"files\":[");
    bool first = true;

    for (uint32_t i = 0; i < g_fs->total_inodes; i++) {
        Inode *inode = &g_fs->inodes[i];
        if (inode->inode_id == 0) continue;
        if (!first) { *ptr++ = ','; } first = false;

        ptr += snprintf(ptr, BUFFER_SIZE - (ptr - json),
            "{\"id\":%u,\"name\":\"%s\",\"size\":%llu,"
            "\"blocks\":%u,\"versions\":%u,"
            "\"isDirectory\":%s,\"immutablePolicy\":%d}",
            inode->inode_id,
            inode->filename,
            (unsigned long long)inode->size,
            inode->block_count,
            inode->version_count,
            inode->is_directory ? "true" : "false",
            inode->immutable_policy
        );
    }
    strcat(ptr, "]}");
    send_json_response(client_socket, 200, json);
}

/* ================= UPDATED BLOCKS API ================= */
void handle_list_blocks(int client_socket) {
    if (!g_fs) { send_error(client_socket, "Filesystem not initialized"); return; }

    char json[BUFFER_SIZE]; char *ptr = json;
    ptr += snprintf(ptr, BUFFER_SIZE, "{\"success\":true,\"blocks\":[");
    bool first = true;

    for (uint32_t i = 0; i < g_fs->total_blocks; i++) {
        BlockMetadata *b = &g_fs->blocks[i]; // get actual block struct
        if (!first) { *ptr++ = ','; } first = false;

        ptr += snprintf(ptr, BUFFER_SIZE - (ptr - json),
            "{\"id\":%u,\"type\":%u,\"refCount\":%u,\"isCow\":%s,\"isDeduplicated\":%s}",
            i,
            b->type,
            b->ref_count,
            b->is_cow ? "true" : "false",
            b->is_deduplicated ? "true" : "false"
        );
    }
    strcat(ptr, "]}");
    send_json_response(client_socket, 200, json);
}

void handle_list_snapshots(int client_socket) {
    if (!g_fs) { send_error(client_socket, "Filesystem not initialized"); return; }

    char json[BUFFER_SIZE]; char *ptr = json;
    ptr += snprintf(ptr, BUFFER_SIZE, "{\"success\":true,\"snapshots\":[");
    bool first = true;

    for (uint32_t i = 0; i < g_fs->snapshot_count; i++) {
        Snapshot *snapshot = &g_fs->snapshots[i];
        if (!first) { *ptr++ = ','; } first = false;
        ptr += snprintf(ptr, BUFFER_SIZE - (ptr - json),
            "{\"id\":%u,\"name\":\"%s\",\"totalSize\":%llu,\"inodeCount\":%u,\"tagCount\":%u}",
            snapshot->snapshot_id,
            snapshot->name,
            (unsigned long long)snapshot->total_size,
            snapshot->inode_count,
            snapshot->tag_count
        );
    }
    strcat(ptr, "]}");
    send_json_response(client_socket, 200, json);
}

/* ================= CREATE FILE ================= */
void handle_create_file(int client_socket, const char *query) {
    char *name = get_param(query, "name");
    if (!name || strlen(name) == 0) { send_error(client_socket, "Missing file name"); return; }

    char decoded_name[256]; url_decode(decoded_name, name);

    if (!fs_create_file(g_fs, decoded_name, 0)) { send_error(client_socket, "Failed to create file"); return; }

    send_success(client_socket, "File created successfully");
}

/* ================= WRITE FILE ================= */
void handle_write_file(int client_socket, const char *body) {
    if (!body || strlen(body) == 0) { send_error(client_socket, "Missing body"); return; }

    char name[256] = {0};
    char strategy[16] = {0};
    char data[BUFFER_SIZE] = {0};

    extract_json_value(body, "\"name\"", name, sizeof(name));
    extract_json_value(body, "\"strategy\"", strategy, sizeof(strategy));
    extract_json_value(body, "\"data\"", data, sizeof(data));

    if (strlen(name) == 0 || strlen(strategy) == 0) {
        send_error(client_socket, "Missing parameters in body");
        return;
    }

    WriteStrategy ws = (strcmp(strategy, "cow") == 0) ? STRATEGY_COW : STRATEGY_ROW;
    Inode updated_inode;

    if (!fs_write_file_api(g_fs, name, data, strlen(data), ws, &updated_inode)) {
        send_error(client_socket, "Failed to write file");
        return;
    }

    char json[1024];
    snprintf(json, sizeof(json),
             "{\"success\":true,\"message\":\"Data written successfully\",\"inodeId\":%u,\"size\":%llu}",
             updated_inode.inode_id,
             (unsigned long long)updated_inode.size);

    send_json_response(client_socket, 200, json);
}

/* ================= CREATE SNAPSHOT ================= */
void handle_create_snapshot(int client_socket, const char *body) {
    if (!body || strlen(body) == 0) { send_error(client_socket, "Missing body"); return; }

    char name[256] = {0};
    char description[256] = {0};

    extract_json_value(body, "\"name\"", name, sizeof(name));
    extract_json_value(body, "\"description\"", description, sizeof(description));

    if (strlen(name) == 0) { send_error(client_socket, "Missing snapshot name"); return; }

    if (!fs_create_snapshot(g_fs, name, description)) {
        send_error(client_socket, "Failed to create snapshot");
        return;
    }

    send_success(client_socket, "Snapshot created successfully");
}

/* ================= ROLLBACK SNAPSHOT BY NAME ================= */
void handle_rollback_snapshot(int client_socket, const char *query) {
    if (!query || strlen(query) == 0) {
        send_error(client_socket, "Missing query");
        return;
    }

    char *raw_name = get_param(query, "name");
    if (!raw_name) {
        send_error(client_socket, "Missing snapshot name");
        return;
    }

    char name[256];
    url_decode(name, raw_name);

    // Find snapshot by name
    uint32_t snapshot_id = 0;
    bool found = false;

    for (uint32_t i = 0; i < g_fs->snapshot_count; i++) {
        if (strcmp(g_fs->snapshots[i].name, name) == 0) {
            snapshot_id = g_fs->snapshots[i].snapshot_id;
            found = true;
            break;
        }
    }

    if (!found) {
        send_error(client_socket, "Snapshot not found");
        return;
    }

    if (!fs_rollback_snapshot(g_fs, snapshot_id)) {
        send_error(client_socket, "Rollback failed");
        return;
    }

    send_success(client_socket, "Rollback successful");
}
void handle_create_version(int client_socket, const char *body) {
    if (!body) { send_error(client_socket, "Missing body"); return; }

    char name[256] = {0};      // filename
    char description[256] = {0};

    extract_json_value(body, "\"name\"", name, sizeof(name));
    extract_json_value(body, "\"description\"", description, sizeof(description));

    if (strlen(name) == 0) { send_error(client_socket, "Missing file name"); return; }

    // Find inode by name
    Inode *inode = fs_get_inode_by_name(g_fs, name);
    if (!inode) { send_error(client_socket, "File not found"); return; }

    uint32_t version_id = fs_create_version(g_fs, inode->inode_id, description);
    if (version_id == 0) { send_error(client_socket, "Failed to create version"); return; }

    char json[512];
    snprintf(json, sizeof(json),
             "{\"success\":true,\"message\":\"Version created\",\"versionId\":%u}",
             version_id);
    send_json_response(client_socket, 200, json);
}

void handle_rollback_version(int client_socket, const char *body) {
    if (!body) { send_error(client_socket, "Missing body"); return; }

    char name[256] = {0};
    char version_str[16] = {0};

    extract_json_value(body, "\"name\"", name, sizeof(name));
    extract_json_value(body, "\"versionId\"", version_str, sizeof(version_str));

    if (strlen(name) == 0 || strlen(version_str) == 0) {
        send_error(client_socket, "Missing parameters");
        return;
    }

    uint32_t version_id = atoi(version_str);

    // Find inode
    Inode *inode = fs_get_inode_by_name(g_fs, name);
    if (!inode) { send_error(client_socket, "File not found"); return; }

    if (!fs_rollback_version(g_fs, inode->inode_id, version_id)) {
        send_error(client_socket, "Rollback failed");
        return;
    }

    send_success(client_socket, "Rollback successful");
}

void handle_list_versions(int client_socket, const char *query) {
    if (!query) { send_error(client_socket, "Missing query"); return; }

    char *raw_name = get_param(query, "name");
    if (!raw_name) { send_error(client_socket, "Missing file name"); return; }

    char name[256]; url_decode(name, raw_name);

    Inode *inode = fs_get_inode_by_name(g_fs, name);
    if (!inode) { send_error(client_socket, "File not found"); return; }

    // For simplicity, just print JSON text directly
    char json[BUFFER_SIZE]; char *ptr = json;
    ptr += snprintf(ptr, BUFFER_SIZE, "{\"success\":true,\"versions\":[");
    bool first = true;

    for (uint32_t v = 0; v < inode->version_count; v++) {
        FileVersion *version = &inode->versions[v];
        if (!first) { *ptr++ = ','; } first = false;

        ptr += snprintf(ptr, BUFFER_SIZE - (ptr - json),
            "{\"versionId\":%u,\"size\":%llu,\"blocks\":%u,\"description\":\"%s\",\"tags\":%u}",
            version->version_id,
            (unsigned long long)version->size,
            version->block_count,
            version->description,
            version->tag_count
        );
    }
    strcat(ptr, "]}");
    send_json_response(client_socket, 200, json);
}

/* ================= REQUEST ROUTER ================= */
void handle_request(int client_socket, const char *request) {
    char method[16], path[256], query[1024] = {0};
    sscanf(request, "%15s %255s", method, path);

    if (strcmp(method, "OPTIONS") == 0) {
        const char *resp =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n\r\n";
        send(client_socket, resp, strlen(resp), 0);
        return;
    }

    char *q = strchr(path, '?');
    if (q) {
        strncpy(query, q + 1, sizeof(query) - 1);
        *q = '\0';
    }

    if (strcmp(path, "/api/status") == 0) handle_get_status(client_socket);
    else if (strcmp(path, "/api/files") == 0) {
        if (strcmp(method, "GET") == 0) handle_list_files(client_socket);
        else if (strcmp(method, "POST") == 0) handle_create_file(client_socket, query);
        else send_error(client_socket, "Method not allowed");
    }
    else if (strcmp(path, "/api/files/write") == 0 && strcmp(method, "POST") == 0) {
        handle_write_file(client_socket, get_body(request));
    }
    else if (strcmp(path, "/api/blocks") == 0 && strcmp(method, "GET") == 0) handle_list_blocks(client_socket);
    else if (strcmp(path, "/api/snapshots") == 0 && strcmp(method, "GET") == 0) handle_list_snapshots(client_socket);
    else if (strcmp(path, "/api/snapshots") == 0 && strcmp(method, "POST") == 0) {
        handle_create_snapshot(client_socket, get_body(request));
    }
    else if (strcmp(path, "/api/snapshots/rollback") == 0 && strcmp(method, "POST") == 0) {
        handle_rollback_snapshot(client_socket, query);
    }
    else if (strcmp(path, "/api/versions/create") == 0 && strcmp(method, "POST") == 0) {
    handle_create_version(client_socket, get_body(request));
    }
    else if (strcmp(path, "/api/versions/rollback") == 0 && strcmp(method, "POST") == 0) {
    handle_rollback_version(client_socket, get_body(request));
    }
    else if (strcmp(path, "/api/versions") == 0 && strcmp(method, "GET") == 0) {
    handle_list_versions(client_socket, query);
    }
    else send_error(client_socket, "Unknown endpoint");
}

/* ================= MAIN ================= */
int main(void) {
    printf("=== Advanced File System Simulator Backend ===\n");

    g_fs = fs_create("filesystem.dat", 1000, 100);
    if (!g_fs) { fprintf(stderr, "Filesystem creation failed\n"); return 1; }

    fs_format(g_fs);
    printf("Filesystem initialized\n");

#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    int server = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 10);

    printf("Server running on http://localhost:%d\n", PORT);

    static char buffer[BUFFER_SIZE];

    while (1) {
        int client = accept(server, NULL, NULL);
        int n = recv(client, buffer, BUFFER_SIZE - 1, 0);
        if (n > 0) { buffer[n] = '\0'; handle_request(client, buffer); }
        close(client);
    }

    return 0;
}