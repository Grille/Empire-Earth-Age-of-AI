# A fake OpenAI chat server for the llm.c tests: logs each request, answers with something checkable.
# usage: python mockllm.py PORT LOGFILE [delay_s]
import json, sys, time, re
from http.server import BaseHTTPRequestHandler, HTTPServer

PORT, LOG = int(sys.argv[1]), sys.argv[2]
DELAY = float(sys.argv[3]) if len(sys.argv) > 3 else 0.3
n = 0
k = 0

class H(BaseHTTPRequestHandler):
    def do_POST(self):
        global n
        n += 1
        body = self.rfile.read(int(self.headers.get('Content-Length', 0)))
        req = json.loads(body.decode('utf-8'))
        with open(LOG, 'a', encoding='utf-8') as f:
            f.write(json.dumps(req, ensure_ascii=False) + '\n')
        last = req['messages'][-1]['content']
        m = re.search(r'The usual line would be: "(.*)" - keep', last)
        if m:
            # every other line with numbers drops them (validation test); markup to be cleaned
            global k
            fixed = m.group(1)
            if re.search(r'\d', fixed):
                k += 1
            text = re.sub(r'\d+', 'many', fixed) if k % 2 == 1 and re.search(r'\d', fixed) else fixed
            content = '<think>planning</think>*grins* „Mock: ' + text + ' — grüße \U0001F600“'
        else:
            content = 'Mock-Antwort (%d Nachrichten) auf: %s' % (len(req['messages']), last)
        time.sleep(DELAY)
        out = json.dumps({'choices': [{'index': 0, 'message': {'role': 'assistant', 'content': content}}]}).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(out)))
        self.end_headers()
        self.wfile.write(out)
    def log_message(self, *a): pass

HTTPServer(('127.0.0.1', PORT), H).serve_forever()
