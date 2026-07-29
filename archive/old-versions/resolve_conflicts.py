#!/usr/bin/env python3
"""
Resolve git merge conflicts in xbox_auth.c.

Strategy:
  - Conflicts in includes / http_post_json signature / GSSV XSTS body/headers:
      Keep HEAD (newer version with ui_log, proper XSTS headers, better error messages)
  - For the xhome streaming login block: HEAD has the correct format
    {token: gssv_token, offeringId: xhome} which is what Greenlight/OpenXbox uses.
  - For the refresh error message: HEAD has more detail, keep HEAD.
"""

import re, sys, shutil, os

filepath = os.path.join(os.path.dirname(__file__), "src", "xbox_auth.c")
backup   = filepath + ".conflict-backup"

# ------------------------------------------------------------------ #
# 1. Read                                                              #
# ------------------------------------------------------------------ #
with open(filepath, "r", encoding="utf-8", errors="replace") as f:
    content = f.read()

# Verify there are conflicts to resolve
conflicts = re.findall(r"<<<<<<<[^\n]*\n", content)
print(f"Found {len(conflicts)} conflict markers (<<<<<<< lines)")

# ------------------------------------------------------------------ #
# 2. Backup                                                            #
# ------------------------------------------------------------------ #
shutil.copy2(filepath, backup)
print(f"Backup saved: {backup}")

# ------------------------------------------------------------------ #
# 3. Parse and resolve each conflict block                            #
# ------------------------------------------------------------------ #
# Pattern: <<<<<<< HEAD\n<ours>\n=======\n<theirs>\n>>>>>>> <hash>\n
CONFLICT_RE = re.compile(
    r"<<<<<<< HEAD\n(.*?)\n=======\n(.*?)\n>>>>>>> [^\n]+\n",
    re.DOTALL
)

decisions = []

def resolver(m):
    ours   = m.group(1)
    theirs = m.group(2)

    # ---- Conflict 1: include "ui.h" vs (nothing) ----
    # HEAD adds #include "ui.h"; theirs has nothing between markers.
    # Resolution: keep both (ui.h is needed for ui_log)
    if '#include "ui.h"' in ours and theirs.strip() == "":
        decisions.append("C1: keep HEAD (#include ui.h)")
        return '#include "ui.h"\n'

    # ---- Conflict 2: json_long — string-encoded number handling ----
    # HEAD: adds `if (*p == '"') p++;` before atol (handles XErr as string)
    # Theirs: missing that guard.
    # Resolution: HEAD (more robust)
    if 'string-encoded number' in ours or ("XErr" in ours and "atol" not in ours):
        decisions.append("C2: keep HEAD (json_long string guard + atol)")
        return ours + "\n    *out = atol(p);\n    return 0;\n}\n"

    # ---- Conflict 3: http_post_json signature ----
    # HEAD: extra_headers (curl_slist*) — more flexible
    # Theirs: auth_header (const char*) — simpler
    # Resolution: HEAD (extra_headers is more flexible, used later in the file)
    if 'extra_headers' in ours and 'auth_header' in theirs:
        decisions.append("C3: keep HEAD (http_post_json with extra_headers)")
        return ours + "\n"

    # ---- Conflict 4: extra_headers loop vs single auth_header append ----
    if 'for (struct curl_slist* h = extra_headers' in ours:
        decisions.append("C4: keep HEAD (extra_headers loop)")
        return ours + "\n"

    # ---- Conflict 5: xbox_auth_get_streaming_tokens guard ----
    # HEAD: checks xsts_token; Theirs: checks xbl_token
    # Resolution: HEAD — xsts_token is the right token for GSSV XSTS
    if 'xsts_token' in ours and 'xbl_token' in theirs:
        decisions.append("C5: keep HEAD (guard on xsts_token)")
        return ours + "\n"

    # ---- Conflict 6: GSSV XSTS body + extra headers (big block) ----
    # HEAD: has correct RelyingParty with trailing slash, browser headers
    # Theirs: simpler body, no browser headers
    # Resolution: HEAD (trailing slash on RelyingParty and browser headers are required)
    if 'gssv.xboxlive.com/' in ours and 'gssv.xboxlive.com"' in theirs:
        decisions.append("C6: keep HEAD (GSSV XSTS body with trailing slash + browser headers)")
        return ours + "\n"

    # ---- Conflict 7: GSSV XSTS error message ----
    # HEAD: detailed message with raw response
    # Theirs: short message
    # Resolution: HEAD (more diagnostic info)
    if 'ui_log' in ours and 'XErr' in ours:
        decisions.append("C7: keep HEAD (GSSV XSTS error with ui_log)")
        return ours + "\n"

    # ---- Conflict 8: GSSV XSTS success — ui_log vs printf ----
    if 'ui_log("[AUTH] GSSV XSTS OK")' in ours:
        decisions.append("C8: keep HEAD (GSSV XSTS OK ui_log)")
        return ours + "\n"

    # ---- Conflict 9: xhome login block ----
    # HEAD: {token: gssv_token, offeringId: xhome} — correct Greenlight format
    # Theirs: Bearer auth header with market/language body — wrong endpoint usage
    # Resolution: HEAD
    if 'offeringId' in ours and 'Authorization: Bearer' in theirs:
        decisions.append("C9: keep HEAD (xhome login with offeringId)")
        return ours + "\n"

    # ---- Conflict 10: xhome error + gsToken parse ----
    # HEAD: ui_log + detailed error
    # Theirs: printf
    if 'ui_log("[AUTH] xhome HTTP' in ours or 'gsToken no encontrado' in ours:
        decisions.append("C10: keep HEAD (xhome ui_log messages)")
        return ours + "\n"

    # ---- Conflict 11: refresh error message ----
    if 'Refresh HTTP' in ours and 'ui_log' in ours:
        decisions.append("C11: keep HEAD (refresh error with ui_log)")
        return ours + "\n"

    # Fallback: keep HEAD for any unmatched conflict
    decisions.append(f"FALLBACK → HEAD: {repr(ours[:60])}")
    return ours + "\n"

resolved = CONFLICT_RE.sub(resolver, content)

# ------------------------------------------------------------------ #
# 4. Verify no markers remain                                          #
# ------------------------------------------------------------------ #
remaining = re.findall(r"<<<<<<< |=======\n|>>>>>>> ", resolved)
if remaining:
    print(f"WARNING: {len(remaining)} conflict markers still present!")
    sys.exit(1)

# ------------------------------------------------------------------ #
# 5. Write resolved file                                               #
# ------------------------------------------------------------------ #
with open(filepath, "w", encoding="utf-8", newline="\r\n") as f:
    f.write(resolved)

print(f"\n✅ Resolved {len(decisions)} conflicts:")
for i, d in enumerate(decisions, 1):
    print(f"  {i}. {d}")
print(f"\nFile written: {filepath}")
print(f"Backup at:   {backup}")
