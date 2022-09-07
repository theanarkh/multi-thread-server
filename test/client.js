const http = require("http");
const child_process = require("child_process");
const assert = require('assert');

const child = child_process.spawn('./multi-thread-server');

function request() {
    let count = 0
    for (let i = 0; i < 50; i++) {
        http.get('http://127.0.0.1:8888', (res) => {
            console.log(res.statusCode)
            assert(res.statusCode === 200);
            count++;
            if (count === 50) {
                child.kill();
            }
        });
    }
}

child.on('spawn', async () => {
    await new Promise((resolve) => {
        setTimeout(resolve, 3000);
    })
    request();
});

child.on('exit', (code) => {
    // Have started
    if (code == 1) {
        request();
    }
});