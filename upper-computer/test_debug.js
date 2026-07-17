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

    setTimeout(() => {
        console.log('\n🧪 发送 uc test 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'uc test' }));
    }, 2000);

    setTimeout(() => {
        console.log('\n📊 发送 uc send 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'uc send' }));
    }, 3000);

    setTimeout(() => {
        console.log('\n🔄 发送 uc stream on 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'uc stream on' }));
    }, 4000);

    setTimeout(() => {
        console.log('\n⏹️ 发送 uc stream off 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'uc stream off' }));
    }, 6000);

    setTimeout(() => {
        console.log('\n✅ 测试完成！');
        console.log(`📨 总共收到 ${messageCount} 条消息`);
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
