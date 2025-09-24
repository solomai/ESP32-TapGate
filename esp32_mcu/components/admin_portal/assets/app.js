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
    if (!data) {
      setMessage(form, "Connection problem. Try again.");
      return;
    }

    if (data.status === "busy") {
      window.location.assign("/busy/");
      return;
    }

    if (data.status === "redirect" && data.redirect) {
      window.location.assign(data.redirect);
      return;
    }

    if (data.status === "ok" && data.redirect) {
      window.location.assign(data.redirect);
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

    fetch(config.url, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: params.toString(),
      credentials: "same-origin"
    })
      .then((response) => response.json().catch(() => null))
      .then((data) => handleActionResponse(form, config, data))
      .catch(() => setMessage(form, "Unable to reach device."));
  }

  const sessionState = {
    domReady: document.readyState === "interactive" || document.readyState === "complete",
    info: null,
  };

  function attachForms() {
    document.querySelectorAll("form[data-action]").forEach((form) => {
      form.addEventListener("submit", submitForm);
      Array.from(form.elements).forEach((el) => {
        if (el && el.tagName === "INPUT") {
          el.addEventListener("input", () => {
            if (el.dataset && el.dataset.bind) {
              el.dataset.userEdited = "true";
            }
            el.classList.remove("error");
            setMessage(form, "");
          });
        }
      });
    });
  }

  function updateBoundElements(bindName, value, respectUserInput) {
    document.querySelectorAll(`[data-bind='${bindName}']`).forEach((el) => {
      if (el.tagName === "INPUT") {
        const shouldUpdateValue = !respectUserInput || !el.dataset || !el.dataset.userEdited;
        if (shouldUpdateValue) {
          el.value = value;
          el.defaultValue = value;
        }
        el.classList.remove("error");
      } else {
        el.textContent = value;
      }
    });
  }

  function applySessionToDom() {
    if (!sessionState.domReady || !sessionState.info || sessionState.info.status !== "ok") return;

    const portalName = sessionState.info.ap_ssid || sessionState.info.portal_name || "";
    const ssidValue = sessionState.info.ap_ssid || sessionState.info.portal_name || "";

    updateBoundElements("portal-name", portalName, true);
    updateBoundElements("ssid", ssidValue, true);
  }

  function handleSessionPayload(data) {
    if (!data) return;
    if (data.status === "busy") {
      window.location.assign("/busy/");
      return;
    }
    if (data.status === "expired" && data.redirect) {
      window.location.assign(data.redirect);
      return;
    }

    sessionState.info = data;
    applySessionToDom();
  }

  function refreshSession() {
    fetch("/api/session", { method: "GET", cache: "no-store", credentials: "same-origin" })
      .then((response) => response.json().catch(() => null))
      .then(handleSessionPayload)
      .catch(() => {});
  }

  if (typeof window !== "undefined" && window.addEventListener) {
    window.addEventListener("tapgate-session", (event) => {
      handleSessionPayload(event && event.detail ? event.detail : null);
    });
  }

  const bootstrapSession =
    typeof window !== "undefined" ? window.__TAPGATE_SESSION__ : null;

  if (bootstrapSession) {
    handleSessionPayload(bootstrapSession);
  } else {
    refreshSession();
  }

  if (!sessionState.domReady) {
    document.addEventListener("DOMContentLoaded", () => {
      sessionState.domReady = true;
      attachForms();
      applySessionToDom();
    });
  } else {
    attachForms();
    applySessionToDom();
  }
})();
