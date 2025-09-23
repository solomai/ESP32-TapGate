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
    enroll: { url: "/api/enroll", fields: ["password"], focus: { invalid_password: "password" } },
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

  function submitForm(event) {
    event.preventDefault();
    const form = event.currentTarget;
    const actionKey = form.dataset.action;
    const config = actionMap[actionKey];
    if (!config) return;

    setMessage(form, "");
    config.fields.forEach((field) => setFieldError(form, field, false));

    const params = new URLSearchParams();
    config.fields.forEach((field) => {
      params.set(field, form.elements[field] ? form.elements[field].value : "");
    });

    fetch(config.url, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: params.toString()
    })
      .then((response) => response.json().catch(() => null))
      .then((data) => handleActionResponse(form, config, data))
      .catch(() => setMessage(form, "Unable to reach device."));
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
    const portalName = `${info.ap_ssid} Admin`;
    document.querySelectorAll("[data-bind='portal-name']").forEach((el) => {
      if (el.tagName === "INPUT") el.value = portalName;
      else el.textContent = portalName;
    });
    document.querySelectorAll("[data-bind='ssid']").forEach((el) => {
      if (el.tagName === "INPUT") el.value = info.ap_ssid;
      else el.textContent = info.ap_ssid;
    });
  }

  function refreshSession() {
    fetch("/api/session", { method: "GET", cache: "no-store" })
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

  document.addEventListener("DOMContentLoaded", () => {
    attachForms();
    refreshSession();
  });
})();
