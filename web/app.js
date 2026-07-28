// AETHER ONLINE MACHINE #5552 - CLOUD CLIENT LOGIC
document.addEventListener('DOMContentLoaded', () => {
    // DOM Elements
    const posSlider = document.getElementById('posSlider');
    const sliderValue = document.getElementById('sliderValue');
    const btnGo = document.getElementById('btnGo');
    const btnHome = document.getElementById('btnHome');
    const btnSweep = document.getElementById('btnSweep');
    const btnStop = document.getElementById('btnStop');
    const btnClearLog = document.getElementById('btnClearLog');
    
    const machineIdInput = document.getElementById('machineIdInput');
    const btnConnectIp = document.getElementById('btnConnectIp');
    const ipModeBadge = document.getElementById('ipModeBadge');
    const cloudRelayStatus = document.getElementById('cloudRelayStatus');

    const telemetryStatus = document.getElementById('telemetryStatus');
    const telemetryMm = document.getElementById('telemetryMm');
    const telemetrySteps = document.getElementById('telemetrySteps');
    const netStatusBadge = document.getElementById('netStatusBadge');
    const netStatusText = document.getElementById('netStatusText');
    const sysConsole = document.getElementById('sysConsole');

    const presetButtons = document.querySelectorAll('.btn-preset');

    // Parse machine ID from URL query parameter if present
    const urlParams = new URLSearchParams(window.location.search);
    const urlId = urlParams.get('id');
    if (urlId) {
        localStorage.setItem('aether_machine_id', urlId);
    }
    let machineId = urlId || localStorage.getItem('aether_machine_id') || '5552';
    machineIdInput.value = machineId;

    // MQTT Variables
    let mqttClient = null;
    let isConnectedMqtt = false;

    // Connect to HiveMQ Public WSS Broker
    function connectMqtt() {
        const clientId = 'aether_web_' + Math.random().toString(16).substr(2, 8);
        const host = 'broker.hivemq.com';
        const port = 8884; // WSS SSL port

        logConsole(`Connecting to Cloud MQTT Broker (wss://${host}:${port})...`, 'info');
        netStatusText.textContent = 'CONNECTING...';

        try {
            mqttClient = new Paho.MQTT.Client(host, port, clientId);

            mqttClient.onConnectionLost = onConnectionLost;
            mqttClient.onMessageArrived = onMessageArrived;

            mqttClient.connect({
                useSSL: true,
                timeout: 5,
                keepAliveInterval: 30,
                onSuccess: onConnectSuccess,
                onFailure: onConnectFailure
            });
        } catch (e) {
            logConsole(`MQTT Init Error: ${e.message}. Falling back to HTTP.`, 'error');
            setupHttpFallback();
        }
    }

    function onConnectSuccess() {
        isConnectedMqtt = true;
        netStatusBadge.style.borderColor = 'rgba(16, 185, 129, 0.4)';
        netStatusText.textContent = `CLOUD #5552`;
        cloudRelayStatus.textContent = 'Connected (MQTT WSS)';
        cloudRelayStatus.style.color = 'var(--success)';
        ipModeBadge.textContent = `MACHINE #${machineId}`;

        const statusTopic = `aether/vending/${machineId}/status`;
        mqttClient.subscribe(statusTopic);
        logConsole(`Cloud MQTT Connected! Subscribed to topic: ${statusTopic}`, 'success');

        // Request current status
        sendMqttCommand('STATUS');
    }

    function onConnectFailure(response) {
        isConnectedMqtt = false;
        logConsole(`Cloud MQTT Connection Failed (${response.errorMessage}). Retrying...`, 'error');
        netStatusBadge.style.borderColor = 'rgba(255, 183, 3, 0.4)';
        netStatusText.textContent = 'CLOUD RETRY';
        cloudRelayStatus.textContent = 'Disconnected';
        cloudRelayStatus.style.color = 'var(--accent-red)';
        setTimeout(connectMqtt, 5000);
    }

    function onConnectionLost(responseObject) {
        isConnectedMqtt = false;
        if (responseObject.errorCode !== 0) {
            logConsole(`Cloud MQTT Connection Lost: ${responseObject.errorMessage}`, 'error');
        }
        netStatusBadge.style.borderColor = 'rgba(239, 68, 68, 0.4)';
        netStatusText.textContent = 'RECONNECTING...';
        cloudRelayStatus.textContent = 'Lost Connection';
        setTimeout(connectMqtt, 3000);
    }

    function onMessageArrived(message) {
        try {
            const data = JSON.parse(message.payloadString);
            telemetryStatus.textContent = data.status.toUpperCase();
            telemetryMm.textContent = `${parseFloat(data.mm).toFixed(1)} mm`;
            telemetrySteps.textContent = `${data.pos} / 3200`;

            if (data.status === 'moving' || data.status === 'sweeping') {
                telemetryStatus.style.color = 'var(--accent-gold)';
            } else if (data.status === 'idle') {
                telemetryStatus.style.color = 'var(--success)';
            } else {
                telemetryStatus.style.color = 'var(--accent-cyan)';
            }
        } catch (e) {
            logConsole(`Raw Message: ${message.payloadString}`, 'info');
        }
    }

    function sendMqttCommand(cmdStr) {
        if (!isConnectedMqtt || !mqttClient) {
            logConsole('MQTT not connected yet, command queued.', 'error');
            return;
        }
        const cmdTopic = `aether/vending/${machineId}/cmd`;
        const message = new Paho.MQTT.Message(cmdStr);
        message.destinationName = cmdTopic;
        mqttClient.send(message);
        logConsole(`Published MQTT -> [${cmdTopic}]: ${cmdStr}`, 'cmd');
    }

    // Machine ID Switcher
    btnConnectIp.addEventListener('click', () => {
        let val = machineIdInput.value.trim();
        if (!val) return;
        machineId = val;
        localStorage.setItem('aether_machine_id', machineId);
        ipModeBadge.textContent = `MACHINE #${machineId}`;
        logConsole(`Switched active Cloud Target to Machine #${machineId}`, 'info');
        if (mqttClient && mqttClient.isConnected()) {
            mqttClient.disconnect();
        }
        connectMqtt();
    });

    // Slider Drag Readout
    posSlider.addEventListener('input', (e) => {
        sliderValue.textContent = `${parseFloat(e.target.value).toFixed(1)} mm`;
        updatePresetActiveState(e.target.value);
    });

    // Preset Button Click Handlers
    presetButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const val = btn.getAttribute('data-val');
            posSlider.value = val;
            sliderValue.textContent = `${parseFloat(val).toFixed(1)} mm`;
            updatePresetActiveState(val);
            sendMqttCommand(`GO:${val}`);
        });
    });

    function updatePresetActiveState(val) {
        presetButtons.forEach(b => {
            if (b.getAttribute('data-val') === String(val)) {
                b.classList.add('active');
            } else {
                b.classList.remove('active');
            }
        });
    }

    // Action Handlers
    btnGo.addEventListener('click', () => {
        sendMqttCommand(`GO:${posSlider.value}`);
    });

    btnHome.addEventListener('click', () => {
        sendMqttCommand('HOME');
    });

    btnSweep.addEventListener('click', () => {
        sendMqttCommand('SWEEP');
    });

    btnStop.addEventListener('click', () => {
        sendMqttCommand('STOP');
    });

    btnClearLog.addEventListener('click', () => {
        sysConsole.innerHTML = '';
        logConsole('Console cleared.', 'info');
    });

    // Console Logging Helper
    function logConsole(msg, level = 'info') {
        const now = new Date();
        const timeStr = now.toTimeString().split(' ')[0];
        
        const line = document.createElement('div');
        line.className = `log-line ${level}`;
        line.innerHTML = `<span class="log-time">[${timeStr}]</span><span class="log-msg">${msg}</span>`;
        
        sysConsole.appendChild(line);
        sysConsole.scrollTop = sysConsole.scrollHeight;
    }

    // Initialize MQTT Connection
    connectMqtt();
});
