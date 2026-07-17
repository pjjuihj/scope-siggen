const express = require('express');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const WebSocket = require('ws');
const path = require('path');

const app = express();
const PORT = 3000;
const WS_PORT = 3001;

// 静态文件服务
app.use(express.static(path.join(__dirname, 'public')));

// 串口配置
let serialPort = null;
let parser = null;
let wsClients = new Set();

// WebSocket 服务器
const wss = new WebSocket.Server({ port: WS_PORT });

wss.on('connection', (ws) => {
    console.log('客户端已连接');
    wsClients.add(ws);

    ws.on('message', (message) => {
        const data = JSON.parse(message);
        handleWebSocketMessage(data, ws);
    });

    ws.on('close', () => {
        wsClients.delete(ws);
        console.log('客户端已断开');
    });
});

// 广播消息给所有客户端
function broadcast(data) {
    const message = JSON.stringify(data);
    wsClients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(message);
        }
    });
}

// 处理 WebSocket 消息
function handleWebSocketMessage(data, ws) {
    switch (data.type) {
        case 'connect':
            connectSerial(data.port, data.baudRate || 115200);
            break;
        case 'disconnect':
            disconnectSerial();
            break;
        case 'command':
            sendCommand(data.cmd);
            break;
        case 'listPorts':
            listSerialPorts(ws);
            break;
    }
}

// 列出可用串口
async function listSerialPorts(ws) {
    try {
        const ports = await SerialPort.list();
        ws.send(JSON.stringify({
            type: 'ports',
            ports: ports.map(p => ({
                path: p.path,
                manufacturer: p.manufacturer,
                serialNumber: p.serialNumber,
                pnpId: p.pnpId
            }))
        }));
    } catch (err) {
        console.error('列出串口失败:', err);
    }
}

// 连接串口
function connectSerial(portPath, baudRate = 115200) {
    if (serialPort && serialPort.isOpen) {
        serialPort.close();
    }

    serialPort = new SerialPort({
        path: portPath,
        baudRate: baudRate,
        dataBits: 8,
        stopBits: 1,
        parity: 'none'
    });

    parser = serialPort.pipe(new ReadlineParser({ delimiter: '\r\n' }));

    serialPort.on('open', () => {
        console.log(`串口已打开: ${portPath} @ ${baudRate}`);
        broadcast({
            type: 'status',
            connected: true,
            port: portPath,
            baudRate: baudRate
        });
    });

    parser.on('data', (line) => {
        parseSerialData(line);
    });

    serialPort.on('error', (err) => {
        console.error('串口错误:', err);
        broadcast({ type: 'error', message: err.message });
    });

    serialPort.on('close', () => {
        console.log('串口已关闭');
        broadcast({ type: 'status', connected: false });
    });
}

// 解析串口数据
function parseSerialData(line) {
    // 只保留可打印ASCII字符
    const clean = line.replace(/[^\x20-\x7E]/g, '');

    // 从任意位置提取 WAVE: 开头的hex数据
    const waveIdx = clean.indexOf('WAVE:');
    if (waveIdx !== -1) {
        const afterWave = clean.substring(waveIdx + 5);
        // 提取连续hex字符
        const hexMatch = afterWave.match(/^([0-9A-Fa-f]+)/);
        if (hexMatch && hexMatch[1].length >= 4) {
            const hexData = hexMatch[1];
            const samples = [];
            for (let i = 0; i < hexData.length; i += 4) {
                const hex = hexData.substring(i, i + 4);
                if (hex.length === 4) {
                    samples.push(parseInt(hex, 16));
                }
            }
            if (samples.length > 0) {
                console.log('📊 波形: ' + samples.length + ' 点');
                broadcast({
                    type: 'waveform',
                    data: samples,
                    timestamp: Date.now()
                });
            }
        }
        return;
    }

    // 解析频率数据
    if (clean.includes('FREQ:')) {
        const freqMatch = clean.match(/FREQ:([\d.]+)/);
        if (freqMatch) {
            broadcast({
                type: 'frequency',
                value: parseFloat(freqMatch[1]),
                timestamp: Date.now()
            });
        }
        return;
    }

    // 解析电压数据
    if (clean.includes('VOLT:')) {
        const voltMatch = clean.match(/VOLT:([\d.]+),([\d.]+),([\d.]+)/);
        if (voltMatch) {
            broadcast({
                type: 'voltage',
                min: parseFloat(voltMatch[1]),
                max: parseFloat(voltMatch[2]),
                pp: parseFloat(voltMatch[3]),
                timestamp: Date.now()
            });
        }
        return;
    }

    // 解析状态数据
    if (clean.startsWith('STATUS:')) {
        try {
            const status = JSON.parse(clean.substring(7));
            broadcast({
                type: 'deviceStatus',
                ...status,
                timestamp: Date.now()
            });
        } catch (e) {
            broadcast({ type: 'message', text: clean, timestamp: Date.now() });
        }
        return;
    }

    // 普通消息（过滤掉太短或乱码的行）
    if (clean.length > 2 && clean.length < 200) {
        broadcast({ type: 'message', text: clean, timestamp: Date.now() });
    }
}

// 发送命令到串口
function sendCommand(cmd) {
    if (serialPort && serialPort.isOpen) {
        serialPort.write(cmd + '\r\n');
        console.log('发送命令:', cmd);
    } else {
        broadcast({ type: 'error', message: '串口未连接' });
    }
}

// 断开串口
function disconnectSerial() {
    if (serialPort && serialPort.isOpen) {
        serialPort.close();
    }
}

// 启动服务器
app.listen(PORT, () => {
    console.log(`上位机服务器运行在 http://localhost:${PORT}`);
    console.log(`WebSocket 服务器运行在 ws://localhost:${WS_PORT}`);
});
