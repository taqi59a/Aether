/*
 * AETHER VEND OS v13.0 Dashboard Logic
 * Secure WebSockets MQTT Client & Simulated Hardware Renderer
 */

const BROKER_HOST = "broker.hivemq.com";
const BROKER_PORT = 8884; // Secure SSL port for WebSockets
let mqttClient = null;
let pairedDeviceId = "";
let helixAnimationId = null;
let helixFrame = 0;

// Initialize Dashboard on Page Load
document.addEventListener("DOMContentLoaded", () => {
    // Attempt to retrieve last paired Device ID from LocalStorage
    const savedId = localStorage.getItem("aether_paired_device_id");
    if (savedId) {
        document.getElementById("deviceIdInput").value = savedId;
        pairedDeviceId = savedId;
        console.log(`[SYSTEM] Loaded saved pairing: ${savedId}`);
    }
});

// MQTT Client Connection Routine
function connectMQTT() {
    const inputVal = document.getElementById("deviceIdInput").value.trim().toUpperCase();
    if (!inputVal) {
        alert("Please enter a valid Device ID (e.g., AETHER-B4CE)");
        return;
    }
    
    // Save Device ID
    pairedDeviceId = inputVal;
    localStorage.setItem("aether_paired_device_id", pairedDeviceId);

    // Update UI during connection
    const btnConnect = document.getElementById("btnConnect");
    btnConnect.innerText = "Connecting...";
    btnConnect.disabled = true;

    const brokerStatus = document.getElementById("brokerStatus");
    brokerStatus.className = "status-badge";
    brokerStatus.querySelector(".dot").className = "dot pulse-amber";
    brokerStatus.querySelector(".status-text").innerText = "Connecting...";

    // Generate unique client ID for this browser tab
    const clientId = "aether_web_" + Math.random().toString(16).substr(2, 8);
    
    // Create new client instance
    mqttClient = new Paho.MQTT.Client(BROKER_HOST, BROKER_PORT, clientId);

    // Bind event handlers
    mqttClient.onConnectionLost = onConnectionLost;
    mqttClient.onMessageArrived = onMessageArrived;

    const options = {
        useSSL: true,
        timeout: 3,
        keepAliveInterval: 30,
        onSuccess: onConnectSuccess,
        onFailure: onConnectFailure
    };

    try {
        mqttClient.connect(options);
    } catch(e) {
        console.error("[MQTT] Connect exception:", e);
        onConnectFailure(e);
    }
}

// MQTT Success Callback
function onConnectSuccess() {
    console.log("[MQTT] Connected to broker successfully.");
    
    const brokerStatus = document.getElementById("brokerStatus");
    brokerStatus.className = "status-badge";
    brokerStatus.querySelector(".dot").className = "dot pulse-green";
    brokerStatus.querySelector(".status-text").innerText = "Broker Linked";

    const btnConnect = document.getElementById("btnConnect");
    btnConnect.innerText = "Re-Link";
    btnConnect.disabled = false;

    // Subscribing to Machine Status Topic (updates from ESP32)
    const statusTopic = `aether/vending/${pairedDeviceId}/status`;
    mqttClient.subscribe(statusTopic);
    console.log(`[MQTT] Subscribed to topic: ${statusTopic}`);

    // Update screen simulator and display indicator
    document.getElementById("oledWifiIndicator").style.display = "block";
    document.getElementById("oledDeviceId").innerText = pairedDeviceId.substring(7) || "----";

    // Set a timeout to check if the machine is actually responding (online)
    // In case no status message is received within 3 seconds, mark it as offline.
    setTimeout(() => {
        const deviceStatus = document.getElementById("deviceStatus");
        if (deviceStatus.style.display === "none") {
            showMachineOffline();
        }
    }, 3000);
}

// MQTT Failure Callback
function onConnectFailure(responseObject) {
    console.error("[MQTT] Connection failed:", responseObject);
    
    const brokerStatus = document.getElementById("brokerStatus");
    brokerStatus.className = "status-badge";
    brokerStatus.querySelector(".dot").className = "dot pulse-red";
    brokerStatus.querySelector(".status-text").innerText = "Link Failed";

    const btnConnect = document.getElementById("btnConnect");
    btnConnect.innerText = "Link Gateway";
    btnConnect.disabled = false;
    
    disableControls();
    showMachineOffline();
}

// Connection Dropped Callback
function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.error("[MQTT] Connection lost:", responseObject.errorMessage);
    }
    
    const brokerStatus = document.getElementById("brokerStatus");
    brokerStatus.className = "status-badge";
    brokerStatus.querySelector(".dot").className = "dot pulse-red";
    brokerStatus.querySelector(".status-text").innerText = "Broker Offline";

    const btnConnect = document.getElementById("btnConnect");
    btnConnect.innerText = "Link Gateway";
    btnConnect.disabled = false;

    disableControls();
    showMachineOffline();
}

// Handle Incoming MQTT messages
function onMessageArrived(message) {
    console.log(`[MQTT] Message Recv: [${message.destinationName}] ${message.payloadString}`);
    
    try {
        const data = JSON.parse(message.payloadString);
        
        // Safety guard: ensure the incoming data corresponds to our paired device
        if (data.device !== pairedDeviceId) return;

        // 1. Process Online/Offline Telemetry
        const deviceStatus = document.getElementById("deviceStatus");
        deviceStatus.style.display = "flex";
        
        if (data.status === "online") {
            deviceStatus.querySelector(".dot").className = "dot pulse-green";
            deviceStatus.querySelector(".status-text").innerText = "Machine Online";
            
            // Enable dashboard controls if machine is idle/ready
            if (data.state === "READY" || data.state === "IDLE") {
                enableControls();
            } else {
                // If it is homing or vending, keep override inputs disabled to prevent user collisions
                disableControlInputsOnly();
            }
        } else {
            showMachineOffline();
            disableControls();
            updateOledSimulator("OFFLINE");
            return;
        }

        // 2. Update Diagnostics Data
        document.getElementById("dispVal").innerText = `${data.pos_mm} mm`;
        document.getElementById("ssidVal").innerText = data.wifi_ssid || "None";
        document.getElementById("stateVal").innerText = data.state.toUpperCase();
        
        // Update range slider manually without triggering a circular event loop
        const slider = document.getElementById("displacementSlider");
        slider.value = data.pos_mm;
        document.getElementById("overrideVal").innerText = `${data.pos_mm} mm`;

        // Update RSSI and signal bars
        if (data.wifi_rssi) {
            document.getElementById("rssiVal").innerText = `${data.wifi_rssi} dBm`;
            updateRssiBars(data.wifi_rssi);
        } else {
            document.getElementById("rssiVal").innerText = "-- dBm";
            document.getElementById("rssiBars").className = "rssi-strength-indicator";
        }

        // 3. Update Hardware Viewport Screen Emulation
        updateOledSimulator(data.state.toUpperCase());

    } catch (e) {
        console.error("[SYSTEM] Error parsing status payload:", e);
    }
}

// Send Command via MQTT Publisher
function sendMQTTCommand(payload) {
    if (!mqttClient || !mqttClient.isConnected()) {
        alert("Dashboard not connected to broker.");
        return;
    }
    const topic = `aether/vending/${pairedDeviceId}/cmd`;
    const message = new Paho.MQTT.Message(payload);
    message.destinationName = topic;
    mqttClient.send(message);
    console.log(`[MQTT] Published command to ${topic}: ${payload}`);
}

// User Action Trigger Functions
function sendVendSwipe() {
    sendMQTTCommand("vendswipe");
}

function sendHome() {
    sendMQTTCommand("home");
}

function sendStop() {
    sendMQTTCommand("stop");
}

function sendLinearMove(mm) {
    sendMQTTCommand(mm.toString());
}

function updateSliderLabel(val) {
    document.getElementById("overrideVal").innerText = `${val} mm`;
}

// UI Helpers
function enableControls() {
    document.getElementById("btnDispense").disabled = false;
    document.getElementById("btnHome").disabled = false;
    document.getElementById("btnStop").disabled = false;
    document.getElementById("displacementSlider").disabled = false;
}

function disableControlInputsOnly() {
    // Dispense is blocked, manual sliders are blocked, but emergency STOP remains enabled!
    document.getElementById("btnDispense").disabled = true;
    document.getElementById("btnHome").disabled = true;
    document.getElementById("btnStop").disabled = false;
    document.getElementById("displacementSlider").disabled = true;
}

function disableControls() {
    document.getElementById("btnDispense").disabled = true;
    document.getElementById("btnHome").disabled = true;
    document.getElementById("btnStop").disabled = true;
    document.getElementById("displacementSlider").disabled = true;
}

function showMachineOffline() {
    const deviceStatus = document.getElementById("deviceStatus");
    deviceStatus.style.display = "flex";
    deviceStatus.querySelector(".dot").className = "dot pulse-red";
    deviceStatus.querySelector(".status-text").innerText = "Machine Offline";
    
    document.getElementById("dispVal").innerText = "0 mm";
    document.getElementById("rssiVal").innerText = "-- dBm";
    document.getElementById("rssiBars").className = "rssi-strength-indicator";
    document.getElementById("ssidVal").innerText = "Disconnected";
    document.getElementById("stateVal").innerText = "OFFLINE";
    document.getElementById("oledWifiIndicator").style.display = "none";
}

function updateRssiBars(rssi) {
    const bars = document.getElementById("rssiBars");
    bars.className = "rssi-strength-indicator";
    
    if (rssi >= -60) {
        bars.classList.add("sig-4"); // Excellent
    } else if (rssi >= -70) {
        bars.classList.add("sig-3"); // Good
    } else if (rssi >= -85) {
        bars.classList.add("sig-2"); // Fair
    } else {
        bars.classList.add("sig-1"); // Weak
    }
}

// OLED Screen emulation content state switcher
function updateOledSimulator(state) {
    // Hide all viewports
    document.getElementById("screenBoot").style.display = "none";
    document.getElementById("screenReady").style.display = "none";
    document.getElementById("screenWait").style.display = "none";
    document.getElementById("screenVending").style.display = "none";
    document.getElementById("screenComplete").style.display = "none";
    
    // Stop any active helix canvas animation loop
    if (helixAnimationId) {
        cancelAnimationFrame(helixAnimationId);
        helixAnimationId = null;
    }

    const oledBg = document.querySelector(".oled-viewport");
    oledBg.style.boxShadow = "inset 0 0 10px rgba(0,0,0,0.8)";
    oledBg.style.border = "3px solid #1a2233";

    if (state === "BOOT") {
        document.getElementById("screenBoot").style.display = "block";
    } 
    else if (state === "READY" || state === "IDLE" || state === "STANDBY") {
        document.getElementById("screenReady").style.display = "block";
        document.getElementById("oledDeviceId").innerText = pairedDeviceId.substring(7);
    } 
    else if (state === "WAIT") {
        document.getElementById("screenWait").style.display = "flex";
        oledBg.style.border = "3px solid rgba(255, 193, 7, 0.4)";
    } 
    else if (state === "VENDING" || state === "EXTENDING" || state === "RETRACTING") {
        document.getElementById("screenVending").style.display = "block";
        oledBg.style.border = "3px solid rgba(0, 234, 135, 0.4)";
        // Start Canvas animation
        helixFrame = 0;
        drawHelix();
    } 
    else if (state === "COMPLETE") {
        document.getElementById("screenComplete").style.display = "block";
        oledBg.style.border = "3px solid rgba(0, 204, 255, 0.4)";
        oledBg.style.boxShadow = "0 0 15px rgba(0, 204, 255, 0.15) inset, inset 0 0 10px rgba(0,0,0,0.8)";
    }
    else if (state === "OFFLINE") {
        document.getElementById("screenWait").style.display = "flex";
        document.querySelector(".wait-flashing").innerText = "OFFLINE";
        oledBg.style.border = "3px solid rgba(255, 59, 48, 0.4)";
    }
}

// Retro pixel-art double-helix simulator canvas drawing logic
function drawHelix() {
    const canvas = document.getElementById("helixCanvas");
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    
    ctx.clearRect(0, 0, 128, 32);
    ctx.fillStyle = "#ffffff";
    ctx.strokeStyle = "rgba(255, 255, 255, 0.35)";
    ctx.lineWidth = 1;

    const t = helixFrame * 0.25;

    // Background particles stream
    for (let p = 0; p < 12; p++) {
        let px = (p * 12 + (helixFrame * 2)) % 128;
        let py = Math.round(16 + 7.0 * Math.sin((px + helixFrame * 4) * 0.06));
        ctx.fillStyle = "rgba(255, 255, 255, 0.25)";
        ctx.fillRect(px, py, 1, 1);
    }

    ctx.fillStyle = "#ffffff";
    
    // Draw helix elements
    for (let x = 8; x < 128 - 8; x += 5) {
        // Spindle envelope shape
        const envelope = Math.sin((x - 8) * Math.PI / 112);
        const amp = 14.0 * envelope;

        const y1 = Math.round(16 + amp * Math.sin(x * 0.15 + t));
        const y2 = Math.round(16 - amp * Math.sin(x * 0.15 + t));

        // Draw node linking line
        ctx.beginPath();
        ctx.moveTo(x, y1);
        ctx.lineTo(x, y2);
        ctx.stroke();

        // Draw endpoint boxes
        ctx.fillRect(x - 1, y1 - 1, 3, 3);
        ctx.fillRect(x - 1, y2 - 1, 3, 3);
    }

    helixFrame++;
    helixAnimationId = requestAnimationFrame(drawHelix);
}
