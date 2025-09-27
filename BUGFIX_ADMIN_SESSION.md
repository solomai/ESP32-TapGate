# Виправлення проблеми з сесією адмін-порталу після енроллменту

## Проблема

Після успішного завершення процесу енроллменту адміністратора (встановлення паролю та назви порталу), користувач отримував повідомлення "Unable to reach device" замість переходу на головну сторінку.

### Симптоми
1. Енроллмент завершувався успішно на бекенді
2. Клієнт отримував відповідь `{"status":"ok","redirect":"/main/"}`
3. При переході на `/main/` користувач переадресовувався на `/enroll/` або `/auth/`
4. При спробі авторизації клієнт отримував помилку `net::ERR_INVALID_HTTP_RESPONSE`

### Логи

**Бекенд:**
```
I (00:02:40.589) AdminPortal: Enrollment successful, redirecting to main page (AP SSID="TapGate Test")
```

**Клієнт:**
```
POST http://10.10.0.1/api/login net::ERR_INVALID_HTTP_RESPONSE
Request failed: TypeError: Failed to fetch
Current path: /auth/
Unable to reach device. Details: Failed to fetch
```

## Причина проблеми

Проблема полягала в неправильному управлінні session cookies після успішного енроллменту та авторизації:

1. **Енроллмент:**
   - Функція `handle_enroll` створювала сесію через `ensure_session_claim`
   - Після успішного енроллменту викликалась `admin_portal_state_authorize_session`
   - **НО session cookie НЕ оновлювався** після авторизації сесії
   - Це призводило до втрати авторизованої сесії на клієнті

2. **Логін:**
   - Аналогічна проблема в `handle_login`
   - Сесія авторизувалась, але session cookie не оновлювався

3. **Наслідки:**
   - Клієнт втрачав авторизовану сесію
   - Усі подальші запити мали статус `session=none`
   - Функція `admin_portal_state_resolve_page` переадресовувала на `/enroll/` або `/auth/`

## Вирішення

Додано встановлення session cookie після успішної авторизації в обох функціях:

### `handle_enroll`
```c
admin_portal_state_authorize_session(&g_state);

// Set session cookie after authorization to ensure authorized session is maintained
uint32_t max_age = g_state.inactivity_timeout_ms ? (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
set_session_cookie(req, token, max_age);
```

### `handle_login`
```c
admin_portal_state_authorize_session(&g_state);

// Set session cookie after authorization to ensure authorized session is maintained  
uint32_t max_age = g_state.inactivity_timeout_ms ? (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
set_session_cookie(req, token, max_age);
```

## Результат

Після виправлення:
1. Енроллмент завершується успішно
2. Session cookie встановлюється з авторизованою сесією
3. Клієнт успішно переходить на головну сторінку (`/main/`)
4. Подальші запити використовують авторизовану сесію

## Файли змінено
- `esp32_mcu/components/admin_portal/http_service.c`

## Тестування
Для тестування потрібно:
1. Підключитись до AP ESP32
2. Відкрити адмін-портал  
3. Пройти процес енроллменту
4. Переконатись що після натискання "Ok" відкривається головна сторінка
5. Перевірити що авторизація працює коректно