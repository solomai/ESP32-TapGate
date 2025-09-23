const form = document.getElementById('ap-form');
const ssidInput = document.getElementById('ssid');
const passwordInput = document.getElementById('password');
const statusLabel = document.getElementById('status');
const cancelButton = document.getElementById('cancel');

function setStatus(message, isError = true) {
  statusLabel.textContent = message || '';
  statusLabel.classList.toggle('success', !isError);
}

function setFieldError(input, hasError) {
  input.classList.toggle('error', hasError);
  const label = input.closest('.field').querySelector('label');
  if (label) {
    label.classList.toggle('focused', input === document.activeElement);
  }
}

const API_ENDPOINT = '/api/v1/ap';

async function loadConfig() {
  try {
    const response = await fetch(API_ENDPOINT, { cache: 'no-store' });
    if (!response.ok) {
      throw new Error('Failed to load configuration');
    }
    const data = await response.json();
    ssidInput.value = data.ssid || '';
    passwordInput.value = data.password || '';
    if (!data.cancel) {
      cancelButton.classList.add('hidden');
    } else {
      cancelButton.classList.remove('hidden');
    }
    setStatus('');
  } catch (error) {
    setStatus(error.message || 'Unable to load configuration');
    cancelButton.classList.add('hidden');
  }
}

async function saveConfig(event) {
  event.preventDefault();
  setStatus('');
  setFieldError(ssidInput, false);
  setFieldError(passwordInput, false);

  const body = new URLSearchParams({
    ssid: ssidInput.value.trim(),
    password: passwordInput.value.trim(),
  });

  try {
    const response = await fetch(API_ENDPOINT, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: body.toString(),
    });

    const result = await response.json();

    if (!response.ok || result.status !== 'ok') {
      const message = result.message || 'Failed to save';
      setStatus(message);
      const lower = message.toLowerCase();
      if (lower.includes('ssid')) {
        setFieldError(ssidInput, true);
      } else {
        setFieldError(passwordInput, true);
      }
      return;
    }

    setStatus('Saved', false);
    const redirect = result.redirect || '/main.html';
    window.location.replace(redirect);
  } catch (error) {
    setStatus('Network error');
  }
}

function init() {
  form.addEventListener('submit', saveConfig);
  cancelButton.addEventListener('click', () => {
    window.location.replace('/main.html');
  });

  [ssidInput, passwordInput].forEach((input) => {
    input.addEventListener('focus', () => setFieldError(input, false));
    input.addEventListener('blur', () => setFieldError(input, false));
  });

  loadConfig();
}

document.addEventListener('DOMContentLoaded', init);
