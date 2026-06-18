#ifndef WEBRTC_SDP_H
#define WEBRTC_SDP_H

/* Build a WebRTC SDP offer (JSON-escaped, ready to embed in a JSON string value)
   for Xbox xhome streaming: H264 video + Opus audio + data channel.
   out:      buffer ≥ 4096 bytes
   Returns 0 on success, -1 on error. */
int webrtc_sdp_create_offer(char *out, int out_size);

/* JSON-escape an SDP string: converts CRLF → \r\n, " → \", \ → \\
   Safe to call with raw = out (in-place not supported — use separate buffers).
   Returns 0. */
int webrtc_sdp_json_escape(const char *raw, char *out, int out_size);

/* Parse a field from the SDP answer.
   field: SDP attribute name without leading "a=" (e.g. "ice-ufrag", "fingerprint")
   out:   receives the value after the colon/space, NUL-terminated
   Returns 0 on success, -1 if not found. */
int webrtc_sdp_parse_answer_field(const char *sdp_json,
                                   const char *field,
                                   char *out, int out_size);

/* ICE credentials used in the most recently created offer (for building
   a matching local host candidate). Valid only after webrtc_sdp_create_offer(). */
const char* webrtc_sdp_get_local_ufrag(void);
const char* webrtc_sdp_get_local_pwd(void);

/* UDP socket opened while building the last offer's embedded host candidate.
   Caller owns it (close() when the connection ends). -1 if none was opened. */
int webrtc_sdp_get_ice_sockfd(void);

#endif /* WEBRTC_SDP_H */
