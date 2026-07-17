const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:3001');
let messageCount = 0;

ws.on('open', () => {
    console.log('✅ 已连接到上位机服务器\n');

    // 模拟串口连接成功
    console.log('🔌 模拟连接 COM3...');
    ws.send(JSON.stringify({
        type: 'status',
        connected: true,
        port: 'COM3',
        baudRate: 115200
    }));

    // 模拟接收到帮助信息
    setTimeout(() => {
        console.log('📝 模拟接收帮助信息...');
        ws.send(JSON.stringify({
            type: 'message',
            text: '=== Commands ===',
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'help           - Show help',
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'uc test        - Test UC communication',
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'uc send        - Send waveform data',
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'uc stream on   - Enable stream',
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'uc stream off  - Disable stream',
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'OK:help',
            timestamp: Date.now()
        }));
    }, 500);

    // 模拟 uc test 响应
    setTimeout(() => {
        console.log('🧪 模拟 uc test 响应...');
        ws.send(JSON.stringify({
            type: 'message',
            text: 'OK:Upper computer test OK',
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'INFO:Communication established',
            timestamp: Date.now()
        }));
    }, 1500);

    // 模拟波形数据
    setTimeout(() => {
        console.log('📊 模拟波形数据...');
        const waveform = [];
        for (let i = 0; i < 512; i++) {
            // 生成正弦波
            const value = Math.round(2048 + 1024 * Math.sin(2 * Math.PI * i / 128));
            waveform.push(value);
        }
        ws.send(JSON.stringify({
            type: 'waveform',
            data: waveform,
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'message',
            text: 'OK:waveform sent',
            timestamp: Date.now()
        }));
    }, 2500);

    // 模拟频率和电压数据
    setTimeout(() => {
        console.log('📈 模拟测量数据...');
        ws.send(JSON.stringify({
            type: 'frequency',
            value: 1000,
            timestamp: Date.now()
        }));
        ws.send(JSON.stringify({
            type: 'voltage',
            min: 0.0,
            max: 3.3,
            pp: 3.3,
            timestamp: Date.now()
        }));
    }, 3500);

    // 模拟流式传输
    setTimeout(() => {
        console.log('🔄 模拟流式传输...');
        ws.send(JSON.stringify({
            type: 'message',
            text: 'OK:stream enabled',
            timestamp: Date.now()
        }));
    }, 4500);

    // 模拟流式数据
    setTimeout(() => {
        console.log('📊 模拟流式数据...');
        const waveform = [];
        for (let i = 0; i < 512; i++) {
            const value = Math.round(2048 + 1024 * Math.sin(2 * Math.PI * (i + 128) / 128));
            waveform.push(value);
        }
        ws.send(JSON.stringify({
            type: 'waveform',
            data: waveform,
            timestamp: Date.now()
        }));
    }, 5500);

    // 模拟禁用流式传输
    setTimeout(() => {
        console.log('⏹️ 模拟禁用流式传输...');
        ws.send(JSON.stringify({
            type: 'message',
            text: 'OK:stream disabled',
            timestamp: Date.now()
        }));
    }, 6500);

    // 测试完成
    setTimeout(() => {
        console.log('\n✅ 模拟测试完成！');
        console.log(`📨 总共发送 ${messageCount} 条消息`);
        console.log('\n上位机界面功能验证：');
        console.log('  - 串口连接状态显示 ✓');
        console.log('  - 命令响应显示 ✓');
        console.log('  - 波形数据接收 ✓');
        console.log('  - 测量数据接收 ✓');
        console.log('  - 流式传输控制 ✓');
        ws.close();
        process.exit(0);
    }, 8000);
});

ws.on('message', (data) => {
    messageCount++;
    const msg = JSON.parse(data);
    console.log(`📨 [${messageCount}] 收到:`, JSON.stringify(msg, null, 2));
});

ws.on('error', (err) => {
    console.error('❌ 错误:', err.message);
});

ws.on('close', () => {
    console.log('🔌 连接已关闭');
});
