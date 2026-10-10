# Minimal MCP server (stdio, newline-delimited JSON-RPC 2.0) exposing the running game's control pipe.
# Tools: ee_command (status, camera, speed, pause, resume, quit, peek, poke, ping) and ee_screenshot.
# Registered for Claude Code in _aimod/.mcp.json; no third-party packages needed.
import base64, json, os, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import eectl

HERE = os.path.dirname(os.path.abspath(__file__))
SCREEN = os.path.join(HERE, '..', 'work', 'screen.png')

TOOLS = [
    {
        'name': 'ee_command',
        'description': (
            'Send one command to the running Empire Earth (eeaimod control pipe). Commands: '
            'ping | status (game time, speed, pause, per-AI-player epoch/stock/workers/targets/demand) | '
            'camera (read position) | camera <x> <y> (center on map tile) | speed <s> (normal 2.5) | '
            'pause | resume | quit | peek <hexaddr> [len] | poke <hexaddr> <hexbytes>'),
        'inputSchema': {
            'type': 'object',
            'properties': {'command': {'type': 'string', 'description': 'e.g. "status" or "camera 40 40"'}},
            'required': ['command'],
        },
    },
    {
        'name': 'ee_screenshot',
        'description': 'Capture the game window (works while it is covered) and return it as a PNG image.',
        'inputSchema': {'type': 'object', 'properties': {}},
    },
]


def call_tool(name, args):
    if name == 'ee_command':
        return [{'type': 'text', 'text': eectl.send(args.get('command', 'status'))}], False
    if name == 'ee_screenshot':
        r = subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                            os.path.join(HERE, 'screenshot.ps1')], capture_output=True, text=True, timeout=30)
        if r.returncode != 0 or not os.path.exists(SCREEN):
            return [{'type': 'text', 'text': 'screenshot failed: ' + (r.stdout + r.stderr).strip()}], True
        with open(SCREEN, 'rb') as f:
            data = base64.b64encode(f.read()).decode()
        return [{'type': 'image', 'data': data, 'mimeType': 'image/png'}], False
    return [{'type': 'text', 'text': 'unknown tool ' + name}], True


def handle(msg):
    method, mid = msg.get('method'), msg.get('id')
    if method == 'initialize':
        return {'protocolVersion': msg.get('params', {}).get('protocolVersion', '2025-06-18'),
                'capabilities': {'tools': {}},
                'serverInfo': {'name': 'empire-earth', 'version': '0.1'}}
    if method == 'tools/list':
        return {'tools': TOOLS}
    if method == 'tools/call':
        p = msg.get('params', {})
        content, is_error = call_tool(p.get('name'), p.get('arguments') or {})
        return {'content': content, 'isError': is_error}
    if method == 'ping':
        return {}
    if mid is None:      # notification (e.g. notifications/initialized)
        return None
    raise LookupError(method)


def main():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        msg = json.loads(line)
        mid = msg.get('id')
        try:
            result = handle(msg)
            if mid is None:
                continue
            reply = {'jsonrpc': '2.0', 'id': mid, 'result': result}
        except LookupError as e:
            reply = {'jsonrpc': '2.0', 'id': mid, 'error': {'code': -32601, 'message': 'method not found: %s' % e}}
        except Exception as e:  # keep the server alive on tool errors
            reply = {'jsonrpc': '2.0', 'id': mid, 'error': {'code': -32603, 'message': repr(e)}}
        sys.stdout.write(json.dumps(reply) + '\n')
        sys.stdout.flush()


if __name__ == '__main__':
    main()
