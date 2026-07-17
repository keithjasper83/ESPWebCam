/* ESPWebCam web interface – app.js
 * Uses the versioned /api/v1 endpoints.
 * Deprecated legacy aliases (/settings, /status) are no longer used here.
 */
'use strict';

const API = '/api/v1';

// --- Stream setup ---
function startStream() {
  const streamPort = parseInt(window.location.port || '80') + 1;
  const streamUrl = window.location.protocol + '//' +
    window.location.hostname + ':' + streamPort + '/stream';
  const img = document.getElementById('stream');
  img.onload = () => {
    document.getElementById('stream-overlay').classList.add('hidden');
  };
  img.onerror = () => {
    document.getElementById('stream-overlay').textContent = 'Stream disconnected – retrying…';
    document.getElementById('stream-overlay').classList.remove('hidden');
    setTimeout(startStream, 3000);
  };
  img.src = streamUrl + '?t=' + Date.now();
}

// --- Settings ---
const RANGES = {
  quality:    { min: 4,  max: 63 },
  brightness: { min: -2, max: 2  },
  contrast:   { min: -2, max: 2  },
  saturation: { min: -2, max: 2  },
};

function setStatus(msg, isErr) {
  const el = document.getElementById('status-text');
  el.textContent = msg;
  el.style.color = isErr ? '#FF3B30' : '#333';
}

async function loadSettings() {
  try {
    const r = await fetch(API + '/camera/settings');
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const d = await r.json();
    if (!d.ok) throw new Error(d.error?.message || 'API error');
    const s = d.data;
    document.getElementById('resolution').value  = String(s.frameSize);
    document.getElementById('quality').value     = s.jpegQuality;
    document.getElementById('brightness').value  = s.brightness;
    document.getElementById('contrast').value    = s.contrast;
    document.getElementById('saturation').value  = s.saturation;
    ['quality','brightness','contrast','saturation'].forEach(id => {
      document.getElementById(id + '-value').textContent = s[id === 'quality' ? 'jpegQuality' : id];
    });
    document.getElementById('quality-value').textContent = s.jpegQuality;
    setStatus('Settings loaded');
    // Update IP display
    const status = await (await fetch(API + '/status')).json();
    if (status.ok) {
      document.getElementById('ip-addr').textContent = status.data.ipAddress || '';
      const badge = document.getElementById('wifi-state');
      badge.textContent = '●';
      badge.className = status.data.wifiConnected ? 'badge badge-ok' : 'badge badge-err';
    }
  } catch (e) {
    setStatus('Error loading settings: ' + e.message, true);
  }
}

async function updateSettings() {
  const frameSize   = parseInt(document.getElementById('resolution').value);
  const jpegQuality = parseInt(document.getElementById('quality').value);
  const brightness  = parseInt(document.getElementById('brightness').value);
  const contrast    = parseInt(document.getElementById('contrast').value);
  const saturation  = parseInt(document.getElementById('saturation').value);

  try {
    const r = await fetch(API + '/camera/settings', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ frameSize, jpegQuality, brightness, contrast, saturation })
    });
    const d = await r.json();
    if (!d.ok) {
      setStatus('Error: ' + (d.error?.message || 'unknown'), true);
      return;
    }
    setStatus('Settings applied');
    setTimeout(startStream, 800);
  } catch (e) {
    setStatus('Network error: ' + e.message, true);
  }
}

// --- Range slider live labels ---
['quality','brightness','contrast','saturation'].forEach(id => {
  const el = document.getElementById(id);
  if (!el) return;
  el.addEventListener('input', function () {
    document.getElementById(id + '-value').textContent = this.value;
  });
});

// --- Doorbell polling ---
let _lastDoorbellSeq = 0;
async function pollDoorbell() {
  try {
    const r = await fetch(API + '/doorbell/status');
    if (!r.ok) return;
    const d = await r.json();
    if (d.ok && d.data.lastEvent) {
      const seq = d.data.lastEvent.sequence;
      if (seq && seq !== _lastDoorbellSeq) {
        _lastDoorbellSeq = seq;
        showDoorbellAlert();
      }
    }
  } catch (_) { /* ignore poll errors */ }
}

function showDoorbellAlert() {
  const card = document.getElementById('doorbell-card');
  card.style.display = 'flex';
  setTimeout(() => { card.style.display = 'none'; }, 6000);
}

// --- Init ---
window.addEventListener('DOMContentLoaded', () => {
  loadSettings();
  startStream();
  setInterval(pollDoorbell, 3000);
});
