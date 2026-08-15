// disasm.c (small starter; grow as you go)
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   // snprintf

static inline uint32_t ror32(uint32_t x, unsigned r) {
    r &= 31;
    return (x >> r) | (x << (32 - r));
}

// imm24<<2 with sign extension (for B/BL)
static inline int32_t signext_imm24_shl2(uint32_t imm24) {
    return ((int32_t)(imm24 << 8)) >> 6; // arithmetic >> keeps sign; net <<2
}

static bool disasm_barrier(uint32_t instr, char *out, size_t n) {
    // Matches: DSB/DMB/ISB with SY option (armv7-A encodings)
    // F57FF04F = DSB SY
    // F57FF05F = DMB SY
    // F57FF06F = ISB SY
    if ((instr & 0xFFFFFFCFu) == 0xF57FF04Fu) {
        uint32_t kind = (instr >> 4) & 0x3u;  // 0=DSB, 1=DMB, 2=ISB
        const char *mn = (kind == 0) ? "dsb" : (kind == 1) ? "dmb" : "isb";
        // Option is SY (0xF) in these encodings
        snprintf(out, n, "%s sy", mn);
        return true;
    }
    return false;
}

static const char* cond_str(uint32_t instr) {
    static const char* cond_names[] = {
        "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
        "hi", "ls", "ge", "lt", "gt", "le", "", "al"
    };
    uint32_t cond = (instr >> 28) & 0xF;
    return cond_names[cond];
}

// ARM Shift Types
static const char *shift_name(uint32_t type) {
	switch (type) {
	case 0: return "lsl";
	case 1: return "lsr";
	case 2: return "asr";
	case 3: return "ror";
	default: return "?";
	}
}

void decode_imm_shift(char *out, size_t len, uint32_t instr) {
    unsigned shift_type = (instr >> 5) & 0x3;
    unsigned shift_amount = (instr >> 7) & 0x1F;
    if (shift_type == 0) {  // LSL
        if (shift_amount == 0) {
            // no shift
            out[0] = '\0';
        } else {
            snprintf(out, len, "lsl #%u", shift_amount);
        }
    } else if (shift_type == 1) {  // LSR
        if (shift_amount == 0) {
            shift_amount = 32;
        }
        snprintf(out, len, "lsr #%u", shift_amount);
    } else if (shift_type == 2) {  // ASR
        if (shift_amount == 0) {
            shift_amount = 32;
        }
        snprintf(out, len, "asr #%u", shift_amount);
    } else if (shift_type == 3) {  // ROR
        if (shift_amount == 0) {
            snprintf(out, len, "rrx");
        } else {
            snprintf(out, len, "ror #%u", shift_amount);
        }
    }
}

static inline uint16_t k12(uint32_t instr) {
    // Match the VM's decode: key= ((instr >> 16) & 0xFF0) | ((instr >> 4) & 0xF)
    return ((instr >> 16) & 0xFF0u) | ((instr >> 4) & 0xFu);
}

void disasm_line(uint32_t pc, uint32_t instr, char *out, size_t out_sz) {
	char buf[128];
	int len = sizeof(buf);
	uint32_t rd, rn, rm, imm12;

	//.. preload
	rd = (instr >> 12) & 0xF;
	rn = (instr >> 16) & 0xF;
	rm = instr & 0xF;
	imm12 = instr & 0xFFF;

    // Default fallback
    snprintf(out, out_sz, ".word 0x%08X", instr);

    // ---- Catch barriers first (preempts LDR/STR mis-decode of F57Fxxxx) ----
    if (disasm_barrier(instr, out, out_sz)) return;

	uint16_t key = k12(instr);
	switch (key) {
		 case 0x180: // EOR register form (older key, if used)
			snprintf(buf, len, "eor r%u, r%u, r%u", rd, rn, rm);
			snprintf(out, out_sz, "%s", buf);
			return;
		 case 0x150: // CMP register
			snprintf(buf, len, "cmp r%u, r%u", rn, rm);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x156:
			snprintf(buf, len, "cmp r%u, r%u", rn, rm);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x370: // CMN immediate
			snprintf(buf, len, "cmn r%u, #0x%x", rn, imm12);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x3A0: // MOV (imm)  (you may keep or remove, since you also decode MOV imm later)
			snprintf(buf, len, "mov r%u, #0x%x", rd, imm12);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x170: // CMN register
			snprintf(buf, len, "cmn r%u, r%u", rn, rm);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x178: // CMN register (variant)
            snprintf(buf, len, "cmn r%u, r%u", rn, rm);
            snprintf(out, out_sz, "%s", buf);
            return;
		case 0x350: // CMP (immediate)
			snprintf(buf, len, "cmp r%u, #0x%x", rn, imm12);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x3E0: // MVN (immediate)
			// mvn Rd, #imm
			snprintf(buf, len, "mvn r%u, #0x%x", rd, imm12);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x020: // EOR register
			snprintf(buf, len, "eor r%u, r%u, r%u", rd, rn, rm);
			snprintf(out, out_sz, "%s", buf);
			return;
		case 0x03C: // EORS (register, shifted)
			{
				// Decode shift from the standard data-processing encoding
				uint32_t shift = (instr >> 4) & 0xFF;
				uint32_t shift_imm  = (shift >> 3) & 0x1F;
				uint32_t shift_type = (shift >> 1) & 0x3;

				const char *stype;
				switch (shift_type) {
					case 0: stype = "lsl"; break;
					case 1: stype = "lsr"; break;
					case 2: stype = "asr"; break;
					case 3: stype = "ror"; break;
					default: stype = "lsl"; break;
				}

				// eors rd, rn, rm, <shift> #imm
				snprintf(buf, len,
						 "eors r%u, r%u, r%u, %s #%u",
						 rd, rn, rm, stype, shift_imm);
				snprintf(out, out_sz, "%s", buf);
				return;
			}
		break;
	}

    // ---- MOVW / MOVT (A32) ----
    // MOVW: cond 0011 0000 xxxx xxxx (0x03000000)
    // MOVT: cond 0011 0100 xxxx xxxx (0x03400000)
    if ((instr & 0x0FF00000u) == 0x03000000u || (instr & 0x0FF00000u) == 0x03400000u) {
        uint32_t Rd    = (instr >> 12) & 0xF;
        uint32_t imm4  = (instr >> 16) & 0xF;
        uint32_t imm12 = instr & 0xFFF;
        uint32_t imm16 = (imm4 << 12) | imm12;
        if ((instr & 0x0FF00000u) == 0x03000000u) {
            snprintf(out, out_sz, "movw r%u, #0x%04X", Rd, imm16);
        } else {
            snprintf(out, out_sz, "movt r%u, #0x%04X", Rd, imm16);
        }
        return;
    }

   // bkpt
	if ((instr & 0x0FF000F0u) == 0x01200070u) {              // BKPT encoding
		uint32_t imm4  = (instr >> 16) & 0xF;
		uint32_t imm12 = instr & 0xFFF;
		uint32_t imm16 = (imm4 << 12) | imm12;
		snprintf(out, out_sz, "bkpt #0x%04X", imm16);
		return;
	}

	if ((instr & 0x0FE000F0) == 0x01A00000) {
	// MOV register with optional shift
	uint32_t rd = (instr >> 12) & 0xF;
	uint32_t rm = instr & 0xF;
	uint32_t shift = (instr >> 4) & 0xFF;
	uint32_t shift_imm = (shift >> 3) & 0x1F;
	uint32_t shift_type = (shift >> 1) & 0x3;


	if ((shift & 0x1) == 0) {
	snprintf(buf, len, "mov%s r%u, r%u, %s #%u", cond_str(instr), rd, rm,
	shift_name(shift_type), shift_imm);
	} else {
	uint32_t rs = (shift >> 4) & 0xF;
	snprintf(buf, len, "mov%s r%u, r%u, %s r%u", cond_str(instr), rd, rm,
	shift_name(shift_type), rs);
	}
	return;
	}

	if ((instr & 0x0FF00FF0) == 0x01B00000) {
	// MOVS with shift
	uint32_t rd = (instr >> 12) & 0xF;
	uint32_t rm = instr & 0xF;
	uint32_t shift = (instr >> 4) & 0xFF;
	uint32_t shift_imm = (shift >> 3) & 0x1F;
	uint32_t shift_type = (shift >> 1) & 0x3;
	snprintf(buf, len, "movs%s r%u, r%u, %s #%u", cond_str(instr), rd, rm,
	shift_name(shift_type), shift_imm);
	return;
	}


	if ((instr & 0x0FF00000) == 0x03200000) {
	// TEQ immediate
	uint32_t rn = (instr >> 16) & 0xF;
	uint32_t imm12 = instr & 0xFFF;
	snprintf(buf, len, "teq%s r%u, #0x%x", cond_str(instr), rn, imm12);
	return;
	}


	if ((instr & 0x0FF00FF0) == 0x01700000) {
	// CMN with shift
	uint32_t rn = (instr >> 16) & 0xF;
	uint32_t rm = instr & 0xF;
	uint32_t shift = (instr >> 4) & 0xFF;
	uint32_t shift_imm = (shift >> 3) & 0x1F;
	uint32_t shift_type = (shift >> 1) & 0x3;
	snprintf(buf, len, "cmn%s r%u, r%u, %s #%u", cond_str(instr), rn, rm,
	shift_name(shift_type), shift_imm);
	return;
	}
	// MRS Rd, CPSR
	if ((instr & 0x0FBF0FFFu) == 0x010F0000u) {
		uint32_t Rd = (instr >> 12) & 0xF;
		snprintf(out, out_sz, "mrs r%u, cpsr", Rd);
		return;
	}

	if ((instr & 0x0FE00010u) == 0x01200010u && ((instr >> 21) & 0xF) == 0x9) {
		uint32_t Rn = (instr >> 16) & 0xF;
		uint32_t Rm = instr & 0xF;
		snprintf(out, out_sz, "teq r%u, r%u", Rn, Rm);
		return;
	}

	if ((instr & 0x0FFF0FF0u) == 0x01A00000u) {
		uint32_t Rd = (instr >> 12) & 0xF;
		uint32_t Rm = instr & 0xF;
		snprintf(out, out_sz, "mov r%u, r%u", Rd, Rm);
		return;
	}
	
	if (((instr >> 21) & 0xF) == 0xC && ((instr >> 25) & 0x7) < 0x2) {
		const char *cond = cond_str(instr);
		char s_flag[2] = "";
		if (instr & (1 << 20)) {
			s_flag[0] = 's';
			s_flag[1] = '\0';
		}
		if (instr & (1 << 25)) {
			// ORR (immediate)
			uint32_t imm8 = instr & 0xFF;
			uint32_t rot  = (instr >> 8) & 0xF;
			uint32_t imm  = (rot == 0) ? imm8 : ((imm8 >> (rot * 2)) | (imm8 << (32 - rot * 2)));
			unsigned rd   = (instr >> 12) & 0xF;
			unsigned rn   = (instr >> 16) & 0xF;
			snprintf(buf, len, "orr%s%s r%u, r%u, #0x%X", cond, s_flag, rd, rn, imm);
		} else {
			// ORR (register or register with shift)
			unsigned rd = (instr >> 12) & 0xF;
			unsigned rn = (instr >> 16) & 0xF;
			unsigned rm = instr & 0xF;
			char shift_op[20];
			decode_imm_shift(shift_op, sizeof(shift_op), instr);
			if (shift_op[0] == '\0') {
				snprintf(buf, len, "orr%s%s r%u, r%u, r%u", cond, s_flag, rd, rn, rm);
			} else {
				snprintf(buf, len, "orr%s%s r%u, r%u, r%u, %s", cond, s_flag, rd, rn, rm, shift_op);
			}
		}
	}

    // ---- B / BL ----
    if ((instr & 0x0E000000u) == 0x0A000000u) {
        uint32_t imm24  = instr & 0x00FFFFFFu;
        int32_t  offset = signext_imm24_shl2(imm24);
        uint32_t target = pc + 8 + (uint32_t)offset;  // ARM pipeline: PC is current+8
        snprintf(out, out_sz, "%s 0x%08X", (instr & 0x01000000u) ? "bl" : "b", target);
        return;
    }

	if ((instr & 0x0FE00000u) == 0x1E00000u) {  // MVN
		snprintf(out, out_sz, "mvn%s r%u, ...", cond_str(instr), (instr >> 12) & 0xF);
		return;
	}

	// Recognize ARM NOP (0xE320F000)
	if (instr == 0xE320F000u) {
		snprintf(out, out_sz, "nop");
		return;
	}

	if ((instr & 0x0FE00000u) == 0x0A00000u) {  // ADC
		snprintf(out, out_sz, "adc%s r%u, ...", cond_str(instr), (instr >> 12) & 0xF);
		return;
	}

	if ((instr & 0x0FE00000u) == 0x04000000u) {  // SUB
		snprintf(out, out_sz, "sub%s r%u, ...", cond_str(instr), (instr >> 12) & 0xF);
		return;
	}

    // ---- Data-processing (immediate) — quick common mnemonics ----
    if ((instr & 0x0C000000u) == 0x00000000u) {
        uint32_t op = (instr >> 21) & 0xF;
        uint32_t S  = (instr >> 20) & 1u;
        uint32_t Rd = (instr >> 12) & 0xF;
        uint32_t Rn = (instr >> 16) & 0xF;

        if (instr & (1u << 25)) { // immediate shifter
            uint32_t imm8  = instr & 0xFF;
            uint32_t rot2  = ((instr >> 8) & 0xF) * 2;
            uint32_t imm32 = ror32(imm8, rot2);

            switch (op) {
                case 0xD: snprintf(out, out_sz, "mov%s r%u, #0x%X", S?"s":"", Rd, imm32); return; // MOV
                case 0xA: snprintf(out, out_sz, "cmp r%u, #0x%X",   Rn, imm32); return;           // CMP
                case 0x8: snprintf(out, out_sz, "tst r%u, #0x%X",   Rn, imm32); return;           // TST
                case 0x2: snprintf(out, out_sz, "sub%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0x4: snprintf(out, out_sz, "add%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0x0: snprintf(out, out_sz, "and%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0x1: snprintf(out, out_sz, "eor%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0xC: snprintf(out, out_sz, "orr%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                // (expand with more ops as you like)
            }
        }
    }

   // ---- Data-processing (register shifted register) ----
    if ((instr & 0x0C000010u) == 0x00000000u && ((instr >> 4) & 0xF) == 0x1) {
        uint32_t op = (instr >> 21) & 0xF;
        uint32_t S  = (instr >> 20) & 1u;
        uint32_t Rn = (instr >> 16) & 0xF;
        uint32_t Rd = (instr >> 12) & 0xF;
        uint32_t Rm = instr & 0xF;
        uint32_t Rs = (instr >> 8) & 0xF;
        uint32_t shift = (instr >> 5) & 0x3;

        const char *shift_types[] = {"lsl", "lsr", "asr", "ror"};
        const char *sh = shift_types[shift];

        const char *mn = NULL;
        switch (op) {
            case 0x0: mn = "and"; break;
            case 0x1: mn = "eor"; break;
            case 0x2: mn = "sub"; break;
            case 0x3: mn = "rsb"; break;
            case 0x4: mn = "add"; break;
            case 0x8: mn = "tst"; break;
            case 0x9: mn = "teq"; break;
            case 0xA: mn = "cmp"; break;
            case 0xC: mn = "orr"; break;
            case 0xD: mn = "mov"; break;
            case 0xE: mn = "bic"; break;
            case 0xF: mn = "mvn"; break;
        }

        if (mn) {
            if (op == 0x8 || op == 0x9 || op == 0xA) // TST, TEQ, CMP: no Rd
                snprintf(out, out_sz, "%s r%u, r%u, %s r%u", mn, Rn, Rm, sh, Rs);
            else
                snprintf(out, out_sz, "%s%s r%u, r%u, r%u, %s r%u", mn, S ? "s" : "", Rd, Rn, Rm, sh, Rs);
            return;
        }
    }

    // ---- Single data transfer (LDR/STR immediate & byte) ----
    if ((instr & 0x0C000000u) == 0x04000000u) {
        uint32_t L = (instr >> 20) & 1u;
        uint32_t B = (instr >> 22) & 1u;
        uint32_t P = (instr >> 24) & 1u;
        uint32_t U = (instr >> 23) & 1u;
        uint32_t Rn= (instr >> 16) & 0xF;
        uint32_t Rd= (instr >> 12) & 0xF;
        uint32_t I = (instr >> 25) & 1u;

        const char *mn = (L ? (B ? "ldrb" : "ldr") : (B ? "strb" : "str"));

        if (!I) { // immediate offset
            uint32_t imm12 = instr & 0xFFF;
            int32_t off = U ? (int32_t)imm12 : -(int32_t)imm12;

            const char *base = (Rn == 13) ? "sp" : (Rn == 15) ? "pc" : NULL;
            char basebuf[6];
            if (!base) { snprintf(basebuf, sizeof basebuf, "r%u", Rn); base = basebuf; }

            if (P) snprintf(out, out_sz, "%s r%u, [%s, #%+d]", mn, Rd, base, off);
            else   snprintf(out, out_sz, "%s r%u, [%s], #%+d", mn, Rd, base, off);
            return;
        }
        // (reg-offset variants can be added later)
    }

    // ---- Block transfers: LDM/STM ----
    if ((instr & 0x0E000000u) == 0x08000000u) {
        uint32_t P = (instr >> 24) & 1u;
        uint32_t U = (instr >> 23) & 1u;
        uint32_t W = (instr >> 21) & 1u;
        uint32_t L = (instr >> 20) & 1u;
        uint32_t Rn = (instr >> 16) & 0xF;
        uint32_t reglist = instr & 0xFFFF;

        const char *mn = L ? "ldm" : "stm";
        const char *am = (!U && !P) ? "da" :
                         (!U &&  P) ? "db" :
                         ( U && !P) ? "ia" : "ib";

        const char *base = (Rn == 13) ? "sp" : (Rn == 15) ? "pc" : NULL;
        char basebuf[8];
        if (!base) { snprintf(basebuf, sizeof basebuf, "r%u", Rn); base = basebuf; }

        char regs[128]; size_t n = 0; bool first = true;
        n += snprintf(regs+n, sizeof regs - n, "{");
        for (int r = 0; r <= 15; ++r) if (reglist & (1u << r)) {
            if (!first) n += snprintf(regs+n, sizeof regs - n, ", ");
            if      (r == 13) n += snprintf(regs+n, sizeof regs - n, "sp");
            else if (r == 14) n += snprintf(regs+n, sizeof regs - n, "lr");
            else if (r == 15) n += snprintf(regs+n, sizeof regs - n, "pc");
            else              n += snprintf(regs+n, sizeof regs - n, "r%d", r);
            first = false;
        }
        n += snprintf(regs+n, sizeof regs - n, "}");
        snprintf(out, out_sz, "%s%s %s%s, %s", mn, am, base, W ? "!" : "", regs);
        return;
    }

    // Fallback already set at the top
}
