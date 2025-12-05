// --- Variáveis Globais ---
let updateInterval = 2000;
let isPaused = false;
let intervalId = null;
let charts = {}; // Armazena instâncias dos gráficos
const MAX_DATA_POINTS = 30; // Mantém o gráfico leve

// Estado para cálculo de Delta (Variação)
let lastValues = { temp: 0, hum: 0, rain: 0 };

// --- Inicialização ---
document.addEventListener('DOMContentLoaded', () => {
    initCharts();
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

// --- Configuração do Chart.js ---
function initCharts() {
    const commonOptions = {
        responsive: true,
        maintainAspectRatio: false,
        animation: { duration: 800, easing: 'easeOutQuart' },
        plugins: { legend: { display: true } },
        scales: { x: { display: false }, y: { beginAtZero: false } },
        elements: { line: { tension: 0.4 } } // Curva suave
    };

    // Gráfico 1: Temperatura
    charts.temp = new Chart(document.getElementById('chartTemp'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Temperatura (°C)',
                borderColor: '#d63031',
                backgroundColor: 'rgba(214, 48, 49, 0.1)',
                fill: true,
                data: []
            }]
        },
        options: commonOptions
    });

    // Gráfico 2: Umidade
    charts.hum = new Chart(document.getElementById('chartHum'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Umidade (%)',
                borderColor: '#0984e3',
                backgroundColor: 'rgba(9, 132, 227, 0.1)',
                fill: true,
                data: []
            }]
        },
        options: commonOptions
    });

    // Gráfico 3: Chuva
    charts.rain = new Chart(document.getElementById('chartRain'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Chuva (%)',
                borderColor: '#6c5ce7',
                fill: false,
                data: []
            }]
        },
        options: commonOptions
    });

    // Gráfico 4: Combinado
    charts.combo = new Chart(document.getElementById('chartCombo'), {
        type: 'bar',
        data: {
            labels: [],
            datasets: [
                { label: 'Temp', data: [], backgroundColor: '#d63031', yAxisID: 'y' },
                { label: 'Hum', data: [], backgroundColor: '#0984e3', yAxisID: 'y1' }
            ]
        },
        options: {
            ...commonOptions,
            scales: {
                y: { type: 'linear', display: true, position: 'left' },
                y1: { type: 'linear', display: true, position: 'right', grid: { drawOnChartArea: false } }
            }
        }
    });
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
        // Faz a requisição ao ESP32
        const response = await fetch('/api/dados');
        
        if (!response.ok) throw new Error("Falha na API");
        
        const data = await response.json();
        
        // Se chegou aqui, a conexão está OK
        setConnectionStatus(true);
        updateDashboard(data);

    } catch (error) {
        // Se falhar, marcamos como desconectado e NÃO geramos dados falsos
        // pois a simulação agora deve vir exclusivamente do backend.
        console.warn("Aguardando conexão com ESP32...", error);
        setConnectionStatus(false);
    }
}

// --- Atualização da UI ---
function updateDashboard(data) {
    const now = new Date().toLocaleTimeString();
    document.getElementById('last-update').innerText = now;

    // Atualiza Valores Texto
    // Usa '0' ou '--' como fallback visual apenas se o campo vier nulo do JSON
    updateCard('temp', data.temp || 0, '°C');
    updateCard('hum', data.hum || 0, '%');
    updateCard('rain', data.rain || 0, '%'); 

    // Atualiza Gráficos
    addDataToChart(charts.temp, now, data.temp);
    addDataToChart(charts.hum, now, data.hum);
    addDataToChart(charts.rain, now, data.rain);
    
    // Combo Chart
    if (charts.combo.data.labels.length > MAX_DATA_POINTS) {
        charts.combo.data.labels.shift();
        charts.combo.data.datasets[0].data.shift();
        charts.combo.data.datasets[1].data.shift();
    }
    charts.combo.data.labels.push(now);
    charts.combo.data.datasets[0].data.push(data.temp);
    charts.combo.data.datasets[1].data.push(data.hum);
    charts.combo.update('none'); // 'none' para animação suave

    if (data.alerts) {
        setAlert('temp', data.alerts.temp);
        setAlert('rain', data.alerts.rain);
    }
}

function updateCard(type, value, unit) {
    const elVal = document.getElementById(`val-${type}`);
    const elDelta = document.getElementById(`delta-${type}`);
    
    // Atualiza valor principal
    elVal.innerText = `${value.toFixed(1)} ${unit}`;

    // Calcula e exibe Delta
    // Nota: lastValues inicia em 0, então o primeiro delta será igual ao valor lido.
    // Isso é normal. Nas próximas leituras ele se ajusta.
    const diff = value - lastValues[type];
    lastValues[type] = value; 

    if (Math.abs(diff) > 0.01) {
        const icon = diff > 0 ? '▲' : '▼';
        const colorClass = diff > 0 ? 'up' : 'down';
        elDelta.innerHTML = `<span class="${colorClass}">${icon} ${Math.abs(diff).toFixed(1)}</span>`;
    } else {
        elDelta.innerHTML = `<span style="color:#aaa">-</span>`;
    }
}

function addDataToChart(chart, label, data) {
    if (chart.data.labels.length > MAX_DATA_POINTS) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
    }
    chart.data.labels.push(label);
    // Validação simples para evitar plotar 'undefined'
    chart.data.datasets[0].data.push(data !== undefined ? data : null);
    chart.update('none');
}

function setAlert(type, level) {
    const el = document.getElementById(`alert-${type}`);
    if (!el) return;
    el.classList.remove('alert-ok','alert-warn','alert-danger');
    let text = '--';
    let cls = 'alert-ok';
    if (type === 'temp') {
        if (level === 'normal') { text = 'Temperatura Normal'; cls = 'alert-ok'; }
        else if (level === 'media') { text = 'Temperatura Média'; cls = 'alert-warn'; }
        else if (level === 'alta') { text = 'Temperatura Alta'; cls = 'alert-danger'; }
    } else if (type === 'rain') {
        if (level === 'sem_chuva') { text = 'Sem Chuva'; cls = 'alert-ok'; }
        else if (level === 'chuva_media') { text = 'Chuva Média'; cls = 'alert-warn'; }
        else if (level === 'chuva_forte') { text = 'Chuva Forte'; cls = 'alert-danger'; }
    }
    el.textContent = text;
    el.classList.add(cls);
}

// --- Conexão e Configuração ---
function setConnectionStatus(isConnected) {
    const led = document.getElementById('connection-led');
    const text = document.getElementById('connection-text');
    
    if (isConnected) {
        led.className = 'led connected';
        text.innerText = 'Conectado (ESP32)';
        text.style.color = 'var(--success)';
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
        const resp = await response.json();
        if (resp.next_url) {
            alert('Configurações salvas. Redirecionando para a sua rede...');
            setTimeout(() => { window.location.href = resp.next_url; }, 1500);
        } else {
            alert('Configurações salvas. O dispositivo vai reiniciar.');
            navigate('home');
        }
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
        // Preenche campos se existirem
        if (cfg.ssid !== undefined) document.getElementById('conf_ssid').value = cfg.ssid || '';
        if (cfg.pass !== undefined) document.getElementById('conf_pass').value = cfg.pass || '';
        if (cfg.broker !== undefined) document.getElementById('conf_broker').value = cfg.broker || '';
        if (cfg.port !== undefined) document.getElementById('conf_port').value = cfg.port;
        if (cfg.topic !== undefined) document.getElementById('conf_topic').value = cfg.topic || '';
        if (cfg.qos !== undefined) document.getElementById('conf_qos').value = cfg.qos;
        if (cfg.user !== undefined) document.getElementById('conf_user').value = cfg.user || '';
        if (cfg.pass_mqtt !== undefined) document.getElementById('conf_pass_mqtt').value = cfg.pass_mqtt || '';
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
            const data = await resp.json();
            alert('Memória limpa. Redirecionando para o modo AP...');
            if (data.ap_url) {
                setTimeout(() => { window.location.href = data.ap_url; }, 1500);
            }
        })
        .catch(err => {
            console.error(err);
            alert('Erro de comunicacao.');
        });
}
