"""Packs vscode/eeaimod-texts into work/eeaimod-texts.vsix (a VSIX is a zip: a manifest, the content types, the
extension's files under extension/) - no Node tools needed. Install it with
    code --install-extension work/eeaimod-texts.vsix
(or in VS Code: Extensions, "...", Install from VSIX). After changing the grammar's source names: first
scripts/gen_vscode_grammar.py, then this, then install again and reload the window."""
import json
import os
import zipfile

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ext = os.path.join(root, 'vscode', 'eeaimod-texts')
pkg = json.load(open(os.path.join(ext, 'package.json'), encoding='utf-8'))
esc = lambda s: s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')
manifest = '''<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011" xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Language="en-US" Id="{name}" Version="{version}" Publisher="{publisher}" />
    <DisplayName>{display}</DisplayName>
    <Description xml:space="preserve">{desc}</Description>
    <Categories>Programming Languages</Categories>
    <Properties>
      <Property Id="Microsoft.VisualStudio.Code.Engine" Value="{engine}" />
      <Property Id="Microsoft.VisualStudio.Code.ExtensionKind" Value="ui,workspace,web" />
    </Properties>
  </Metadata>
  <Installation><InstallationTarget Id="Microsoft.VisualStudio.Code"/></Installation>
  <Dependencies/>
  <Assets><Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true" /></Assets>
</PackageManifest>
'''.format(name=esc(pkg['name']), version=esc(pkg['version']), publisher=esc(pkg['publisher']),
           display=esc(pkg['displayName']), desc=esc(pkg['description']), engine=esc(pkg['engines']['vscode']))
types = ('<?xml version="1.0" encoding="utf-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
         '<Default Extension=".json" ContentType="application/json"/><Default Extension=".vsixmanifest" ContentType="text/xml"/>'
         '</Types>')
os.makedirs(os.path.join(root, 'work'), exist_ok=True)
out = os.path.join(root, 'work', pkg['name'] + '.vsix')
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
    z.writestr('extension.vsixmanifest', manifest)
    z.writestr('[Content_Types].xml', types)
    for d, _, files in os.walk(ext):
        for f in files:
            p = os.path.join(d, f)
            z.write(p, 'extension/' + os.path.relpath(p, ext).replace(os.sep, '/'))
print(out, [n for n in zipfile.ZipFile(out).namelist()])
