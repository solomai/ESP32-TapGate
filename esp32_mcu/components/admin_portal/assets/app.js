(function () {
  const MIN_PASSWORD_LENGTH = 8;

  const errorMessages = {
    invalid_password: `Password must be at least ${MIN_PASSWORD_LENGTH} characters long.`,
    wrong_password: "Password is incorrect. Try again.",
    invalid_new_password: `New password must be at least ${MIN_PASSWORD_LENGTH} characters long.`,
    invalid_ssid: "Please enter a valid network name.",
    storage_failed: "Unable to save changes. Please try again.",
    invalid_request: "Request could not be processed."
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

  function showError(form, code) {
    const message = errorMessages[code] || "Unexpected error.";
    setMessage(form, message);
  }

  // Simple form validation before submit
  function validateForm(form, actionKey) {
    // Clear previous errors
    setMessage(form, "");
    Array.from(form.elements).forEach(el => {
      if (el.classList) el.classList.remove("error");
    });

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
        setMessage(form, errorMessages.invalid_password);
        focusField(form, "password");
        if (typeof passwordInput.select === "function") {
          passwordInput.select();
        }
        return false;
      }
    }

    if (actionKey === "login") {
      const passwordInput = form.elements["password"];
      if (passwordInput && !passwordInput.value) {
        setFieldError(form, "password", true);
        setMessage(form, "Please enter password.");
        focusField(form, "password");
        return false;
      }
    }

    if (actionKey === "changePassword") {
      const currentInput = form.elements["current"];
      const nextInput = form.elements["next"];
      
      if (currentInput && !currentInput.value) {
        setFieldError(form, "current", true);
        setMessage(form, "Please enter current password.");
        focusField(form, "current");
        return false;
      }
      
      if (nextInput && nextInput.value.length < MIN_PASSWORD_LENGTH) {
        setFieldError(form, "next", true);
        setMessage(form, errorMessages.invalid_new_password);
        focusField(form, "next");
        if (typeof nextInput.select === "function") {
          nextInput.select();
        }
        return false;
      }
    }

    if (actionKey === "device") {
      const ssidInput = form.elements["ssid"];
      if (ssidInput && !ssidInput.value.trim()) {
        setFieldError(form, "ssid", true);
        setMessage(form, "Please enter network name.");
        focusField(form, "ssid");
        if (typeof ssidInput.select === "function") {
          ssidInput.select();
        }
        return false;
      }
    }

    return true;
  }

  function handleFormSubmit(event) {
    const form = event.target;
    const actionKey = form.getAttribute("data-action");
    
    if (!actionKey) {
      console.warn("Form missing data-action attribute");
      return true; // Allow normal submit
    }

    console.log(`Validating form for action: ${actionKey}`);
    
    // Validate form before submit
    if (!validateForm(form, actionKey)) {
      event.preventDefault(); // Prevent submit if validation fails
      return false;
    }

    // For mobile compatibility: prevent default submit and handle manually
    event.preventDefault();
    
    console.log(`Form validation passed, submitting manually to: ${form.action}`);
    
    // Show loading state
    const submitButton = form.querySelector('button[type="submit"]');
    const originalText = submitButton ? submitButton.textContent : '';
    if (submitButton) {
      submitButton.disabled = true;
      submitButton.textContent = 'Processing...';
    }
    
    // Create form data and submit via fetch for better control
    const formData = new FormData(form);
    
    fetch(form.action, {
      method: 'POST',
      body: formData,
      credentials: 'include',
      redirect: 'manual'  // Handle redirects manually
    })
    .then(response => {
      console.log(`Response status: ${response.status}, redirected: ${response.redirected}`);
      
      // Handle redirects manually for mobile compatibility
      if (response.status === 302) {
        const location = response.headers.get('Location');
        if (location) {
          console.log(`Got redirect location from header: ${location}`);
          window.location.href = location;
          return;
        }
        
        // Fallback: try to read JSON response for redirect URL
        return response.json().then(data => {
          if (data && data.redirect) {
            console.log(`Got redirect location from JSON: ${data.redirect}`);
            window.location.href = data.redirect;
            return;
          }
          throw new Error('Redirect response without location');
        }).catch(() => {
          // Final fallback: use expected redirect
          const location = getExpectedRedirect(actionKey);
          console.log(`Using expected redirect location: ${location}`);
          window.location.href = location;
        });
      }
      
      if (response.ok) {
        // Success without redirect - navigate to expected page
        const location = getExpectedRedirect(actionKey);
        console.log(`Success response, navigating to: ${location}`);
        window.location.href = location;
        return;
      }
      
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    })
    .catch(error => {
      console.error('Form submission error:', error);
      setMessage(form, "Connection problem. Try again.");
      
      // Restore button state
      if (submitButton) {
        submitButton.disabled = false;
        submitButton.textContent = originalText;
      }
    });
    
    return false; // Prevent default form submission
  }
  
  function getExpectedRedirect(actionKey) {
    switch(actionKey) {
      case 'enroll':
      case 'login':
        return '/main/';
      case 'changePassword':
        return '/device/';
      case 'device':
        return '/main/';
      default:
        return '/';
    }
  }

  function attachFormHandlers() {
    document.querySelectorAll("form[data-action]").forEach((form) => {
      console.log(`Attaching handler to form with action: ${form.getAttribute("data-action")}`);
      form.addEventListener("submit", handleFormSubmit);
      
      // Clear errors on input
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

  function ensureRequiredElements() {
    document.querySelectorAll("[data-message]").forEach((msg) => {
      if (!msg.textContent) msg.textContent = "";
    });
  }

  function initializePage() {
    console.log("Initializing simple form handling (no AJAX)");
    ensureRequiredElements();
    attachFormHandlers();
  }

  // Initialize when DOM is ready
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initializePage);
  } else {
    initializePage();
  }
})();