const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:3001');
let messageCount = 0;
let waveformCount = 0;
let testResults = [];

ws.on('open', () => {
    console.log('✅ 已连接到上位机服务器\n');
    testResults.push({ name: 'WebSocket 连接', status: 'PASS' });

    // 测试 1: 列出串口
    console.log('📋 测试 1: 列出串口...');
    ws.send(JSON.stringify({ type: 'listPorts' }));
});

ws.on('message', (data) => {
    messageCount++;
    const msg = JSON.parse(data);

    // 处理串口列表
    if (msg.type === 'ports') {
        console.log('   发现 ' + msg.ports.length + ' 个串口');
        testResults.push({ name: '列出串口', status: 'PASS' });

        // 测试 2: 连接串口
        console.log('\n🔌 测试 2: 连接 COM3...');
        ws.send(JSON.stringify({ type: 'connect', port: 'COM3', baudRate: 115200 }));
    }

    // 处理连接状态
    if (msg.type === 'status') {
        if (msg.connected) {
            console.log('   串口连接成功');
            testResults.push({ name: '串口连接', status: 'PASS' });

            // 测试 3: 发送 help 命令
            console.log('\n📝 测试 3: 发送 help 命令...');
            ws.send(JSON.stringify({ type: 'command', cmd: 'help' }));
        }
    }

    // 处理 help 响应
    if (msg.type === 'message' && msg.text.includes('=== Commands ===')) {
        console.log('   收到帮助信息');
        testResults.push({ name: 'help 命令', status: 'PASS' });

        // 测试 4: 发送 uc test 命令
        console.log('\n🧪 测试 4: 发送 uc test 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'uc test' }));
    }

    // 处理 uc test 响应
    if (msg.type === 'message' && msg.text.includes('OK:Upper computer test OK')) {
        console.log('   收到测试响应');
        testResults.push({ name: 'uc test 命令', status: 'PASS' });

        // 测试 5: 发送 uc send 命令
        console.log('\n📊 测试 5: 发送 uc send 命令...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'uc send' }));
    }

    // 处理波形数据
    if (msg.type === 'waveform') {
        waveformCount++;
        if (waveformCount === 1) {
            console.log('   收到波形数据 (' + msg.data.length + ' 个点)');
            testResults.push({ name: 'uc send 命令', status: 'PASS' });

            // 测试 6: 启用流式传输
            console.log('\n🔄 测试 6: 启用流式传输...');
            ws.send(JSON.stringify({ type: 'command', cmd: 'uc stream on' }));
        }
    }

    // 处理流式传输响应
    if (msg.type === 'message' && msg.text.includes('OK:stream enabled')) {
        console.log('   流式传输已启用');
        testResults.push({ name: 'uc stream on 命令', status: 'PASS' });

        // 等待 2 秒收集流式数据
        console.log('   等待 2 秒收集流式数据...');
        setTimeout(() => {
            console.log('   收到 ' + waveformCount + ' 个波形数据包');

            // 测试 7: 禁用流式传输
            console.log('\n⏹️ 测试 7: 禁用流式传输...');
            ws.send(JSON.stringify({ type: 'command', cmd: 'uc stream off' }));
        }, 2000);
    }

    // 处理禁用流式传输响应
    if (msg.type === 'message' && msg.text.includes('OK:stream disabled')) {
        console.log('   流式传输已禁用');
        testResults.push({ name: 'uc stream off 命令', status: 'PASS' });

        // 测试 8: 启动示波器
        console.log('\n📈 测试 8: 启动示波器...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'start_osc' }));
    }

    // 处理示波器启动响应
    if (msg.type === 'message' && msg.text.includes('OK:oscilloscope started')) {
        console.log('   示波器已启动');
        testResults.push({ name: 'start_osc 命令', status: 'PASS' });

        // 测试 9: 停止示波器
        console.log('\n📉 测试 9: 停止示波器...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'stop_osc' }));
    }

    // 处理示波器停止响应
    if (msg.type === 'message' && msg.text.includes('OK:oscilloscope stopped')) {
        console.log('   示波器已停止');
        testResults.push({ name: 'stop_osc 命令', status: 'PASS' });

        // 测试 10: 启动信号发生器
        console.log('\n🔊 测试 10: 启动信号发生器...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'set_wave sine' }));
        ws.send(JSON.stringify({ type: 'command', cmd: 'set_freq 1000' }));
        ws.send(JSON.stringify({ type: 'command', cmd: 'set_amp 3300' }));
        ws.send(JSON.stringify({ type: 'command', cmd: 'start_gen' }));
    }

    // 处理信号发生器启动响应
    if (msg.type === 'message' && msg.text.includes('OK:generator started')) {
        console.log('   信号发生器已启动');
        testResults.push({ name: 'start_gen 命令', status: 'PASS' });

        // 测试 11: 停止信号发生器
        console.log('\n🔇 测试 11: 停止信号发生器...');
        ws.send(JSON.stringify({ type: 'command', cmd: 'stop_gen' }));
    }

    // 处理信号发生器停止响应
    if (msg.type === 'message' && msg.text.includes('OK:generator stopped')) {
        console.log('   信号发生器已停止');
        testResults.push({ name: 'stop_gen 命令', status: 'PASS' });

        // 输出测试结果
        console.log('\n' + '='.repeat(50));
        console.log('自动测试完成');
        console.log('='.repeat(50));
        console.log('\n测试结果:');
        testResults.forEach((result, index) => {
            console.log('  ' + (index + 1) + '. ' + result.name + ': ' + result.status);
        });
        console.log('\n总计: ' + testResults.length + ' 项测试');
        console.log('通过: ' + testResults.filter(r => r.status === 'PASS').length + ' 项');
        console.log('失败: ' + testResults.filter(r => r.status !== 'PASS').length + ' 项');
        console.log('\n波形数据包: ' + waveformCount + ' 个');

        ws.close();
        process.exit(0);
    }
});

ws.on('error', (err) => {
    console.error('❌ 错误:', err.message);
});

// 30 秒后超时
setTimeout(() => {
    console.log('\n' + '='.repeat(50));
    console.log('自动测试超时');
    console.log('='.repeat(50));
    console.log('\n测试结果:');
    testResults.forEach((result, index) => {
        console.log('  ' + (index + 1) + '. ' + result.name + ': ' + result.status);
    });
    console.log('\n总计: ' + testResults.length + ' 项测试');
    console.log('通过: ' + testResults.filter(r => r.status === 'PASS').length + ' 项');
    console.log('失败: ' + testResults.filter(r => r.status !== 'PASS').length + ' 项');

    ws.close();
    process.exit(0);
}, 30000);
