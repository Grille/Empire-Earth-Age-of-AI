// The language server's client: ageofai_check --lsp (dll/check/lsp.c) started for the .eeai files, the open files and
// their changes sent to it, its mistakes shown as they are typed (the Problems panel, the lines underlined). The
// Language Server Protocol by hand - a few messages, no packages: the extension stays files only (pack_vscode.py).
// The checker: the setting eeai.checker, else ageofai_check.exe in a folder above the file (the game folder: a
// personality in <game>\ageofai\personalities\, the mod's sources in <game>\_aimod\), else the workspace's
// dll\ageofai_check.exe (sh build.sh makes it).
const vscode = require('vscode');
const cp = require('child_process');
const fs = require('fs');
const path = require('path');

let server = null, diags = null, out = null, restarts = 0;

function findChecker(doc) {
    const set = vscode.workspace.getConfiguration('eeai').get('checker');
    if (set) return fs.existsSync(set) ? set : null;
    for (let d = path.dirname(doc.uri.fsPath), i = 0; i < 8; i++) {
        const p = path.join(d, 'ageofai_check.exe');
        if (fs.existsSync(p)) return p;
        const up = path.dirname(d);
        if (up === d) break;
        d = up;
    }
    for (const w of vscode.workspace.workspaceFolders || []) {
        const p = path.join(w.uri.fsPath, 'dll', 'ageofai_check.exe');
        if (fs.existsSync(p)) return p;
    }
    return null;
}

class Server {
    constructor(exe) {
        this.exe = exe;
        this.buf = Buffer.alloc(0);
        this.next = 1;
        this.ready = false;
        this.queue = [];
        this.sent = new Set();   // the files told of (didOpen)
        this.proc = cp.spawn(exe, ['--lsp'], { stdio: ['pipe', 'pipe', 'pipe'], windowsHide: true });
        this.proc.stdout.on('data', (b) => this.read(b));
        this.proc.stderr.on('data', (b) => out.append(b.toString()));
        this.proc.on('exit', (code) => this.exited(code));
        this.proc.on('error', (e) => { out.appendLine('ageofai_check: ' + e.message); this.exited(-1); });
        out.appendLine('started: ' + exe + ' --lsp');
        this.write({ jsonrpc: '2.0', id: this.next++, method: 'initialize', params: { processId: process.pid, rootUri: null, capabilities: {} } });
    }
    write(m) {
        const body = Buffer.from(JSON.stringify(m), 'utf8');
        try { this.proc.stdin.write('Content-Length: ' + body.length + '\r\n\r\n'); this.proc.stdin.write(body); } catch (e) { }
    }
    send(m) { if (this.ready) this.write(m); else this.queue.push(m); }
    read(b) {
        this.buf = Buffer.concat([this.buf, b]);
        for (;;) {
            const end = this.buf.indexOf('\r\n\r\n');
            if (end < 0) return;
            const head = this.buf.slice(0, end).toString();
            const m = /Content-Length:\s*(\d+)/i.exec(head);
            const n = m ? parseInt(m[1], 10) : 0;
            if (this.buf.length < end + 4 + n) return;
            const body = this.buf.slice(end + 4, end + 4 + n).toString('utf8');
            this.buf = this.buf.slice(end + 4 + n);
            let msg;
            try { msg = JSON.parse(body); } catch (e) { continue; }
            this.handle(msg);
        }
    }
    handle(msg) {
        if (msg.id === 1 && msg.result) {   // initialize answered: the files open now, then what waited
            this.ready = true;
            this.write({ jsonrpc: '2.0', method: 'initialized', params: {} });
            for (const m of this.queue) this.write(m);
            this.queue = [];
            return;
        }
        if (msg.method === 'textDocument/publishDiagnostics') {
            const uri = vscode.Uri.parse(msg.params.uri);
            diags.set(uri, msg.params.diagnostics.map((d) => {
                const r = new vscode.Range(d.range.start.line, d.range.start.character, d.range.end.line, d.range.end.character);
                const x = new vscode.Diagnostic(r, d.message, vscode.DiagnosticSeverity.Error);
                x.source = d.source;
                return x;
            }));
        }
    }
    exited(code) {
        if (this.gone) return;
        this.gone = true;
        out.appendLine('ageofai_check --lsp ended (' + code + ')');
        if (server === this) {
            server = null;
            if (!this.stopping && restarts++ < 3) setTimeout(() => { for (const d of vscode.workspace.textDocuments) opened(d); }, 1000);
        }
    }
    stop() {
        this.stopping = true;
        this.write({ jsonrpc: '2.0', id: this.next++, method: 'shutdown' });
        this.write({ jsonrpc: '2.0', method: 'exit' });
        setTimeout(() => { try { this.proc.kill(); } catch (e) { } }, 1000);
    }
}

function isEeai(doc) { return doc.languageId === 'eeai' && doc.uri.scheme === 'file'; }
function opened(doc) {
    if (!isEeai(doc)) return;
    if (!server) {
        const exe = findChecker(doc);
        if (!exe) { out.appendLine('no ageofai_check.exe found above ' + doc.uri.fsPath + ' (setting eeai.checker): no checks while typing'); return; }
        server = new Server(exe);
        for (const d of vscode.workspace.textDocuments) if (isEeai(d) && d !== doc) open(d);
    }
    open(doc);
}
function open(doc) {   // once a server (VS Code tells of a file once; the activation goes through those open already)
    const u = doc.uri.toString();
    if (server.sent.has(u)) return;
    server.sent.add(u);
    server.send({ jsonrpc: '2.0', method: 'textDocument/didOpen', params: { textDocument: { uri: doc.uri.toString(), languageId: 'eeai', version: doc.version, text: doc.getText() } } });
}

function activate(context) {
    diags = vscode.languages.createDiagnosticCollection('eeai');
    out = vscode.window.createOutputChannel('eeai checks');
    context.subscriptions.push(diags, out,
        vscode.workspace.onDidOpenTextDocument(opened),
        vscode.workspace.onDidChangeTextDocument((e) => {
            if (!server || !isEeai(e.document)) return;
            server.send({ jsonrpc: '2.0', method: 'textDocument/didChange', params: { textDocument: { uri: e.document.uri.toString(), version: e.document.version }, contentChanges: [{ text: e.document.getText() }] } });
        }),
        vscode.workspace.onDidSaveTextDocument((doc) => {
            if (server && isEeai(doc)) server.send({ jsonrpc: '2.0', method: 'textDocument/didSave', params: { textDocument: { uri: doc.uri.toString() } } });
        }),
        vscode.workspace.onDidCloseTextDocument((doc) => {
            if (!server || !isEeai(doc)) return;
            server.sent.delete(doc.uri.toString());
            server.send({ jsonrpc: '2.0', method: 'textDocument/didClose', params: { textDocument: { uri: doc.uri.toString() } } });
        }),
        vscode.workspace.onDidChangeConfiguration((e) => {
            if (!e.affectsConfiguration('eeai.checker')) return;
            if (server) { server.stop(); server = null; }
            restarts = 0;
            for (const d of vscode.workspace.textDocuments) opened(d);
        }),
        { dispose: () => { if (server) server.stop(); } });
    for (const d of vscode.workspace.textDocuments) opened(d);
}
function deactivate() { if (server) server.stop(); }
module.exports = { activate, deactivate };
