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

      const confirmPasswordInput = form.elements["confirm_password"];
      if (passwordInput && confirmPasswordInput && passwordInput.value !== confirmPasswordInput.value) {
        setFieldError(form, "confirm_password", true);
        setMessage(form, "Passwords do not match.");
        focusField(form, "confirm_password");
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
      const confirmPasswordInput = form.elements["confirm_password"];
      
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
      
      if (nextInput && confirmPasswordInput && nextInput.value !== confirmPasswordInput.value) {
        setFieldError(form, "confirm_password", true);
        setMessage(form, "Passwords do not match.");
        focusField(form, "confirm_password");
        if (typeof confirmPasswordInput.select === "function") {
          confirmPasswordInput.select();
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

    if (actionKey === "connectWifi") {
      const passwordInput = form.elements["password"];
      if (passwordInput && !passwordInput.value) {
        setFieldError(form, "password", true);
        setMessage(form, "Please enter the network password.");
        focusField(form, "password");
        return false;
      }
    }

    if (actionKey === "addWifi") {
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
    
    console.log("=== FORM SUBMIT START ===");
    console.log("Form:", form);
    console.log("Action key:", actionKey);
    console.log("Form action:", form.action);
    console.log("Form method:", form.method);
    
    if (!actionKey) {
      console.warn("Form missing data-action attribute, allowing normal submit");
      return true; // Allow normal submit
    }

    console.log(`Validating form for action: ${actionKey}`);
    
    // Validate form before submit
    if (!validateForm(form, actionKey)) {
      console.log("Validation failed, preventing submit");
      event.preventDefault(); // Prevent submit if validation fails
      return false;
    }

    console.log("Validation passed");
    
    // For mobile browsers, try allowing natural form submission first
    // If the action and method are set properly, let browser handle it
    if (form.action && form.method && form.method.toLowerCase() === 'post') {
      console.log("Allowing natural form submission - browser will handle redirect");
      
      // Show loading state
      const submitButton = form.querySelector('button[type="submit"]');
      if (submitButton) {
        submitButton.disabled = true;
        submitButton.textContent = 'Processing...';
        console.log("Submit button disabled, showing 'Processing...'");
      }
      
      // Allow browser to handle the submission naturally
      return true; // Let the form submit normally
    }
    
    // Fallback: manual handling for forms without proper action/method
    console.log("Form missing action/method, handling manually");
    event.preventDefault();
    handleManualSubmit(form, actionKey);
    return false;
  }
  
  function handleManualSubmit(form, actionKey) {
    console.log("=== MANUAL SUBMIT START ===");
    
    // Show loading state
    const submitButton = form.querySelector('button[type="submit"]');
    const originalText = submitButton ? submitButton.textContent : '';
    if (submitButton) {
      submitButton.disabled = true;
      submitButton.textContent = 'Processing...';
      console.log("Submit button disabled, showing 'Processing...'");
    }
    
    // Create form data and submit via fetch
    const formData = new FormData(form);
    console.log("Form data created:", Array.from(formData.entries()));
    
    const targetAction = form.action || `/api/${actionKey}`;
    console.log(`Starting manual fetch request to: ${targetAction}`);
    
    fetch(targetAction, {
      method: 'POST',
      body: formData,
      credentials: 'include'
    })
    .then(response => {
      console.log("=== MANUAL FETCH RESPONSE ===");
      console.log(`Response status: ${response.status}`);
      console.log(`Response statusText: ${response.statusText}`);
      console.log(`Response redirected: ${response.redirected}`);
      console.log(`Final response URL: ${response.url}`);
      console.log(`Response headers:`, Object.fromEntries(response.headers.entries()));
      
      // Check if the response was redirected to the expected page
      if (response.redirected) {
        console.log("Browser followed redirect automatically");
        window.location.href = response.url;
        return;
      }
      
      if (response.ok) {
        console.log("Got OK response, navigating to expected page");
        const location = getExpectedRedirect(actionKey);
        console.log(`Success response, navigating to: ${location}`);
        window.location.href = location;
        return;
      }
      
      // Handle manual redirects
      if ([301, 302, 303, 307, 308].includes(response.status)) {
        const location = response.headers.get('Location');
        console.log(`Got ${response.status} redirect, Location header: ${location}`);
        
        if (location) {
          console.log(`Manually redirecting to: ${location}`);
          window.location.href = location;
          return;
        }
      }
      
      console.log("Unhandled response");
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    })
    .catch(error => {
      console.log("=== MANUAL FETCH ERROR ===");
      console.error('Manual form submission error:', error);
      setMessage(form, "Connection problem. Try again.");
      
      // Restore button state
      if (submitButton) {
        submitButton.disabled = false;
        submitButton.textContent = originalText;
        console.log("Submit button restored");
      }
    });
    
    console.log("=== MANUAL SUBMIT END ===");
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
    
    // Attach logoff button handler
    const logoffBtn = document.getElementById("logoff-btn");
    if (logoffBtn) {
      console.log("Attaching logoff button handler");
      logoffBtn.addEventListener("click", handleLogoff);
    }
  }

  function handleLogoff(event) {
    event.preventDefault();
    console.log("Logoff button clicked");
    
    const button = event.target;
    const originalText = button.textContent;
    button.disabled = true;
    button.textContent = "Logging off...";
    
    fetch('/api/logoff', {
      method: 'POST',
      credentials: 'include'
    })
    .then(response => {
      console.log(`Logoff response: ${response.status}`);
      if (response.redirected) {
        console.log("Browser followed redirect automatically");
        window.location.href = response.url;
        return;
      }
      
      if (response.ok || [301, 302, 303, 307, 308].includes(response.status)) {
        console.log("Logoff successful, redirecting to auth page");
        window.location.href = "/auth/";
        return;
      }
      
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    })
    .catch(error => {
      console.error('Logoff error:', error);
      // Restore button state on error
      button.disabled = false;
      button.textContent = originalText;
      alert("Error logging off. Please try again.");
    });
  }

  function ensureRequiredElements() {
    document.querySelectorAll("[data-message]").forEach((msg) => {
      if (!msg.textContent) msg.textContent = "";
    });
  }

  function applyInitialData() {
    const initialData = window.TAPGATE_INITIAL_DATA;
    if (!initialData || !initialData.ap_ssid) {
      console.log("No initial data from server");
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
      if (el.tagName === "INPUT") el.value = initialData.ap_ssid;
      else el.textContent = initialData.ap_ssid;
    });
  }

  // WiFi functionality
  function getSignalIcon(rssi) {
    // Standard WiFi RSSI levels for optimal user experience
    if (rssi >= -55) return '/assets/wifi0.svg';  // Excellent signal (-30 to -55 dBm)
    if (rssi >= -70) return '/assets/wifi1.svg';  // Good signal (-55 to -70 dBm)
    if (rssi >= -85) return '/assets/wifi2.svg';  // Fair signal (-70 to -85 dBm)
    return '/assets/wifi3.svg';                   // Poor signal (< -85 dBm)
  }

  function initializeWiFiPage() {
    console.log("Initializing WiFi page functionality");
    
    const addNetworkBtn = document.getElementById('add-network-btn');
    if (addNetworkBtn) {
      addNetworkBtn.addEventListener('click', () => {
        window.location.href = '/add-network/';
      });
    }

    // Load available networks
    loadAvailableNetworks();
    
    // Auto-refresh networks every 30 seconds
    setInterval(loadAvailableNetworks, 30000);
  }

  async function loadAvailableNetworks() {
    const scanStatus = document.getElementById('scan-status');
    const networksList = document.getElementById('networks-list');
    
    if (!networksList) return;

    try {
      // Show scanning status
      if (scanStatus) {
        scanStatus.style.display = 'block';
        scanStatus.textContent = 'Scanning...';
      }

      // Trigger scan first
      await fetch('/api/wifi/scan', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        }
      });

      // Wait a bit for scan to complete
      await new Promise(resolve => setTimeout(resolve, 3000));

      // Get networks
      const response = await fetch('/api/wifi/networks');
      if (!response.ok) throw new Error('Failed to load networks');
      
      const networks = await response.json();
      
      // Hide scanning status
      if (scanStatus) {
        scanStatus.style.display = 'none';
      }

      // Clear existing networks
      networksList.innerHTML = '';

      if (!networks || networks.length === 0) {
        networksList.innerHTML = '<div class="network-item">No networks found</div>';
        return;
      }

      // Sort networks by signal strength
      networks.sort((a, b) => b.rssi - a.rssi);

      // Create network items
      networks.forEach(network => {
        const networkItem = document.createElement('div');
        networkItem.className = 'network-item clickable';
        
        const icon = document.createElement('img');
        icon.className = 'wifi-icon';
        icon.src = getSignalIcon(network.rssi);
        icon.alt = 'WiFi Signal';

        const name = document.createElement('span');
        name.className = 'network-name';
        name.textContent = network.ssid;

        const lockIcon = document.createElement('span');
        lockIcon.className = 'lock-indicator';
        if (network.auth > 0) {
          lockIcon.innerHTML = '🔒';
        }

        networkItem.appendChild(icon);
        networkItem.appendChild(name);
        networkItem.appendChild(lockIcon);

        networkItem.addEventListener('click', () => {
          connectToNetwork(network.ssid);
        });

        networksList.appendChild(networkItem);
      });

    } catch (error) {
      console.error('Error loading networks:', error);
      if (scanStatus) {
        scanStatus.textContent = 'Error loading networks';
      }
    }
  }

  function connectToNetwork(ssid) {
    // Store the selected SSID for the connect page
    sessionStorage.setItem('selectedSSID', ssid);
    window.location.href = '/network-connect/';
  }

  function initializeNetworkConnectPage() {
    const title = document.getElementById('network-title');
    const ssidFromStorage = sessionStorage.getItem('selectedSSID');
    
    if (ssidFromStorage && title) {
      title.textContent = `Connect to ${ssidFromStorage}`;
    }

    // Add hidden SSID field to form
    const form = document.querySelector('form[data-action="connectWifi"]');
    if (form && ssidFromStorage) {
      const hiddenSSID = document.createElement('input');
      hiddenSSID.type = 'hidden';
      hiddenSSID.name = 'ssid';
      hiddenSSID.value = ssidFromStorage;
      form.appendChild(hiddenSSID);
    }
  }

  function initializeAddNetworkPage() {
    // Add form validation for add network
    const form = document.querySelector('form[data-action="addWifi"]');
    if (form) {
      form.addEventListener('submit', (e) => {
        const ssidField = form.elements['ssid'];
        if (!ssidField || !ssidField.value.trim()) {
          e.preventDefault();
          setMessage(form, 'Please enter a network name.');
          setFieldError(form, 'ssid', true);
          focusField(form, 'ssid');
          return false;
        }
      });
    }
  }

  function initializePage() {
    console.log("Initializing simple form handling (no AJAX)");
    ensureRequiredElements();
    attachFormHandlers();
    applyInitialData();

    // Initialize page-specific functionality
    const body = document.body;
    if (body.classList.contains('page-wifi')) {
      initializeWiFiPage();
    } else if (body.classList.contains('page-network-connect')) {
      initializeNetworkConnectPage();
    } else if (body.classList.contains('page-add-network')) {
      initializeAddNetworkPage();
    }
  }

  // Initialize when DOM is ready
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initializePage);
  } else {
    initializePage();
  }
})();