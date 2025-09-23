const versionLabel = document.getElementById('version');
const apButton = document.getElementById('ap-settings');

async function loadInfo() {
  try {
    const response = await fetch('/api/v1/main/info', { cache: 'no-store' });
    if (!response.ok) {
      throw new Error('Failed to load info');
    }
    const data = await response.json();
    versionLabel.textContent = data.version || 'TapGate ver: 1';
  } catch (error) {
    versionLabel.textContent = 'TapGate ver: 1';
  }
}

function init() {
  apButton.addEventListener('click', () => {
    window.location.replace('/ap.html');
  });
  loadInfo();
}

document.addEventListener('DOMContentLoaded', init);
