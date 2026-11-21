#!/usr/bin/env python3
"""Back up UE 5.2 packaged saves and top up seaweed without changing other progress."""
from datetime import datetime
import json
from pathlib import Path
import shutil
import struct
import zlib

def fstring(value):
    encoded = value.encode('ascii') + b'\0'
    return struct.pack('<i', len(encoded)) + encoded

def read_string(data, offset):
    length = struct.unpack_from('<i', data, offset)[0]
    assert abs(length) < 1000000
    start = offset + 4
    end = start + abs(length) * (2 if length < 0 else 1)
    assert end <= len(data)
    return data[start:end].decode('utf-16-le' if length < 0 else 'ascii').rstrip('\0'), end

def property_tag(data, offset):
    name, cursor = read_string(data, offset)
    if name == 'None':
        return dict(name=name, offset=offset, end=cursor)
    kind, cursor = read_string(data, cursor)
    size_at = cursor
    size, index = struct.unpack_from('<ii', data, cursor)
    assert size >= 0 and index == 0
    cursor += 8
    if kind == 'StructProperty':
        _, cursor = read_string(data, cursor)
        cursor += 16
    elif kind in ('ArrayProperty', 'SetProperty', 'ByteProperty', 'EnumProperty'):
        _, cursor = read_string(data, cursor)
    elif kind == 'MapProperty':
        _, cursor = read_string(data, cursor)
        _, cursor = read_string(data, cursor)
    elif kind == 'BoolProperty':
        cursor += 1
    has_guid = data[cursor]
    assert has_guid in (0, 1)
    cursor += 1 + 16 * has_guid
    assert cursor + size <= len(data)
    return dict(name=name, kind=kind, offset=offset, size_at=size_at,
                size=size, payload=cursor, end=cursor + size)

def progress_fields(payload):
    marker = fstring('Progress') + fstring('StructProperty')
    offset = payload.find(marker)
    assert offset >= 0 and payload.find(marker, offset + 1) < 0
    root = property_tag(payload, offset)
    cursor, fields = root['payload'], []
    while cursor < root['end']:
        field = property_tag(payload, cursor)
        fields.append(field)
        cursor = field['end']
        if field['name'] == 'None':
            break
    assert cursor == root['end'] and fields[-1]['name'] == 'None'
    return root, fields

def top_up(original):
    assert original.startswith(b'GVAS')
    payload = original[:-4]
    assert zlib.crc32(payload) == struct.unpack('<I', original[-4:])[0]
    root, fields = progress_fields(payload)
    money = next((f for f in fields if f['name'] == 'Seaweed'), None)
    before = struct.unpack_from('<i', payload, money['payload'])[0] if money else 0
    after = max(before, 1000)
    updated = bytearray(payload)
    if money:
        assert money['kind'] == 'IntProperty' and money['size'] == 4
        struct.pack_into('<i', updated, money['payload'], after)
    else:
        tag = fstring('Seaweed') + fstring('IntProperty') + struct.pack('<iiBi', 4, 0, 0, after)
        updated[fields[-1]['offset']:fields[-1]['offset']] = tag
        struct.pack_into('<i', updated, root['size_at'], root['size'] + len(tag))
    _, checked = progress_fields(updated)
    money = next(f for f in checked if f['name'] == 'Seaweed')
    assert struct.unpack_from('<i', updated, money['payload'])[0] == after
    kept = lambda data, tags: [(f['name'], bytes(data[f['offset']:f['end']]))
                              for f in tags if f['name'] != 'Seaweed']
    assert kept(payload, fields) == kept(updated, checked)
    return bytes(updated) + struct.pack('<I', zlib.crc32(updated)), before, after

def main():
    project = Path(__file__).resolve().parents[1]
    source = Path.home() / 'Library/Application Support/Epic/HumanFishing/Saved/SaveGames'
    backup = project / 'Saved/WalkthroughBackups' / datetime.now().strftime('%Y%m%d-%H%M%S')
    prepared = [(p, *top_up(p.read_bytes())) for p in sorted(source.glob('HumanFishing_*.sav'))]
    assert prepared, 'No existing packaged saves found'
    backup.mkdir(parents=True, exist_ok=True)
    report = []
    for path, replacement, before, after in prepared:
        shutil.copy2(path, backup / path.name)
        temporary = path.with_suffix('.sav.new')
        temporary.write_bytes(replacement)
        temporary.replace(path)
        report.append(dict(file=str(path), before=before, after=after))
    result = dict(backup=str(backup), changes=report)
    (backup / 'changes.json').write_text(json.dumps(result, indent=2))
    (project / 'Saved/WalkthroughSaveReport.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
