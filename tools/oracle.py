#!/usr/bin/env python3
"""Run original CONFLICT functions in an x86 emulator (Unicorn) as a reference for the port.

The EXE is loaded with relocations applied for base segment 0x1000, so code addresses match
Ghidra's (e.g. 1000:3138) and DGROUP is segment 0x228e. A caller supplies a 64 KB DGROUP image;
the function is called far with cdecl int arguments and the resulting DGROUP is returned.

Driver (segment 1f39) calls are stubbed as no-ops; BIOS/DOS interrupts are mostly refused, so a
logic function that unexpectedly does I/O fails loudly.
"""
import struct
import sys

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
                               UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
                               UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS,
                               UC_X86_REG_IP)

BASE_SEG = 0x1000
DGROUP = 0x228e
DRIVER_SEG = 0x1f39
STOP_SEG, STOP_OFF = 0x0050, 0x0000  # HLT stub used as the far return address
LONGJMP = (0x20ba, 0x002c)
STUB_SEG = 0x0060  # interrupt stubs: STUB_SEG:n*4 = int n; iret


class Oracle:
    def __init__(self, exe_path):
        d = open(exe_path, 'rb').read()
        hdr = struct.unpack('<14H', d[:28])
        header = hdr[4] * 16
        nrel, rel_off = hdr[3], hdr[12]
        self.image = bytearray(d[header:])
        for i in range(nrel):
            off, seg = struct.unpack_from('<HH', d, rel_off + 4 * i)
            at = seg * 16 + off
            v = struct.unpack_from('<H', self.image, at)[0]
            struct.pack_into('<H', self.image, at, (v + BASE_SEG) & 0xffff)
        self.out = []  # text written via int 21h AH=40h
        self.keys = []  # BIOS key codes returned by int 16h AH=0, then Enter

    def _new_uc(self, dgroup):
        uc = Uc(UC_ARCH_X86, UC_MODE_16)
        uc.mem_map(0, 0x100000)
        uc.mem_write(BASE_SEG * 16, bytes(self.image))
        uc.mem_write(DGROUP * 16, bytes(dgroup))
        uc.mem_write(STOP_SEG * 16 + STOP_OFF, b'\xf4')  # hlt
        # Interrupt handlers: int86() fetches a vector (int 21h AH=35h) and far-calls it, so every
        # vector points at a stub "int N; iret" whose int we then service in _on_interrupt.
        for n in range(256):
            uc.mem_write(STUB_SEG * 16 + n * 4, bytes([0xcd, n, 0xcf, 0x90]))
            uc.mem_write(n * 4, struct.pack('<HH', n * 4, STUB_SEG))
        uc.hook_add(UC_HOOK_INTR, self._on_interrupt)
        # longjmp(restart_jmpbuf, value) ends the run: record the value and stop.
        lj = LONGJMP[0] * 16 + LONGJMP[1]
        uc.hook_add(UC_HOOK_CODE, self._on_longjmp, begin=lj, end=lj)
        drv = DRIVER_SEG * 16
        uc.hook_add(UC_HOOK_CODE, self._on_driver, begin=drv, end=drv + 0xa72)
        return uc

    def _on_longjmp(self, uc, address, size, user):
        sp = uc.reg_read(UC_X86_REG_SP)
        ss = uc.reg_read(UC_X86_REG_SS)
        self.restart = struct.unpack('<h', uc.mem_read(ss * 16 + sp + 6, 2))[0]
        uc.emu_stop()

    def _on_driver(self, uc, address, size, user):
        # Driver entry: emulate `retf` without doing anything (logic tests never draw).
        sp = uc.reg_read(UC_X86_REG_SP)
        ss = uc.reg_read(UC_X86_REG_SS)
        ip, cs = struct.unpack('<HH', uc.mem_read(ss * 16 + sp, 4))
        uc.reg_write(UC_X86_REG_SP, sp + 4)
        uc.reg_write(UC_X86_REG_CS, cs)
        uc.reg_write(UC_X86_REG_IP, ip)
        uc.reg_write(UC_X86_REG_AX, 0)

    def _on_interrupt(self, uc, intno, user):
        ax = uc.reg_read(UC_X86_REG_AX)
        ah = ax >> 8
        if intno == 0x21 and ah == 0x40:  # write: capture (puts)
            ds = uc.reg_read(UC_X86_REG_DS)
            n = uc.reg_read(UC_X86_REG_CX)
            p = uc.reg_read(UC_X86_REG_DX)
            self.out.append(bytes(uc.mem_read(ds * 16 + p, n)))
            uc.reg_write(UC_X86_REG_AX, n)
            return
        if intno == 0x16 and ah in (0x00, 0x10):  # read key (get_key -> int86(0x16))
            uc.reg_write(UC_X86_REG_AX, self.keys.pop(0) if self.keys else 0x1c0d)
            return
        if intno == 0x10:  # video BIOS (text mode restore): ignore
            return
        if intno == 0x21 and ah == 0x35:  # get vector: point at our stub
            n = ax & 0xff
            uc.reg_write(UC_X86_REG_ES, STUB_SEG)
            uc.reg_write(UC_X86_REG_BX, n * 4)
            return
        if intno == 0x21 and ah == 0x44:  # ioctl (isatty): say "device"
            uc.reg_write(UC_X86_REG_DX, 0x80)
            return
        raise RuntimeError(f'unexpected int {intno:#x} AX={ax:#06x}')

    def call(self, dgroup, func, args=(), sp=0xfff0, max_insns=50_000_000, keys=()):
        """Call far `func` ("SSSS:OOOO") with int args. Returns (new_dgroup, ax)."""
        self.keys = list(keys)
        self.restart = None
        seg, off = (int(x, 16) for x in func.split(':'))
        uc = self._new_uc(dgroup)
        stack = b''.join(struct.pack('<H', a & 0xffff) for a in args)
        frame = struct.pack('<HH', STOP_OFF, STOP_SEG) + stack
        sp -= len(frame)
        uc.mem_write(DGROUP * 16 + sp, frame)
        for r, v in ((UC_X86_REG_SS, DGROUP), (UC_X86_REG_DS, DGROUP), (UC_X86_REG_ES, DGROUP),
                     (UC_X86_REG_SP, sp), (UC_X86_REG_BP, 0), (UC_X86_REG_CS, seg)):
            uc.reg_write(r, v)
        try:
            uc.emu_start(seg * 16 + off, STOP_SEG * 16 + STOP_OFF, count=max_insns)
        except UcError as e:
            cs, ip = uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP)
            raise RuntimeError(f'emulation error at {cs:04x}:{ip:04x}: {e}') from e
        cs, ip = uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP)
        if self.restart is not None:
            return bytearray(uc.mem_read(DGROUP * 16, 0x10000)), 0x10000 + self.restart
        if (cs, ip) != (STOP_SEG, STOP_OFF):
            raise RuntimeError(f'did not return (stopped at {cs:04x}:{ip:04x})')
        return bytearray(uc.mem_read(DGROUP * 16, 0x10000)), uc.reg_read(UC_X86_REG_AX)


def diff(a, b, limit=0xf000, names=None):
    """Differences between two DGROUP images below `limit` (the stack lives above)."""
    out = []
    i = 0
    while i < limit:
        if a[i] != b[i]:
            j = i
            while j < limit and a[j] != b[j]:
                j += 1
            out.append((i, bytes(a[i:j]), bytes(b[i:j])))
            i = j
        else:
            i += 1
    return out


if __name__ == '__main__':
    exe, before, func = sys.argv[1], sys.argv[2], sys.argv[3]
    args = [int(x, 0) for x in sys.argv[4:]]
    o = Oracle(exe)
    after, ax = o.call(bytearray(open(before, 'rb').read()), func, args)
    sys.stdout.buffer.write(after)
