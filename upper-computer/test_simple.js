const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:3001');
let messageCount = 0;

ws.on('open', () => {
    console.log('✅ 已连接到上位机服务器\n');

    // 连接串口
    console.log('🔌 连接 COM3...');
    ws.send(JSON.stringify({
        type: 'connect',
        port: 'COM3',
        baudRate: 115200
    }));

    // 等待连接建立
    setTimeout(() => {
        console.log('\n📝 发送 help 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'help' }));
    }, 1000);

    // 等待更长时间
    setTimeout(() => {
        console.log('\n📝 再次发送 help 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'help' }));
    }, 3000);

    setTimeout(() => {
        console.log('\n📝 发送 status 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'status' }));
    }, 5000);

    setTimeout(() => {
        console.log('\n📝 发送 version 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'version' }));
    }, 7000);

    setTimeout(() => {
        console.log('\n✅ 测试完成！');
        console.log(`📨 总共收到 ${messageCount} 条消息`);
        if (messageCount > 1) {
            console.log('\n✅ 上位机通信正常！');
        } else {
            console.log('\n⚠️ 未收到设备响应');
            console.log('可能原因：');
            console.log('  1. 设备未上电');
            console.log('  2. 串口连接错误（TX→RX, RX→TX）');
            console.log('  3. 设备固件未烧录');
            console.log('  4. 波特率不匹配');
        }
        ws.close();
        process.exit(0);
    }, 10000);
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
