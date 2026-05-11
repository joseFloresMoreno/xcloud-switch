#ifndef XCLOUD_SESSION_H
#define XCLOUD_SESSION_H

#include <time.h>

typedef enum {
    SESSION_STATE_PENDING,
    SESSION_STATE_STARTED,
    SESSION_STATE_QUEUED,
    SESSION_STATE_FAILED,
} SessionState;

typedef enum {
    SESSION_TYPE_HOME,
    SESSION_TYPE_CLOUD,
} SessionType;

typedef struct {
    char id[256];
    char target[256];
    char path[512];
    SessionType type;
    SessionState state;
    time_t created_at;
    char error_message[512];
} XCloudSession;

// Session manager functions
XCloudSession* session_create(const char* sessionId, const char* target, const char* path, SessionType type);
void session_destroy(XCloudSession* session);
void session_set_state(XCloudSession* session, SessionState state);
void session_set_error(XCloudSession* session, const char* error);
const char* session_state_to_string(SessionState state);
const char* session_type_to_string(SessionType type);

#endif // XCLOUD_SESSION_H
