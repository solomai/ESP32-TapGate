(function () {
  const MIN_PASSWORD_LENGTH = 8;

  const errorMessages = {
    invalid_password: `Password must be at least ${MIN_PASSWORD_LENGTH} characters long.` ,
    wrong_password: "Password is incorrect. Try again.",
    invalid_new_password: `New password must be at least ${MIN_PASSWORD_LENGTH} characters long.` ,
    invalid_ssid: "Please enter a valid network name.",
    storage_failed: "Unable to save changes. Please try again.",
    invalid_request: "Request could not be processed."
  };

  const actionMap = {
    enroll: {
      url: "/api/enroll",
      fields: ["password", "portal"],
      focus: { invalid_password: "password", invalid_ssid: "portal" }
    },
    login: { url: "/api/login", fields: ["password"], focus: { wrong_password: "password" } },
    changePassword: {
      url: "/api/change-password",
      fields: ["current", "next"],
      focus: { wrong_password: "current", invalid_new_password: "next" }
    },
    device: { url: "/api/device", fields: ["ssid"], focus: { invalid_ssid: "ssid" } }
  };

  function setMessage(form, text) {
    const message = form.querySelector("[data-message]");
    if (message) message.textContent = text || "";
  }

  function setFieldError(form, name, hasError) {
    if (!name) return;
    const field = form.elements[name];
    if (field) {
      if (hasError) field.classList.add("error");
      else field.classList.remove("error");
    }
  }

  function focusField(form, name) {
    if (!name) return;
    const field = form.elements[name];
    if (field) field.focus();
  }

  function showError(form, code, config) {
    const message = errorMessages[code] || "Unexpected error.";
    setMessage(form, message);
    Array.from(form.elements).forEach((element) => {
      if (element && element.name) {
        setFieldError(form, element.name, false);
      }
    });
    if (config && config.focus && config.focus[code]) {
      setFieldError(form, config.focus[code], true);
      focusField(form, config.focus[code]);
    }
  }

  function handleActionResponse(form, config, data) {
    console.log('handleActionResponse called with:', data);
    
    if (!data) {
      setMessage(form, "Connection problem. Try again.");
      return;
    }

    if (data.status === "busy") {
      console.log('Redirecting to busy page');
      window.location.assign("/busy/");
      return;
    }

    if (data.status === "redirect" && data.redirect) {
      console.log('Redirecting to:', data.redirect);
      window.location.assign(data.redirect);
      return;
    }

    if (data.status === "ok" && data.redirect) {
      console.log('Success redirect to:', data.redirect);
      // Force page reload to ensure cookies are properly set
      // Use window.location.href instead of assign() for full page reload
      setTimeout(() => {
        window.location.href = data.redirect;
      }, 100);  // Small delay to ensure response is processed
      return;
    }

    if (data.status === "error" && data.code) {
      showError(form, data.code, config);
      return;
    }

    setMessage(form, "Unexpected server reply.");
  }

  function runClientValidation(form, actionKey) {
    if (actionKey === "enroll") {
      const portalInput = form.elements["portal"];
      if (portalInput && !portalInput.value.trim()) {
        setFieldError(form, "portal", true);
        setMessage(form, "Please enter a portal name.");
        focusField(form, "portal");
        if (typeof portalInput.select === "function") {
          portalInput.select();
        }
        return false;
      }

      const passwordInput = form.elements["password"];
      if (passwordInput && passwordInput.value.length < MIN_PASSWORD_LENGTH) {
        setFieldError(form, "password", true);
        passwordInput.focus();
        setMessage(form, errorMessages.invalid_password);
        return false;
      }
    }

    return true;
  }

  function submitForm(event) {
    event.preventDefault();
    const form = event.currentTarget;
    const actionKey = form.dataset.action;
    const config = actionMap[actionKey];
    if (!config) return;

    setMessage(form, "");
    config.fields.forEach((field) => setFieldError(form, field, false));

    if (!runClientValidation(form, actionKey)) return;

    const params = new URLSearchParams();
    config.fields.forEach((field) => {
      params.set(field, form.elements[field] ? form.elements[field].value : "");
    });

    console.log(`Submitting form for ${config.url} with data:`, params.toString());
    
    fetch(config.url, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: params.toString(),
      credentials: "include"  // Ensure cookies are included and saved
    })
      .then((response) => {
        console.log(`Response for ${config.url}: status=${response.status}, ok=${response.ok}`);
        console.log('Response headers:', [...response.headers.entries()]);
        
        // Extract Set-Cookie header and set it manually if present
        const setCookie = response.headers.get('set-cookie');
        if (setCookie) {
          console.log('Found Set-Cookie header:', setCookie);
          // Extract cookie name and value
          const cookieMatch = setCookie.match(/tg_session=([^;]+)/);
          if (cookieMatch) {
            const cookieValue = cookieMatch[1];
            console.log('Setting cookie manually:', cookieValue);
            document.cookie = `tg_session=${cookieValue}; Path=/; SameSite=Lax; Max-Age=900`;
          }
        }
        
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        return response.json().catch((err) => {
          console.error('JSON parse error:', err);
          throw new Error('Invalid JSON response');
        });
      })
      .then((data) => {
        console.log(`JSON response for ${config.url}:`, data);
        handleActionResponse(form, config, data);
      })
      .catch((error) => {
        console.error(`Fetch error for ${config.url}:`, error);
        setMessage(form, "Connection problem. Try again.");
      });
  }

  function attachForms() {
    document.querySelectorAll("form[data-action]").forEach((form) => {
      form.addEventListener("submit", submitForm);
      Array.from(form.elements).forEach((el) => {
        if (el && el.tagName === "INPUT") {
          el.addEventListener("input", () => {
            el.classList.remove("error");
            setMessage(form, "");
          });
        }
      });
    });
  }

  function applySession(info) {
    if (!info || info.status !== "ok") return;
    const portalName = info.ap_ssid || "";
    document.querySelectorAll("[data-bind='portal-name']").forEach((el) => {
      if (el.tagName === "INPUT") {
        if (el.hasAttribute("readonly") || !el.value) {
          el.value = portalName;
        }
        el.defaultValue = portalName;
        el.classList.remove("error");
      } else {
        el.textContent = portalName;
      }
    });
    document.querySelectorAll("[data-bind='ssid']").forEach((el) => {
      if (el.tagName === "INPUT") el.value = info.ap_ssid;
      else el.textContent = info.ap_ssid;
    });
  }

  function applyInitialData() {
    const initialData = window.TAPGATE_INITIAL_DATA;
    if (!initialData || !initialData.ap_ssid) {
      console.log("No initial data from server, falling back to API");
      refreshSession();
      return;
    }

    console.log("Applying initial data from server:", initialData);

    const portalName = initialData.ap_ssid || "";

    document.querySelectorAll("[data-bind='portal-name']").forEach((el) => {
      if (el.tagName === "INPUT") {
        if (!el.value.trim()) {
          el.value = portalName;
          console.log(`Set portal input value to: "${portalName}"`);
        }
        el.defaultValue = portalName;
        el.classList.remove("error");
      } else {
        el.textContent = portalName;
      }
    });

    document.querySelectorAll("[data-bind='ssid']").forEach((el) => {
      if (el.tagName === "INPUT") {
        el.value = initialData.ap_ssid || "";
      } else {
        el.textContent = initialData.ap_ssid || "";
      }
    });
  }

  function refreshSession() {
    fetch("/api/session", { method: "GET", cache: "no-store", credentials: "same-origin" })
      .then((response) => response.json().catch(() => null))
      .then((data) => {
        if (!data) return;
        if (data.status === "busy") {
          window.location.assign("/busy/");
          return;
        }
        if (data.status === "expired" && data.redirect) {
          window.location.assign(data.redirect);
          return;
        }
        applySession(data);
      })
      .catch(() => {});
  }

  function initialize() {
    console.log("Initializing app...");
    attachForms();
    applyInitialData();
  }

  initialize();
})();
