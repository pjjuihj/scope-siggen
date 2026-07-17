const WebSocket = require('ws');

// 连接到 WebSocket 服务器
const ws = new WebSocket('ws://localhost:3001');

ws.on('open', () => {
    console.log('✅ 已连接到上位机服务器');

    // 测试 1: 列出串口
    console.log('\n📋 测试 1: 列出串口...');
    ws.send(JSON.stringify({ type: 'listPorts' }));

    // 测试 2: 连接串口
    setTimeout(() => {
        console.log('\n🔌 测试 2: 连接 COM3...');
        ws.send(JSON.stringify({
            type: 'connect',
            port: 'COM3',
            baudRate: 115200
        }));
    }, 1000);

    // 测试 3: 发送命令
    setTimeout(() => {
        console.log('\n📝 测试 3: 发送 help 命令...');
        ws.send(JSON.stringify({
            type: 'command',
            cmd: 'help'
        }));
    }, 2000);

    // 测试 4: 测试上位机通信
    setTimeout(() => {
        console.log('\n🧪 测试 4: 发送 uc test 命令...');
        ws.send(JSON.stringify({
            type: 'command',
            cmd: 'uc test'
        }));
    }, 3000);

    // 测试 5: 发送波形数据
    setTimeout(() => {
        console.log('\n📊 测试 5: 发送 uc send 命令...');
        ws.send(JSON.stringify({
            type: 'command',
            cmd: 'uc send'
        }));
    }, 4000);

    // 测试 6: 启用流式传输
    setTimeout(() => {
        console.log('\n🔄 测试 6: 启用流式传输...');
        ws.send(JSON.stringify({
            type: 'command',
            cmd: 'uc stream on'
        }));
    }, 5000);

    // 测试 7: 禁用流式传输
    setTimeout(() => {
        console.log('\n⏹️ 测试 7: 禁用流式传输...');
        ws.send(JSON.stringify({
            type: 'command',
            cmd: 'uc stream off'
        }));
    }, 8000);

    // 测试完成
    setTimeout(() => {
        console.log('\n✅ 测试完成！');
        ws.close();
        process.exit(0);
    }, 9000);
});

ws.on('message', (data) => {
    const msg = JSON.parse(data);
    console.log('📨 收到消息:', JSON.stringify(msg, null, 2));
});

ws.on('error', (err) => {
    console.error('❌ 错误:', err.message);
});

ws.on('close', () => {
    console.log('🔌 连接已关闭');
});
