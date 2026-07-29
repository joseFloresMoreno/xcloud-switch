#ifndef WEBRTC_ICE_H
#define WEBRTC_ICE_H

/* Opens a UDP socket bound to an ephemeral local port and builds a WebRTC
   "host" ICE candidate string describing it (our LAN IP + that port).
   ufrag: the ICE username fragment to embed in the candidate line (must
          match the one used in our SDP offer).
   out:   receives the candidate JSON object, e.g.
          {"candidate":"candidate:1 1 udp 2122260223 192.168.1.50 53421 typ host ...","sdpMid":"0","sdpMLineIndex":0}
   sockfd_out: receives the open UDP socket fd (caller owns it — close() when done).
   Returns 0 on success, -1 on error. */
int webrtc_ice_create_local_candidate(const char *ufrag,
                                       char *out, int out_size,
                                       int *sockfd_out);

/* Opens a UDP socket bound to an ephemeral local port and returns our LAN
   IP + that port, without building any candidate string. Used to embed a
   real host candidate directly into the SDP offer (non-trickle ICE).
   ip_out: buffer >= INET_ADDRSTRLEN bytes.
   Returns 0 on success, -1 on error. Caller owns the returned sockfd. */
int webrtc_ice_get_local_endpoint(char *ip_out, int ip_size,
                                   int *port_out, int *sockfd_out);

/* Sends authenticated STUN Binding Requests (with USERNAME and MESSAGE-INTEGRITY)
   to Xbox ports to perform ICE connectivity checks and wake up Xbox listener.
   Returns the active remote port, or default_port if fallback. */
int webrtc_ice_probe_remote_port(int sockfd, const char *remote_ip, int default_port,
                                  const char *remote_ufrag, const char *local_ufrag,
                                  const char *remote_pwd);

#endif /* WEBRTC_ICE_H */
