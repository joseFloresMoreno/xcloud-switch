#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "session.h"

XCloudSession* session_create(const char* sessionId, const char* target, const char* path, SessionType type) {
    XCloudSession* session = (XCloudSession*)malloc(sizeof(XCloudSession));
    if (!session) return NULL;
    
    memset(session, 0, sizeof(XCloudSession));
    strncpy(session->id, sessionId, sizeof(session->id) - 1);
    strncpy(session->target, target, sizeof(session->target) - 1);
    strncpy(session->path, path, sizeof(session->path) - 1);
    session->type = type;
    session->state = SESSION_STATE_PENDING;
    session->created_at = time(NULL);
    
    return session;
}

void session_destroy(XCloudSession* session) {
    if (session) {
        free(session);
    }
}

void session_set_state(XCloudSession* session, SessionState state) {
    if (session) {
        session->state = state;
    }
}

void session_set_error(XCloudSession* session, const char* error) {
    if (session && error) {
        strncpy(session->error_message, error, sizeof(session->error_message) - 1);
        session->state = SESSION_STATE_FAILED;
    }
}

const char* session_state_to_string(SessionState state) {
    switch (state) {
        case SESSION_STATE_PENDING: return "Pending";
        case SESSION_STATE_STARTED: return "Started";
        case SESSION_STATE_QUEUED: return "Queued";
        case SESSION_STATE_FAILED: return "Failed";
        default: return "Unknown";
    }
}

const char* session_type_to_string(SessionType type) {
    switch (type) {
        case SESSION_TYPE_HOME: return "Home";
        case SESSION_TYPE_CLOUD: return "Cloud";
        default: return "Unknown";
    }
}
