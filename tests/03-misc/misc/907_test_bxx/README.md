If you’d like to also assert the branch opcodes are all present, you can add a few opcode breadcrumbs (little-endian shown in your hexdump):

- BEQ @ 0x800C: `0A000002`
- BNE @ 0x802C: `1A000002`
- BHS @ 0x8050: `2A000002`
- BLO @ 0x8070: `3A000002`
- BMI @ 0x8090: `4A000002`
- BPL @ 0x80B0: `5A000002`
- BVS @ 0x80D8: `6A000002`
- BVC @ 0x80FC: `7A000002`
- BGE @ 0x8120: `AA000002`
- BLT @ 0x8148: `BA000002`
- BGT @ 0x816C: `CA000002`
- BLE @ 0x8190: `DA000002`
- BHI @ 0x81B4: `8A000002`
- BLS @ 0x81D8: `9A000002`

Run it—if all tags show up (no `0xEE` writes) and you hit the halt, your conditional branch decode + cond logic are solid.