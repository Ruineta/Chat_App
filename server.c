#include "server.h"
#include <ctype.h>
#include "common.h"


ServerState server_state;

#define ACCOUNT_FILE "account.txt"
#define MAX_POLL_CLIENTS 1024 // Maximum concurrent connections

// Session structure to replace thread-local variables
typedef struct {
    int fd;
    User* user; // Pointer to logged-in user in server_state, or NULL
    char current_chat_partner[MAX_USERNAME]; // Target being actively viewed
} ClientSession;

struct pollfd poll_fds[MAX_POLL_CLIENTS];
ClientSession sessions[MAX_POLL_CLIENTS];
int max_nfds = 0; // Current number of monitored FDs

// --- Forward Declarations ---
void remove_client(int index);
void handle_client_message(int index);
void process_command(int client_fd, ProtocolMessage* msg, ClientSession* session, int index);
// ----------------------------

int load_accounts(const char *filename){
    FILE *file = fopen(filename, "r");
    if (!file) {
        FILE *f = fopen(filename, "a");
        if (!f) {
            perror("Could not create account file");
            return -1;
        }
        fclose(f);
        return 0;
    }

    char username[MAX_USERNAME], password[MAX_USERNAME];
    int loaded = 0;
    while (fscanf(file, "%49s %49s", username, password) == 2) {
        if (server_state.user_count >= (int)(sizeof(server_state.users) / sizeof(server_state.users[0]))) break;
        add_user(&server_state, username, password);
        loaded++;
    }
    fclose(file);
    return loaded;
}

int save_account(const char *filename, const char *username, const char *password) {
    FILE *file = fopen(filename, "a");
    if (!file) { perror("Could not open account file"); return -1; }
    if (fprintf(file, "%s %s\n", username, password) < 0) { fclose(file); return -1; }
    fclose(file);
    return 0;
}

void save_friends_state(ServerState* state) {
    FILE* file = fopen("friends.txt", "w");
    if (!file) return;
    for (int i = 0; i < state->user_count; i++) {
        User* u = &state->users[i];
        fprintf(file, "%s|", u->username);
        for (int j = 0; j < u->friend_count; j++) fprintf(file, "%s%s", u->friends[j], (j < u->friend_count - 1) ? "," : "");
        fprintf(file, "|");
        for (int j = 0; j < u->request_count; j++) fprintf(file, "%s%s", u->friend_requests[j], (j < u->request_count - 1) ? "," : "");
        fprintf(file, "|");
        for (int j = 0; j < u->blocked_count; j++) fprintf(file, "%s%s", u->blocked_users[j], (j < u->blocked_count - 1) ? "," : "");
        fprintf(file, "\n");
    }
    fclose(file);
}

void load_friends_state(ServerState* state) {
    FILE* file = fopen("friends.txt", "r");
    if (!file) return;
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        trim_newline(line);
        char* p = line;
        char* pipe1 = strchr(p, '|'); if (!pipe1) continue;
        *pipe1 = '\0';
        char* username = p; p = pipe1 + 1;
        User* u = find_user(state, username); if (!u) continue;
        char* pipe2 = strchr(p, '|'); if (!pipe2) continue;
        *pipe2 = '\0';
        char* friends_part = p; p = pipe2 + 1;
        char* pipe3 = strchr(p, '|'); if (!pipe3) continue;
        *pipe3 = '\0';
        char* requests_part = p; char* blocked_part = pipe3 + 1;
        
        if (strlen(friends_part) > 0) {
            char* f = strtok(friends_part, ",");
            while (f) { if (u->friend_count < MAX_FRIENDS) strncpy(u->friends[u->friend_count++], f, MAX_USERNAME - 1); f = strtok(NULL, ","); }
        }
        if (strlen(requests_part) > 0) {
            char* req = strtok(requests_part, ",");
            while (req) { if (u->request_count < MAX_FRIENDS) strncpy(u->friend_requests[u->request_count++], req, MAX_USERNAME - 1); req = strtok(NULL, ","); }
        }
        if (strlen(blocked_part) > 0) {
            char* blk = strtok(blocked_part, ",");
            while (blk) { if (u->blocked_count < MAX_FRIENDS) strncpy(u->blocked_users[u->blocked_count++], blk, MAX_USERNAME - 1); blk = strtok(NULL, ","); }
        }
    }
    fclose(file);
}

// Updated Message Saving with Human Readable Time
void save_message_to_file(const char* sender, const char* recipient, const char* content, bool is_group) {
    (void)is_group; // Suppress unused warning
    FILE* file = fopen("messages.txt", "a");
    if (file) {
        time_t now = time(NULL);
        time_t now_vn = now + 7 * 3600; // Manual UTC+7
        char date_str[64];
        struct tm* tm_info = gmtime(&now_vn);
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        char msg_id[100];
        snprintf(msg_id, sizeof(msg_id), "%s_%lld", sender, (long long)now);
        
        // FORMAT: ID|UnixTimestamp|DateTime_String|Sender|Receiver|Content
        fprintf(file, "%s|%lld|%s|%s|%s|%s\n", 
                msg_id, (long long)now, date_str, sender, recipient, content);
        fclose(file);
    }
}

// Helpers
void send_friend_request_count(User* user) {
    if (!user || !user->is_online || user->socket == INVALID_SOCKET) return;
    ProtocolMessage pm; memset(&pm, 0, sizeof(pm));
    pm.cmd = CMD_RECEIVE_MESSAGE;
    pm.msg_type = MSG_SYSTEM;
    strcpy(pm.sender, "SYSTEM");
    snprintf(pm.content, MAX_CONTENT, "[FRIEND_COUNT] %d", user->request_count);
    int len; char* b = serialize_protocol_message(&pm, &len);
    if (b) { send_all(user->socket, b, len); free(b); }
}

// --- UNREAD MESSAGE HELPERS ---
void update_last_read(const char* user, const char* partner) {
    FILE* f = fopen("last_read.txt", "r");
    char** lines = NULL; int count = 0;
    if (f) {
        char buf[BUFFER_SIZE];
        while(fgets(buf, sizeof(buf), f)) {
            lines = realloc(lines, sizeof(char*) * (count+1));
            lines[count] = strdup(buf);
            count++;
        }
        fclose(f);
    }
    
    FILE* out = fopen("last_read.txt", "w");
    if (!out) return;
    
    time_t now = time(NULL);
    bool found = false;
    for(int i=0; i<count; i++) {
        char* copy = strdup(lines[i]);
        char* u = strtok(copy, "|"); char* p = strtok(NULL, "|");
        if (u && p && strcmp(u, user) == 0 && strcmp(p, partner) == 0) {
            fprintf(out, "%s|%s|%lld\n", user, partner, (long long)now);
            found = true;
        } else {
            fprintf(out, "%s", lines[i]);
        }
        free(copy); free(lines[i]);
    }
    if (lines) free(lines);
    
    if (!found) {
        fprintf(out, "%s|%s|%lld\n", user, partner, (long long)now);
    }
    fclose(out);
}

time_t get_last_read(const char* user, const char* partner) {
    FILE* f = fopen("last_read.txt", "r");
    if (!f) return 0;
    char buf[BUFFER_SIZE];
    time_t ts = 0;
    while(fgets(buf, sizeof(buf), f)) {
         char* copy = strdup(buf);
         char* u = strtok(copy, "|"); char* p = strtok(NULL, "|"); char* t_str = strtok(NULL, "|");
         if (u && p && t_str && strcmp(u, user) == 0 && strcmp(p, partner) == 0) {
             ts = (time_t)atoll(t_str);
             free(copy); break;
         }
         free(copy);
    }
    fclose(f);
    return ts;
}
// ------------------------------

User* find_user(ServerState* state, const char* username) {
    for (int i = 0; i < state->user_count; i++) if (strcmp(state->users[i].username, username) == 0) return &state->users[i];
    return NULL;
}
Group* find_group(ServerState* state, const char* group_id) {
    for (int i = 0; i < state->group_count; i++) if (strcmp(state->groups[i].group_id, group_id) == 0) return &state->groups[i];
    return NULL;
}

bool is_member_of_group(Group* g, const char* username) {
    if (!g || !username) return false;
    for (int i = 0; i < g->member_count; i++) {
        if (strcmp(g->members[i], username) == 0) return true;
    }
    return false;
}

void get_unread_summary_string(ServerState* state, const char* username, char* output, size_t size) {
    if (!output || size == 0) return;
    output[0] = '\0';

    typedef struct { char name[MAX_GROUP_ID]; int count; bool is_group; } UnreadStat;
    UnreadStat stats[100]; int stat_count = 0;
    
    // CACHE LAST READS for this user
    typedef struct { char p[MAX_GROUP_ID]; time_t t; } LR;
    LR lrs[200]; int lr_count = 0;
    FILE* flr = fopen("last_read.txt", "r");
    if (flr) {
        char b[BUFFER_SIZE];
        while(fgets(b, sizeof(b), flr)) {
            char* tmp = strdup(b);
            char* u = strtok(tmp, "|"); char* p = strtok(NULL, "|"); char* t_str = strtok(NULL, "|");
            if (u && p && t_str && strcmp(u, username) == 0 && lr_count < 200) {
                strncpy(lrs[lr_count].p, p, MAX_GROUP_ID-1);
                lrs[lr_count].t = (time_t)atoll(t_str);
                lr_count++;
            }
            free(tmp);
        }
        fclose(flr);
    }

    // CACHE GROUP MEMBERSHIP for current user
    bool is_member[MAX_GROUPS]; memset(is_member, 0, sizeof(is_member));
    for (int i=0; i<state->group_count; i++) {
        is_member[i] = is_member_of_group(&state->groups[i], username);
    }

    FILE* file = fopen("messages.txt", "r");
    if (!file) {
        strncpy(output, "No messages yet.", size-1);
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char* tmp = strdup(line);
        char* id = strtok(tmp, "|");
        char* ts_str = strtok(NULL, "|");
        char* dt = strtok(NULL, "|");
        char* snd = strtok(NULL, "|");
        char* rcv = strtok(NULL, "|");
        
        if (ts_str && snd && rcv && strcmp(snd, username) != 0) {
            bool relevant = false;
            bool is_grp = (strncmp(rcv, "GRP_", 4) == 0);
            
            if (!is_grp) {
                if (strcmp(rcv, username) == 0) relevant = true;
            } else {
                for(int i=0; i<state->group_count; i++) {
                    if (strcmp(state->groups[i].group_id, rcv) == 0) {
                        if (is_member[i]) relevant = true;
                        break;
                    }
                }
            }

            if (relevant) {
                // For Private: context is Snd (who sent it). For Group: context is Rcv (the GroupID)
                char* context_id = is_grp ? rcv : snd;
                
                time_t t_last = 0;
                for(int i=0; i<lr_count; i++) {
                    if (strcmp(lrs[i].p, context_id) == 0) { t_last = lrs[i].t; break; }
                }
                
                time_t t_msg = (time_t)atoll(ts_str);
                if (t_msg > t_last) {
                    char* target_key = context_id; 
                    bool found = false;
                    for(int i=0; i<stat_count; i++) {
                        if (strcmp(stats[i].name, target_key) == 0) { stats[i].count++; found=true; break; }
                    }
                    if (!found && stat_count < 100) {
                        strncpy(stats[stat_count].name, target_key, MAX_GROUP_ID-1);
                        stats[stat_count].count = 1;
                        stats[stat_count].is_group = is_grp;
                        stat_count++;
                    }
                }
            }
        }
        free(tmp);
    }
    fclose(file);

    if (stat_count == 0) {
        strncpy(output, "No new messages.", size-1);
    } else {
        snprintf(output, size, "Welcome back! UNREAD MESSAGES:\n");
        for (int i = 0; i < stat_count; i++) {
            char row[150];
            if (stats[i].is_group) {
                Group* g = find_group(state, stats[i].name);
                snprintf(row, sizeof(row), "- Group %s: %d new\n", g ? g->name : stats[i].name, stats[i].count);
            } else {
                snprintf(row, sizeof(row), "- From %s: %d new\n", stats[i].name, stats[i].count);
            }
            if (strlen(output) + strlen(row) < size - 1) strcat(output, row);
        }
    }
}
void add_user(ServerState* state, const char* username, const char* password) {
    if (find_user(state, username)) return;
    User* new_user = &state->users[state->user_count++];
    strncpy(new_user->username, username, MAX_USERNAME - 1);
    strncpy(new_user->password, password, MAX_USERNAME - 1);
    new_user->is_online = false;
    new_user->socket = INVALID_SOCKET; // Used for direct lookup, synced with sessions
    new_user->last_seen = 0;
}
void send_response(socket_t socket, CommandType cmd, const char* content) {
    ProtocolMessage msg; memset(&msg, 0, sizeof(msg));
    msg.cmd = cmd; strncpy(msg.content, content, MAX_CONTENT - 1);
    int len; char* buffer = serialize_protocol_message(&msg, &len);
    if (buffer) { send_all(socket, buffer, len); free(buffer); }
}
bool is_blocked(User* user, const char* username) {
    for (int i=0; i<user->blocked_count; i++) if (strcmp(user->blocked_users[i], username) == 0) return true;
    return false;
}
bool are_friends(User* u1, User* u2) {
    for (int i=0; i<u1->friend_count; i++) if (strcmp(u1->friends[i], u2->username) == 0) return true;
    return false;
}
void add_friend(User* u1, User* u2) {
    if (!are_friends(u1, u2)) strncpy(u1->friends[u1->friend_count++], u2->username, MAX_USERNAME - 1);
    if (!are_friends(u2, u1)) strncpy(u2->friends[u2->friend_count++], u1->username, MAX_USERNAME - 1);
}
void broadcast_to_friends(ServerState* state, const char* username, const char* message) {
    User* user = find_user(state, username); if (!user) return;
    ProtocolMessage msg; memset(&msg, 0, sizeof(msg));
    msg.cmd = CMD_RECEIVE_MESSAGE; strncpy(msg.sender, username, MAX_USERNAME-1); 
    strncpy(msg.content, message, MAX_CONTENT-1); msg.msg_type = MSG_SYSTEM;
    int len; char* buffer = serialize_protocol_message(&msg, &len);
    for(int i=0; i<user->friend_count; i++) {
        User* f = find_user(state, user->friends[i]);
        if(f && f->is_online && f->socket != INVALID_SOCKET) send_all(f->socket, buffer, len);
    }
    free(buffer);
}
char** search_messages(const char* keyword, const char* username, const char* recipient, int* result_count) {
    char** results = malloc(100 * sizeof(char*)); *result_count = 0;
    FILE* file = fopen("messages.txt", "r"); if (!file) return results;
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file) && *result_count < 100) {
        char* tmp = strdup(line);
        char* id = strtok(tmp, "|");
        char* ts = strtok(NULL, "|");
        char* dt = strtok(NULL, "|");
        char* snd = strtok(NULL, "|");
        char* rcv = strtok(NULL, "|");
        char* cnt = strtok(NULL, "|");
        
        if (snd && rcv && cnt) {
            trim_newline(cnt);
            // Relevance check: username must be sender or receiver
            bool is_relevant = (strcmp(snd, username) == 0 || strcmp(rcv, username) == 0);
            
            // Recipient filter (if specified)
            if (is_relevant && strlen(recipient) > 0) {
                if (strcmp(rcv, recipient) != 0) is_relevant = false;
            }
            
            // Keyword check in content
            if (is_relevant && strstr(cnt, keyword)) {
                char result[MAX_CONTENT];
                bool is_grp = (strncmp(rcv, "GRP_", 4) == 0);
                if (is_grp) {
                    snprintf(result, sizeof(result), "[%s] %s @ Group %s: \"%s\"", dt, snd, rcv, cnt);
                } else {
                    snprintf(result, sizeof(result), "[%s] %s -> %s: \"%s\"", dt, snd, rcv, cnt);
                }
                results[*result_count] = strdup(result);
                (*result_count)++;
            }
        }
        free(tmp);
    }
    fclose(file); return results;
}

// --- GROUP PERSISTENCE ---
void save_groups(ServerState* state) {
    FILE* f = fopen("groups.txt", "w");
    if (!f) return;
    for(int i=0; i<state->group_count; i++) {
        Group* g = &state->groups[i];
        // Format: ID|NAME|CREATOR|ADMIN_COUNT|MEMBER_COUNT|ADMINs...|MEMBERs...
        fprintf(f, "%s|%s|%s|%d|%d", g->group_id, g->name, g->creator, g->admin_count, g->member_count);
        for(int j=0; j<g->admin_count; j++) fprintf(f, "|%s", g->admins[j]);
        for(int j=0; j<g->member_count; j++) fprintf(f, "|%s", g->members[j]);
        fprintf(f, "\n");
    }
    fclose(f);
}

void load_groups(ServerState* state) {
    FILE* f = fopen("groups.txt", "r");
    if (!f) return;
    char line[4096];
    while(fgets(line, sizeof(line), f)) {
        if (state->group_count >= MAX_GROUPS) break;
        Group* g = &state->groups[state->group_count];
        trim_newline(line);
        
        char* token = strtok(line, "|"); if(!token) continue; strncpy(g->group_id, token, MAX_GROUP_ID-1);
        token = strtok(NULL, "|"); if(!token) continue; strncpy(g->name, token, MAX_GROUP_NAME-1);
        token = strtok(NULL, "|"); if(!token) continue; strncpy(g->creator, token, MAX_USERNAME-1);
        
        token = strtok(NULL, "|"); if(!token) continue; int ac = atoi(token);
        token = strtok(NULL, "|"); if(!token) continue; int mc = atoi(token);
        
        g->admin_count = 0;
        for(int i=0; i<ac; i++) {
            token = strtok(NULL, "|");
            if(token) strncpy(g->admins[g->admin_count++], token, MAX_USERNAME-1);
        }
        
        g->member_count = 0;
        for(int i=0; i<mc; i++) {
            token = strtok(NULL, "|");
            if(token) strncpy(g->members[g->member_count++], token, MAX_USERNAME-1);
        }
        state->group_count++;
    }
    fclose(f);
    printf("[SERVER] Loaded %d groups.\n", state->group_count);
}
// -------------------------

// --------------------------------------------------------
// CORE POLL LOGIC
// --------------------------------------------------------

int init_server(socket_t* server_socket) {
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_socket == INVALID_SOCKET) { perror("socket"); return -1; }
    int opt = 1; setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    struct sockaddr_in server_addr; memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; server_addr.sin_addr.s_addr = INADDR_ANY; server_addr.sin_port = htons(PORT);
    if (bind(*server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { perror("bind"); return -1; }
    if (listen(*server_socket, 10) < 0) { perror("listen"); return -1; }
    
    server_state.user_count = 0; load_accounts(ACCOUNT_FILE); load_friends_state(&server_state);
    load_groups(&server_state); // Load groups on startup
    return 0;
}

int main() {
    socket_t server_fd;
    if (init_server(&server_fd) < 0) return 1;
    printf("Server POLL-based started on port %d\n", PORT);

    // Initialize Poll
    memset(poll_fds, 0, sizeof(poll_fds));
    for(int i=0; i<MAX_POLL_CLIENTS; i++) poll_fds[i].fd = -1;

    poll_fds[0].fd = server_fd;
    poll_fds[0].events = POLLIN;
    max_nfds = 1;

    while (1) {
        int poll_count = poll(poll_fds, max_nfds, -1); // Block indefinitely
        if (poll_count == -1) { perror("poll"); break; }

        int current_nfds = max_nfds; // Snapshot to handle new additions safely
        for (int i = 0; i < current_nfds; i++) {
            if (poll_fds[i].fd == -1) continue;

            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == server_fd) {
                    // Accept New Connection
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int new_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
                    
                    if (new_fd >= 0) {
                        // Find slot
                        int j;
                        for(j=1; j<MAX_POLL_CLIENTS; j++) {
                            if (poll_fds[j].fd == -1) {
                                poll_fds[j].fd = new_fd;
                                poll_fds[j].events = POLLIN;
                                sessions[j].fd = new_fd;
                                sessions[j].user = NULL;
                                memset(sessions[j].current_chat_partner, 0, MAX_USERNAME);
                                if (j >= max_nfds) max_nfds = j + 1;
                                printf("[Slot %d] New connection accepted\n", j);
                                break;
                            }
                        }
                        if (j == MAX_POLL_CLIENTS) {
                            close(new_fd); // Too many clients state
                        }
                    }
                } else {
                    // Handle Client Data
                    handle_client_message(i);
                }
            }
        }
    }
    return 0;
}

void remove_client(int index) {
    if (index <= 0 || index >= MAX_POLL_CLIENTS) return; // Protect server_fd
    int fd = poll_fds[index].fd;
    
    // Logic Cleanup
    ClientSession* session = &sessions[index];
    if (session->user) {
        session->user->is_online = false;
        session->user->socket = INVALID_SOCKET;
        session->user->last_seen = time(NULL);
        broadcast_to_friends(&server_state, session->user->username, "User went offline");
        printf("[Slot %d] User %s disconnected\n", index, session->user->username);
    }
    
    // Update last read if they were in a chat
    if (session->user && strlen(session->current_chat_partner) > 0) {
        update_last_read(session->user->username, session->current_chat_partner);
    }

    close(fd);
    poll_fds[index].fd = -1;
    sessions[index].user = NULL;
    sessions[index].fd = -1;
}

void handle_client_message(int index) {
    char buffer[BUFFER_SIZE];
    int client_fd = poll_fds[index].fd;
    
    // NOTE: Using recv_line from code old. 
    // In strict non-blocking poll, we should read(fd) -> accumulate -> parse.
    // But per requirements "KHOAN hãy thực hiện... phức tạp", we reuse recv_line.
    // Assuming clients send atomic lines. 
    memset(buffer, 0, BUFFER_SIZE);
    int bytes = recv_line(client_fd, buffer, BUFFER_SIZE);
    
    if (bytes <= 0) {
        remove_client(index);
        return;
    }
    
    buffer[bytes] = '\0';
    ProtocolMessage* msg = deserialize_protocol_message(buffer, bytes);
    if (msg) {
        process_command(client_fd, msg, &sessions[index], index);
        free(msg);
    }
}

// Map logic from old 'handle_client' switch case
void process_command(int client_fd, ProtocolMessage* msg, ClientSession* session, int index) {
    User* current_user = session->user; // Get from session
    ServerState* state = &server_state;

    switch (msg->cmd) {
        case CMD_LOGIN: {
            User* user = find_user(state, msg->sender);
            if (user) {
                if (user->is_online) {
                    send_response(client_fd, CMD_ERROR, "User already logged in on another device");
                } else if (strcmp(user->password, msg->content) == 0) {
                    user->is_online = true;
                    user->socket = client_fd;
                    session->user = user; // BIND SESSION
                    send_response(client_fd, CMD_SUCCESS, "Login successful");
                    log_activity(index, msg->sender, "LOGIN", "User logged in successfully");
                    
                    // Offline Message Check (Improved detailed breakdown)
                    char unread_sum[MAX_CONTENT];
                    get_unread_summary_string(state, user->username, unread_sum, sizeof(unread_sum));
                    if (strstr(unread_sum, "UNREAD MESSAGES:")) {
                        ProtocolMessage pm; memset(&pm,0,sizeof(pm)); 
                        pm.cmd = CMD_RECEIVE_MESSAGE; 
                        pm.msg_type = MSG_TEXT;
                        strcpy(pm.sender, "SYSTEM");
                        strncpy(pm.content, unread_sum, MAX_CONTENT-1);
                        int len; char* b = serialize_protocol_message(&pm, &len); 
                        if(b) { send_all(client_fd, b, len); free(b); }
                    }
                    
                    // NEW: Send pending friend request count on login
                    if (user->request_count > 0) {
                        send_friend_request_count(user);
                    }
                } else {
                    send_response(client_fd, CMD_ERROR, "Invalid credentials");
                }
            } else {
                send_response(client_fd, CMD_ERROR, "Invalid credentials");
            }
            break;
        }
        case CMD_REGISTER: {
            if (find_user(state, msg->sender)) {
                send_response(client_fd, CMD_ERROR, "Username already exists");
            } else {
                if (save_account(ACCOUNT_FILE, msg->sender, msg->content) == 0) {
                    add_user(state, msg->sender, msg->content);
                    send_response(client_fd, CMD_SUCCESS, "Registration successful");
                    log_activity(index, msg->sender, "REGISTER", "New user registered");
                } else {
                    send_response(client_fd, CMD_ERROR, "Failed to persist account");
                }
            }
            break;
        }
        case CMD_LOGOUT: {
            if (current_user) {
                current_user->is_online = false;
                current_user->socket = INVALID_SOCKET;
                current_user->last_seen = time(NULL);
                send_response(client_fd, CMD_SUCCESS, "Logged out");
                log_activity(index, current_user->username, "LOGOUT", "User logged out");
                broadcast_to_friends(state, current_user->username, "User logged out");
                session->user = NULL; // Unbind
                memset(session->current_chat_partner, 0, MAX_USERNAME);
            } else {
                 send_response(client_fd, CMD_ERROR, "Not logged in");
            }
            break;
        }

        case CMD_LIST_GROUPS:
        case CMD_LIST_OWNED_GROUPS: {
             if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
             bool owned_only = (msg->cmd == CMD_LIST_OWNED_GROUPS);
             char list_buf[4096];
             strcpy(list_buf, owned_only ? "Owned Groups:\n" : "Your Groups:\n");
             int found = 0;
             for(int i=0; i<state->group_count; i++) {
                 Group* g = &state->groups[i];
                 bool match = false;
                 if (owned_only) {
                     if (strcmp(g->creator, current_user->username) == 0) match = true;
                 } else {
                     // Check if user is member
                     for(int j=0; j<g->member_count; j++) {
                         if(strcmp(g->members[j], current_user->username) == 0) { match = true; break; }
                     }
                 }
                 
                 if(match) {
                     char line[300]; snprintf(line, sizeof(line), "%d. [%s] %s\n", found + 1, g->group_id, g->name);
                     strncat(list_buf, line, sizeof(list_buf) - strlen(list_buf) - 1);
                     found++;
                 }
             }
             if(found == 0) strcat(list_buf, "(No groups found)");
             else {
                 strcat(list_buf, owned_only ? 
                         "\nEnter index to invite to, or press Enter to return: " : 
                         "\nEnter index to join chat, or press Enter to return: ");
             }
             send_response(client_fd, CMD_SUCCESS, list_buf);
             break;
        }
        case CMD_GET_FRIENDS: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            char friend_list[BUFFER_SIZE] = "Friends: ";
            if (current_user->friend_count == 0) strcat(friend_list, "(none)");
            else {
                for (int i = 0; i < current_user->friend_count; i++) {
                    User* f = find_user(state, current_user->friends[i]);
                    if (f) {
                        char info[200];
                        if (f->is_online) snprintf(info, sizeof(info), "%s [ONLINE]; ", f->username);
                        else {
                            char* last_seen = get_timestamp_string(f->last_seen);
                            snprintf(info, sizeof(info), "%s [OFFLINE] (Last seen: %s); ", f->username, last_seen);
                            free(last_seen);
                        }
                        strcat(friend_list, info);
                    }
                }
            }
            send_response(client_fd, CMD_GET_FRIENDS, friend_list);
            break;
        }
        case CMD_CHECK_STATUS: {
             User* target = find_user(state, msg->recipient);
             if (target) {
                 char status[BUFFER_SIZE];
                 if (target->is_online) snprintf(status, sizeof(status), "User %s is ONLINE", target->username);
                 else {
                     char* t = get_timestamp_string(target->last_seen);
                     snprintf(status, sizeof(status), "User %s is OFFLINE (Last seen: %s)", target->username, t);
                     free(t);
                 }
                 send_response(client_fd, CMD_SUCCESS, status);
             } else send_response(client_fd, CMD_ERROR, "User not found");
             break;
        }
        case CMD_ADD_FRIEND: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            User* friend_user = find_user(state, msg->recipient);
            if (!friend_user || strcmp(current_user->username, msg->recipient) == 0 || are_friends(current_user, friend_user)) {
                send_response(client_fd, CMD_ERROR, "Invalid friend request"); break;
            }
            add_friend(current_user, friend_user); save_friends_state(state);
            send_response(client_fd, CMD_SUCCESS, "Friend added");
            break;
        }
        case CMD_FRIEND_REQUEST: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            User* target = find_user(state, msg->recipient);
            if (!target || are_friends(current_user, target)) { send_response(client_fd, CMD_ERROR, "Invalid"); break; }
            strncpy(target->friend_requests[target->request_count++], current_user->username, MAX_USERNAME-1);
            save_friends_state(state);
            send_response(client_fd, CMD_SUCCESS, "Request sent");
            
            // NEW: Notify target user about the updated count
            send_friend_request_count(target);
            
            if (target->is_online) {
               ProtocolMessage noti; memset(&noti,0,sizeof(noti)); noti.cmd=CMD_RECEIVE_MESSAGE; noti.msg_type=MSG_SYSTEM;
               snprintf(noti.content, sizeof(noti.content), "New friend request from %s", current_user->username);
               int l; char* b = serialize_protocol_message(&noti, &l);
               send_all(target->socket, b, l); free(b);
            }
            break;
        }
         case CMD_FRIEND_ACCEPT:
         case CMD_FRIEND_REJECT: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            User* requester = find_user(state, msg->recipient);
            if (!requester) { send_response(client_fd, CMD_ERROR, "User not found"); break; }
            
            // VALIDATION: Is this person actually in my request list?
            int request_index = -1;
            for (int i = 0; i < current_user->request_count; i++) {
                if (strcmp(current_user->friend_requests[i], msg->recipient) == 0) {
                    request_index = i;
                    break;
                }
            }
            
            if (request_index == -1) {
                send_response(client_fd, CMD_ERROR, "No pending request from this user");
                break;
            }
            
            if (msg->cmd == CMD_FRIEND_ACCEPT) {
                add_friend(current_user, requester);
                send_response(client_fd, CMD_SUCCESS, "Friend accepted");
                if (requester->is_online) {
                   ProtocolMessage noti; memset(&noti,0,sizeof(noti)); noti.cmd = CMD_RECEIVE_MESSAGE; noti.msg_type = MSG_SYSTEM;
                   snprintf(noti.content, sizeof(noti.content), "%s accepted your friend request", current_user->username);
                   int l; char* b = serialize_protocol_message(&noti, &l);
                   send_all(requester->socket, b, l); free(b);
                }
            } else {
                send_response(client_fd, CMD_SUCCESS, "Friend request rejected");
            }
            
            // REMOVE from request list
            for (int i = request_index; i < current_user->request_count - 1; i++) {
                strcpy(current_user->friend_requests[i], current_user->friend_requests[i+1]);
            }
            current_user->request_count--;
            save_friends_state(state);
            
            // NEW: Send updated count to self
            send_friend_request_count(current_user);
            
            break;
        }
        case CMD_SEND_MESSAGE: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            User* recipient = find_user(state, msg->recipient);
            if (!recipient) {
                 log_activity(index, current_user->username, "SEND_FAIL", "Recipient not found");
                 send_response(client_fd, CMD_ERROR, "Cannot send message (User not found)"); break;
            }
            if (is_blocked(current_user, msg->recipient) || is_blocked(recipient, current_user->username)) {
                 log_activity(index, current_user->username, "SEND_FAIL", "Blocked");
                 send_response(client_fd, CMD_ERROR, "Cannot send message (Blocked)"); break;
            }
            save_message_to_file(current_user->username, msg->recipient, msg->content, false);
            if (recipient && recipient->is_online) {
                ProtocolMessage fwd = *msg; // Copy
                strncpy(fwd.sender, current_user->username, MAX_USERNAME-1);
                fwd.cmd = CMD_RECEIVE_MESSAGE;
                int l; char* b = serialize_protocol_message(&fwd, &l);
                send_all(recipient->socket, b, l); free(b);
                send_response(client_fd, CMD_SUCCESS, "Message sent");

                // AUTO-UPDATE RECIPIENT'S LAST READ (If they are in chat with sender)
                for(int i=0; i<max_nfds; i++) {
                    if (sessions[i].user && strcmp(sessions[i].user->username, msg->recipient) == 0) {
                        if (strcmp(sessions[i].current_chat_partner, current_user->username) == 0) {
                            update_last_read(msg->recipient, current_user->username);
                        }
                        break;
                    }
                }
                
                char log_detail[300]; snprintf(log_detail, sizeof(log_detail), "To: %s | Content: %s", msg->recipient, msg->content);
                log_activity(index, current_user->username, "SEND_MSG", log_detail);
            } else {
                send_response(client_fd, CMD_SUCCESS, "Message sent (User Offline)");
                char log_detail[300]; snprintf(log_detail, sizeof(log_detail), "[OFFLINE] To: %s | Content: %s", msg->recipient, msg->content);
                log_activity(index, current_user->username, "SEND_MSG", log_detail);
            }
            
            // Fix for Unread Logic:
            // Since the user is sending a message, they are obviously "in chat" and have read previous context.
            // Update the last_read timestamp to NOW so these messages aren't marked as unread later.
            update_last_read(current_user->username, msg->recipient);
            
            break;
        }
        case CMD_BLOCK_USER: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            if (strcmp(current_user->username, msg->recipient) == 0) { send_response(client_fd, CMD_ERROR, "Cannot block self"); break; }
            if (is_blocked(current_user, msg->recipient)) { send_response(client_fd, CMD_ERROR, "Already blocked"); break; }
            strncpy(current_user->blocked_users[current_user->blocked_count++], msg->recipient, MAX_USERNAME-1);
            save_friends_state(state);
            send_response(client_fd, CMD_SUCCESS, "User blocked");
            break;
        }
        case CMD_UNBLOCK_USER: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            bool found = false;
            for(int i=0; i<current_user->blocked_count; i++) {
                if (strcmp(current_user->blocked_users[i], msg->recipient) == 0) {
                    for(int j=i; j<current_user->blocked_count-1; j++) strcpy(current_user->blocked_users[j], current_user->blocked_users[j+1]);
                    current_user->blocked_count--;
                    found = true;
                    save_friends_state(state);
                    send_response(client_fd, CMD_SUCCESS, "User unblocked");
                    break;
                }
            }
            if (!found) send_response(client_fd, CMD_ERROR, "User not in block list");
            break;
        }
        case CMD_CREATE_GROUP: {
             if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
             char gid[MAX_GROUP_ID]; snprintf(gid, sizeof(gid), "GRP_%s_%lld", current_user->username, (long long)time(NULL));
             Group* g = &state->groups[state->group_count++];
             strncpy(g->group_id, gid, MAX_GROUP_ID-1); strncpy(g->name, msg->content, MAX_GROUP_NAME-1);
             strncpy(g->creator, current_user->username, MAX_USERNAME-1);
             g->member_count = 1; strncpy(g->members[0], current_user->username, MAX_USERNAME-1);
             g->admin_count = 1; strncpy(g->admins[0], current_user->username, MAX_USERNAME-1);
             char resp[300]; snprintf(resp, sizeof(resp), "Group created: %s", gid);
             send_response(client_fd, CMD_SUCCESS, resp);
             
             save_groups(state); // Persist
             
             char log_detail[300]; snprintf(log_detail, sizeof(log_detail), "ID: %s | Name: %s", gid, msg->content);
             log_activity(index, current_user->username, "CREATE_GRP", log_detail);
             break;
        }
        case CMD_ADD_TO_GROUP: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            // Msg content format: "GROUP_ID MEMBER_NAME"
            char gid[MAX_GROUP_ID], mem[MAX_USERNAME];
            if (sscanf(msg->content, "%s %s", gid, mem) < 2) { send_response(client_fd, CMD_ERROR, "Invalid format"); break; }
            Group* g = find_group(state, gid);
            if (!g) { send_response(client_fd, CMD_ERROR, "Group not found"); break; }
            if (strcmp(g->creator, current_user->username) != 0) { 
                send_response(client_fd, CMD_ERROR, "Only the group creator can add members"); 
                break; 
            }
            if (g->member_count >= MAX_MEMBERS) { send_response(client_fd, CMD_ERROR, "Group full"); break; }
            bool exists = false; for(int i=0; i<g->member_count; i++) if (strcmp(g->members[i], mem) == 0) exists = true;
            if (exists) { send_response(client_fd, CMD_ERROR, "Already member"); break; }
            strncpy(g->members[g->member_count++], mem, MAX_USERNAME-1);
            save_groups(state); // Persist
            send_response(client_fd, CMD_SUCCESS, "Member added");
            break;
        }
        case CMD_GROUP_MESSAGE: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            Group* g = find_group(state, msg->recipient); // Recipient is Group ID
            if (!g) { send_response(client_fd, CMD_ERROR, "Group not found"); break; }
            bool is_mem = false; for(int i=0; i<g->member_count; i++) if (strcmp(g->members[i], current_user->username) == 0) is_mem=true;
            if (!is_mem) { send_response(client_fd, CMD_ERROR, "Not a member"); break; }
            
            // Save for Offline/History
            // Recipient = GroupID, Sender = User
            save_message_to_file(current_user->username, msg->recipient, msg->content, true);
            update_last_read(current_user->username, msg->recipient); // User has read this group
            
            // Broadcast
            ProtocolMessage fwd = *msg; 
            strncpy(fwd.sender, current_user->username, MAX_USERNAME-1);
            fwd.cmd = CMD_RECEIVE_MESSAGE; 
            char group_tag[MAX_CONTENT + 200]; 
            // Format: [Group Name] [Original Content] - Note: Client logic might parse this?
            // Wait, previous logic was: "[Group Name] Content". 
            // Client checks for "[Group ". 
            // Let's keep it consistent.
            snprintf(group_tag, sizeof(group_tag), "[Group %s] %s", g->name, msg->content);
            strncpy(fwd.content, group_tag, MAX_CONTENT-1);
            
            int l; char* b = serialize_protocol_message(&fwd, &l);
            for(int i=0; i<g->member_count; i++) {
                if(strcmp(g->members[i], current_user->username) == 0) continue; // Don't echo
                User* u = find_user(state, g->members[i]);
                if (u && u->is_online && u->socket != INVALID_SOCKET) {
                    send_all(u->socket, b, l);
                    
                    // AUTO-UPDATE RECIPIENT'S LAST READ (If they are in this group chat)
                    for(int k=0; k<max_nfds; k++) {
                        if (sessions[k].user && strcmp(sessions[k].user->username, g->members[i]) == 0) {
                            if (strcmp(sessions[k].current_chat_partner, g->group_id) == 0) {
                                update_last_read(g->members[i], g->group_id);
                            }
                            break;
                        }
                    }
                }
            }
            free(b);
            send_response(client_fd, CMD_SUCCESS, "Group message sent");
            break;
        }
        case CMD_SEARCH_HISTORY: {
             if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
             int count;
             char** results = search_messages(msg->content, current_user->username, msg->recipient, &count);
             if (count == 0) send_response(client_fd, CMD_ERROR, "No messages found");
             else {
                 for(int i=0; i<count; i++) {
                     ProtocolMessage info; memset(&info,0,sizeof(info)); info.cmd=CMD_RECEIVE_MESSAGE; info.msg_type=MSG_SYSTEM;
                     strncpy(info.sender, "SEARCH", 10); strncpy(info.content, results[i], MAX_CONTENT-1);
                     int l; char* b = serialize_protocol_message(&info, &l); send_all(client_fd, b, l); free(b);
                     free(results[i]);
                 }
                 free(results);
                 send_response(client_fd, CMD_SUCCESS, "Search completed");
             }
             break;
        }
        case CMD_GET_REQUESTS: {
            if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
            if (current_user->request_count == 0) { send_response(client_fd, CMD_SUCCESS, "--- No pending requests ---"); break; }
            char list[BUFFER_SIZE] = "PENDING REQUESTS:\n";
            for(int i=0; i<current_user->request_count; i++) {
                char line[MAX_USERNAME + 10];
                snprintf(line, sizeof(line), "- %s\n", current_user->friend_requests[i]);
                strcat(list, line);
            }
            send_response(client_fd, CMD_SUCCESS, list);
            break;
        }
        case CMD_REMOVE_FRIEND: { 
             if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
             User* target = find_user(state, msg->recipient);
             if (!target) { send_response(client_fd, CMD_ERROR, "User not found"); break; }
             
             // Remove from current_user's friend list
             bool found1 = false;
             for (int i = 0; i < current_user->friend_count; i++) {
                 if (strcmp(current_user->friends[i], target->username) == 0) {
                     for (int j = i; j < current_user->friend_count - 1; j++) {
                         strcpy(current_user->friends[j], current_user->friends[j + 1]);
                     }
                     current_user->friend_count--;
                     found1 = true;
                     break;
                 }
             }
             
             // Remove from target's friend list
             bool found2 = false;
             for (int i = 0; i < target->friend_count; i++) {
                 if (strcmp(target->friends[i], current_user->username) == 0) {
                     for (int j = i; j < target->friend_count - 1; j++) {
                         strcpy(target->friends[j], target->friends[j + 1]);
                     }
                     target->friend_count--;
                     found2 = true;
                     break;
                 }
             }
             
             if (found1 || found2) {
                 save_friends_state(state); // Persist changes
                 send_response(client_fd, CMD_SUCCESS, "Friend removed successfully");
                 if (target->is_online && target->socket != INVALID_SOCKET) {
                     // Evaluate if we should notify target? "User X unfriended you"? 
                     // Usually standard apps don't notify unfriend. But system update is nice.
                 }
             } else {
                 send_response(client_fd, CMD_ERROR, "Friend not found in list");
             }
             break;
        }
        case CMD_HISTORY: {
             if (!current_user) { 
                 send_response(client_fd, CMD_ERROR, "Not logged in"); 
                 break; 
             }
             
             char target_id[MAX_USERNAME];
             strncpy(target_id, msg->recipient, MAX_USERNAME - 1);
             
             // VALIDATION: Does target exist?
             bool target_exists = false;
             if (strncmp(target_id, "GRP_", 4) == 0) {
                 if (find_group(state, target_id)) target_exists = true;
             } else {
                 if (find_user(state, target_id)) target_exists = true;
             }
             
             if (!target_exists) {
                 send_response(client_fd, CMD_ERROR, "User or Group does not exist");
                 break;
             }
             
             // TRACK CONTEXT: User is now viewing this target
             strncpy(session->current_chat_partner, target_id, MAX_USERNAME - 1);
             update_last_read(current_user->username, target_id);
             
             // Check if it is a group to get the name
             char group_name_info[MAX_CONTENT] = "";
             if (strncmp(target_id, "GRP_", 4) == 0) {
                 Group* g = find_group(state, target_id);
                 if (g) {
                     snprintf(group_name_info, sizeof(group_name_info), "[SYSTEM] Connected to Group: %s", g->name);
                 } else {
                     snprintf(group_name_info, sizeof(group_name_info), "[SYSTEM] Group Info Not Found");
                 }
             }

             // Send Group Name Info First (if group)
             if (strlen(group_name_info) > 0) {
                 ProtocolMessage info; memset(&info, 0, sizeof(info));
                 info.cmd = CMD_RECEIVE_MESSAGE;
                 strcpy(info.sender, "SYSTEM");
                 strcpy(info.content, group_name_info);
                 int l; char* b = serialize_protocol_message(&info, &l);
                 send_all(client_fd, b, l); free(b);
                 #ifdef _WIN32
                 Sleep(20);
                 #else
                 usleep(20000);
                 #endif
             }

             // Load History Logic
             FILE* file = fopen("messages.txt", "r");
             if (!file) {
                  send_response(client_fd, CMD_SUCCESS, "History loaded");
                  break;
             }
             
             char* history_lines[10]; int match_count = 0;
             for(int k=0; k<10; k++) history_lines[k] = NULL;
             
             char line[BUFFER_SIZE];
             while(fgets(line, sizeof(line), file)) {
                 char* tmp = strdup(line);
                 char* token = strtok(tmp, "|"); // ID
                 char* ts = strtok(NULL, "|");
                 char* type = strtok(NULL, "|"); // GROUP or PRIVATE
                 char* snd = strtok(NULL, "|");
                 char* rcv = strtok(NULL, "|");
                 char* cnt = strtok(NULL, "|");
                 
                 if (snd && rcv && cnt) {
                     trim_newline(cnt);
                     
                     bool is_group_target = (strncmp(target_id, "GRP_", 4) == 0);
                     bool match = false;
                     
                     if (is_group_target) {
                         // For Group History: Recipient MUST be the Group ID
                         if (strcmp(rcv, target_id) == 0) match = true;
                     } else {
                         // For Private History: (Sender=Me & Rcv=Target) OR (Sender=Target & Rcv=Me)
                         // AND Rcv is NOT a group (start with GRP_)
                         if (strncmp(rcv, "GRP_", 4) != 0) {
                             if ((strcmp(snd, current_user->username) == 0 && strcmp(rcv, target_id) == 0) ||
                                 (strcmp(snd, target_id) == 0 && strcmp(rcv, current_user->username) == 0)) {
                                 match = true;
                             }
                         }
                     }
                     
                     if (match) {
                        time_t t = (time_t)atoll(ts);
                        t += 7 * 3600; // Manual UTC+7
                        struct tm* tm_info = gmtime(&t);
                        char date_display[20];
                        strftime(date_display, 20, "%H:%M", tm_info); // %H is 00-23
                        
                        char formatted[MAX_CONTENT];
                        snprintf(formatted, sizeof(formatted), "[%s %s] %s", snd, date_display, cnt);
                        
                        if (match_count < 10) {
                            history_lines[match_count++] = strdup(formatted);
                        } else {
                            free(history_lines[0]);
                            for(int k=0; k<9; k++) history_lines[k] = history_lines[k+1];
                            history_lines[9] = strdup(formatted);
                        }
                     }
                 }
                 free(tmp);
             }
             fclose(file);
             
             // Send gathered history
             for(int k=0; k<match_count; k++) {
                 ProtocolMessage h; memset(&h, 0, sizeof(h));
                 h.cmd = CMD_RECEIVE_MESSAGE;
                 strcpy(h.sender, "HISTORY");
                 strcpy(h.content, history_lines[k]);
                 int l; char* b = serialize_protocol_message(&h, &l);
                 send_all(client_fd, b, l); free(b);
                 free(history_lines[k]);
                 #ifdef _WIN32
                 Sleep(20);
                 #else
                 usleep(20000);
                 #endif
             }
             
             send_response(client_fd, CMD_SUCCESS, "History loaded");
             break;
        }
        case CMD_GET_UNREAD_SUMMARY: {
             if (!current_user) { send_response(client_fd, CMD_ERROR, "Not logged in"); break; }
             char summary[MAX_CONTENT];
             get_unread_summary_string(state, current_user->username, summary, sizeof(summary));
             send_response(client_fd, CMD_SUCCESS, summary);
             break;
        }
        case CMD_EXIT_CHAT: {
             if (current_user && strlen(session->current_chat_partner) > 0) {
                 update_last_read(current_user->username, session->current_chat_partner);
                 memset(session->current_chat_partner, 0, MAX_USERNAME);
             }
             break;
        }
        default:
             break;
    }
}
