// 全局变量
let ws = null;
let waveformChart = null;
let isPaused = false;
let waveformData = [];
const MAX_DATA_POINTS = 512;
const TIME_WINDOW = 512; // 显示512个点的时间窗口

// DOM 元素
const elements = {
    connectionStatus: document.getElementById('connectionStatus'),
    deviceInfo: document.getElementById('deviceInfo'),
    portSelect: document.getElementById('portSelect'),
    baudRate: document.getElementById('baudRate'),
    connectBtn: document.getElementById('connectBtn'),
    disconnectBtn: document.getElementById('disconnectBtn'),
    refreshPorts: document.getElementById('refreshPorts'),
    sampleRate: document.getElementById('sampleRate'),
    bufferSize: document.getElementById('bufferSize'),
    scopeStartBtn: document.getElementById('scopeStartBtn'),
    scopeStopBtn: document.getElementById('scopeStopBtn'),
    waveformType: document.getElementById('waveformType'),
    waveformFreq: document.getElementById('waveformFreq'),
    waveformAmp: document.getElementById('waveformAmp'),
    sigGenStartBtn: document.getElementById('sigGenStartBtn'),
    sigGenStopBtn: document.getElementById('sigGenStopBtn'),
    commandInput: document.getElementById('commandInput'),
    sendCmdBtn: document.getElementById('sendCmdBtn'),
    clearWaveform: document.getElementById('clearWaveform'),
    pauseWaveform: document.getElementById('pauseWaveform'),
    sendWaveformBtn: document.getElementById('sendWaveformBtn'),
    streamOnBtn: document.getElementById('streamOnBtn'),
    streamOffBtn: document.getElementById('streamOffBtn'),
    autoScale: document.getElementById('autoScale'),
    clearLog: document.getElementById('clearLog'),
    logOutput: document.getElementById('logOutput'),
    freqValue: document.getElementById('freqValue'),
    ppValue: document.getElementById('ppValue'),
    minValue: document.getElementById('minValue'),
    maxValue: document.getElementById('maxValue'),
    sampleRateValue: document.getElementById('sampleRateValue'),
    bufferSizeValue: document.getElementById('bufferSizeValue')
};

// 初始化
document.addEventListener('DOMContentLoaded', () => {
    initChart();
    initWebSocket();
    initEventListeners();
    addLog('系统启动', 'info');
});

// 初始化图表（示波器风格）
function initChart() {
    const ctx = document.getElementById('waveformChart').getContext('2d');

    // 创建网格插件
    const gridPlugin = {
        id: 'oscilloscopeGrid',
        beforeDraw: (chart) => {
            const { ctx, chartArea: { left, right, top, bottom } } = chart;
            const width = right - left;
            const height = bottom - top;

            ctx.save();
            ctx.strokeStyle = '#1a3a1a';
            ctx.lineWidth = 1;

            // 绘制垂直网格线（10格）
            const xGridLines = 10;
            for (let i = 0; i <= xGridLines; i++) {
                const x = left + (width * i / xGridLines);
                ctx.beginPath();
                ctx.moveTo(x, top);
                ctx.lineTo(x, bottom);
                ctx.stroke();
            }

            // 绘制水平网格线（8格）
            const yGridLines = 8;
            for (let i = 0; i <= yGridLines; i++) {
                const y = top + (height * i / yGridLines);
                ctx.beginPath();
                ctx.moveTo(left, y);
                ctx.lineTo(right, y);
                ctx.stroke();
            }

            // 绘制中心十字线
            ctx.strokeStyle = '#2a5a2a';
            ctx.lineWidth = 2;
            const centerX = left + width / 2;
            const centerY = top + height / 2;

            ctx.beginPath();
            ctx.moveTo(centerX, top);
            ctx.lineTo(centerX, bottom);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(left, centerY);
            ctx.lineTo(right, centerY);
            ctx.stroke();

            ctx.restore();
        }
    };

    // 注册插件
    Chart.register(gridPlugin);

    waveformChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: '波形',
                data: [],
                borderColor: '#00ff00',
                backgroundColor: 'rgba(0, 255, 0, 0.05)',
                borderWidth: 2,
                pointRadius: 0,
                fill: false,
                tension: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: {
                duration: 0
            },
            scales: {
                x: {
                    display: true,
                    title: {
                        display: true,
                        text: '时间 (μs)',
                        color: '#00ff00',
                        font: {
                            family: 'Consolas',
                            size: 12
                        }
                    },
                    ticks: {
                        color: '#00ff00',
                        maxTicksLimit: 10,
                        maxRotation: 0,
                        font: {
                            family: 'Consolas',
                            size: 10
                        }
                    },
                    grid: {
                        color: 'rgba(0, 255, 0, 0.1)',
                        lineWidth: 1
                    }
                },
                y: {
                    display: true,
                    title: {
                        display: true,
                        text: '电压 (V)',
                        color: '#00ff00',
                        font: {
                            family: 'Consolas',
                            size: 12
                        }
                    },
                    ticks: {
                        color: '#00ff00',
                        callback: function(value) {
                            return (value * 3.3 / 4095).toFixed(1) + 'V';
                        },
                        font: {
                            family: 'Consolas',
                            size: 10
                        }
                    },
                    grid: {
                        color: 'rgba(0, 255, 0, 0.1)',
                        lineWidth: 1
                    },
                    min: 0,
                    max: 4095
                }
            },
            plugins: {
                legend: {
                    display: false
                },
                tooltip: {
                    enabled: true,
                    backgroundColor: 'rgba(0, 0, 0, 0.8)',
                    titleColor: '#00ff00',
                    bodyColor: '#00ff00',
                    borderColor: '#00ff00',
                    borderWidth: 1,
                    callbacks: {
                        title: function(context) {
                            return '时间: ' + context[0].label;
                        },
                        label: function(context) {
                            const voltage = (context.parsed.y * 3.3 / 4095).toFixed(3);
                            return '电压: ' + voltage + 'V (' + context.parsed.y + ')';
                        }
                    }
                }
            }
        }
    });
}

// 初始化 WebSocket
function initWebSocket() {
    const wsUrl = `ws://${window.location.hostname}:3001`;
    ws = new WebSocket(wsUrl);

    ws.onopen = () => {
        addLog('WebSocket 已连接', 'success');
        // 请求串口列表
        listPorts();
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        handleServerMessage(data);
    };

    ws.onclose = () => {
        addLog('WebSocket 已断开', 'warning');
        setTimeout(initWebSocket, 3000);
    };

    ws.onerror = (error) => {
        addLog('WebSocket 错误', 'error');
    };
}

// 处理服务器消息
function handleServerMessage(data) {
    switch (data.type) {
        case 'ports':
            updatePortList(data.ports);
            break;
        case 'status':
            updateConnectionStatus(data);
            break;
        case 'waveform':
            if (!isPaused) {
                updateWaveform(data.data);
            }
            break;
        case 'frequency':
            updateFrequency(data.value);
            break;
        case 'voltage':
            updateVoltage(data);
            break;
        case 'deviceStatus':
            updateDeviceStatus(data);
            break;
        case 'message':
            addLog(data.text, 'info');
            break;
        case 'error':
            addLog(data.message, 'error');
            break;
    }
}

// 更新串口列表
function updatePortList(ports) {
    elements.portSelect.innerHTML = '<option value="">选择串口...</option>';
    ports.forEach(port => {
        const option = document.createElement('option');
        option.value = port.path;
        option.textContent = `${port.path} - ${port.manufacturer || '未知设备'}`;
        elements.portSelect.appendChild(option);
    });
    addLog(`发现 ${ports.length} 个串口`, 'info');
}

// 更新连接状态
function updateConnectionStatus(data) {
    if (data.connected) {
        elements.connectionStatus.textContent = '● 已连接';
        elements.connectionStatus.className = 'status connected';
        elements.deviceInfo.textContent = `${data.port} @ ${data.baudRate}`;
        elements.connectBtn.disabled = true;
        elements.disconnectBtn.disabled = false;
        addLog(`已连接到 ${data.port}`, 'success');
    } else {
        elements.connectionStatus.textContent = '● 未连接';
        elements.connectionStatus.className = 'status disconnected';
        elements.deviceInfo.textContent = '';
        elements.connectBtn.disabled = false;
        elements.disconnectBtn.disabled = true;
        addLog('已断开连接', 'warning');
    }
}

// 更新波形
function updateWaveform(samples) {
    // 限制数据点数
    if (samples.length > MAX_DATA_POINTS) {
        samples = samples.slice(0, MAX_DATA_POINTS);
    }

    // 添加新数据到缓冲区
    waveformData = waveformData.concat(samples);

    // 保持缓冲区大小
    if (waveformData.length > TIME_WINDOW) {
        waveformData = waveformData.slice(waveformData.length - TIME_WINDOW);
    }

    // 生成时间轴标签
    const timeLabels = [];
    for (let i = 0; i < waveformData.length; i++) {
        // 假设采样率1MHz，每个点1us
        timeLabels.push((i * 1).toFixed(0) + 'us');
    }

    // 更新图表数据
    waveformChart.data.labels = timeLabels;
    waveformChart.data.datasets[0].data = [...waveformData];

    // 自动缩放
    if (elements.autoScale.checked) {
        const min = Math.min(...waveformData);
        const max = Math.max(...waveformData);
        const padding = (max - min) * 0.1 || 100;
        waveformChart.options.scales.y.min = Math.max(0, min - padding);
        waveformChart.options.scales.y.max = Math.min(4095, max + padding);
    }

    waveformChart.update();

    // 更新测量值
    updateMeasurements(samples);
}

// 更新测量值
function updateMeasurements(samples) {
    const min = Math.min(...samples);
    const max = Math.max(...samples);
    const pp = max - min;

    elements.minValue.textContent = (min * 3.3 / 4095).toFixed(3) + ' V';
    elements.maxValue.textContent = (max * 3.3 / 4095).toFixed(3) + ' V';
    elements.ppValue.textContent = (pp * 3.3 / 4095).toFixed(3) + ' V';
}

// 更新频率显示
function updateFrequency(freq) {
    if (freq >= 1000000) {
        elements.freqValue.textContent = (freq / 1000000).toFixed(2) + ' MHz';
    } else if (freq >= 1000) {
        elements.freqValue.textContent = (freq / 1000).toFixed(2) + ' kHz';
    } else {
        elements.freqValue.textContent = freq.toFixed(2) + ' Hz';
    }
}

// 更新电压显示
function updateVoltage(data) {
    elements.minValue.textContent = data.min.toFixed(3) + ' V';
    elements.maxValue.textContent = data.max.toFixed(3) + ' V';
    elements.ppValue.textContent = data.pp.toFixed(3) + ' V';
}

// 更新设备状态
function updateDeviceStatus(data) {
    if (data.sample_rate) {
        elements.sampleRateValue.textContent = (data.sample_rate / 1000).toFixed(0) + ' kSa/s';
    }
    if (data.buffer_size) {
        elements.bufferSizeValue.textContent = data.buffer_size + ' samples';
    }
}

// 初始化事件监听
function initEventListeners() {
    // 串口连接
    elements.connectBtn.addEventListener('click', () => {
        const port = elements.portSelect.value;
        const baudRate = parseInt(elements.baudRate.value);
        if (port) {
            ws.send(JSON.stringify({
                type: 'connect',
                port: port,
                baudRate: baudRate
            }));
        } else {
            addLog('请选择串口', 'warning');
        }
    });

    elements.disconnectBtn.addEventListener('click', () => {
        ws.send(JSON.stringify({ type: 'disconnect' }));
    });

    elements.refreshPorts.addEventListener('click', listPorts);

    // 示波器控制
    elements.scopeStartBtn.addEventListener('click', () => {
        sendCommand('start_osc');
        elements.scopeStartBtn.disabled = true;
        elements.scopeStopBtn.disabled = false;
    });

    elements.scopeStopBtn.addEventListener('click', () => {
        sendCommand('stop_osc');
        elements.scopeStartBtn.disabled = false;
        elements.scopeStopBtn.disabled = true;
    });

    // 信号发生器控制
    elements.sigGenStartBtn.addEventListener('click', () => {
        const type = elements.waveformType.value;
        const freq = elements.waveformFreq.value;
        const amp = elements.waveformAmp.value;
        sendCommand(`set_wave ${type}`);
        sendCommand(`set_freq ${freq}`);
        sendCommand(`set_amp ${amp}`);
        sendCommand('start_gen');
        elements.sigGenStartBtn.disabled = true;
        elements.sigGenStopBtn.disabled = false;
    });

    elements.sigGenStopBtn.addEventListener('click', () => {
        sendCommand('stop_gen');
        elements.sigGenStartBtn.disabled = false;
        elements.sigGenStopBtn.disabled = true;
    });

    // 命令发送
    elements.sendCmdBtn.addEventListener('click', sendCommandFromInput);
    elements.commandInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            sendCommandFromInput();
        }
    });

    // 快捷命令
    document.querySelectorAll('.cmd-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            sendCommand(btn.dataset.cmd);
        });
    });

    // 波形控制
    elements.clearWaveform.addEventListener('click', () => {
        waveformData = [];
        waveformChart.data.labels = [];
        waveformChart.data.datasets[0].data = [];
        waveformChart.update();
    });

    elements.pauseWaveform.addEventListener('click', () => {
        isPaused = !isPaused;
        elements.pauseWaveform.textContent = isPaused ? '继续' : '暂停';
    });

    // 发送波形按钮
    elements.sendWaveformBtn.addEventListener('click', () => {
        sendCommand('uc send');
    });

    // 流式传输按钮
    elements.streamOnBtn.addEventListener('click', () => {
        sendCommand('uc stream on');
    });

    elements.streamOffBtn.addEventListener('click', () => {
        sendCommand('uc stream off');
    });

    elements.clearLog.addEventListener('click', () => {
        elements.logOutput.innerHTML = '';
    });
}

// 列出串口
function listPorts() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: 'listPorts' }));
    }
}

// 发送命令
function sendCommand(cmd) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
            type: 'command',
            cmd: cmd
        }));
        addLog(`> ${cmd}`, 'info');
    } else {
        addLog('未连接到服务器', 'error');
    }
}

// 从输入框发送命令
function sendCommandFromInput() {
    const cmd = elements.commandInput.value.trim();
    if (cmd) {
        sendCommand(cmd);
        elements.commandInput.value = '';
    }
}

// 添加日志
function addLog(message, type = 'info') {
    const timestamp = new Date().toLocaleTimeString();
    const logEntry = document.createElement('div');
    logEntry.className = `log-entry ${type}`;
    logEntry.innerHTML = `<span class="timestamp">[${timestamp}]</span>${message}`;
    elements.logOutput.appendChild(logEntry);
    elements.logOutput.scrollTop = elements.logOutput.scrollHeight;
}
