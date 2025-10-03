(function () {
  let scanTimeout = null;
  let currentNetworks = [];

  // WiFi signal strength mapping
  function getWifiIcon(rssi) {
    if (rssi >= -50) return '/assets/wifi3.svg'; // Excellent
    if (rssi >= -60) return '/assets/wifi2.svg'; // Good
    if (rssi >= -70) return '/assets/wifi1.svg'; // Fair
    return '/assets/wifi0.svg'; // Poor
  }

  // Format security type
  function formatAuth(auth) {
    switch (auth) {
      case 0: return 'Open';
      case 1: return 'WEP';
      case 2: return 'WPA';
      case 3: return 'WPA2';
      case 4: return 'WPA/WPA2';
      default: return 'Secured';
    }
  }

  // Create network item HTML
  function createNetworkItem(network, isClickable = true) {
    const icon = getWifiIcon(network.rssi);
    const security = network.auth > 0 ? '🔒' : '';
    const clickableClass = isClickable ? 'clickable' : '';
    
    return `
      <div class="network-item ${clickableClass}" data-ssid="${network.ssid}" data-auth="${network.auth}">
        <img src="${icon}" alt="WiFi" class="wifi-icon">
        <div class="network-info">
          <span class="network-name">${network.ssid}</span>
          <span class="network-security">${security} ${formatAuth(network.auth)}</span>
        </div>
        <span class="network-signal">${network.rssi} dBm</span>
      </div>
    `;
  }

  // Handle network item click
  function handleNetworkClick(event) {
    const networkItem = event.target.closest('.network-item.clickable');
    if (!networkItem) return;

    const ssid = networkItem.getAttribute('data-ssid');
    const auth = parseInt(networkItem.getAttribute('data-auth'));
    
    if (auth === 0) {
      // Open network - connect directly
      connectToNetwork(ssid, '');
    } else {
      // Secured network - go to password page
      window.location.href = `/network_connect/?ssid=${encodeURIComponent(ssid)}`;
    }
  }

  // Connect to a network
  function connectToNetwork(ssid, password) {
    const formData = new FormData();
    formData.append('ssid', ssid);
    formData.append('password', password);

    fetch('/api/connect_network', {
      method: 'POST',
      body: formData,
      credentials: 'include'
    })
    .then(response => {
      if (response.ok || response.redirected) {
        window.location.href = '/wifi/';
      } else {
        throw new Error(`HTTP ${response.status}`);
      }
    })
    .catch(error => {
      console.error('Connection error:', error);
      alert('Failed to connect to network. Please try again.');
    });
  }

  // Start WiFi scan
  function startScan() {
    const scanStatus = document.getElementById('scan-status');
    const networksList = document.getElementById('networks-list');
    
    if (scanStatus) {
      scanStatus.textContent = 'Scanning...';
      scanStatus.style.display = 'block';
    }
    
    if (networksList) {
      networksList.style.display = 'none';
    }

    // Trigger scan on the device
    fetch('/api/scan_wifi', {
      method: 'POST',
      credentials: 'include'
    })
    .then(response => {
      if (response.ok) {
        // Wait a bit then fetch results
        scanTimeout = setTimeout(fetchScanResults, 3000);
      } else {
        throw new Error(`HTTP ${response.status}`);
      }
    })
    .catch(error => {
      console.error('Scan trigger error:', error);
      if (scanStatus) {
        scanStatus.textContent = 'Scan failed. Tap to retry.';
        scanStatus.style.cursor = 'pointer';
        scanStatus.onclick = startScan;
      }
    });
  }

  // Fetch scan results
  function fetchScanResults() {
    fetch('/api/wifi_networks', {
      method: 'GET',
      credentials: 'include'
    })
    .then(response => response.json())
    .then(data => {
      displayNetworks(data.networks || []);
    })
    .catch(error => {
      console.error('Fetch results error:', error);
      const scanStatus = document.getElementById('scan-status');
      if (scanStatus) {
        scanStatus.textContent = 'Failed to load networks. Tap to retry.';
        scanStatus.style.cursor = 'pointer';
        scanStatus.onclick = startScan;
      }
    });
  }

  // Display networks in the list
  function displayNetworks(networks) {
    const scanStatus = document.getElementById('scan-status');
    const networksList = document.getElementById('networks-list');
    
    if (!networksList) return;

    currentNetworks = networks;

    if (networks.length === 0) {
      if (scanStatus) {
        scanStatus.textContent = 'No networks found. Tap to scan again.';
        scanStatus.style.cursor = 'pointer';
        scanStatus.onclick = startScan;
      }
      return;
    }

    // Hide scan status
    if (scanStatus) {
      scanStatus.style.display = 'none';
    }

    // Sort networks by signal strength
    networks.sort((a, b) => b.rssi - a.rssi);

    // Generate HTML for all networks
    networksList.innerHTML = networks.map(network => createNetworkItem(network, true)).join('');
    networksList.style.display = 'block';

    // Attach click handlers
    networksList.addEventListener('click', handleNetworkClick);
  }

  // Load current network status
  function loadCurrentNetwork() {
    fetch('/api/current_network', {
      method: 'GET',
      credentials: 'include'
    })
    .then(response => response.json())
    .then(data => {
      const currentNetworkElement = document.getElementById('current-network-name');
      const currentIconElement = document.getElementById('current-wifi-icon');
      
      if (data.connected && data.ssid) {
        if (currentNetworkElement) {
          currentNetworkElement.textContent = data.ssid;
        }
        if (currentIconElement) {
          currentIconElement.src = getWifiIcon(data.rssi || -50);
        }
      } else {
        if (currentNetworkElement) {
          currentNetworkElement.textContent = 'Not connected';
        }
        if (currentIconElement) {
          currentIconElement.src = '/assets/wifi0.svg';
        }
      }
    })
    .catch(error => {
      console.error('Current network error:', error);
    });
  }

  // Initialize WiFi page
  function initWifiPage() {
    console.log('Initializing WiFi page');

    // Load current network status
    loadCurrentNetwork();

    // Start initial scan
    startScan();

    // Add network button handler
    const addNetworkBtn = document.getElementById('add-network-btn');
    if (addNetworkBtn) {
      addNetworkBtn.addEventListener('click', () => {
        window.location.href = '/add_network/';
      });
    }

    // Refresh scan every 30 seconds
    setInterval(startScan, 30000);
  }

  // Initialize network connect page
  function initNetworkConnectPage() {
    console.log('Initializing network connect page');
    
    // Get SSID from URL parameters
    const urlParams = new URLSearchParams(window.location.search);
    const ssid = urlParams.get('ssid');
    
    if (ssid) {
      const titleElement = document.getElementById('network-title');
      const ssidInput = document.getElementById('selected-ssid');
      
      if (titleElement) {
        titleElement.textContent = `Connect to "${ssid}"`;
      }
      
      if (ssidInput) {
        ssidInput.value = ssid;
      }
    }
  }

  // Initialize based on page type
  function initPage() {
    const body = document.body;
    
    if (body.classList.contains('page-wifi')) {
      initWifiPage();
    } else if (body.classList.contains('page-network-connect')) {
      initNetworkConnectPage();
    }
  }

  // Initialize when DOM is ready
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initPage);
  } else {
    initPage();
  }
})();