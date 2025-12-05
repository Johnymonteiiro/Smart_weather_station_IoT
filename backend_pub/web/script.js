// --- Variáveis Globais ---
let updateInterval = 5000; // Padrão 5s para combinar com o seletor
let isPaused = false;
let intervalId = null;
let lastSeq = 0; // controle para evitar duplicar logs
let mqttChart = null; // instância do gráfico
let lastCfg = null; // configurações atuais

// --- Inicialização ---
document.addEventListener('DOMContentLoaded', () => {
    // Ajusta o intervalo inicial conforme o seletor
    const select = document.getElementById('refresh-rate');
    if (select) updateInterval = parseInt(select.value);
    initTrafficChart();
    startDataLoop();
    // Tenta uma leitura imediata ao carregar
    fetchData();
    loadConfig();
});

// --- Navegação (SPA) ---
function navigate(pageId) {
    // Remove classe ativa de tudo
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.querySelectorAll('.nav-links li').forEach(l => l.classList.remove('active'));
    
    // Ativa o selecionado
    document.getElementById(pageId).classList.add('active');
    
    // Lógica visual da sidebar
    const navIndex = pageId === 'home' ? 0 : 1;
    document.querySelectorAll('.nav-links li')[navIndex].classList.add('active');

    // Fecha sidebar no mobile
    if(window.innerWidth <= 768) toggleSidebar();
}

function toggleSidebar() {
    document.getElementById('sidebar').classList.toggle('open');
}

// --- Gráfico de Tráfego MQTT ---
function initTrafficChart() {
    const ctx = document.getElementById('mqttChart');
    if (!ctx) return;
    const labels = Array.from({ length: 24 }, (_, i) => `${i}:00`);
    mqttChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels,
            datasets: [{
                label: 'Mensagens publicadas por hora',
                data: Array(24).fill(0),
                borderColor: '#00d284',
                backgroundColor: 'rgba(0, 210, 132, 0.15)',
                tension: 0.35,
                fill: true,
                pointRadius: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false }
            },
            scales: {
                x: {
                    grid: { color: 'rgba(255,255,255,0.06)' },
                    ticks: { color: '#8b8d96' }
                },
                y: {
                    beginAtZero: true,
                    grid: { color: 'rgba(255,255,255,0.06)' },
                    ticks: { color: '#8b8d96' }
                }
            }
        }
    });
}

function updateTrafficChart(logs, uptimeMs) {
    if (!mqttChart || !Array.isArray(logs)) return;
    const counts = Array(24).fill(0);
    const nowAbs = Date.now();
    const hasUptime = typeof uptimeMs === 'number' && uptimeMs > 0;

    for (const e of logs) {
        const isMqttMsg = (e.tag === 'MQTT' || e.tag === 'mqtt' || e.tag === 'Mqtt') &&
            (typeof e.msg === 'string') && /payload|publish|publicado/i.test(e.msg);
        if (!isMqttMsg) continue;

        let hourIdx = null;
        if (hasUptime && typeof e.ts_ms === 'number') {
            const eventAbs = nowAbs - (uptimeMs - e.ts_ms);
            hourIdx = new Date(eventAbs).getHours();
        } else {
            hourIdx = new Date().getHours();
        }
        counts[hourIdx] = (counts[hourIdx] || 0) + 1;
    }

    mqttChart.data.datasets[0].data = counts;
    mqttChart.update('none');
}

// --- Core Loop de Dados ---
function startDataLoop() {
    if (intervalId) clearInterval(intervalId);
    intervalId = setInterval(fetchData, updateInterval);
}

function togglePause() {
    isPaused = !isPaused;
    const icon = document.getElementById('pause-icon');
    icon.className = isPaused ? 'fa-solid fa-play' : 'fa-solid fa-pause';
}

function updateIntervalTime() {
    const select = document.getElementById('refresh-rate');
    updateInterval = parseInt(select.value);
    startDataLoop();
}

// --- Busca de Dados (Agora confiando 100% no Backend) ---
async function fetchData() {
    if (isPaused) return;

    try {
        // Atualiza cartões de status
        const respStatus = await fetch('/status');
        if (!respStatus.ok) throw new Error("Falha na API /status");
        const statusData = await respStatus.json();
        setConnectionStatus(statusData.wifi_connected);
        updateStatusCards(statusData);

        // Busca logs da aplicação
        const respLogs = await fetch('/logs');
        if (respLogs.ok) {
            const logs = await respLogs.json();
            appendLogRows(logs, statusData.uptime_ms);
            updateTrafficChart(logs, statusData.uptime_ms);
        }

    } catch (error) {
        // Se falhar, marcamos como desconectado e NÃO geramos dados falsos
        // pois a simulação agora deve vir exclusivamente do backend.
        console.warn("Aguardando conexão com ESP32...", error);
        setConnectionStatus(false);
    }
}

// --- Atualização da UI: apenas cartões de status ---
function updateStatusCards(data) {
    const now = new Date().toLocaleTimeString();
    document.getElementById('last-update').innerText = now;
    document.getElementById('status-wifi').innerText = data.wifi_connected ? 'Conectado' : 'Desconectado';
    document.getElementById('status-ip').innerText = data.ip || '-';
    document.getElementById('status-mqtt').innerText = data.mqtt_connected ? 'Conectado ao Broker' : 'Erro';
    document.getElementById('status-uptime').innerText = data.uptime || '-';

    // Badge de modo (AP/STA) e badge IP (DHCP/AP)
    const bw = document.getElementById('badge-wifi');
    if (bw) bw.innerText = data.mode || (data.wifi_connected ? 'STA' : 'AP');

    const bip = document.getElementById('badge-ip');
    if (bip) bip.innerText = (data.mode === 'AP' || (!data.wifi_connected && (data.ip === '192.168.4.1'))) ? 'AP' : 'DHCP';

    // Gateway
    const gwEl = document.getElementById('status-gateway');
    if (gwEl) gwEl.innerText = data.gw || '--';

    // RSSI -> % aproximada
    const sigEl = document.getElementById('wifi-signal');
    if (sigEl) {
        const rssi = typeof data.rssi === 'number' ? data.rssi : -127;
        const pct = Math.max(0, Math.min(100, Math.round((rssi + 90) * (100 / 60))));
        sigEl.innerText = (rssi <= -120) ? '--' : `${pct}% (${rssi} dBm)`;
    }

    // MQTT QoS e Broker usando configuração carregada
    const badgeMqtt = document.getElementById('badge-mqtt');
    if (badgeMqtt) badgeMqtt.innerText = `QoS ${lastCfg && typeof lastCfg.qos === 'number' ? lastCfg.qos : '--'}`;
    const brokerEl = document.getElementById('mqtt-broker');
    if (brokerEl) brokerEl.innerText = lastCfg && lastCfg.broker ? lastCfg.broker : '--';

    // Uptime badge (mantém Running), boot count não disponível -> --
    const bc = document.getElementById('boot-count');
    if (bc) bc.innerText = '--';
}

// Removido: updateCard e deltas (cards de métricas não são mais usados)

// Removido: addDataToChart e gráficos

// --- Lista de Logs da Aplicação (melhorada) ---
function appendLogRows(list, uptimeMs) {
    const ul = document.getElementById('app-log-body');
    if (!ul || !Array.isArray(list)) return;

    let appended = 0;
    const nowAbs = Date.now();
    const hasUptime = typeof uptimeMs === 'number' && uptimeMs > 0;

    for (const e of list) {
        if (typeof e.seq !== 'number') continue;
        if (e.seq <= lastSeq) continue; // já processado

        let tsStr;
        if (hasUptime && typeof e.ts_ms === 'number') {
            const eventAbs = nowAbs - (uptimeMs - e.ts_ms);
            tsStr = new Date(eventAbs).toLocaleTimeString();
        } else {
            tsStr = new Date().toLocaleTimeString();
        }

        const li = document.createElement('li');
        const lvl = e.level || 'INFO';
        li.className = `log-item ${lvl}`;
        li.innerHTML = `
            <div class="log-header">
                <span>${tsStr}</span>
                <span class="log-tag">${e.tag || '-'}</span>
            </div>
            <div class="log-msg">${e.msg || '-'}</div>
        `;
        ul.appendChild(li);
        if (e.seq > lastSeq) lastSeq = e.seq;
        appended++;
    }

    if (appended > 0) {
        const container = document.querySelector('.log-list-container');
        if (container) container.scrollTop = container.scrollHeight;
    }
}

// --- Conexão e Configuração ---
function setConnectionStatus(isConnected) {
    const led = document.getElementById('connection-led');
    const text = document.getElementById('connection-text');
    
    if (isConnected) {
        led.className = 'led connected';
        text.innerText = 'Conectado (ESP32)';
        text.style.color = 'var(--accent-color)';
    } else {
        led.className = 'led disconnected';
        text.innerText = 'Desconectado';
        text.style.color = 'var(--danger)';
    }
}

function saveConfig() {
    const btn = document.querySelector('.btn-primary');
    const originalText = btn.innerHTML;
    
    btn.innerHTML = '<i class="fa-solid fa-spinner fa-spin"></i> Salvando...';
    btn.disabled = true;

    const data = {
        ssid: document.getElementById('conf_ssid').value,
        pass: document.getElementById('conf_pass').value,
        broker: document.getElementById('conf_broker').value,
        port: document.getElementById('conf_port').value,
        topic: document.getElementById('conf_topic').value,
        qos: document.getElementById('conf_qos').value,
        user: document.getElementById('conf_user').value,
        pass_mqtt: document.getElementById('conf_pass_mqtt').value
    };

    // Envia para o ESP32
    fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    })
    .then(async response => {
        if(!response.ok) throw new Error('Erro ao salvar');
        // Aguarda o IP real do STA consultando /status
        alert('Configurações salvas. Conectando à sua rede...');
        const start = Date.now();
        const timeoutMs = 60000; // 60s de espera
        const poll = setInterval(async () => {
            try {
                const r = await fetch('/status');
                if (r.ok) {
                    const s = await r.json();
                    if (s.wifi_connected && s.ip && s.ip !== '0.0.0.0') {
                        clearInterval(poll);
                        window.location.href = `http://${s.ip}/`;
                    }
                }
            } catch (_) {}
            if (Date.now() - start > timeoutMs) {
                clearInterval(poll);
                alert('Não foi possível obter o IP da rede automaticamente. Tente acessar manualmente pelo IP do seu roteador.');
            }
        }, 2000);
    })
    .catch(err => {
        console.error(err);
        alert("Erro de comunicação.");
    })
    .finally(() => {
        btn.innerHTML = originalText;
        btn.disabled = false;
    });
}

// --- Carrega Configurações salvas e preenche o formulário ---
async function loadConfig() {
    try {
        const resp = await fetch('/api/config');
        if (!resp.ok) return;
        const cfg = await resp.json();
        lastCfg = cfg;
        // Preenche campos se existirem
        if (cfg.ssid !== undefined) document.getElementById('conf_ssid').value = cfg.ssid || '';
        if (cfg.pass !== undefined) document.getElementById('conf_pass').value = cfg.pass || '';
        if (cfg.broker !== undefined) document.getElementById('conf_broker').value = cfg.broker || '';
        if (cfg.port !== undefined) document.getElementById('conf_port').value = cfg.port;
        if (cfg.topic !== undefined) document.getElementById('conf_topic').value = cfg.topic || '';
        if (cfg.qos !== undefined) document.getElementById('conf_qos').value = cfg.qos;
        if (cfg.user !== undefined) document.getElementById('conf_user').value = cfg.user || '';
        if (cfg.pass_mqtt !== undefined) document.getElementById('conf_pass_mqtt').value = cfg.pass_mqtt || '';

        // Atualiza badges MQTT com base na config
        const badgeMqtt = document.getElementById('badge-mqtt');
        if (badgeMqtt) badgeMqtt.innerText = `QoS ${cfg.qos ?? '--'}`;
        const brokerEl = document.getElementById('mqtt-broker');
        if (brokerEl) brokerEl.innerText = cfg.broker || '--';
    } catch (e) {
        console.warn('Nao foi possivel carregar configuracoes:', e);
    }
}

// --- Limpa NVS via backend ---
function clearConfig() {
    if (!confirm('Tem certeza que deseja APAGAR todas as configuracoes?')) return;
    fetch('/api/config/clear', { method: 'POST' })
        .then(async resp => {
            if (!resp.ok) throw new Error('Falha ao limpar memoria');
            // Em AP, utilizamos sempre o IP padrão 192.168.4.1
            alert('Memória limpa. Redirecionando para o modo AP...');
            setTimeout(() => { window.location.href = 'http://192.168.4.1/'; }, 1500);
        })
        .catch(err => {
            console.error(err);
            alert('Erro de comunicacao.');
        });
}
