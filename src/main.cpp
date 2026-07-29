#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern "C" {
#include "ui.h"
#include "xbox_auth.h"
#include "session.h"
#include "xcloud_api.h"
#include "xbox_host.h"
#include "webrtc_sdp.h"
#include "webrtc_ice.h"
#include "webrtc_client.h"
#include "audio_decoder.h"
#include "video_decoder.h"
#include "input_handler.h"
}

/* ------------------------------------------------------------------ */
/* Console-mode button hints (SDL2 draws its own)                      */
/* ------------------------------------------------------------------ */

#ifndef HAVE_SDL2
static void draw_hint(UIScreen screen) {
    switch (screen) {
        case UI_SCREEN_LOGIN:
            printf("\n  [B] Cancelar   [+] Salir\n"); break;
        case UI_SCREEN_HOST_LIST:
            printf("\n  [A] Conectar   [Y] Agregar IP   [X] Buscar   [+] Salir\n"); break;
        case UI_SCREEN_CONNECTING:
            printf("\n  [B] Cancelar   [+] Salir\n"); break;
        case UI_SCREEN_ERROR:
            printf("\n  [B] Volver     [+] Salir\n"); break;
        default:
            printf("\n  [+] Salir\n"); break;
    }
}
#endif

static void refresh_host_list(XboxHostList* hosts, XboxAuthContext* auth, bool has_internet) {
    if (!hosts) return;
    ui_log("[CONSOLES] Limpiando y buscando consolas activas...");
    xbox_host_list_clear(hosts);

    if (has_internet && auth && auth->xsts_token[0]) {
        int found = xbox_host_fetch_from_api(hosts, auth->uhs, auth->xsts_token);
        if (found < 0 && auth->refresh_token && auth->refresh_token[0]) {
            ui_log("[CONSOLES] token vencido, renovando...");
            if (xbox_auth_refresh(auth) == 0) {
                xbox_auth_save(auth, XBOX_AUTH_TOKEN_FILE);
                xbox_host_fetch_from_api(hosts, auth->uhs, auth->xsts_token);
            }
        }
        xbox_host_discover(hosts);
    }
    /* Load optional manual IP configuration from SD card */
    xbox_host_load_config(hosts, "/switch/greenlight/config.txt");
    if (hosts->count == 0)
        xbox_host_add_manual(hosts, "No se encontraron consolas", "");

    ui_set_host_list(hosts, 0);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void) {
#ifndef HAVE_SDL2
    consoleInit(NULL);
    printf("=== Greenlight Switch ===\n\nIniciando...\n");
    consoleUpdate(NULL);
#endif

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();
    input_handler_init();

    XCloudWebRTCClient* webrtc = webrtc_create();
    XCloudAudioDecoder* audio  = audio_decoder_create();
    XCloudVideoDecoder* video  = video_decoder_create();
    audio_decoder_init(audio, 48000, 2);
    video_decoder_init(video);

    XboxHostList*    hosts = xbox_host_list_create();
    XCloudApiClient* api   = NULL;   /* created after auth */
    XboxAuthContext* auth  = xbox_auth_create();
    bool             has_internet = true;

    if (ui_init() != 0) {
        xbox_auth_destroy(auth);
        xbox_host_list_destroy(hosts);
        webrtc_destroy(webrtc);
        audio_decoder_destroy(audio);
        video_decoder_destroy(video);
        input_handler_deinit();
        socketExit();
#ifndef HAVE_SDL2
        consoleExit(NULL);
#endif
        return 1;
    }
    ui_log("==== nueva sesion ====");

    /* ---------------------------------------------------------------- */
    /* Auth startup: try saved token → refresh → device code flow       */
    /* ---------------------------------------------------------------- */

    UIScreen screen  = UI_SCREEN_LOGIN;
    bool     running = true;

    /* Try loading saved tokens from SD */
    if (xbox_auth_load(auth, XBOX_AUTH_TOKEN_FILE) == 0 && xbox_auth_is_valid(auth)) {
        ui_log("Token OK, saltando login");
        screen = UI_SCREEN_HOST_LIST;
    } else if (auth->refresh_token && auth->refresh_token[0]) {
        ui_log("Token expirado, renovando...");
        ui_set_login_state(NULL, "Renovando sesion...", 0);
        ui_draw_screen(UI_SCREEN_LOGIN);
        if (xbox_auth_refresh(auth) == 0) {
            xbox_auth_save(auth, XBOX_AUTH_TOKEN_FILE);
            screen = UI_SCREEN_HOST_LIST;
        } else {
            /* Refresh failed — need device code */
            ui_set_login_state(NULL, "Sesion expirada. Reautenticando...", 0);
        }
    }

    /* If we still need login, request the device code now */
    if (screen == UI_SCREEN_LOGIN) {
        ui_set_login_state(NULL, "Solicitando codigo...", 0);
        ui_draw_screen(UI_SCREEN_LOGIN);

        if (xbox_auth_request_device_code(auth) == 0) {
            char status[128];
            snprintf(status, sizeof(status),
                     "Expira en %d min — esperando...", auth->expires_in / 60);
            ui_set_login_state(auth->user_code, status, auth->expires_in);
        } else {
            ui_set_error(auth->error[0] ? auth->error : "No se pudo obtener el codigo de autenticacion.");
            screen = UI_SCREEN_ERROR;
        }
    }

    /* If auth succeeded, get streaming tokens and populate host list */
    if (screen == UI_SCREEN_HOST_LIST) {
        /* Check internet connectivity before any network call */
        {
            NifmInternetConnectionType ct;
            u32 ws;
            NifmInternetConnectionStatus cs;
            nifmInitialize(NifmServiceType_User);
            has_internet = R_SUCCEEDED(nifmGetInternetConnectionStatus(&ct, &ws, &cs))
                           && cs == NifmInternetConnectionStatus_Connected;
            nifmExit();
        }
        ui_log(has_internet ? "Red: OK" : "Sin internet — usando cache");

        if (has_internet) {
            ui_log("Obteniendo gsToken...");
            if (xbox_auth_get_streaming_tokens(auth) != 0) {
                ui_log("%.80s", auth->error[0] ? auth->error : "gsToken: error desconocido");
                if (auth->refresh_token && auth->refresh_token[0]) {
                    ui_log("Intentando refresh de sesion...");
                    if (xbox_auth_refresh(auth) == 0) {
                        if (xbox_auth_get_streaming_tokens(auth) != 0)
                            ui_log("post-refresh: %.80s", auth->error);
                    } else {
                        ui_log("refresh err: %.80s", auth->error);
                    }
                }
            }
        }

        if (auth->gs_token[0]) {
            ui_log("gsToken OK host=%s", auth->streaming_host);
            ui_log("gsToken len=%d pref=%.20s", (int)strlen(auth->gs_token), auth->gs_token);
            if (has_internet)
                xbox_auth_save(auth, XBOX_AUTH_TOKEN_FILE);
            api = xcloud_api_create(auth->streaming_host[0]
                    ? auth->streaming_host
                    : "uks.core.gssv-play-prodxhome.xboxlive.com",
                auth->gs_token, XCLOUD_TYPE_HOME);
        } else {
            ui_log("Sin gsToken — stream no disponible");
            api = NULL;
        }

        refresh_host_list(hosts, auth, has_internet);
    }

    /* ---------------------------------------------------------------- */
    /* Main loop                                                         */
    /* ---------------------------------------------------------------- */

    XCloudSession* session         = NULL;
    int            sel             = 0;
    time_t         last_poll       = 0;
    int            poll_interval   = (auth->poll_interval > 0) ? auth->poll_interval : 5;

    /* Streaming state machine */
    char   conn_session_id[256] = "";
    char   conn_state[64]       = "";
    bool   conn_started         = false;
    bool   conn_msal_sent       = false;  /* MSAL auth sent at ReadyToConnect */
    bool   conn_sdp_sent        = false;  /* SDP offer POST sent at Provisioned */
    bool   conn_sdp_done        = false;  /* SDP answer received */
    bool   conn_ice_sent        = false;  /* local ICE candidate POSTed */
    int    conn_ice_sockfd      = -1;     /* UDP socket bound for ICE/STUN */
    time_t conn_last_poll       = 0;

#ifndef HAVE_SDL2
    if (screen == UI_SCREEN_LOGIN) {
        printf("\nVisita microsoft.com/link e ingresa: %s\n", auth->user_code);
        consoleUpdate(NULL);
    }
    /* Console mode: wait for first button press */
    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad)) break;
        consoleUpdate(NULL);
        svcSleepThread(16666666ULL);
    }
#endif

    while (running && appletMainLoop()) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);

        if (down & HidNpadButton_Plus)
            running = false;

        switch (screen) {

            /* ---- LOGIN ---- */
            case UI_SCREEN_LOGIN: {
                if (down & HidNpadButton_B) { running = false; break; }

                /* Poll Microsoft every poll_interval seconds */
                time_t now = time(NULL);
                if (now - last_poll >= poll_interval) {
                    last_poll = now;
                    int result = xbox_auth_poll_token(auth);

                    if (result == 1) {
                        /* Got MSA token — exchange for XBL/XSTS, then streaming tokens */
                        ui_set_login_state(auth->user_code, "Verificando con Xbox Live...", 0);
                        if (xbox_auth_exchange_tokens(auth) == 0) {
                            xbox_auth_save(auth, XBOX_AUTH_TOKEN_FILE);  /* save now — streaming tokens are bonus */
                            ui_set_login_state(auth->user_code, "Obteniendo token de streaming...", 0);
                            xbox_auth_get_streaming_tokens(auth);
                            xbox_auth_save(auth, XBOX_AUTH_TOKEN_FILE);
                            if (api) { xcloud_api_destroy(api); api = NULL; }
                            if (auth->gs_token[0]) {
                                ui_log("gsToken OK");
                                api = xcloud_api_create(
                                    auth->streaming_host[0]
                                        ? auth->streaming_host
                                        : "uks.core.gssv-play-prodxhome.xboxlive.com",
                                    auth->gs_token, XCLOUD_TYPE_HOME);
                            } else {
                                ui_log("Sin gsToken");
                            }
                            xbox_host_fetch_from_api(hosts, auth->uhs, auth->xsts_token);
                            xbox_host_discover(hosts);
                            ui_set_host_list(hosts, 0);
                            screen = UI_SCREEN_HOST_LIST;
                        } else {
                            ui_set_error(auth->error[0] ? auth->error
                                         : "Error al verificar con Xbox Live.");
                            screen = UI_SCREEN_ERROR;
                        }
                    } else if (result == -1) {
                        ui_set_error(auth->error[0] ? auth->error
                                     : "Autenticacion cancelada o expirada.");
                        screen = UI_SCREEN_ERROR;
                    }
                    /* result == 0: still pending, update countdown */
                    else {
                        char status[128];
                        time_t elapsed = now - (time(NULL) - auth->expires_in);
                        int remaining  = auth->expires_in - (int)(now - last_poll + poll_interval);
                        if (remaining < 0) remaining = 0;
                        snprintf(status, sizeof(status),
                                 "Esperando... (%d min restantes)", remaining / 60 + 1);
                        ui_set_login_state(auth->user_code, status, remaining);
                        (void)elapsed;
                    }
                }
                break;
            }

            /* ---- HOST LIST ---- */
            case UI_SCREEN_HOST_LIST:
                if (down & HidNpadButton_Down) {
                    if (hosts->count > 0) sel = (sel + 1) % hosts->count;
                    ui_set_host_list(hosts, sel);
                }
                if (down & HidNpadButton_Up) {
                    if (hosts->count > 0) sel = (sel - 1 + hosts->count) % hosts->count;
                    ui_set_host_list(hosts, sel);
                }
                if (down & HidNpadButton_X) {
                    refresh_host_list(hosts, auth, has_internet);
                    sel = 0;
                }
                if ((down & HidNpadButton_A) && hosts->count > 0) {
                    XboxHost* h = &hosts->hosts[sel];
                    if (h->ip[0] == '\0') {
                        ui_log("[CONSOLES] Sin IP local. Crea /switch/greenlight/config.txt con 'ip=192.168.X.X'");
                        ui_set_error("Sin IP local. Crea /switch/greenlight/config.txt con ip=TU_XBOX_IP");
                        screen = UI_SCREEN_ERROR;
                        break;
                    }
                    if (session) { session_destroy(session); session = NULL; }
                    session = session_create("pending", h->ip, "", SESSION_TYPE_HOME);
                    /* Reset streaming state machine */
                    conn_session_id[0] = '\0';
                    conn_state[0]      = '\0';
                    conn_started       = false;
                    conn_msal_sent     = false;
                    conn_sdp_sent      = false;
                    conn_sdp_done      = false;
                    if (conn_ice_sockfd >= 0) { close(conn_ice_sockfd); conn_ice_sockfd = -1; }
                    conn_ice_sent      = false;
                    conn_last_poll     = 0;
                    ui_set_connecting_host(h, "Iniciando sesion de streaming...");
                    screen = UI_SCREEN_CONNECTING;
                }
                break;

            /* ---- CONNECTING ---- */
            case UI_SCREEN_CONNECTING: {
                XboxHost* ch = &hosts->hosts[sel];

                if (down & HidNpadButton_B) {
                    /* Cancel — stop stream if session was created */
                    if (conn_started && conn_session_id[0] && api)
                        xcloud_api_stop_stream(api, conn_session_id, NULL, 0);
                    if (session) { session_destroy(session); session = NULL; }
                    conn_started       = false;
                    conn_msal_sent     = false;
                    conn_sdp_sent      = false;
                    conn_sdp_done      = false;
                    if (conn_ice_sockfd >= 0) { close(conn_ice_sockfd); conn_ice_sockfd = -1; }
                    conn_ice_sent      = false;
                    conn_session_id[0] = '\0';
                    conn_state[0]      = '\0';
                    conn_last_poll     = 0;
                    screen = UI_SCREEN_HOST_LIST;
                    break;
                }

                /* Step 1 — request the streaming session (once) */
                if (!conn_started && api) {
                    ui_log("start id=%.30s", ch->id);
                    char start_resp[4096] = "";
                    int rc = xcloud_api_start_stream(api, ch->id,
                                                     start_resp, sizeof(start_resp));
                    if (!api) {
                        ui_log("api=NULL (sin gsToken)");
                        ui_set_error("No hay token de streaming. Reinicia la app.");
                        screen = UI_SCREEN_ERROR;
                    } else if (rc == 0) {
                        if (xcloud_api_parse_session_id(start_resp,
                                conn_session_id, sizeof(conn_session_id)) == 0) {
                            conn_started = true;
                            session_destroy(session);
                            session = session_create(conn_session_id, ch->ip,
                                                     start_resp, SESSION_TYPE_HOME);
                            ui_log("Session: %.60s", conn_session_id);
                            ui_set_connecting_host(ch, "Sesion creada, esperando Xbox...");
                        } else {
                            ui_log("parse fail: %.80s", start_resp);
                            ui_set_error("No se pudo obtener el ID de sesion");
                            screen = UI_SCREEN_ERROR;
                        }
                    } else {
                        ui_log("start HTTP %d resp=%.80s", rc,
                               start_resp[0] ? start_resp : "(empty)");
                        char emsg[256];
                        snprintf(emsg, sizeof(emsg),
                                 "Error al iniciar sesion (HTTP %d)", rc < 0 ? 0 : rc);
                        ui_set_error(emsg);
                        screen = UI_SCREEN_ERROR;
                    }
                    break;
                }

                /* Step 2 — poll state every 2 seconds */
                if (conn_started && api) {
                    time_t now = time(NULL);
                    if (now - conn_last_poll >= 2) {
                        conn_last_poll = now;
                        char state_resp[2048] = "";
                        int rc = xcloud_api_get_stream_state(api, conn_session_id,
                                                             state_resp, sizeof(state_resp));
                        if (rc == 0) {
                            /* Parse "state":"..." */
                            const char* sp = strstr(state_resp, "\"state\"");
                            if (sp) {
                                sp = strchr(sp, '"');          /* opening of key */
                                sp = strchr(sp + 1, '"');      /* closing of key */
                                sp = strchr(sp + 1, '"');      /* opening of value */
                                if (sp) {
                                    sp++;
                                    const char* ep = strchr(sp, '"');
                                    if (ep) {
                                        size_t slen = (size_t)(ep - sp);
                                        if (slen < sizeof(conn_state)) {
                                            memcpy(conn_state, sp, slen);
                                            conn_state[slen] = '\0';
                                        }
                                    }
                                }
                            }
                            ui_log("state: %s", conn_state);

                            /* SDP answer polling runs every tick regardless of conn_state —
                               the server can revert to "Provisioning" while it processes our
                               offer, and stops there until it sees us fetch the SDP answer
                               (the "SdpExchangeComplete" state Xbox is waiting for). */
                            if (conn_sdp_sent && !conn_sdp_done) {
                                static char sdp_ans[8192];
                                sdp_ans[0] = '\0';
                                int gret = xcloud_api_get_sdp(api, conn_session_id,
                                                               sdp_ans, sizeof(sdp_ans));
                                if (gret == 0 && sdp_ans[0] != '\0') {
                                    conn_sdp_done = true;
                                    char fp[128] = "", remote_ufrag[32] = "", remote_pwd[64] = "", ice_cand[256] = "";
                                    webrtc_sdp_parse_answer_field(sdp_ans, "fingerprint", fp, sizeof(fp));
                                    webrtc_sdp_parse_answer_field(sdp_ans, "ice-ufrag", remote_ufrag, sizeof(remote_ufrag));
                                    webrtc_sdp_parse_answer_field(sdp_ans, "ice-pwd", remote_pwd, sizeof(remote_pwd));
                                    webrtc_sdp_parse_answer_field(sdp_ans, "candidate", ice_cand, sizeof(ice_cand));
                                    ui_log("SDP ANS OK fp=%.30s", fp);
                                    ui_log("remote ufrag=%s pwd=%.10s...", remote_ufrag, remote_pwd);
                                    ui_log_dump("SDP ANSWER (raw)", sdp_ans);

                                    conn_ice_sockfd = webrtc_sdp_get_ice_sockfd();
                                    conn_ice_sent    = true;

                                    /* Parse remote port from candidate line e.g. "a=candidate:1 1 UDP 2122260223 192.168.1.X 50001 typ host..." */
                                    char remote_ip[64] = "";
                                    int remote_port = 0;
                                    if (ch->ip[0]) {
                                        strncpy(remote_ip, ch->ip, sizeof(remote_ip) - 1);
                                    }
                                    if (ice_cand[0] != '\0') {
                                        /* Scan candidate tokens: candidate:1 1 UDP prio ip port ... */
                                        char proto[16]; u32 comp, prio;
                                        if (sscanf(ice_cand, "1 %u %15s %u %63s %d", &comp, proto, &prio, remote_ip, &remote_port) < 5) {
                                            /* Try alternate format if sscanf failed */
                                            char *p = strstr(ice_cand, "typ ");
                                            (void)p;
                                        }
                                    }
                                    if (remote_port <= 0) remote_port = 10257; /* Xbox xHome local streaming port */

                                    /* Perform STUN hole-punching using the exact bound UDP socket from SDP offer */
                                    const char *local_ufrag = webrtc_sdp_get_local_ufrag();

                                    remote_port = webrtc_ice_probe_remote_port(conn_ice_sockfd, remote_ip, remote_port,
                                                                                remote_ufrag, local_ufrag, remote_pwd);

                                    ui_log("DTLS connecting to %s:%d...", remote_ip, remote_port);
                                    if (webrtc_dtls_connect(webrtc, conn_ice_sockfd, remote_ip, remote_port) == 0) {
                                        ui_log("DTLS connected successfully!");
                                        session_set_state(session, SESSION_STATE_STARTED);
                                    } else {
                                        ui_log("DTLS connect failed");
                                    }
                                } else {
                                    ui_log("SDP GET %d (esperando...)", gret);
                                }
                            }

                            if (strcmp(conn_state, "Provisioning") == 0) {
                                ui_set_connecting_host(ch, "Aprovisionando consola...");

                            } else if (strcmp(conn_state, "WaitingForResources") == 0) {
                                ui_set_connecting_host(ch, "En cola de recursos...");

                            } else if (strcmp(conn_state, "ReadyToConnect") == 0) {
                                if (!conn_msal_sent && auth->access_token &&
                                    auth->access_token[0]) {
                                    ui_set_connecting_host(ch, "Autenticando con Xbox...");
                                    char msal_resp[512] = "";
                                    xcloud_api_send_msal_auth(api, conn_session_id,
                                        auth->access_token, msal_resp, sizeof(msal_resp));
                                    ui_log("MSAL auth sent");
                                    conn_msal_sent = true;
                                }

                            } else if (strcmp(conn_state, "Provisioned") == 0) {
                                if (!conn_sdp_sent) {
                                    /* Generate a real WebRTC SDP offer and POST it */
                                    conn_sdp_sent = true;
                                    ui_set_connecting_host(ch, "Provisionado! Generando SDP...");
                                    static char real_sdp[4096];
                                    real_sdp[0] = '\0';
                                    if (webrtc_sdp_create_offer(real_sdp, sizeof(real_sdp)) != 0) {
                                        ui_log("SDP create FAIL");
                                    } else {
                                        ui_log_dump("SDP OFFER (sent)", real_sdp);
                                        ui_set_connecting_host(ch, "Enviando SDP offer...");
                                        char sdp_resp[512] = "";
                                        int sret = xcloud_api_send_sdp(api, conn_session_id,
                                                                       real_sdp, sdp_resp, sizeof(sdp_resp));
                                        ui_log("SDP POST %d %.60s", sret, sdp_resp);
                                    }
                                } else if (!conn_sdp_done) {
                                    ui_set_connecting_host(ch, "Esperando respuesta SDP...");
                                }

                            } else if (strcmp(conn_state, "Failed") == 0) {
                                /* Try to get error message */
                                char emsg[256] = "La consola rechazo la conexion";
                                const char* em = strstr(state_resp, "\"message\"");
                                if (em) {
                                    em = strchr(em, ':');
                                    if (em) {
                                        em = strchr(em, '"');
                                        if (em) {
                                            em++;
                                            const char* ee = strchr(em, '"');
                                            if (ee && ee - em < 200) {
                                                int elen = (int)(ee - em);
                                                snprintf(emsg, sizeof(emsg),
                                                         "%.*s", elen, em);
                                            }
                                        }
                                    }
                                }
                                ui_set_error(emsg);
                                screen = UI_SCREEN_ERROR;

                            } else if (conn_state[0] != '\0') {
                                /* Unknown state */
                                char msg[128];
                                snprintf(msg, sizeof(msg), "Estado: %s", conn_state);
                                ui_set_connecting_host(ch, msg);
                            }

                        } else if (rc == 404) {
                            ui_set_error("Sesion no encontrada en el servidor");
                            screen = UI_SCREEN_ERROR;
                        }
                        /* Other HTTP errors: keep polling */
                    }
                }
                break;
            }

            /* ---- ERROR ---- */
            case UI_SCREEN_ERROR:
                if (down & HidNpadButton_B) {
                    ui_set_error(NULL);
                    screen = (auth->state == XBOX_AUTH_STATE_OK)
                             ? UI_SCREEN_HOST_LIST
                             : UI_SCREEN_LOGIN;
                }
                break;

            default:
                screen = UI_SCREEN_HOST_LIST;
                break;
        }

        /* Render */
#ifndef HAVE_SDL2
        printf("\x1b[2J\x1b[H");
#endif
        ui_draw_screen(screen);
#ifndef HAVE_SDL2
        draw_hint(screen);
        consoleUpdate(NULL);
#endif

        svcSleepThread(16666666ULL);
    }

    /* ---- Cleanup ---- */
    if (session) session_destroy(session);
    if (api)     xcloud_api_destroy(api);
    xbox_auth_destroy(auth);
    xbox_host_list_destroy(hosts);
    webrtc_destroy(webrtc);
    audio_decoder_destroy(audio);
    video_decoder_destroy(video);
    input_handler_deinit();
    ui_deinit();
    socketExit();
#ifndef HAVE_SDL2
    consoleExit(NULL);
#endif
    return 0;
}
