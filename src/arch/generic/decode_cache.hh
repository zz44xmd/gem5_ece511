/*
 * Copyright (c) 2011 Google
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ARCH_GENERIC_DECODE_CACHE_HH__
#define __ARCH_GENERIC_DECODE_CACHE_HH__

#include "base/types.hh"
#include "cpu/decode_cache.hh"
#include "cpu/static_inst_fwd.hh"

namespace gem5
{

namespace GenericISA
{

template <typename Decoder, typename EMI>
class BasicDecodeCache
{
  private:
    decode_cache::InstMap<EMI> instMap;
    struct AddrMapEntry
    {
        StaticInstPtr inst;
        EMI machInst;
    };
    decode_cache::AddrMap<AddrMapEntry> decodePages;

  public:
    /// Decode a machine instruction.
    /// @param mach_inst The binary instruction to decode.
    /// @retval A pointer to the corresponding StaticInst object.
    StaticInstPtr
    decode(Decoder *const decoder, EMI mach_inst, Addr addr)
    {
        auto &entry = decodePages.lookup(addr);
        if (entry.inst && (entry.machInst == mach_inst))
            return entry.inst;

        entry.machInst = mach_inst;

        auto iter = instMap.find(mach_inst);
        if (iter != instMap.end()) {
            entry.inst = iter->second;
            return entry.inst;
        }

        entry.inst = decoder->decodeInst(mach_inst);
        printf("Generic ISA Working\n");
        instMap[mach_inst] = entry.inst;
        return entry.inst;
    }
};

} // namespace GenericISA
} // namespace gem5

#endif // __ARCH_GENERIC_DECODE_CACHE_HH__
