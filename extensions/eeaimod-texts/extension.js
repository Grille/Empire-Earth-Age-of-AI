// The language server's client: ageofai_check --lsp (dll/check/lsp.c) started for the .eeai files, the open files and
// their changes sent to it, its mistakes shown as they are typed (the Problems panel, the lines underlined), its
// completion and hover asked at the cursor (keys, members, values, the file's names, with their docs). The
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
        this.waiting = new Map();   // the requests not answered yet, by id
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
    request(method, params) {   // its answer's result (null after 3 s without one)
        const id = this.next++;
        return new Promise((resolve) => {
            this.waiting.set(id, resolve);
            setTimeout(() => { if (this.waiting.delete(id)) resolve(null); }, 3000);
            this.send({ jsonrpc: '2.0', id, method, params });
        });
    }
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
        if (msg.id !== undefined && !msg.method && this.waiting.has(msg.id)) {   // an answer
            const resolve = this.waiting.get(msg.id);
            this.waiting.delete(msg.id);
            resolve(msg.result === undefined ? null : msg.result);
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

// completion and hover (check/lsp_know.c): asked of the server at the place; LSP's item kinds are VS Code's + 1
function at(doc, pos) { return { textDocument: { uri: doc.uri.toString() }, position: { line: pos.line, character: pos.character } }; }
const selector = { language: 'eeai', scheme: 'file' };
const completion = {
    provideCompletionItems(doc, pos) {
        if (!server) return null;
        return server.request('textDocument/completion', at(doc, pos)).then((r) => !r ? null : r.items.map((x) => {
            const it = new vscode.CompletionItem(x.label, (x.kind || 1) - 1);
            if (x.detail) it.detail = x.detail;
            if (x.documentation) it.documentation = new vscode.MarkdownString(x.documentation.value);
            if (x.insertText) it.insertText = x.insertTextFormat === 2 ? new vscode.SnippetString(x.insertText) : x.insertText;
            if (x.sortText) it.sortText = x.sortText + x.label;
            return it;
        }));
    },
};
const hover = {
    provideHover(doc, pos) {
        if (!server) return null;
        return server.request('textDocument/hover', at(doc, pos)).then((r) => r && r.contents ? new vscode.Hover(new vscode.MarkdownString(r.contents.value)) : null);
    },
};

function activate(context) {
    diags = vscode.languages.createDiagnosticCollection('eeai');
    out = vscode.window.createOutputChannel('eeai checks');
    context.subscriptions.push(diags, out,
        vscode.languages.registerCompletionItemProvider(selector, completion, '.'),
        vscode.languages.registerHoverProvider(selector, hover),
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
