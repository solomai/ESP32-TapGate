# Debug Scenario - Multiple Client Enrollment

## Expected Behavior After Fix

### Scenario 1: Multiple clients during enrollment
1. **Client A (10.10.0.2)** connects → Creates session, accesses `/enroll/` ✅
2. **Client B (10.10.0.3)** connects → Should get new session, access `/enroll/` ✅

### Scenario 2: After enrollment completion
1. **Client A** completes enrollment → Password set, session authorized ✅
2. **Client B** tries to access → Should get BUSY ✅
3. **Client A** reconnects → Should reconnect seamlessly ✅

## Debug Logs to Watch For

### Expected logs for Client B during enrollment:
```
[INFO] AdminPortal: evaluate_unified_session: IP 10.10.0.3 check result: NONE
[INFO] AdminPortal: evaluate_unified_session: Cookie check (has_token=no) result: NONE  
[INFO] AdminPortal: create_unified_session: client_ip=10.10.0.3, has_authorized_session=no, has_password=no
[INFO] AdminPortal: Creating new session for client IP 10.10.0.3
[INFO] AdminPortal: Resolve page: session status: MATCH, requested page: ENROLL, AP PSW: not set, authorized: no, state obj: yes
```

### Expected logs for Client B after enrollment (should be BUSY):
```
[INFO] AdminPortal: evaluate_unified_session: IP 10.10.0.3 check result: BUSY
[INFO] AdminPortal: Resolve page: session status: BUSY, requested page: MAIN, AP PSW: set, authorized: yes, state obj: yes
```

## Testing Steps
1. Flash updated firmware
2. Connect Client A → Access /enroll/
3. Connect Client B → Should access /enroll/ (not BUSY)
4. Complete enrollment with Client A
5. Try Client B → Should get BUSY
6. Reconnect Client A → Should work seamlessly