#include "webrtc_sdp.h"
#include "webrtc_ice.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <switch.h>

/*
 * Pre-generated ECDSA P-256 self-signed certificate fingerprint.
 * Generated with: openssl ecparam -name prime256v1 -genkey | openssl req -new -x509 -days 3650 -subj /CN=WebRTC
 * The matching private key DER is stored in webrtc_dtls_key[] below for the eventual DTLS handshake.
 */
#define DTLS_FINGERPRINT \
    "DA:E4:D3:93:1B:58:10:24:BD:EE:7B:26:C0:EB:52:2E:" \
    "BA:CB:66:7B:45:13:86:43:59:8A:46:5F:F1:53:31:B2"

/* Private key DER (PKCS#8 EC, P-256) — kept for DTLS handshake */
static const uint8_t webrtc_dtls_key[] = {
    0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x7e, 0x98, 0xa2, 0xae, 0x2e,
    0xeb, 0x04, 0x62, 0xf8, 0x83, 0xf2, 0x42, 0x24, 0x02, 0xbd, 0x51, 0x0e,
    0xac, 0xd4, 0x80, 0xa4, 0x3b, 0xb1, 0x34, 0xf6, 0x9b, 0x11, 0xe5, 0x5e,
    0x91, 0xbd, 0xda, 0xa0, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x03, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0x4f, 0x57, 0xfc,
    0xae, 0x0c, 0xdf, 0x5d, 0x26, 0xde, 0x98, 0xfc, 0x06, 0xf9, 0xc6, 0xb4,
    0xb0, 0x18, 0xc7, 0xd3, 0x2d, 0x74, 0x95, 0xfa, 0xfb, 0xfc, 0x8d, 0xb7,
    0x25, 0x90, 0xae, 0x14, 0x51, 0xea, 0x04, 0x72, 0x35, 0x5f, 0xcb, 0xe2,
    0x52, 0xc5, 0xc3, 0xb1, 0x42, 0x49, 0x07, 0x23, 0x16, 0x77, 0x3f, 0x0e,
    0x16, 0x61, 0x55, 0xa3, 0x1b, 0x54, 0xe9, 0x87, 0x96, 0xef, 0x93, 0x58,
    0x02
};
static const uint32_t webrtc_dtls_key_len = sizeof(webrtc_dtls_key);

/* Certificate DER — sent in DTLS ClientHello/Certificate message */
static const uint8_t webrtc_dtls_cert[] = {
    0x30, 0x82, 0x01, 0x78, 0x30, 0x82, 0x01, 0x1d, 0xa0, 0x03, 0x02, 0x01,
    0x02, 0x02, 0x14, 0x00, 0xf2, 0x35, 0xeb, 0x82, 0x3b, 0xcf, 0x96, 0xf4,
    0x51, 0xc8, 0x88, 0xc5, 0xb3, 0xf8, 0x2e, 0x5e, 0x11, 0xd1, 0xec, 0x30,
    0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30,
    0x11, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x06,
    0x57, 0x65, 0x62, 0x52, 0x54, 0x43, 0x30, 0x1e, 0x17, 0x0d, 0x32, 0x36,
    0x30, 0x36, 0x31, 0x36, 0x30, 0x31, 0x32, 0x30, 0x32, 0x36, 0x5a, 0x17,
    0x0d, 0x33, 0x36, 0x30, 0x36, 0x31, 0x33, 0x30, 0x31, 0x32, 0x30, 0x32,
    0x36, 0x5a, 0x30, 0x11, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04,
    0x03, 0x0c, 0x06, 0x57, 0x65, 0x62, 0x52, 0x54, 0x43, 0x30, 0x59, 0x30,
    0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08,
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04,
    0x4f, 0x57, 0xfc, 0xae, 0x0c, 0xdf, 0x5d, 0x26, 0xde, 0x98, 0xfc, 0x06,
    0xf9, 0xc6, 0xb4, 0xb0, 0x18, 0xc7, 0xd3, 0x2d, 0x74, 0x95, 0xfa, 0xfb,
    0xfc, 0x8d, 0xb7, 0x25, 0x90, 0xae, 0x14, 0x51, 0xea, 0x04, 0x72, 0x35,
    0x5f, 0xcb, 0xe2, 0x52, 0xc5, 0xc3, 0xb1, 0x42, 0x49, 0x07, 0x23, 0x16,
    0x77, 0x3f, 0x0e, 0x16, 0x61, 0x55, 0xa3, 0x1b, 0x54, 0xe9, 0x87, 0x96,
    0xef, 0x93, 0x58, 0x02, 0xa3, 0x53, 0x30, 0x51, 0x30, 0x1d, 0x06, 0x03,
    0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0xf7, 0x8d, 0x1c, 0xb9, 0x8b,
    0xe2, 0x1b, 0xdd, 0x1b, 0x06, 0xb9, 0xbc, 0x66, 0xb5, 0x52, 0x94, 0xb8,
    0x6a, 0x7d, 0x9f, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18,
    0x30, 0x16, 0x80, 0x14, 0xf7, 0x8d, 0x1c, 0xb9, 0x8b, 0xe2, 0x1b, 0xdd,
    0x1b, 0x06, 0xb9, 0xbc, 0x66, 0xb5, 0x52, 0x94, 0xb8, 0x6a, 0x7d, 0x9f,
    0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05,
    0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48,
    0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21,
    0x00, 0xa1, 0xb6, 0x41, 0xba, 0xf8, 0x60, 0x05, 0x75, 0x1f, 0xc7, 0xf9,
    0xf3, 0x57, 0xa0, 0xa4, 0x47, 0x83, 0x2c, 0x31, 0x59, 0x0a, 0xea, 0x0f,
    0x0a, 0x18, 0x54, 0x8f, 0x97, 0x85, 0x98, 0xf7, 0xd4, 0x02, 0x21, 0x00,
    0x84, 0x20, 0x42, 0x74, 0xc9, 0x72, 0xf6, 0xc6, 0xc4, 0xd8, 0x24, 0x9d,
    0xdd, 0x64, 0xce, 0x6e, 0x36, 0x29, 0xb4, 0xd7, 0x26, 0x08, 0x74, 0xab,
    0xa6, 0x71, 0x34, 0xb5, 0x60, 0xfa, 0x5d, 0xdc
};
static const uint32_t webrtc_dtls_cert_len = sizeof(webrtc_dtls_cert);

/* Suppress unused-variable warning until DTLS handshake is implemented */
static void _dtls_refs_used(void) __attribute__((unused));
static void _dtls_refs_used(void) {
    (void)webrtc_dtls_key;     (void)webrtc_dtls_key_len;
    (void)webrtc_dtls_cert;    (void)webrtc_dtls_cert_len;
}

/* ------------------------------------------------------------------ */

static const char b64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void random_ice_string(char *out, int len) {
    uint8_t rnd[64];
    randomGet(rnd, (size_t)len);
    for (int i = 0; i < len; i++)
        out[i] = b64_chars[rnd[i] & 0x3F];
    out[len] = '\0';
}

/* Remember the ICE credentials used in the last offer, so a local host
   candidate can be sent afterwards with a matching ufrag/pwd. */
static char g_local_ufrag[8] = "";
static char g_local_pwd[28]  = "";
static int  g_ice_sockfd      = -1;

const char* webrtc_sdp_get_local_ufrag(void) { return g_local_ufrag; }
const char* webrtc_sdp_get_local_pwd(void)   { return g_local_pwd; }
int         webrtc_sdp_get_ice_sockfd(void)  { return g_ice_sockfd; }

int webrtc_sdp_json_escape(const char *raw, char *out, int out_size) {
    int j = 0;
    for (int i = 0; raw[i] != '\0' && j < out_size - 4; i++) {
        unsigned char c = (unsigned char)raw[i];
        if (c == '\r') {
            out[j++] = '\\'; out[j++] = 'r';
        } else if (c == '\n') {
            out[j++] = '\\'; out[j++] = 'n';
        } else if (c == '"') {
            out[j++] = '\\'; out[j++] = '"';
        } else if (c == '\\') {
            out[j++] = '\\'; out[j++] = '\\';
        } else {
            out[j++] = (char)c;
        }
    }
    out[j] = '\0';
    return 0;
}

int webrtc_sdp_create_offer(char *out, int out_size) {
    char ufrag[8];
    char pwd[28];
    random_ice_string(ufrag, 4);
    random_ice_string(pwd, 24);
    strncpy(g_local_ufrag, ufrag, sizeof(g_local_ufrag) - 1);
    strncpy(g_local_pwd, pwd, sizeof(g_local_pwd) - 1);

    ui_log("[SDP] ice-ufrag=%s", ufrag);

    /* The streaming session is created with useIceConnection:false (see
       xcloud_api_start_stream settings), meaning the server does NOT accept
       trickled candidates over the separate /ice endpoint (it 500s). Instead
       we must gather our candidate up front and embed it directly in the
       offer ("full ICE", no a=ice-options:trickle). */
    char local_ip[16] = "0.0.0.0";
    int  local_port    = 0;
    if (webrtc_ice_get_local_endpoint(local_ip, sizeof(local_ip),
                                       &local_port, &g_ice_sockfd) != 0) {
        ui_log("[SDP] local endpoint FAIL — offer will have no candidate");
    }

    /* Raw SDP with actual CRLF — will be JSON-escaped before returning */
    static char raw[6144];
    int n = 0;

#define L(fmt, ...) n += snprintf(raw + n, (int)sizeof(raw) - n, fmt "\r\n", ##__VA_ARGS__)

    /* Session header */
    L("v=0");
    L("o=- 8775432198765432 2 IN IP4 127.0.0.1");
    L("s=-");
    L("t=0 0");
    L("a=group:BUNDLE 0 1 2");
    L("a=extmap-allow-mixed");
    L("a=msid-semantic: WMS");

    /* ---- Video (receive-only, H264) ---- */
    L("m=video 9 UDP/TLS/RTP/SAVPF 96 97 98 99");
    L("c=IN IP4 0.0.0.0");
    L("a=rtcp:9 IN IP4 0.0.0.0");
    L("a=ice-ufrag:%s", ufrag);
    L("a=ice-pwd:%s", pwd);
    L("a=candidate:1 1 udp 2122260223 %s %d typ host generation 0", local_ip, local_port);
    L("a=end-of-candidates");
    L("a=fingerprint:sha-256 " DTLS_FINGERPRINT);
    L("a=setup:actpass");
    L("a=mid:0");
    L("a=extmap:1 urn:ietf:params:rtp-hdrext:toffset");
    L("a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time");
    L("a=extmap:3 urn:3gpp:video-orientation");
    L("a=extmap:4 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01");
    L("a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid");
    L("a=recvonly");
    L("a=rtcp-mux");
    L("a=rtcp-rsize");
    /* H264 High Profile 3.1 — main codec Xbox uses */
    L("a=rtpmap:96 H264/90000");
    L("a=rtcp-fb:96 goog-remb");
    L("a=rtcp-fb:96 transport-cc");
    L("a=rtcp-fb:96 ccm fir");
    L("a=rtcp-fb:96 nack");
    L("a=rtcp-fb:96 nack pli");
    L("a=fmtp:96 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=640c1f");
    /* RTX (retransmission) for pt 96 */
    L("a=rtpmap:97 rtx/90000");
    L("a=fmtp:97 apt=96");
    /* H264 packetization-mode=0 fallback */
    L("a=rtpmap:98 H264/90000");
    L("a=rtcp-fb:98 goog-remb");
    L("a=rtcp-fb:98 transport-cc");
    L("a=rtcp-fb:98 ccm fir");
    L("a=rtcp-fb:98 nack");
    L("a=rtcp-fb:98 nack pli");
    L("a=fmtp:98 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=640c1f");
    L("a=rtpmap:99 rtx/90000");
    L("a=fmtp:99 apt=98");

    /* ---- Audio (receive-only, Opus) ---- */
    L("m=audio 9 UDP/TLS/RTP/SAVPF 111 103 9 0 8");
    L("c=IN IP4 0.0.0.0");
    L("a=rtcp:9 IN IP4 0.0.0.0");
    L("a=ice-ufrag:%s", ufrag);
    L("a=ice-pwd:%s", pwd);
    L("a=candidate:1 1 udp 2122260223 %s %d typ host generation 0", local_ip, local_port);
    L("a=end-of-candidates");
    L("a=fingerprint:sha-256 " DTLS_FINGERPRINT);
    L("a=setup:actpass");
    L("a=mid:1");
    L("a=extmap:14 urn:ietf:params:rtp-hdrext:ssrc-audio-level");
    L("a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time");
    L("a=extmap:4 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01");
    L("a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid");
    L("a=recvonly");
    L("a=rtcp-mux");
    L("a=rtpmap:111 opus/48000/2");
    L("a=rtcp-fb:111 transport-cc");
    L("a=fmtp:111 minptime=10;useinbandfec=1");
    L("a=rtpmap:103 ISAC/16000");
    L("a=rtpmap:9 G722/8000");
    L("a=rtpmap:0 PCMU/8000");
    L("a=rtpmap:8 PCMA/8000");

    /* ---- Data channel (for controller input / control messages) ---- */
    L("m=application 9 UDP/DTLS/SCTP webrtc-datachannel");
    L("c=IN IP4 0.0.0.0");
    L("a=ice-ufrag:%s", ufrag);
    L("a=ice-pwd:%s", pwd);
    L("a=candidate:1 1 udp 2122260223 %s %d typ host generation 0", local_ip, local_port);
    L("a=end-of-candidates");
    L("a=fingerprint:sha-256 " DTLS_FINGERPRINT);
    L("a=setup:actpass");
    L("a=mid:2");
    L("a=sctp-port:5000");
    L("a=max-message-size:262144");

#undef L

    ui_log("[SDP] raw %d bytes, JSON-escaping...", n);
    return webrtc_sdp_json_escape(raw, out, out_size);
}

/* ------------------------------------------------------------------ */
/* Parse fields from a JSON-encoded SDP answer                         */
/* ------------------------------------------------------------------ */

int webrtc_sdp_parse_answer_field(const char *sdp_json,
                                   const char *field,
                                   char *out, int out_size) {
    if (!sdp_json || !field || !out) return -1;

    /* Prioritize active media section (m=application or m=audio) for ICE credentials,
       because m=video 0 is often rejected/disabled in BUNDLE and has dummy credentials */
    const char *start_search = sdp_json;
    if (strcmp(field, "ice-ufrag") == 0 || strcmp(field, "ice-pwd") == 0) {
        const char *app_media = strstr(sdp_json, "m=application");
        if (!app_media) app_media = strstr(sdp_json, "m=audio");
        if (app_media) start_search = app_media;
    }

    /* Look for the field as an SDP attribute: "a=<field>:" or "a=<field> " */
    char needle_colon[64], needle_space[64];
    snprintf(needle_colon, sizeof(needle_colon), "a=%s:", field);
    snprintf(needle_space, sizeof(needle_space), "a=%s ", field);

    const char *p = strstr(start_search, needle_colon);
    if (!p) p = strstr(start_search, needle_space);
    if (!p && start_search != sdp_json) {
        p = strstr(sdp_json, needle_colon);
        if (!p) p = strstr(sdp_json, needle_space);
    }
    if (!p) return -1;

    /* Advance past the field name and separator */
    p += strlen(needle_colon) - 1;  /* lands on ':' or ' ' */
    p++;                              /* skip separator */

    /* Copy until next \r, \n, ", backslash (start of any JSON escape) or end of string.
       SDP attribute values (ufrag/pwd/fingerprint) never contain a literal backslash,
       so stopping there is always safe and avoids leaking part of a \r\n escape. */
    int i = 0;
    while (*p && *p != '\r' && *p != '\n' && *p != '"' && *p != '\\' && i < out_size - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return (i > 0) ? 0 : -1;
}
