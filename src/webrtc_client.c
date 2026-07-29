#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "webrtc_client.h"
#include "ui.h"

#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/timing.h>
#include <mbedtls/error.h>

typedef struct {
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_timing_delay_context timer;
    struct sockaddr_in remote_addr;
    int sockfd;
} DTLSContext;

/* Key export callback for DTLS-SRTP key material extraction */
static int export_keys_cb(void *p_expkey,
                           const unsigned char *ms,
                           const unsigned char *kb,
                           size_t maclen,
                           size_t keylen,
                           size_t ivlen,
                           const unsigned char client_random[32],
                           const unsigned char server_random[32],
                           mbedtls_tls_prf_types tls_prf_type) {
    (void)ms; (void)maclen; (void)keylen; (void)ivlen;
    (void)client_random; (void)server_random; (void)tls_prf_type;

    XCloudWebRTCClient *client = (XCloudWebRTCClient *)p_expkey;
    if (!client || !kb) return 0;

    /* Key Block Layout for SRTP (RFC 5764 Sec 4.2):
       Client Write Key (16 bytes)
       Server Write Key (16 bytes)
       Client Write Salt (14 bytes)
       Server Write Salt (14 bytes)
    */
    memcpy(client->srtp_keys.client_master_key, kb, 16);
    memcpy(client->srtp_keys.server_master_key, kb + 16, 16);
    memcpy(client->srtp_keys.client_master_salt, kb + 32, 14);
    memcpy(client->srtp_keys.server_master_salt, kb + 46, 14);
    client->srtp_keys.valid = 1;

    return 0;
}

/* Custom UDP send callback for mbedtls */
static int dtls_bio_send(void *ctx, const unsigned char *buf, size_t len) {
    DTLSContext *dtls = (DTLSContext *)ctx;
    ssize_t ret = sendto(dtls->sockfd, buf, len, 0,
                         (struct sockaddr *)&dtls->remote_addr,
                         sizeof(dtls->remote_addr));
    if (ret < 0) {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }
    return (int)ret;
}

/* Custom UDP recv callback for mbedtls (non-blocking tolerant) */
static int dtls_bio_recv(void *ctx, unsigned char *buf, size_t len) {
    DTLSContext *dtls = (DTLSContext *)ctx;
    socklen_t addrlen = sizeof(dtls->remote_addr);
    ssize_t ret = recvfrom(dtls->sockfd, buf, len, MSG_DONTWAIT,
                           (struct sockaddr *)&dtls->remote_addr, &addrlen);
    if (ret < 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }
    return (int)ret;
}

XCloudWebRTCClient* webrtc_create(void) {
    XCloudWebRTCClient* client = (XCloudWebRTCClient*)malloc(sizeof(XCloudWebRTCClient));
    if (!client) return NULL;
    
    memset(client, 0, sizeof(XCloudWebRTCClient));
    client->is_connected = 0;
    client->sockfd = -1;
    strncpy(client->connection_state, "disconnected", sizeof(client->connection_state) - 1);
    
    return client;
}

void webrtc_destroy(XCloudWebRTCClient* client) {
    if (!client) return;

    if (client->dtls_ssl_ctx) {
        DTLSContext *dtls = (DTLSContext *)client->dtls_ssl_ctx;
        mbedtls_ssl_free(&dtls->ssl);
        mbedtls_ssl_config_free(&dtls->conf);
        mbedtls_ctr_drbg_free(&dtls->ctr_drbg);
        mbedtls_entropy_free(&dtls->entropy);
        free(dtls);
        client->dtls_ssl_ctx = NULL;
    }

    free(client);
}

int webrtc_create_offer(XCloudWebRTCClient* client, char* sdp_out, int sdp_out_size) {
    if (!client || !sdp_out) return -1;
    snprintf(sdp_out, sdp_out_size, "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\n");
    return 0;
}

int webrtc_set_remote_sdp(XCloudWebRTCClient* client, const char* sdp) {
    if (!client || !sdp) return -1;
    strncpy(client->remote_sdp, sdp, sizeof(client->remote_sdp) - 1);
    return 0;
}

int webrtc_add_ice_candidate(XCloudWebRTCClient* client, const char* candidate) {
    if (!client || !candidate) return -1;
    return 0;
}

int webrtc_is_connected(XCloudWebRTCClient* client) {
    if (!client) return 0;
    return client->is_connected;
}

void webrtc_send_input(XCloudWebRTCClient* client, const char* data, int size) {
    if (!client || !data) return;
    (void)size;
}

int webrtc_dtls_connect(XCloudWebRTCClient* client, int sockfd, const char* remote_ip, int remote_port) {
    if (!client || sockfd < 0 || !remote_ip || remote_port <= 0) return -1;

    client->sockfd = sockfd;
    ui_log("[DTLS] Initiating handshake with %s:%d...", remote_ip, remote_port);

    DTLSContext *dtls = (DTLSContext *)malloc(sizeof(DTLSContext));
    if (!dtls) return -1;
    memset(dtls, 0, sizeof(DTLSContext));
    dtls->sockfd = sockfd;

    memset(&dtls->remote_addr, 0, sizeof(dtls->remote_addr));
    dtls->remote_addr.sin_family = AF_INET;
    dtls->remote_addr.sin_port = htons((uint16_t)remote_port);
    inet_pton(AF_INET, remote_ip, &dtls->remote_addr.sin_addr);

    mbedtls_ssl_init(&dtls->ssl);
    mbedtls_ssl_config_init(&dtls->conf);
    mbedtls_ctr_drbg_init(&dtls->ctr_drbg);
    mbedtls_entropy_init(&dtls->entropy);

    const char *pers = "webrtc_dtls_client";
    if (mbedtls_ctr_drbg_seed(&dtls->ctr_drbg, mbedtls_entropy_func, &dtls->entropy,
                               (const unsigned char *)pers, strlen(pers)) != 0) {
        ui_log("[DTLS] Failed to seed DRBG");
        free(dtls);
        return -1;
    }

    if (mbedtls_ssl_config_defaults(&dtls->conf,
                                     MBEDTLS_SSL_IS_CLIENT,
                                     MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                     MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        ui_log("[DTLS] Failed to set SSL defaults");
        free(dtls);
        return -1;
    }

    mbedtls_ssl_conf_rng(&dtls->conf, mbedtls_ctr_drbg_random, &dtls->ctr_drbg);
    mbedtls_ssl_conf_authmode(&dtls->conf, MBEDTLS_SSL_VERIFY_NONE);

    /* Register Key Export Callback */
    mbedtls_ssl_conf_export_keys_ext_cb(&dtls->conf, export_keys_cb, client);

    if (mbedtls_ssl_setup(&dtls->ssl, &dtls->conf) != 0) {
        ui_log("[DTLS] SSL setup failed");
        free(dtls);
        return -1;
    }

    mbedtls_ssl_set_hostname(&dtls->ssl, remote_ip);
    mbedtls_ssl_set_timer_cb(&dtls->ssl, &dtls->timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
    mbedtls_ssl_set_bio(&dtls->ssl, dtls, dtls_bio_send, dtls_bio_recv, NULL);

    client->dtls_ssl_ctx = dtls;

    /* Set 100ms socket timeouts */
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    ui_log("[DTLS] Starting non-blocking SSL handshake loop...");
    int ret;
    int tries = 0;
    while ((ret = mbedtls_ssl_handshake(&dtls->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            char error_buf[100];
            mbedtls_strerror(ret, error_buf, sizeof(error_buf));
            ui_log("[DTLS] Handshake failed (-0x%04X): %s", -ret, error_buf);
            return -1;
        }
        tries++;
        if (tries % 50 == 0) {
            ui_log("[DTLS] Waiting handshake reply... (%d/300)", tries);
        }
        if (tries > 300) {
            ui_log("[DTLS] Handshake timeout (no response from Xbox)");
            return -1;
        }
        usleep(10000); /* 10ms wait */
    }

    ui_log("[DTLS] Handshake SUCCESS!");

    if (client->srtp_keys.valid) {
        ui_log("[SRTP] Key material extracted successfully!");
    } else {
        ui_log("[SRTP] Handshake completed successfully");
    }

    client->is_connected = 1;
    strncpy(client->connection_state, "connected", sizeof(client->connection_state) - 1);

    return 0;
}
