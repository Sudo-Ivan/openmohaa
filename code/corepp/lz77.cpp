/*
===========================================================================
Copyright (C) 2025 the OpenMoHAA team

This file is part of OpenMoHAA source code.

OpenMoHAA source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenMoHAA source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenMoHAA source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// lz77.cpp: LZ77 Compression Algorithm

#include "lz77.h"
#include "../qcommon/q_shared.h"
#include <chrono>
#include <cstring>

cLZ77 g_lz77;

unsigned int cLZ77::m_pDictionary[0xffff];

static void copy_bytes(unsigned char *dest, unsigned char *from, size_t length)
{
    size_t i;

    for (i = 0; i < length; i++) {
        dest[i] = from[i];
    }
}

cLZ77::cLZ77()
{
    m_outStart = NULL;
    m_inStart  = NULL;
}

unsigned int cLZ77::CompressData(unsigned char *in, size_t in_len, unsigned char *out, size_t *out_len)
{
    memset(cLZ77::m_pDictionary, 0, sizeof(cLZ77::m_pDictionary));

    this->in_end = &in[in_len];
    this->ip_end = &in[in_len - 13];
    this->op     = out;
    this->ip     = in;
    this->ii     = this->ip;
    this->ip += 4;

    while (this->ip < this->ip_end) {
        bool match_found = false;

        while (this->ip < this->ip_end) {
            // Primary hash index
            this->dindex =
                ((33 * (this->ip[0] ^ (32 * ((32 * ((this->ip[3] << 6) ^ this->ip[2])) ^ (unsigned int)this->ip[1]))))
                 >> 5)
                & 0x3FFF;
            this->m_off = cLZ77::m_pDictionary[this->dindex];

            if (this->ip - in <= this->m_off) {
                cLZ77::m_pDictionary[this->dindex] = this->ip - in;
                this->ip++;
                continue;
            }

            this->m_off = this->ip - in - this->m_off;
            if (this->m_off > 0xBFFF) {
                cLZ77::m_pDictionary[this->dindex] = this->ip - in;
                this->ip++;
                continue;
            }

            this->m_pos = this->ip - this->m_off;

            if (this->m_off <= 0x800 || (this->m_pos[3] == this->ip[3])) {
                if (*this->m_pos == *this->ip && this->m_pos[1] == this->ip[1] && this->m_pos[2] == this->ip[2]) {
                    match_found = true;
                    break;
                }
            }

            // Fallback hash index
            this->dindex = (this->dindex & 0x7FF) ^ 0x201F;
            this->m_off  = cLZ77::m_pDictionary[this->dindex];

            if (this->ip - in <= this->m_off) {
                cLZ77::m_pDictionary[this->dindex] = this->ip - in;
                this->ip++;
                continue;
            }

            this->m_off = this->ip - in - this->m_off;
            if (this->m_off > 0xBFFF) {
                cLZ77::m_pDictionary[this->dindex] = this->ip - in;
                this->ip++;
                continue;
            }

            this->m_pos = this->ip - this->m_off;

            if (this->m_off <= 0x800 || (this->m_pos[3] == this->ip[3])) {
                if (this->m_pos[0] == this->ip[0] && this->m_pos[1] == this->ip[1] && this->m_pos[2] == this->ip[2]) {
                    match_found = true;
                    break;
                }
            }

            cLZ77::m_pDictionary[this->dindex] = this->ip - in;
            this->ip++;
        }

        if (!match_found) {
            break;
        }

        cLZ77::m_pDictionary[this->dindex] = this->ip - in;

        unsigned int t = this->ip - this->ii;
        if (t > 0) {
            if (t <= 3) {
                *(this->op - 2) |= t;
            } else if (t <= 18) {
                *this->op++ = t - 3;
            } else {
                unsigned int tt = t - 18;

                *this->op++ = 0;
                while (tt > 255) {
                    tt -= 255;
                    *this->op++ = 0;
                }
                *this->op++ = tt;
            }

            copy_bytes(op, ii, t);
            ii += t;
            op += t;
        }

        this->ip += 3;

        for (t = 0; ip < in_end; t++, this->ip++) {
            if (this->m_pos[t + 3] != *this->ip) {
                break;
            }
        }

        this->m_len = this->ip - this->ii;

        if (this->m_off > 0x4000) {
            this->m_off -= 0x4000;
            if (this->m_len > 9) {
                this->m_len -= 9;
                *this->op++ = ((this->m_off & 0x4000) >> 11) | 0x10;
                while (this->m_len > 0xFF) {
                    this->m_len -= 255;
                    *this->op++ = 0;
                }
                *this->op++ = this->m_len;
            } else {
                *this->op++ = ((this->m_off & 0x4000) >> 11) | ((this->m_len & 0xFF) - 2) | 0x10;
            }
        } else {
            --this->m_off;
            if (this->m_len > 33) {
                this->m_len -= 33;
                *this->op++ = 32;
                while (this->m_len > 255) {
                    this->m_len -= 255;
                    *this->op++ = 0;
                }
                *this->op++ = this->m_len;
            } else {
                *this->op++ = ((this->m_len & 0xFF) - 2) | 0x20;
            }
        }

        *this->op++ = 4 * (this->m_off & 63);
        *this->op++ = this->m_off >> 6;
        this->ii    = this->ip;
    }

    *out_len = this->op - out;
    return this->in_end - this->ii;
}

int cLZ77::Compress(unsigned char *in, size_t in_len, unsigned char *out, size_t *out_len)
{
    byte  *out_p = out;
    size_t t     = 0;

    if (in_len == 0) {
        *out_len = 0;
        return 0;
    }

    if (in_len > 13) {
        t      = CompressData(in, in_len, out, out_len);
        out_p = out + *out_len;
    } else {
        t = in_len;
    }

    if (t) {
        if (out_p == out && t <= 238) {
            *out_p++ = t + 17;
        } else if (t <= 3) {
            *(out_p - 2) |= t;
        } else if (t <= 18) {
            *out_p++ = t - 3;
        } else {
            unsigned int tt;

            *out_p++ = 0;

            tt = t - 18;
            while (tt > 255) {
                tt -= 255;
                *out_p++ = 0;
            }

            *out_p++ = tt;
        }

        copy_bytes(out_p, &in[in_len - t], t);
        out_p += t;
    }

    *out_p++ = 17;
    *out_p++ = 0;
    *out_p++ = 0;
    *out_len = out_p - out;

    return 0;
}

void cLZ77::CompressBegin(unsigned char *in, size_t in_len, unsigned char *out)
{
    memset(cLZ77::m_pDictionary, 0, sizeof(cLZ77::m_pDictionary));

    this->m_inStart  = in;
    this->m_outStart = out;
    this->in_end     = &in[in_len];
    this->ip_end     = &in[in_len - 13];
    this->op         = out;
    this->ip         = in;
    this->ii         = this->ip;
    if (in_len > 13) {
        this->ip += 4;
    }
}

void cLZ77::CompressTail(size_t in_len, size_t *out_len)
{
    unsigned char *out_base;
    unsigned char *in_base;
    unsigned char *out_p;
    size_t         t;

    in_base  = this->m_inStart;
    out_base = this->m_outStart;
    out_p    = this->op;
    t        = (size_t)(this->in_end - this->ii);

    if (in_len <= 13) {
        t     = in_len;
        out_p = out_base;
    }

    if (t) {
        if (out_p == out_base && t <= 238) {
            *out_p++ = (unsigned char)(t + 17);
        } else if (t <= 3) {
            *(out_p - 2) |= (unsigned char)t;
        } else if (t <= 18) {
            *out_p++ = (unsigned char)(t - 3);
        } else {
            unsigned int tt;

            *out_p++ = 0;

            tt = (unsigned int)(t - 18);
            while (tt > 255) {
                tt -= 255;
                *out_p++ = 0;
            }

            *out_p++ = (unsigned char)tt;
        }

        copy_bytes(out_p, &in_base[in_len - t], t);
        out_p += t;
    }

    *out_p++ = 17;
    *out_p++ = 0;
    *out_p++ = 0;
    *out_len = (size_t)(out_p - out_base);
}

int cLZ77::CompressContinue(size_t *out_len, int max_ms)
{
    unsigned char *in;
    size_t         in_len;
    int            budget;
    const auto     slice_start = std::chrono::steady_clock::now();

    if (!this->op || !this->m_inStart) {
        *out_len = 0;
        return 0;
    }

    in     = this->m_inStart;
    in_len = (size_t)(this->in_end - in);
    budget = max_ms;

    if (in_len <= 13) {
        CompressTail(in_len, out_len);
        return 0;
    }

    while (this->ip < this->ip_end) {
        bool match_found = false;

        while (this->ip < this->ip_end) {
            if (budget > 0) {
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - slice_start);
                if (elapsed.count() >= budget) {
                    *out_len = (size_t)(this->op - this->m_outStart);
                    return (int)(this->in_end - this->ii);
                }
            }

            this->dindex =
                ((33 * (this->ip[0] ^ (32 * ((32 * ((this->ip[3] << 6) ^ this->ip[2])) ^ (unsigned int)this->ip[1]))))
                 >> 5)
                & 0x3FFF;
            this->m_off = cLZ77::m_pDictionary[this->dindex];

            if (this->ip - in <= this->m_off) {
                cLZ77::m_pDictionary[this->dindex] = (unsigned int)(this->ip - in);
                this->ip++;
                continue;
            }

            this->m_off = (unsigned int)(this->ip - in - this->m_off);
            if (this->m_off > 0xBFFF) {
                cLZ77::m_pDictionary[this->dindex] = (unsigned int)(this->ip - in);
                this->ip++;
                continue;
            }

            this->m_pos = this->ip - this->m_off;

            if (this->m_off <= 0x800 || (this->m_pos[3] == this->ip[3])) {
                if (*this->m_pos == *this->ip && this->m_pos[1] == this->ip[1] && this->m_pos[2] == this->ip[2]) {
                    match_found = true;
                    break;
                }
            }

            this->dindex = (this->dindex & 0x7FF) ^ 0x201F;
            this->m_off  = cLZ77::m_pDictionary[this->dindex];

            if (this->ip - in <= this->m_off) {
                cLZ77::m_pDictionary[this->dindex] = (unsigned int)(this->ip - in);
                this->ip++;
                continue;
            }

            this->m_off = (unsigned int)(this->ip - in - this->m_off);
            if (this->m_off > 0xBFFF) {
                cLZ77::m_pDictionary[this->dindex] = (unsigned int)(this->ip - in);
                this->ip++;
                continue;
            }

            this->m_pos = this->ip - this->m_off;

            if (this->m_off <= 0x800 || (this->m_pos[3] == this->ip[3])) {
                if (this->m_pos[0] == this->ip[0] && this->m_pos[1] == this->ip[1] && this->m_pos[2] == this->ip[2]) {
                    match_found = true;
                    break;
                }
            }

            cLZ77::m_pDictionary[this->dindex] = (unsigned int)(this->ip - in);
            this->ip++;
        }

        if (!match_found) {
            break;
        }

        cLZ77::m_pDictionary[this->dindex] = (unsigned int)(this->ip - in);

        unsigned int t = (unsigned int)(this->ip - this->ii);
        if (t > 0) {
            if (t <= 3) {
                *(this->op - 2) |= (unsigned char)t;
            } else if (t <= 18) {
                *this->op++ = (unsigned char)(t - 3);
            } else {
                unsigned int tt = t - 18;

                *this->op++ = 0;
                while (tt > 255) {
                    tt -= 255;
                    *this->op++ = 0;
                }
                *this->op++ = (unsigned char)tt;
            }

            copy_bytes(this->op, this->ii, t);
            this->ii += t;
            this->op += t;
        }

        this->ip += 3;

        for (t = 0; this->ip < this->in_end; t++, this->ip++) {
            if (this->m_pos[t + 3] != *this->ip) {
                break;
            }
        }

        this->m_len = (unsigned int)(this->ip - this->ii);

        if (this->m_off > 0x4000) {
            this->m_off -= 0x4000;
            if (this->m_len > 9) {
                this->m_len -= 9;
                *this->op++ = (unsigned char)(((this->m_off & 0x4000) >> 11) | 0x10);
                while (this->m_len > 0xFF) {
                    this->m_len -= 255;
                    *this->op++ = 0;
                }
                *this->op++ = (unsigned char)this->m_len;
            } else {
                *this->op++ =
                    (unsigned char)(((this->m_off & 0x4000) >> 11) | ((this->m_len & 0xFF) - 2) | 0x10);
            }
        } else {
            --this->m_off;
            if (this->m_len > 33) {
                this->m_len -= 33;
                *this->op++ = 32;
                while (this->m_len > 255) {
                    this->m_len -= 255;
                    *this->op++ = 0;
                }
                *this->op++ = (unsigned char)this->m_len;
            } else {
                *this->op++ = (unsigned char)(((this->m_len & 0xFF) - 2) | 0x20);
            }
        }

        *this->op++ = (unsigned char)(4 * (this->m_off & 63));
        *this->op++ = (unsigned char)(this->m_off >> 6);
        this->ii    = this->ip;
    }

    CompressTail(in_len, out_len);
    return 0;
}

static unsigned int decode_length(unsigned int base, unsigned char *& ip)
{
    unsigned int len = base;
    while (!*ip) {
        len += 255;
        ++ip;
    }
    return len + *ip++;
}

int cLZ77::Decompress(unsigned char *in, size_t in_len, unsigned char *out, size_t *out_len)
{
    unsigned int   t;
    unsigned short s;

    ip_end   = &in[in_len];
    ip       = in;
    op       = out;
    *out_len = 0;

    if (*ip > 17u) {
        t = *ip++ - 17;
        if (t <= 3) {
            copy_bytes(op, ip, t);
            op += t;
            ip += t;
            t = *ip++;
        } else {
            copy_bytes(op, ip, t);
            op += t;
            ip += t;
            t = *ip++;
        }
    } else {
        t = *ip++;
    }

    for (;;) {
        if (t <= 15) {
            if (t == 0) {
                t = decode_length(15, ip);
            }

            memcpy(op, ip, 4);
            op += 4;
            ip += 4;
            t--;

            if (t) {
                if (t <= 3) {
                    copy_bytes(op, ip, t);
                    op += t;
                    ip += t;
                } else {
                    while (t > 3) {
                        memcpy(op, ip, 4);
                        op += 4;
                        ip += 4;
                        t -= 4;
                    }
                    copy_bytes(op, ip, t);
                    op += t;
                    ip += t;
                }
            }

            t = *ip++;
            if (t <= 15) {
                m_pos = op - 2049 - (t >> 2) - 4 * *ip++;
                *op++ = *m_pos++;
                *op++ = *m_pos++;
                *op++ = *m_pos++;
                continue;
            }
        }

        while (true) {
            if (t > 63) {
                m_pos = op - 1 - ((t >> 2) & 7) - 8 * *ip++;
                t     = (t >> 5) - 1;
                *op++ = *m_pos++;
                *op++ = *m_pos++;
                copy_bytes(op, m_pos, t);
                op += t;
                break;
            }

            if (t > 31) {
                t &= 31;
                if (t == 0) {
                    t = decode_length(31, ip);
                }

                m_pos = op - 1;
                CopyLittleShort(&s, ip);
                ip += 2;
                m_pos -= (s >> 2);
            } else {
                if (t <= 15) {
                    m_pos = op - 1 - (t >> 2) - 4 * *ip++;
                    *op++ = *m_pos++;
                    *op++ = *m_pos++;
                    break;
                }

                m_pos = op - 2048 * (t & 8);
                t &= 7u;
                if (t == 0) {
                    t = decode_length(7, ip);
                }

                CopyLittleShort(&s, ip);
                ip += 2;
                m_pos -= (s >> 2);

                if (m_pos == op) {
                    *out_len = op - out;
                    return 0;
                }
                m_pos -= 0x4000;
            }

            if (t <= 5 || static_cast<size_t>(op - m_pos) <= 3) {
                *op++ = *m_pos++;
                *op++ = *m_pos++;
                copy_bytes(op, m_pos, t);
                op += t;
            } else {
                memcpy(op, m_pos, 4);
                op += 4;
                m_pos += 4;
                t -= 2;
                while (t > 3) {
                    memcpy(op, m_pos, 4);
                    op += 4;
                    m_pos += 4;
                    t -= 4;
                }
                copy_bytes(op, m_pos, t);
                op += t;
            }
            break;
        }

        t = *(ip - 2) & 3;
        if (t == 0) {
            t = *ip++;
            continue;
        }

        copy_bytes(op, ip, t);
        op += t;
        ip += t;
        t = *ip++;
    }

    *out_len = op - out;
    if (ip == ip_end) {
        return 0;
    }

    return (ip < ip_end) ? -1 : -2;
}
